#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "wm_include.h"
#include "sockets.h"
#include "netif.h"

#include "py/runtime.h"
#include "mpthreadport.h"

#include "extmod/vfs_lfs.h"
#include "lib/littlefs/lfs2.h"
#include "mp_vfs_lfs.h"

// #define FTPS_DBG printf("%ld: ", mp_hal_ticks_us());printf
#define FTPS_DBG(...)

#define FTP_SRV_ROOT        "/"
#define FTP_MAX_CONNECTION  2
#define FTP_WELCOME_MSG     "220-= welcome on W600 FTP server =-\r\n220 \r\n"
#define FTP_BUFFER_SIZE     512
#define FTP_PATH_SIZE       256
#define FTP_LINE_SIZE       256

struct ftp_session {
    bool is_anonymous;

    int sockfd;
    struct sockaddr_in remote;

    /* pasv data */
    char pasv_active;
    int pasv_sockfd;

    unsigned short pasv_port;
    size_t offset;

    /* current directory */
    char currentdir[FTP_PATH_SIZE];
    char rename[FTP_PATH_SIZE];

    struct netif *netif;
    int pasv_acpt_sockfd;

    struct ftp_session *next;
};
static struct ftp_session *session_list = NULL;

#define MPY_FTPS_SIZE   1024
static unsigned char *mpy_ftps_stk;


static int is_run = 0;
static int ftpsport = 21;
static char username[32] = {0};
static char userpwd[64] = {0};

fs_user_mount_t *spi_fls_vfs;

int ftp_process_request(struct ftp_session *session, char *buf);
int ftp_get_filesize(char *filename);

struct ftp_session *ftp_new_session(void) {
    struct ftp_session *session;

    session = (struct ftp_session *)tls_mem_alloc(sizeof(struct ftp_session));

    if (session != NULL) {
        session->sockfd = -1;
        session->pasv_sockfd = -1;
        session->pasv_acpt_sockfd = -1;

        session->next = session_list;
        session_list = session;
    }
    return session;
}

void ftp_close_session(struct ftp_session *session) {
    struct ftp_session *list = session_list;
    struct ftp_session *prev = NULL;

    while (list != NULL) { // walk through the list
        if (list == session) { // match
            if (prev == NULL) {  // at the head; remove it
                session_list = list->next; // tail will get the list
            } else { // Middle element
                prev->next = list->next; // link the tail to the head
            }
            tls_mem_free(session);
            break;
        }
        list = list->next;
    }
}

int ftp_get_filesize(char *filename) {
    fs_user_mount_t *vfs_fs = spi_fls_vfs;

    struct lfs2_info fno;
    int res = lfs2_stat(&vfs_fs->lfs, filename, &fno);
    if (LFS2_ERR_OK != res) {
        return -1;
    }
    return fno.size;

}

bool is_dir(char *directory) {

    if (strcmp(directory, FTP_SRV_ROOT) == 0) {  // root dir is one
        return true;
    }

    fs_user_mount_t *vfs_fs = spi_fls_vfs;

    lfs2_dir_t dir;
    int res = lfs2_dir_open(&vfs_fs->lfs, &dir, directory);
    if (res == LFS2_ERR_OK) {
        lfs2_dir_close(&vfs_fs->lfs, &dir);
        return true;
    }

    return false;
}

void build_full_path(struct ftp_session *session, char *path, char *new_path) {
    if (path[0] == '/' || session->currentdir[0] == '\0') {
        strcpy(new_path, "/");
    } else {
        strcpy(new_path, session->currentdir);
    }
    // Normalize path
    for (char *token = strtok(path, "/"); token != NULL; token = strtok(NULL, "/")) {
        if (strlen(token) == 2 && token[0] == '.' && token[1] == '.') { // double dot, skip back in new_path
            char *p = strrchr(new_path, '/'); // should always work
            if (p) { // just for being sure
                p[p == new_path ? 1 : 0] = '\0'; // cut off the tail, but not the head
            }
        } else if (strlen(token) != 1 || token[0] != '.') {
            if (strlen(new_path) > 1) { // not at the start
                strcat(new_path, "/");
            }
            strcat(new_path, token);
        }
    }
}

static void w600_ftps_task(void *param) {
    int numbytes;
    struct timeval tv;
    int sockfd, maxfdp1;
    struct sockaddr_in local;
    fd_set readfds, tmpfds;
    struct ftp_session *session;
    int ret;
    u32 addr_len = sizeof(struct sockaddr);
    char *buffer = (char *)tls_mem_alloc(FTP_BUFFER_SIZE);
    if (buffer == NULL) { // Alloc failed
        return;
    }

    local.sin_port = htons(ftpsport);
    local.sin_family = PF_INET;
    local.sin_addr.s_addr = INADDR_ANY;

    FD_ZERO(&readfds);
    FD_ZERO(&tmpfds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        FTPS_DBG("create socket failed\n");
        tls_mem_free(buffer);
        return;
    }

    ret = bind(sockfd, (struct sockaddr *)&local, addr_len);
    ret |= listen(sockfd, FTP_MAX_CONNECTION);
    if (ret) {
        closesocket(sockfd);
        tls_mem_free(buffer);
        return;
    }

    FTPS_DBG("ftpserver is running.\r\n");
    FD_SET(sockfd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    for (;;) {
        /* get maximum fd */
        maxfdp1 = sockfd + 1;
        session = session_list;
        while (session != NULL) {
            if (maxfdp1 < session->sockfd + 1) {
                maxfdp1 = session->sockfd + 1;
            }

            FD_SET(session->sockfd, &readfds);
            session = session->next;
        }

        tmpfds = readfds;
        if (select(maxfdp1, &tmpfds, 0, 0, &tv) == 0) {
            continue;
        }

        if (FD_ISSET(sockfd, &tmpfds)) {
            int com_socket;
            struct sockaddr_in remote;

            com_socket = accept(sockfd, (struct sockaddr *)&remote, &addr_len);
            if (com_socket == -1) {
                FTPS_DBG("Error on accept()\nContinuing...\n");
                tls_os_time_delay(2);
                continue;
            } else {
                FTPS_DBG("Got connection from %s\n", inet_ntoa(remote.sin_addr));
                send(com_socket, FTP_WELCOME_MSG, strlen(FTP_WELCOME_MSG), 0);
                FD_SET(com_socket, &readfds);

                /* new session */
                session = ftp_new_session();
                if (session != NULL) {
                    strcpy(session->currentdir, FTP_SRV_ROOT);
                    session->offset = 0; // Initialize offset
                    session->sockfd = com_socket;
                    session->remote = remote;

                    struct netif *netif = tls_get_netif();
                    if ((netif->ip_addr.addr & 0xFFFFFF) == (remote.sin_addr.s_addr & 0xFFFFFF)) {
                        session->netif = netif;
                    } else {
                        session->netif = netif->next;
                    }
                }
            }
        }

        {
            struct ftp_session *next;

            session = session_list;
            while (session != NULL) {
                next = session->next;
                if (FD_ISSET(session->sockfd, &tmpfds)) {
                    numbytes = recv(session->sockfd, buffer, FTP_BUFFER_SIZE, 0);
                    if (numbytes == 0 || numbytes == -1) {
                        FTPS_DBG("Client %s disconnected %d, %d\n", inet_ntoa(session->remote.sin_addr), __LINE__, session->sockfd);
                        FD_CLR(session->sockfd, &readfds);
                        closesocket(session->sockfd);
                        ftp_close_session(session);
                    } else {
                        buffer[numbytes] = 0;
                        if (ftp_process_request(session, buffer) == -1) {
                            FTPS_DBG("Client %s disconnected %d, %d\r\n", inet_ntoa(session->remote.sin_addr), __LINE__, session->sockfd);
                            FD_CLR(session->sockfd, &readfds);
                            closesocket(session->sockfd);
                            ftp_close_session(session);
                        }
                    }
                }

                session = next;
            }
        }
    }
}

void w600_ftps_start(int port, const char *user, const char *pass) {
    ftpsport = port;
    strcpy(username, user);
    strcpy(userpwd, pass);

    if (is_run) {
        return;
    }
    // Get the root file system and remember it for the ftp server
    // It relies on _boot.py having mounted the internal flash file system.
    mp_vfs_mount_t **vfsp = &MP_STATE_VM(vfs_mount_table);
    if (vfsp) {
        spi_fls_vfs = (*vfsp)->obj;
    }
    MP_STATE_PORT(mpy_ftps_stk) = m_malloc(MPY_FTPS_SIZE * sizeof(OS_STK));

    tls_os_task_create(NULL, "w60xftps", w600_ftps_task, NULL,
        (void *)mpy_ftps_stk, MPY_FTPS_SIZE * sizeof(OS_STK),
        MPY_FTPS_PRIO, 0);
    is_run = 1;
}

int do_list(char *directory, int sockfd, char *line_buffer, bool full_dir) {
    int line_length;

    fs_user_mount_t *vfs_fs = spi_fls_vfs;

    lfs2_dir_t dir;
    struct lfs2_info fno;
    int res = lfs2_dir_open(&vfs_fs->lfs, &dir, directory);

    if (res != LFS2_ERR_OK) {

        line_length = sprintf(line_buffer, "500 Internal Error\r\n");
        send(sockfd, line_buffer, line_length, 0);
        return -1;
    }


    while (1) {
        res = lfs2_dir_read(&vfs_fs->lfs, &dir, &fno);
        if (res <= 0) {
            break;
        }
        if (full_dir) {
            line_length = sprintf(line_buffer, "%srwxrwxrwx %3d root root %6ld Jan 1 2018 %s\r\n",
                (fno.type & LFS2_TYPE_DIR) ? "d" : "-", 0, fno.size, fno.name);
        } else {
            line_length = sprintf(line_buffer, "%s\r\n", fno.name);
        }
        send(sockfd, line_buffer, line_length, 0);
    }



    lfs2_dir_close(&vfs_fs->lfs, &dir);


    return 0;
}

int str_begin_with(char *src, char *match) {
    while (*match) {
        /* check source */
        if (*src == 0) {
            return -1;
        }

        if (*match != *src) {
            return -1;
        }
        match++;
        src++;
    }

    return 0;
}

int ftp_process_request(struct ftp_session *session, char *buf) {
    struct timeval tv;
    fd_set readfds;
    char filename[FTP_PATH_SIZE];
    int numbytes;
    char *sbuf;
    char *parameter_ptr, *ptr;
    u32 addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in local, pasvremote;

    sbuf = (char *)tls_mem_alloc(FTP_BUFFER_SIZE);
    if (sbuf == NULL) { // Alloc failed
        return -1;
    }

    tv.tv_sec = 3, tv.tv_usec = 0;
    local.sin_family = PF_INET;
    local.sin_addr.s_addr = INADDR_ANY;

    /* remove \r\n */
    ptr = buf;
    while (*ptr) {
        if (*ptr == '\r' || *ptr == '\n') {
            *ptr = 0;
        }
        ptr++;
    }

    /* get request parameter */
    parameter_ptr = strchr(buf, ' ');
    if (parameter_ptr) {
        while (*parameter_ptr == ' ' && *parameter_ptr != '\0') {
            parameter_ptr++;
        }
        // Check the size of the path name, in case it is needed later.
        if (*parameter_ptr && (3 + strlen(parameter_ptr) +
                               (*parameter_ptr == '/' ? 0 : strlen(session->currentdir))) >= FTP_PATH_SIZE) {
            sprintf(sbuf, "553 Path name too long\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
    }

    // debug:
    FTPS_DBG("%s requested: \"%s\"\n", inet_ntoa(session->remote.sin_addr), buf);

    //
    // -----------------------
    if (str_begin_with(buf, "USER") == 0) {
        FTPS_DBG("%s sent login \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter_ptr);
        // login correct
        if (strcmp(parameter_ptr, "anonymous") == 0) {
            session->is_anonymous = TRUE;
            sprintf(sbuf, "331 Anonymous login OK send e-mail address for password.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else if (strcmp(parameter_ptr, username) == 0) {
            session->is_anonymous = FALSE;
            sprintf(sbuf, "331 Password required for %s\r\n", parameter_ptr);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else {
            // incorrect login
            sprintf(sbuf, "530 Login incorrect. Bye.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return -1;
        }
        tls_mem_free(sbuf);
        return 0;
    } else if (str_begin_with(buf, "PASS") == 0) {
        FTPS_DBG("%s sent password \"%s\"\n", inet_ntoa(session->remote.sin_addr), parameter_ptr);
        if (strcmp(parameter_ptr, userpwd) == 0 || session->is_anonymous == TRUE) {
            // password correct
            sprintf(sbuf, "230 User logged in\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            // session->is_anonymous == FALSE;
            return 0;
        }

        // incorrect password
        sprintf(sbuf, "530 Login or Password incorrect. Bye!\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        tls_mem_free(sbuf);
        return -1;
    } else if (str_begin_with(buf, "LIST") == 0) {
        memset(sbuf, 0, FTP_BUFFER_SIZE);
        sprintf(sbuf, "150 Opening Binary mode connection for file list.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        do_list(session->currentdir, session->pasv_sockfd, sbuf, true);
        closesocket(session->pasv_sockfd);
        session->pasv_sockfd = -1;
        session->pasv_active = 0;
        sprintf(sbuf, "226 Transfer complete.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "NLST") == 0) {
        memset(sbuf, 0, FTP_BUFFER_SIZE);
        sprintf(sbuf, "150 Opening Binary mode connection for file list.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        do_list(session->currentdir, session->pasv_sockfd, sbuf, false);
        closesocket(session->pasv_sockfd);
        session->pasv_sockfd = -1;
        session->pasv_active = 0;
        sprintf(sbuf, "226 Transfer complete.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "PWD") == 0 || str_begin_with(buf, "XPWD") == 0) {
        sprintf(sbuf, "257 \"%s\" is current directory.\r\n", session->currentdir);
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "TYPE") == 0) {
        // Ignore it
        if (strcmp(parameter_ptr, "I") == 0) {
            sprintf(sbuf, "200 Type set to binary.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else {
            sprintf(sbuf, "200 Type set to ascii.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        }
    } else if (str_begin_with(buf, "PASV") == 0) {
        int dig1, dig2;
        int sockfd;
        char optval = '1';

        session->pasv_port = 10000;
        session->pasv_active = 1;
        local.sin_port = htons(session->pasv_port);
        local.sin_addr.s_addr = INADDR_ANY;

        dig1 = (int)(session->pasv_port / 256);
        dig2 = session->pasv_port % 256;

        if (-1 != session->pasv_sockfd) {
            closesocket(session->pasv_sockfd);

            session->pasv_sockfd = -1;
        }
        if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
            sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            goto err1;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
            sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            goto err1;
        }
        if (bind(sockfd, (struct sockaddr *)&local, addr_len) == -1) {
            sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            goto err1;
        }
        if (listen(sockfd, 1) == -1) {
            sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            goto err1;
        }
        sprintf(sbuf, "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n", ip4_addr1(ip_2_ip4(&session->netif->ip_addr)),
            ip4_addr2(ip_2_ip4(&session->netif->ip_addr)),
            ip4_addr3(ip_2_ip4(&session->netif->ip_addr)),
            ip4_addr4(ip_2_ip4(&session->netif->ip_addr)),
            dig1, dig2);
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        if (-1 != session->pasv_acpt_sockfd) {
            closesocket(session->pasv_acpt_sockfd);
        }
        session->pasv_acpt_sockfd = sockfd;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FTPS_DBG("Listening %d seconds @ port %d\n", tv.tv_sec, session->pasv_port);
        select(sockfd + 1, &readfds, 0, 0, &tv);
        if (FD_ISSET(sockfd, &readfds)) {
            if ((session->pasv_sockfd = accept(sockfd, (struct sockaddr *)&pasvremote, &addr_len)) == -1) {
                sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
                send(session->sockfd, sbuf, strlen(sbuf), 0);
                goto err1;
            } else {
                FTPS_DBG("Got Data(PASV) connection from %s\n", inet_ntoa(pasvremote.sin_addr));
                session->pasv_active = 1;
                closesocket(sockfd);
            }
        } else {
        err1:
            if (-1 != sockfd) {
                closesocket(sockfd);
            }
            if (-1 != session->pasv_sockfd) {
                closesocket(session->pasv_sockfd);
                session->pasv_sockfd = -1;
            }
            session->pasv_active = 0;
            tls_mem_free(sbuf);
            return 0;
        }
    } else if (str_begin_with(buf, "RETR") == 0) {
        int file_size;
        build_full_path(session, parameter_ptr, filename);
        file_size = ftp_get_filesize(filename);
        if (file_size == -1) {
            sprintf(sbuf, "550 \"%s\" : not a regular file\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            session->offset = 0;
            tls_mem_free(sbuf);
            return 0;
        }

        fs_user_mount_t *vfs_fs = spi_fls_vfs;

        lfs2_file_t fp;
        int res = lfs2_file_open(&vfs_fs->lfs, &fp, filename, LFS2_O_RDONLY);

        if (res != LFS2_ERR_OK) {

            tls_mem_free(sbuf);
            return 0;
        }

        if (session->offset > 0 && session->offset < file_size) {

            lfs2_file_seek(&vfs_fs->lfs, &fp, session->offset, LFS2_SEEK_SET);

            sprintf(sbuf, "150 Opening binary mode data connection for partial \"%s\" (%d/%d bytes).\r\n",
                filename, file_size - session->offset, file_size);
        } else {
            sprintf(sbuf, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n", filename, file_size);
        }
        send(session->sockfd, sbuf, strlen(sbuf), 0);


        while ((numbytes = lfs2_file_read(&vfs_fs->lfs, &fp, sbuf, FTP_BUFFER_SIZE)) > 0) {

            if (send(session->pasv_sockfd, sbuf, numbytes, 0) <= 0) {
                break;
            }
        }
        sprintf(sbuf, "226 Finished.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);

        lfs2_file_close(&vfs_fs->lfs, &fp);

        closesocket(session->pasv_sockfd);
        session->pasv_sockfd = -1;
        session->offset = 0;  // Reset offset after a download
    } else if (str_begin_with(buf, "STOR") == 0) {
        if (session->is_anonymous == TRUE) {
            sprintf(sbuf, "550 Permission denied.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        build_full_path(session, parameter_ptr, filename);

        fs_user_mount_t *vfs_fs = spi_fls_vfs;

        lfs2_file_t fp;
        int res = lfs2_file_open(&vfs_fs->lfs, &fp, filename, LFS2_O_WRONLY | LFS2_O_CREAT);
        if (res != LFS2_ERR_OK) {

            sprintf(sbuf, "550 Cannot open \"%s\" for writing.\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        sprintf(sbuf, "150 Opening binary mode data connection for \"%s\".\r\n", filename);
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        FD_ZERO(&readfds);
        FD_SET(session->pasv_sockfd, &readfds);
        FTPS_DBG("Waiting %d seconds(%d) for data...\n", tv.tv_sec, session->pasv_sockfd);
        while (select(session->pasv_sockfd + 1, &readfds, 0, 0, &tv) > 0) {
            if ((numbytes = recv(session->pasv_sockfd, sbuf, FTP_BUFFER_SIZE, 0)) > 0) {
                FTPS_DBG("numbytes = %d\r\n", numbytes);

                lfs2_file_write(&vfs_fs->lfs, &fp, sbuf, numbytes);

            } else if (numbytes == 0) {
                FTPS_DBG("numbytes = %d\r\n", numbytes);
                closesocket(session->pasv_sockfd);
                session->pasv_sockfd = -1;
                sprintf(sbuf, "226 Finished.\r\n");
                send(session->sockfd, sbuf, strlen(sbuf), 0);
                break;
            } else if (numbytes == -1) {
                FTPS_DBG("numbytes = %d\r\n", numbytes);

                lfs2_file_close(&vfs_fs->lfs, &fp);

                closesocket(session->pasv_sockfd);
                session->pasv_sockfd = -1;
                tls_mem_free(sbuf);
                return -1;
            }
        }

        lfs2_file_close(&vfs_fs->lfs, &fp);

        closesocket(session->pasv_sockfd);
        session->pasv_sockfd = -1;
    } else if (str_begin_with(buf, "SIZE") == 0) {
        int file_size;
        build_full_path(session, parameter_ptr, filename);
        file_size = ftp_get_filesize(filename);
        if (file_size == -1) {
            sprintf(sbuf, "550 \"%s\" : not a regular file\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else {
            sprintf(sbuf, "213 %d\r\n", file_size);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        }
    } else if (str_begin_with(buf, "MDTM") == 0) {
        sprintf(sbuf, "550 \"/\" : not a regular file\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "SYST") == 0) {
        sprintf(sbuf, "215 %s\r\n", "UNIX system type: W600 FreeRTOS");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "CWD") == 0) {
        // Test for path existence by using filesize, which in turn calls stat()
        build_full_path(session, parameter_ptr, filename);
        if (is_dir(filename)) {
            strcpy(session->currentdir, filename);
            sprintf(sbuf, "250 Changed to directory \"%s\"\r\n", filename);
        } else {
            sprintf(sbuf, "550 Directory \"%s\" does not exist\r\n", filename);
        }
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        FTPS_DBG("Changed to directory %s", filename);
    } else if (str_begin_with(buf, "CDUP") == 0) {
        build_full_path(session, "..", filename);
        sprintf(sbuf, "250 Changed to directory \"%s\"\r\n", filename);
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        strcpy(session->currentdir, filename);
        FTPS_DBG("Changed to directory %s", filename);
    } else if (str_begin_with(buf, "PORT") == 0) {
        int i;
        int portcom[6];
        char tmpip[100];

        i = 0;
        portcom[i++] = atoi(strtok(parameter_ptr, ".,;()"));
        for (; i < 6; i++) {
            portcom[i] = atoi(strtok(0, ".,;()"));
        }
        sprintf(tmpip, "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

        FD_ZERO(&readfds);
        if (-1 != session->pasv_sockfd) {
            closesocket(session->pasv_sockfd);
        }
        if ((session->pasv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            closesocket(session->pasv_sockfd);
            session->pasv_sockfd = -1;
            session->pasv_active = 0;
            tls_mem_free(sbuf);
            return 0;
        }
        FTPS_DBG("pacvfd=%d\r\n", session->pasv_sockfd);
        pasvremote.sin_addr.s_addr = inet_addr(tmpip);
        pasvremote.sin_port = htons(portcom[4] * 256 + portcom[5]);
        pasvremote.sin_family = PF_INET;
        if (connect(session->pasv_sockfd, (struct sockaddr *)&pasvremote, addr_len) == -1) {
            // is it only local address?try using gloal ip addr
            FTPS_DBG("connect failed\r\n");
            pasvremote.sin_addr = session->remote.sin_addr;
            if (connect(session->pasv_sockfd, (struct sockaddr *)&pasvremote, addr_len) == -1) {
                sprintf(sbuf, "425 Can't open data connection %d.\r\n", __LINE__);
                send(session->sockfd, sbuf, strlen(sbuf), 0);
                closesocket(session->pasv_sockfd);
                session->pasv_sockfd = -1;
                tls_mem_free(sbuf);
                return 0;
            }
        }
        session->pasv_active = 1;
        session->pasv_port = portcom[4] * 256 + portcom[5];
        FTPS_DBG("Connected to Data(PORT) %s @ %d\n", tmpip, portcom[4] * 256 + portcom[5]);
        sprintf(sbuf, "200 Port Command Successful.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "REST") == 0) {
        if (atoi(parameter_ptr) >= 0) {
            session->offset = atoi(parameter_ptr);
            sprintf(sbuf, "350 Send RETR or STOR to start a transfer.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        }
    } else if (str_begin_with(buf, "MKD") == 0) {
        if (session->is_anonymous == TRUE) {
            sprintf(sbuf, "550 Permission denied.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        build_full_path(session, parameter_ptr, filename);
        fs_user_mount_t *vfs_fs = spi_fls_vfs;

        int res = lfs2_mkdir(&vfs_fs->lfs, filename);

        if (res != LFS2_ERR_OK) {

            sprintf(sbuf, "550 File \"%s\" exists.\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else {
            sprintf(sbuf, "257 directory \"%s\" successfully created.\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        }
    } else if (str_begin_with(buf, "DELE") == 0) {
        if (session->is_anonymous == TRUE) {
            sprintf(sbuf, "550 Permission denied.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        build_full_path(session, parameter_ptr, filename);
        fs_user_mount_t *vfs_fs = spi_fls_vfs;

        int res = lfs2_remove(&vfs_fs->lfs, filename);
        if (res == LFS2_ERR_NOTEMPTY) {
            printf("Dir is not empty\r\n");
        }

        if (res == LFS2_ERR_OK) {

            sprintf(sbuf, "250 Successfully deleted file \"%s\".\r\n", filename);
        } else {
            sprintf(sbuf, "550 Not such file or directory: %s.\r\n", filename);
        }
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "RMD") == 0) {
        if (session->is_anonymous == TRUE) {
            sprintf(sbuf, "550 Permission denied.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        build_full_path(session, parameter_ptr, filename);
        fs_user_mount_t *vfs_fs = spi_fls_vfs;

        int res = lfs2_remove(&vfs_fs->lfs, filename);
        if (res == LFS2_ERR_NOTEMPTY) {
            printf("Dir is not empty\r\n");
        }

        if (res != LFS2_ERR_OK) {

            sprintf(sbuf, "550 Directory \"%s\" doesn't exist.\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        } else {
            sprintf(sbuf, "257 directory \"%s\" successfully deleted.\r\n", filename);
            send(session->sockfd, sbuf, strlen(sbuf), 0);
        }
    } else if (str_begin_with(buf, "RNFR") == 0) {
        if (session->is_anonymous == TRUE) {
            sprintf(sbuf, "550 Permission denied.\r\n");
            send(session->sockfd, sbuf, strlen(sbuf), 0);
            tls_mem_free(sbuf);
            return 0;
        }
        build_full_path(session, parameter_ptr, session->rename);
        sprintf(sbuf, "350 Requested file action pending further information.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "RNTO") == 0) {
        fs_user_mount_t *vfs_fs = spi_fls_vfs;
        build_full_path(session, parameter_ptr, filename);

        int res = lfs2_rename(&vfs_fs->lfs, session->rename, filename);
        if (res == LFS2_ERR_NOTEMPTY) {
            printf("Dir is not empty\r\n");
        }
        if (res != LFS2_ERR_OK) {

            sprintf(sbuf, "550 rename err.\r\n");
        } else {
            sprintf(sbuf, "200 command successful.\r\n");
        }
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else if (str_begin_with(buf, "QUIT") == 0) {
        sprintf(sbuf, "221 Bye!\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
        tls_mem_free(sbuf);
        return -1;
    } else if ((str_begin_with(buf, "NOOP") == 0) || (str_begin_with(buf, "noop") == 0)) {
        sprintf(sbuf, "200 Command okay.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    } else {
        sprintf(sbuf, "502 Not Implemented.\r\n");
        send(session->sockfd, sbuf, strlen(sbuf), 0);
    }
    tls_mem_free(sbuf);
    return 0;
}

MP_REGISTER_ROOT_POINTER(unsigned char *mpy_ftps_stk);
