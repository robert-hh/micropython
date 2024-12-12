/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/bc.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "extmod/vfs.h"
#include "extmod/vfs_rom.h"

#if MICROPY_VFS_ROM

#define MAGIC_BYTE0 (0x80 | 'R')
#define MAGIC_BYTE1 (0x80 | 'M')
#define MAGIC_BYTE2 (0x00 | '1')

struct _mp_obj_vfs_rom_t {
    mp_obj_base_t base;
    mp_obj_t memory;
    const uint8_t *filesystem;
    const uint8_t *filesystem_top;
};

mp_import_stat_t mp_vfs_rom_search_filesystem(mp_obj_vfs_rom_t *self, const char *path, size_t *size_out, const uint8_t **data_out) {
    size_t path_len = strlen(path);
    const uint8_t *fs = self->filesystem;
    const uint8_t *fs_top = self->filesystem_top;
    /*
    if (*path == '/') {
        ++path;
        --path_len;
    }
    if (path_len == 0) {
        if (size_out != NULL) {
            *size_out = fs_top - fs;
            *data_out = fs;
        }
        return MP_IMPORT_STAT_DIR;
    }
    */
    while (fs < fs_top) {
        mp_uint_t record_kind = mp_decode_uint(&fs);
        mp_uint_t record_len = mp_decode_uint(&fs);
        const uint8_t *fs_next = fs + record_len;
        if (record_kind == 1) {
            // A directory.
            mp_uint_t name_len = mp_decode_uint(&fs);
            if (name_len <= path_len && (path[name_len] == '\0' || path[name_len] == '/') && memcmp(path, fs, name_len) == 0) {
                // Enter this directory.
                fs += name_len;
                fs_top = fs_next;
                path += name_len;
                path_len -= name_len;
                if (*path == '/') {
                    ++path;
                    --path_len;
                }
                if (path_len == 0) {
                    if (size_out != NULL) {
                        *size_out = fs_top - fs;
                        *data_out = fs;
                    }
                    return MP_IMPORT_STAT_DIR;
                }
            } else {
                // Skip this directory.
                fs = fs_next;
            }
        } else if (record_kind == 2) {
            // A file.
            mp_uint_t name_len = mp_decode_uint(&fs);
            if (name_len == path_len && memcmp(path, fs, path_len) == 0) {
                // Return this file.
                fs += name_len;
                if (size_out != NULL) {
                    *size_out = mp_decode_uint(&fs);
                    *data_out = fs;
                }
                return MP_IMPORT_STAT_FILE;
            } else {
                // Skip this file.
                fs = fs_next;
            }
        } else {
            // Skip this record.
            fs = fs_next;
        }
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

static inline const char *vfs_rom_get_path_str(mp_obj_vfs_rom_t *self, mp_obj_t path) {
    return mp_obj_str_get_str(path);
}

static mp_obj_t vfs_rom_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_vfs_rom_t *self = m_new_obj(mp_obj_vfs_rom_t);
    self->base.type = type;
    self->memory = args[0];

    mp_buffer_info_t bufinfo;
    if (mp_get_buffer(self->memory, &bufinfo, MP_BUFFER_READ)) {
        self->filesystem = bufinfo.buf;
    } else {
        self->filesystem = (const uint8_t *)(uintptr_t)mp_obj_get_int_truncated(self->memory);
    }

    // Verify it is a ROMFS.
    if (!(self->filesystem[0] == MAGIC_BYTE0
          && self->filesystem[1] == MAGIC_BYTE1
          && self->filesystem[2] == MAGIC_BYTE2)) {
        mp_raise_OSError(MP_ENODEV);
    }

    mp_decode_uint(&self->filesystem);
    mp_uint_t len = mp_decode_uint(&self->filesystem);
    self->filesystem_top = self->filesystem + len;

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t vfs_rom_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs) {
    (void)self_in;
    (void)readonly;
    if (mp_obj_is_true(mkfs)) {
        mp_raise_OSError(MP_EPERM);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_rom_mount_obj, vfs_rom_mount);

static mp_obj_t vfs_rom_umount(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(vfs_rom_umount_obj, vfs_rom_umount);

// mp_vfs_rom_file_open is implemented in vfs_rom_file.c.
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_rom_open_obj, mp_vfs_rom_file_open);

static mp_obj_t vfs_rom_chdir(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_rom_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = vfs_rom_get_path_str(self, path_in);
    if (path[0] == '/' && path[1] == '\0') {
        // Allow chdir to the root of the filesystem.
    } else {
        // Don't allow chdir to any subdirectory (not currently implemented).
        mp_raise_OSError(MP_EOPNOTSUPP);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_rom_chdir_obj, vfs_rom_chdir);

typedef struct _vfs_rom_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    bool is_str;
    const uint8_t *index;
    const uint8_t *index_top;
} vfs_rom_ilistdir_it_t;

static mp_obj_t vfs_rom_ilistdir_it_iternext(mp_obj_t self_in) {
    vfs_rom_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);

    while (self->index < self->index_top) {
        mp_uint_t record_kind = mp_decode_uint(&self->index);
        mp_uint_t record_len = mp_decode_uint(&self->index);
        const uint8_t *index_next = self->index + record_len;
        mp_uint_t name_len;
        mp_uint_t data_len;
        uint32_t type;
        if (record_kind == 1) {
            // A directory.
            type = MP_S_IFDIR;
            name_len = mp_decode_uint(&self->index);
            data_len = index_next - self->index - name_len;
        } else if (record_kind == 2) {
            // A file.
            type = MP_S_IFREG;
            name_len = mp_decode_uint(&self->index);
            data_len = mp_decode_uint_value(self->index + name_len);
        } else {
            // Skip this record.
            self->index = index_next;
            continue;
        }

        const uint8_t *name_str = self->index;
        self->index = index_next;

        // Make 4-tuple with info about this entry: (name, attr, inode, size)
        mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(4, NULL));

        if (self->is_str) {
            t->items[0] = mp_obj_new_str((const char *)name_str, name_len);
        } else {
            t->items[0] = mp_obj_new_bytes(name_str, name_len);
        }

        t->items[1] = MP_OBJ_NEW_SMALL_INT(type);
        t->items[2] = MP_OBJ_NEW_SMALL_INT(0);
        t->items[3] = mp_obj_new_int(data_len);

        return MP_OBJ_FROM_PTR(t);
    }

    return MP_OBJ_STOP_ITERATION;
}

static mp_obj_t vfs_rom_ilistdir(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_rom_t *self = MP_OBJ_TO_PTR(self_in);
    vfs_rom_ilistdir_it_t *iter = m_new_obj(vfs_rom_ilistdir_it_t);
    iter->base.type = &mp_type_polymorph_iter;
    iter->iternext = vfs_rom_ilistdir_it_iternext;
    iter->is_str = mp_obj_get_type(path_in) == &mp_type_str;
    const char *path = vfs_rom_get_path_str(self, path_in);
    size_t size;
    if (mp_vfs_rom_search_filesystem(self, path, &size, &iter->index) != MP_IMPORT_STAT_DIR) {
        mp_raise_OSError(MP_ENOENT);
    }
    iter->index_top = iter->index + size;
    return MP_OBJ_FROM_PTR(iter);
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_rom_ilistdir_obj, vfs_rom_ilistdir);

static mp_obj_t vfs_rom_stat(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_rom_t *self = MP_OBJ_TO_PTR(self_in);
    const char *path = vfs_rom_get_path_str(self, path_in);
    size_t file_size;
    const uint8_t *file_data;
    mp_import_stat_t stat = mp_vfs_rom_search_filesystem(self, path, &file_size, &file_data);
    if (stat == MP_IMPORT_STAT_NO_EXIST) {
        mp_raise_OSError(MP_ENOENT);
    }
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(stat == MP_IMPORT_STAT_FILE ? MP_S_IFREG : MP_S_IFDIR); // st_mode
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0); // st_ino
    t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // st_dev
    t->items[3] = MP_OBJ_NEW_SMALL_INT(0); // st_nlink
    t->items[4] = MP_OBJ_NEW_SMALL_INT(0); // st_uid
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // st_gid
    t->items[6] = MP_OBJ_NEW_SMALL_INT(file_size); // st_size
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // st_atime
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // st_mtime
    t->items[9] = MP_OBJ_NEW_SMALL_INT(0); // st_ctime
    return MP_OBJ_FROM_PTR(t);
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_rom_stat_obj, vfs_rom_stat);

static mp_obj_t vfs_rom_statvfs(mp_obj_t self_in, mp_obj_t path_in) {
    mp_obj_vfs_rom_t *self = self_in;
    (void)path_in;
    size_t filesystem_len = self->filesystem_top - self->filesystem;
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(1); // f_bsize
    t->items[1] = MP_OBJ_NEW_SMALL_INT(0); // f_frsize
    t->items[2] = mp_obj_new_int_from_uint(filesystem_len); // f_blocks
    t->items[3] = MP_OBJ_NEW_SMALL_INT(0); // f_bfree
    t->items[4] = MP_OBJ_NEW_SMALL_INT(0); // f_bavail
    t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // f_files
    t->items[6] = MP_OBJ_NEW_SMALL_INT(0); // f_ffree
    t->items[7] = MP_OBJ_NEW_SMALL_INT(0); // f_favail
    t->items[8] = MP_OBJ_NEW_SMALL_INT(0); // f_flags
    t->items[9] = MP_OBJ_NEW_SMALL_INT(32767); // f_namemax
    return MP_OBJ_FROM_PTR(t);
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_rom_statvfs_obj, vfs_rom_statvfs);

static const mp_rom_map_elem_t vfs_rom_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_rom_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&vfs_rom_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&vfs_rom_open_obj) },

    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&vfs_rom_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&vfs_rom_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&vfs_rom_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&vfs_rom_statvfs_obj) },
};
static MP_DEFINE_CONST_DICT(vfs_rom_locals_dict, vfs_rom_locals_dict_table);

static mp_import_stat_t mp_vfs_rom_import_stat(void *self_in, const char *path) {
    mp_obj_vfs_rom_t *self = self_in;
    return mp_vfs_rom_search_filesystem(self, path, NULL, NULL);
}

static const mp_vfs_proto_t vfs_rom_proto = {
    .import_stat = mp_vfs_rom_import_stat,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_rom,
    MP_QSTR_VfsRom,
    MP_TYPE_FLAG_NONE,
    make_new, vfs_rom_make_new,
    protocol, &vfs_rom_proto,
    locals_dict, &vfs_rom_locals_dict
    );

#endif // MICROPY_VFS_ROM
