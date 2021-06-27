
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Philipp Ebensberger
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

#include "py/runtime.h"
#include "extmod/vfs.h"
#include "fsl_iomuxc.h"
#include "fsl_cache.h"
#include "fsl_usdhc.h"

#include "ticks.h"
#include "modmachine.h"


#define SDCARD_INIT_ARG_ID          (0)
#define SDMMC_VOLTAGE_WINDOW_SD     (0x80100000U)
#define SDMMC_HIGH_CAPACITY         (0x40000000U)
#define SD_SWITCH_1_8V_CAPACITY     ((uint32_t)0x01000000U)
#define SDMMC_MAX_VOLT_TRIAL        ((uint32_t)0x000000FFU)
#define SDMMC_DEFAULT_BLOCK_SIZE    (512U)

// Error
#define SDCARD_STATUS_OUT_OF_RANGE_SHIFT        (31U)
#define SDCARD_STATUS_ADDRESS_ERROR_SHIFT       (30U)
#define SDCARD_STATUS_BLOCK_LEN_ERROR_SHIFT     (29U)
#define SDCARD_STATUS_ERASE_SEQ_ERROR_SHIFT     (28U)
#define SDCARD_STATUS_ERASE_PARAM_SHIFT         (27U)
#define SDCARD_STATUS_WP_VIOLATION_SHIFT        (26U)
#define SDCARD_STATUS_LOCK_UNLOCK_FAILED_SHIFT  (24U)
#define SDCARD_STATUS_COM_CRC_ERROR_SHIFT       (23U)
#define SDCARD_STATUS_ILLEGAL_COMMAND_SHIFT     (22U)
#define SDCARD_STATUS_CARD_ECC_FAILED_SHIFT     (21U)
#define SDCARD_STATUS_CC_ERROR_SHIFT            (20U)
#define SDCARD_STATUS_ERROR_SHIFT               (19U)
#define SDCARD_STATUS_CSD_OVERWRITE_SHIFT       (16U)
#define SDCARD_STATUS_WP_ERASE_SKIP_SHIFT       (15U)
#define SDCARD_STATUS_AUTH_SEQ_ERR_SHIFT        (3U)

// Status Flags
#define SDCARD_STATUS_CARD_IS_LOCKED_SHIFT      (25U)
#define SDCARD_STATUS_CARD_ECC_DISABLED_SHIFT   (14U)
#define SDCARD_STATUS_ERASE_RESET_SHIFT         (13U)
#define SDCARD_STATUS_READY_FOR_DATA_SHIFT      (8U)
#define SDCARD_STATUS_FX_EVENT_SHIFT            (6U)
#define SDCARD_STATUS_APP_CMD_SHIFT             (5U)


#define SDMMC_MASK(bit) (1U << (bit))
#define SDMMC_R1_ALL_ERROR_FLAG \
            (SDMMC_MASK(SDCARD_STATUS_OUT_OF_RANGE_SHIFT))          | \
            (SDMMC_MASK(SDCARD_STATUS_ADDRESS_ERROR_SHIFT))         | \
            (SDMMC_MASK(SDCARD_STATUS_BLOCK_LEN_ERROR_SHIFT))       | \
            (SDMMC_MASK(SDCARD_STATUS_ERASE_SEQ_ERROR_SHIFT))       | \
            (SDMMC_MASK(SDCARD_STATUS_ERASE_PARAM_SHIFT))           | \
            (SDMMC_MASK(SDCARD_STATUS_WP_VIOLATION_SHIFT))          | \
            (SDMMC_MASK(SDCARD_STATUS_LOCK_UNLOCK_FAILED_SHIFT))    | \
            (SDMMC_MASK(SDCARD_STATUS_COM_CRC_ERROR_SHIFT))         | \
            (SDMMC_MASK(SDCARD_STATUS_ILLEGAL_COMMAND_SHIFT))       | \
            (SDMMC_MASK(SDCARD_STATUS_CARD_ECC_FAILED_SHIFT))       | \
            (SDMMC_MASK(SDCARD_STATUS_CC_ERROR_SHIFT))              | \
            (SDMMC_MASK(SDCARD_STATUS_ERROR_SHIFT))                 | \
            (SDMMC_MASK(SDCARD_STATUS_CSD_OVERWRITE_SHIFT))         | \
            (SDMMC_MASK(SDCARD_STATUS_WP_ERASE_SKIP_SHIFT))         | \
            (SDMMC_MASK(SDCARD_STATUS_AUTH_SEQ_ERR_SHIFT))

#define SDMMC_R1_CURRENT_STATE(x) (((x) & 0x00001E00U) >> 9U)


typedef struct _mimxrt_sdcard_obj_t {
    mp_obj_base_t base;
    USDHC_Type *sdcard;
    bool initialized;
    bool inserted;
    uint32_t rca;
    uint16_t block_len;
    uint32_t block_count;
} mimxrt_sdcard_obj_t;

typedef struct _cid_t {
    uint8_t reserved_0;
    uint16_t mdt : 12;
    uint16_t reserved_1 : 4;
    uint32_t psn;
    uint8_t prv;
    char pnm[6];
    uint16_t oid;
    uint8_t mid;
} __attribute__((packed)) cid_t;

typedef struct _csd_t {
    uint32_t reserved_0;
    uint32_t reserved_1 : 2;
    uint32_t file_format : 2;
    uint32_t temp_write_protect : 1;
    uint32_t perm_write_protecct : 1;
    uint32_t copy : 1;
    uint32_t file_format_grp : 1;
    uint32_t reserved_2 : 5;
    uint32_t write_bl_partial : 1;
    uint32_t write_bl_len : 4;
    uint32_t r2w_factor : 3;
    uint32_t reserved_3 : 2;
    uint32_t wr_grp_enable : 1;
    uint64_t wp_grp_size : 7;
    uint64_t sector_size : 7;
    uint64_t erase_blk_en : 1;
    uint64_t c_size_mult : 3;
    uint64_t vdd_w_curr_max : 3;
    uint64_t vdd_w_curr_min : 3;
    uint64_t vdd_r_curr_max : 3;
    uint64_t vdd_r_curr_min : 3;
    uint64_t c_size : 12;
    uint64_t reserved_4 : 2;
    uint64_t dsr_imp : 1;
    uint64_t read_blk_misalign : 1;
    uint64_t write_blk_misalign : 1;
    uint64_t read_bl_partial : 1;
    uint64_t read_bl_len : 4;
    uint64_t ccc : 12;
    uint32_t tran_speed : 8;
    uint32_t nsac : 8;
    uint32_t taac : 8;
    uint32_t reserved_5 : 6;
    uint32_t csd_structure : 2;
} __attribute__((packed)) csd_t;

typedef enum _sdcard_state_t
{
    SDCARD_STATE_IDLE        = 0U,
    SDCARD_STATE_READY       = 1U,
    SDCARD_STATE_IDENTIFY    = 2U,
    SDCARD_STATE_STANDBY     = 3U,
    SDCARD_STATE_TRANSFER    = 4U,
    SDCARD_STATE_SENDDATA    = 5U,
    SDCARD_STATE_RECEIVEDATA = 6U,
    SDCARD_STATE_PROGRAM     = 7U,
    SDCARD_STATE_DISCONNECT  = 8U,
} sdcard_state_t;

enum
{
    SDCARD_CMD_GO_IDLE_STATE        = 0U,
    SDCARD_CMD_ALL_SEND_CID         = 2U,
    SDCARD_CMD_SEND_REL_ADDR        = 3U,
    SDCARD_CMD_SET_DSR              = 4U,
    SDCARD_CMD_SELECT_CARD          = 7U,
    SDCARD_CMD_SEND_IF_COND         = 8U,
    SDCARD_CMD_SEND_CSD             = 9U,
    SDCARD_CMD_SEND_CID             = 10U,
    SDCARD_CMD_STOP_TRANSMISSION    = 12U,
    SDCARD_CMD_SEND_STATUS          = 13U,
    SDCARD_CMD_GO_INACTIVE_STATE    = 15U,
    SDCARD_CMD_SET_BLOCKLENGTH      = 16U,
    SDCARD_CMD_READ_SINGLE_BLOCK    = 17U,
    SDCARD_CMD_READ_MULTIPLE_BLOCK  = 18U,
    SDCARD_CMD_SET_BLOCK_COUNT      = 23U,
    SDCARD_CMD_WRITE_SINGLE_BLOCK   = 24U,
    SDCARD_CMD_WRITE_MULTIPLE_BLOCK = 25U,
    SDCARD_CMD_PROGRAM_CSD          = 27U,
    SDCARD_CMD_SET_WRITE_PROTECT    = 28U,
    SDCARD_CMD_CLEAR_WRITE_PROTECT  = 29U,
    SDCARD_CMD_SEND_WRITE_PROTECT   = 30U,
    SDCARD_CMD_ERASE                = 38U,
    SDCARD_CMD_LOCK_UNLOCK          = 42U,
    SDCARD_CMD_APP_CMD              = 55U,
    SDCARD_CMD_GEN_CMD              = 56U,
    SDCARD_CMD_READ_OCR             = 58U,
};

enum
{
    SDCARD_ACMD_SD_SEND_OP_COND     = 41U,
};



STATIC mimxrt_sdcard_obj_t mimxrt_sdcard_objs[1] = {
    {
        .base.type = &machine_sdcard_type,
        .sdcard = USDHC1,
        .initialized = false,
        .inserted = true,
        .rca = 0x0UL,
        .block_len = SDMMC_DEFAULT_BLOCK_SIZE,
        .block_count = 0UL,
    }
};

STATIC const mp_arg_t allowed_args[] = {
    [SDCARD_INIT_ARG_ID]     { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
};


static void decode_csd(mimxrt_sdcard_obj_t *sdcard, csd_t *csd) {
    sdcard->block_len = (1U << (csd->read_bl_len));
    sdcard->block_count = ((csd->c_size + 1U) << (csd->c_size_mult + 2U));

    if (sdcard->block_len != SDMMC_DEFAULT_BLOCK_SIZE) {
        sdcard->block_count = (sdcard->block_count * sdcard->block_len);
        sdcard->block_len = SDMMC_DEFAULT_BLOCK_SIZE;
        sdcard->block_count = (sdcard->block_count / sdcard->block_len);
    }
}

static void sdcard_check_status(status_t status, uint8_t command) {
    if (status != kStatus_Success) {
        mp_printf(&mp_plat_print, "CMD %d failed!", command);
        switch (status)
        {
            case (kStatus_USDHC_BusyTransferring): {
                mp_printf(&mp_plat_print, " - BusyTransferring (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_PrepareAdmaDescriptorFailed): {
                mp_printf(&mp_plat_print, " - PrepareAdmaDescriptorFailed (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_SendCommandFailed): {
                mp_printf(&mp_plat_print, " - SendCommandFailed (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_TransferDataFailed): {
                mp_printf(&mp_plat_print, " - TransferDataFailed (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_DMADataAddrNotAlign): {
                mp_printf(&mp_plat_print, " - DMADataAddrNotAlign (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_ReTuningRequest): {
                mp_printf(&mp_plat_print, " - ReTuningRequest (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_TuningError): {
                mp_printf(&mp_plat_print, " - TuningError (%d)\n", status);
                break;
            }
            case (kStatus_USDHC_NotSupport): {
                mp_printf(&mp_plat_print, " - NotSupport (%d)\n", status);
                break;
            }
            default: {
                mp_printf(&mp_plat_print, " - Unknown\n");
                break;
            }
        }
    }
}

static void sdcard_card_status(uint8_t current_state) {
    switch (current_state)
    {
        case SDCARD_STATE_IDLE: {
            mp_printf(&mp_plat_print, "Card State: StateIdle\n");
            break;
        }
        case SDCARD_STATE_READY: {
            mp_printf(&mp_plat_print, "Card State: StateReady\n");
            break;
        }
        case SDCARD_STATE_IDENTIFY: {
            mp_printf(&mp_plat_print, "Card State: StateIdentify\n");
            break;
        }
        case SDCARD_STATE_STANDBY: {
            mp_printf(&mp_plat_print, "Card State: StateStandby\n");
            break;
        }
        case SDCARD_STATE_TRANSFER: {
            mp_printf(&mp_plat_print, "Card State: StateTransfer\n");
            break;
        }
        case SDCARD_STATE_SENDDATA: {
            mp_printf(&mp_plat_print, "Card State: StateSendData\n");
            break;
        }
        case SDCARD_STATE_RECEIVEDATA: {
            mp_printf(&mp_plat_print, "Card State: StateReceiveData\n");
            break;
        }
        case SDCARD_STATE_PROGRAM: {
            mp_printf(&mp_plat_print, "Card State: StateProgram\n");
            break;
        }
        case SDCARD_STATE_DISCONNECT: {
            mp_printf(&mp_plat_print, "Card State: StateDisconnect\n");
            break;
        }
        default: {
            mp_printf(&mp_plat_print, "Card State: INVALID!!!\n");
        }
    }
}

static bool sdcard_cmd_go_idle_state(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_GO_IDLE_STATE,
        .argument = 0UL,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeNone,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_GO_IDLE_STATE);

    if(status == kStatus_Success)
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool sdcard_cmd_oper_cond(USDHC_Type *base, uint32_t *oper_cond) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SEND_IF_COND,
        .argument = 0x000001AAU,       // 2.7-3.3V range and 0xAA check pattern
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR7,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SEND_IF_COND);
    
    if (status == kStatus_Success)
    {
        *oper_cond = command.response[0];
        return true;
    }
    else
    {
        return false;
    }

}

static uint32_t sdcard_cmd_app_cmd(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_APP_CMD,
        .argument = 0UL,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_APP_CMD);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static uint32_t sdcard_cmd_sd_app_op_cond(USDHC_Type *base, uint32_t argument) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_ACMD_SD_SEND_OP_COND,
        .argument = argument,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR3,
    };

    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_ACMD_SD_SEND_OP_COND);
    return command.response[0];
}

static bool sdcard_cmd_all_send_cid(USDHC_Type *base, cid_t *cid) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_ALL_SEND_CID,
        .argument = 0UL,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR2,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_ALL_SEND_CID);
    
    if(status == kStatus_Success) {
        cid->mdt = (uint16_t)((command.response[0] & 0xFFF00U) >> 8U);
        cid->psn = (uint32_t)(((command.response[1] & 0xFFFFFFU) << 8U) | ((command.response[0] & 0xFF000000U) >> 24U));
        cid->prv = (uint8_t)((command.response[1] & 0xFF000000U) >> 24U);
        cid->pnm[0] = (char)(command.response[2] & 0xFFU);
        cid->pnm[1] = (char)((command.response[2] & 0xFF00U) >> 8U);
        cid->pnm[2] = (char)((command.response[2] & 0xFF0000U) >> 16U);
        cid->pnm[3] = (char)((command.response[2] & 0xFF000000U) >> 24U);
        cid->pnm[4] = (char)(command.response[3] & 0xFFU);
        cid->pnm[5] = '\0';
        cid->oid = (uint16_t)((command.response[3] & 0xFFFF00U) >> 8U);
        cid->mid = (uint8_t)((command.response[3] & 0xFF000000U) >> 24U);  
        return true;      
    }
    else
    {
        return false;
    }
}

static bool sdcard_cmd_send_cid(USDHC_Type *base, const uint32_t rca, cid_t *cid) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SEND_CID,
        .argument = (rca << 16),
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR2,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SEND_CID);

    if(status == kStatus_Success) {
        cid->mdt = (uint16_t)((command.response[0] & 0xFFF00U) >> 8U);
        cid->psn = (uint32_t)(((command.response[1] & 0xFFFFFFU) << 8U) | ((command.response[0] & 0xFF000000U) >> 24U));
        cid->prv = (uint8_t)((command.response[1] & 0xFF000000U) >> 24U);
        cid->pnm[0] = (char)(command.response[2] & 0xFFU);
        cid->pnm[1] = (char)((command.response[2] & 0xFF00U) >> 8U);
        cid->pnm[2] = (char)((command.response[2] & 0xFF0000U) >> 16U);
        cid->pnm[3] = (char)((command.response[2] & 0xFF000000U) >> 24U);
        cid->pnm[4] = (char)(command.response[3] & 0xFFU);
        cid->pnm[5] = '\0';
        cid->oid = (uint16_t)((command.response[3] & 0xFFFF00U) >> 8U);
        cid->mid = (uint8_t)((command.response[3] & 0xFF000000U) >> 24U);
        return true;
    }
    else
    {
        return false;
    }
}

static uint32_t sdcard_send_status(USDHC_Type *base, uint32_t rca) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SEND_STATUS,
        .argument = (rca << 16),
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SEND_STATUS);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static bool sdcard_cmd_set_rel_add(USDHC_Type *base, uint32_t *rca) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SEND_REL_ADDR,
        .argument = 0UL,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR6,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SEND_REL_ADDR);
    
    if (status == kStatus_Success)
    {
        *rca = 0xFFFFFFFF & (command.response[0] >> 16);
        return true;
    }
    else
    {
        return false;
    }
}

static bool sdcard_cmd_send_csd(USDHC_Type *base, const uint32_t rca, csd_t *csd) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SEND_CSD,
        .argument = (rca << 16),
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR2,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SEND_CSD);
    
    if (status == kStatus_Success)
    {
        csd->csd_structure = 0x3 & (command.response[3] >> 30);
        csd->taac = 0xFF & (command.response[3] >> 16);
        csd->nsac = 0xFF & (command.response[3] >> 8);
        csd->tran_speed = 0xFF & (command.response[3]);

        csd->ccc = 0xFFF & (command.response[2] >> 20);
        csd->read_bl_len = 0xF & (command.response[2] >> 16);
        csd->read_bl_partial = 0x1 & (command.response[2] >> 15);
        csd->write_blk_misalign = 0x1 & (command.response[2] >> 14);
        csd->read_blk_misalign = 0x1 & (command.response[2] >> 13);
        csd->dsr_imp = 0x1 & (command.response[2] >> 12);
        csd->c_size = (0x3FF & (command.response[2] << 2)) |
            (0x3 & (command.response[1] >> 30));

        csd->vdd_r_curr_min = 0x7 & (command.response[1] >> 27);
        csd->vdd_r_curr_max = 0x7 & (command.response[1] >> 24);
        csd->vdd_w_curr_min = 0x7 & (command.response[1] >> 21);
        csd->vdd_w_curr_max = 0x7 & (command.response[1] >> 18);
        csd->c_size_mult = 0x7 & (command.response[1] >> 15);
        csd->erase_blk_en = 0x1 & (command.response[1] >> 14);
        csd->sector_size = 0x7F & (command.response[1] >> 7);
        csd->wp_grp_size = 0x7F & (command.response[1]);

        csd->wr_grp_enable = 0x1 & (command.response[0] >> 31);
        csd->r2w_factor = 0x7 & (command.response[0] >> 26);
        csd->write_bl_len = 0xF & (command.response[0] >> 22);
        csd->write_bl_partial = 0x1 & (command.response[0] >> 21);
        csd->file_format_grp = 0x1 & (command.response[0] >> 15);
        csd->copy = 0x1 & (command.response[0] >> 14);
        csd->perm_write_protecct = 0x1 & (command.response[0] >> 13);
        csd->temp_write_protect = 0x1 & (command.response[0] >> 12);
        csd->file_format = 0x3 & (command.response[0] >> 10);

        return true;
    }
    else
    {
        return false;
    }
}

static bool sdcard_cmd_select_card(USDHC_Type *base, const uint32_t rca) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SELECT_CARD,
        .argument = (rca << 16),
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1b,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };
    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SELECT_CARD);

    if(status == kStatus_Success) {
        sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
        return true;
    }
    else
    {
        return false;
    }
}

static uint32_t sdcard_cmd_stop_transmission(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_STOP_TRANSMISSION,
        .argument = 0UL,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1b,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };
    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_STOP_TRANSMISSION);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static uint32_t sdcard_cmd_set_blocklen(USDHC_Type *base, uint32_t block_size) {
    status_t status;
    usdhc_command_t command = {
        .index = SDCARD_CMD_SET_BLOCKLENGTH,
        .argument = block_size,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };
    usdhc_transfer_t transfer = {
        .data = NULL,
        .command = &command,
    };
    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, SDCARD_CMD_SET_BLOCKLENGTH);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static status_t sdcard_read(mimxrt_sdcard_obj_t *sdcard, uint8_t *buffer, uint32_t block_num, uint32_t block_count) {
    usdhc_data_t data = {
        .enableAutoCommand12 = true,
        .enableAutoCommand23 = false,
        .enableIgnoreError = false,
        .dataType = kUSDHC_TransferDataNormal,
        .blockSize = sdcard->block_len,
        .blockCount = block_count,
        .rxData = (uint32_t *)buffer,
        .txData = NULL,
    };

    usdhc_command_t command = {
        .index = (block_count == 1U) ? (uint32_t)SDCARD_CMD_READ_SINGLE_BLOCK : (uint32_t)SDCARD_CMD_READ_MULTIPLE_BLOCK,
        .argument = block_num,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };

    usdhc_transfer_t transfer = {
        .data = &data,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);

    status_t status = USDHC_TransferBlocking(sdcard->sdcard, NULL, &transfer);

    sdcard_check_status(status, command.index);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));

    return status;
}

static status_t sdcard_write(mimxrt_sdcard_obj_t *sdcard, uint8_t *buffer, uint32_t block_num, uint32_t block_count) {
    usdhc_data_t data = {
        .enableAutoCommand12 = true,
        .enableAutoCommand23 = false,
        .enableIgnoreError = false,
        .dataType = kUSDHC_TransferDataNormal,
        .blockSize = sdcard->block_len,
        .blockCount = block_count,
        .rxData = NULL,
        .txData = (uint32_t *)buffer,
    };

    usdhc_command_t command = {
        .index = (block_count == 1U) ? (uint32_t)SDCARD_CMD_WRITE_SINGLE_BLOCK : (uint32_t)SDCARD_CMD_WRITE_MULTIPLE_BLOCK,
        .argument = block_num,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };

    usdhc_transfer_t transfer = {
        .data = &data,
        .command = &command,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);

    status_t status = USDHC_TransferBlocking(sdcard->sdcard, NULL, &transfer);

    sdcard_check_status(status, command.index);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));

    return status;
}

static void sdcard_print_cid(cid_t *cid) {
    mp_printf(&mp_plat_print, "\tManufacturer ID: %d\n", cid->mid);
    mp_printf(&mp_plat_print, "\tOEM/Application ID: %d\n", cid->oid);
    mp_printf(&mp_plat_print, "\tProduct name: %s\n", cid->pnm);
    mp_printf(&mp_plat_print, "\tProduct revision: 0x%02X\n", cid->prv);
    mp_printf(&mp_plat_print, "\tProduct serial number: 0x%08X\n", cid->psn);
    mp_printf(&mp_plat_print, "\tManufacturing date: %d.%d\n", (cid->mdt & 0xF), ((cid->mdt >> 4) & 0xFF) + 2000);
}

static void sdcard_print_csd(csd_t *csd) {
    mp_printf(&mp_plat_print, "\tFILE_FORMAT: 0x%08X\n", csd->file_format);
    mp_printf(&mp_plat_print, "\tTEMP_WRITE_PROTECT: 0x%08X\n", csd->temp_write_protect);
    mp_printf(&mp_plat_print, "\tPERM_WRITE_PROTECCT: 0x%08X\n", csd->perm_write_protecct);
    mp_printf(&mp_plat_print, "\tCOPY: 0x%08X\n", csd->copy);
    mp_printf(&mp_plat_print, "\tFILE_FORMAT_GRP: 0x%08X\n", csd->file_format_grp);
    mp_printf(&mp_plat_print, "\tWRITE_BL_PARTIAL: 0x%08X\n", csd->write_bl_partial);
    mp_printf(&mp_plat_print, "\tWRITE_BL_LEN: 0x%08X\n", csd->write_bl_len);
    mp_printf(&mp_plat_print, "\tR2W_FACTOR: 0x%08X\n", csd->r2w_factor);
    mp_printf(&mp_plat_print, "\tWR_GRP_ENABLE: 0x%08X\n", csd->wr_grp_enable);
    mp_printf(&mp_plat_print, "\tWP_GRP_SIZE: 0x%08X\n", csd->wp_grp_size);
    mp_printf(&mp_plat_print, "\tSECTOR_SIZE: 0x%08X\n", csd->sector_size);
    mp_printf(&mp_plat_print, "\tERASE_BLK_EN: 0x%08X\n", csd->erase_blk_en);
    mp_printf(&mp_plat_print, "\tC_SIZE_MULT: 0x%08X\n", csd->c_size_mult);
    mp_printf(&mp_plat_print, "\tVDD_W_CURR_MAX: 0x%08X\n", csd->vdd_w_curr_max);
    mp_printf(&mp_plat_print, "\tVDD_W_CURR_MIN: 0x%08X\n", csd->vdd_w_curr_min);
    mp_printf(&mp_plat_print, "\tVDD_R_CURR_MAX: 0x%08X\n", csd->vdd_r_curr_max);
    mp_printf(&mp_plat_print, "\tVDD_R_CURR_MIN: 0x%08X\n", csd->vdd_r_curr_min);
    mp_printf(&mp_plat_print, "\tC_SIZE: 0x%08X\n", csd->c_size);
    mp_printf(&mp_plat_print, "\tDSR_IMP: 0x%08X\n", csd->dsr_imp);
    mp_printf(&mp_plat_print, "\tREAD_BLK_MISALIGN: 0x%08X\n", csd->read_blk_misalign);
    mp_printf(&mp_plat_print, "\tWRITE_BLK_MISALIGN: 0x%08X\n", csd->write_blk_misalign);
    mp_printf(&mp_plat_print, "\tREAD_BL_PARTIAL: 0x%08X\n", csd->read_bl_partial);
    mp_printf(&mp_plat_print, "\tREAD_BL_LEN: 0x%08X\n", csd->read_bl_len);
    mp_printf(&mp_plat_print, "\tCCC: 0x%08X\n", csd->ccc);
    mp_printf(&mp_plat_print, "\tTRAN_SPEED: 0x%08X\n", csd->tran_speed);
    mp_printf(&mp_plat_print, "\tNSAC: 0x%08X\n", csd->nsac);
    mp_printf(&mp_plat_print, "\tTAAC: 0x%08X\n", csd->taac);
    mp_printf(&mp_plat_print, "\tCSD_STRUCTURE: 0x%08X\n", csd->csd_structure);
}

static bool sdcard_reset(USDHC_Type *base) {
    // TODO: Specify timeout
    return USDHC_Reset(base, (USDHC_SYS_CTRL_RSTA_MASK | USDHC_SYS_CTRL_RSTC_MASK | USDHC_SYS_CTRL_RSTD_MASK), 2048);
}

static bool sdcard_set_active(USDHC_Type *base) {
    // TODO: Specify timeout
    return USDHC_SetCardActive(base, 8192);
}

static bool sdcard_volt_validation(USDHC_Type *base) {
    bool valid_voltage = false;
    uint32_t count = 0UL;
    uint32_t response;

    // Perform voltage validation
    while ((count < SDMMC_MAX_VOLT_TRIAL) && (valid_voltage == false)) {
        sdcard_cmd_app_cmd(base);
        response = sdcard_cmd_sd_app_op_cond(base, (uint32_t)(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY));

        /* Get operating voltage*/
        valid_voltage = (((response >> 31U) == 1U) ? true : false);
        count++;
    }

    if (count >= SDMMC_MAX_VOLT_TRIAL) {
        return false;
    } else {
        return true;
    }    
}

static bool sdcard_power_on(mimxrt_sdcard_obj_t *self) {
    bool status = false;

    // Check if card is already initialized and powered on
    if(self->initialized)
    {
        return true;
    }

    // Start initialization process
    status = sdcard_reset(self->sdcard);
    if(!status) {
        return false;
    }

    status = sdcard_set_active(self->sdcard);
    if(!status) {
        return false;
    }

    status = sdcard_cmd_go_idle_state(self->sdcard);
    if(!status) {
        return false;
    }

    uint32_t oper_cond;
    status = sdcard_cmd_oper_cond(self->sdcard, &oper_cond);
    if(!status) {
        return false;
    }

    status = sdcard_volt_validation(self->sdcard);
    if(!status) {
        return false;
    }
    // Todo: evaluate operation conditions

    // ===
    // Ready State
    // ===
    cid_t cid_all;
    status = sdcard_cmd_all_send_cid(self->sdcard, &cid_all);
    if(!status) {
        return false;
    }
    sdcard_print_cid(&cid_all);
    // Todo: handle multiple cid receptions if multiple cards available

    // ===
    // Identification State
    // ===
    status = sdcard_cmd_set_rel_add(self->sdcard, &self->rca);
    if(!status) {
        return false;
    }
    mp_printf(&mp_plat_print, "RCA received - 0x%04X\n", self->rca);

    // ===
    // Standby State
    // ===
    uint32_t usdhc_clk = USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 20000000UL);
    mp_printf(&mp_plat_print, "Actual USDHC clock: %dHz (%dHz)\n", usdhc_clk, 20000000UL);

    csd_t csd;
    status = sdcard_cmd_send_csd(self->sdcard, self->rca, &csd);
    if(!status) {
        return false;
    }    
    decode_csd(self, &csd);
    sdcard_print_csd(&csd);

    cid_t cid;
    status = sdcard_cmd_send_cid(self->sdcard, self->rca, &cid);
    if(!status) {
        return false;
    }
    sdcard_print_cid(&cid);

    // ===
    // Transfer State
    // ===
    status = sdcard_cmd_select_card(self->sdcard, self->rca);
    if(!status) {
        return false;
    }
    else {
        self->initialized = true;
        return true;
    }    
}


STATIC int machine_sdcard_init_helper(mimxrt_sdcard_obj_t *self, const mp_arg_val_t *args) {
    if(self->initialized)
    {
        return 1;
    }

    // Initialize pins

    // TODO: Implement APi for pin.c to initialize a GPIO for an ALT function

    // GPIO_SD_B0_00 | ALT0 | USDHC1_DATA2 | SD1_D2    | DAT2
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_00_USDHC1_DATA2,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_USDHC1_DATA2, 0x10B0u);

    // GPIO_SD_B0_01 | ALT0 | USDHC1_DATA3 | SD1_D3    | CD/DAT3
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_01_USDHC1_DATA3,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_USDHC1_DATA3, 0x10B0u);

    // GPIO_SD_B0_02 | ALT0 | USDHC1_CMD   | SD1_CMD   | CMD
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_02_USDHC1_CMD,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_USDHC1_CMD, 0x10B0u);

    // GPIO_SD_B0_03 | ALT0 | USDHC1_CLK   | SD1_CLK   | CLK
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_03_USDHC1_CLK,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_USDHC1_CLK, 0x10B0u);

    // GPIO_SD_B0_04 | ALT0 | USDHC1_DATA0 | SD1_D0    | DAT0
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA0,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA0, 0x10B0u);

    // GPIO_SD_B0_05 | ALT0 | USDHC1_DATA1 | SD1_D1    | DAT1
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA1,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA1, 0x10B0u);

    // GPIO_SD_B0_06 | ALT0 | USDHC1_CD_B  | SD_CD_SW  | DETECT
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_06_USDHC1_CD_B,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_06_USDHC1_CD_B, 0x10B0u);


    // TODO: Remove debug clock output
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B1_03_CCM_CLKO2, 0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B1_03_CCM_CLKO2, 0x10B0u);

    mp_printf(&mp_plat_print, "Current clock of PFD2: %dHz\n", CLOCK_GetSysPfdFreq(kCLOCK_Pfd2));
    mp_printf(&mp_plat_print, "USDHC1_CLK_ROOT: %dHz\n", CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3);

    CCM->CCOSR |= CCM_CCOSR_CLKO2_SEL(0b00011);  // usdhc1_clk_root
    CCM->CCOSR |= CCM_CCOSR_CLKO2_DIV(0b000);    // divide by 1
    CCM->CCOSR |= CCM_CCOSR_CLKO2_EN(0b1);       // CCM_CLKO2 enabled


    // Initialize USDHC
    const usdhc_config_t config = {
        .endianMode = kUSDHC_EndianModeLittle,
        .dataTimeout = 0xFU,
        .readWatermarkLevel = 16U,
        .writeWatermarkLevel = 16U,
    };

    USDHC_Init(self->sdcard, &config);
    if (USDHC_DetectCardInsert(self->sdcard)) {
        uint32_t usdhc_clk = USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 300000UL);
        mp_printf(&mp_plat_print, "Actual USDHC clock: %dHz (%dHz)\n", usdhc_clk, 300000UL);
        USDHC_SetDataBusWidth(self->sdcard, kUSDHC_DataBusWidth1Bit);
        self->inserted = true;
        return 1;
    } else {
        return -1;
    }
}

STATIC mp_obj_t sdcard_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mimxrt_sdcard_obj_t *self = &mimxrt_sdcard_objs[(args[0].u_int - 1)];
    if (machine_sdcard_init_helper(self, args)) {
        return MP_OBJ_FROM_PTR(self);
    } else {
        return MP_OBJ_NULL;
    }
}

// init()
STATIC mp_obj_t machine_sdcard_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args) - 1];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_sdcard_init_helper(pos_args[0], args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_sdcard_init_obj, 1, machine_sdcard_init);

// deinit()
STATIC mp_obj_t machine_sdcard_deinit(mp_obj_t self_in) {
    // TODO: Implement deinit function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_sdcard_deinit_obj, machine_sdcard_deinit);

// readblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_readblocks(mp_obj_t self_in, mp_obj_t _block_num, mp_obj_t _buf) {
    mp_buffer_info_t bufinfo;
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_get_buffer_raise(_buf, &bufinfo, MP_BUFFER_WRITE);
    // ---
    sdcard_cmd_set_blocklen(self->sdcard, SDMMC_DEFAULT_BLOCK_SIZE);
    status_t status = sdcard_read(self, bufinfo.buf, mp_obj_get_int(_block_num), bufinfo.len / SDMMC_DEFAULT_BLOCK_SIZE);
    // ---
    return MP_OBJ_NEW_SMALL_INT(status);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_readblocks_obj, machine_sdcard_readblocks);

// writeblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_writeblocks(mp_obj_t self_in, mp_obj_t _block_num, mp_obj_t _buf) {
    mp_buffer_info_t bufinfo;
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_get_buffer_raise(_buf, &bufinfo, MP_BUFFER_WRITE);
    // ---
    sdcard_cmd_set_blocklen(self->sdcard, SDMMC_DEFAULT_BLOCK_SIZE);
    status_t status = sdcard_write(self, bufinfo.buf, mp_obj_get_int(_block_num), bufinfo.len / SDMMC_DEFAULT_BLOCK_SIZE);
    // ---
    return MP_OBJ_NEW_SMALL_INT(status);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_writeblocks_obj, machine_sdcard_writeblocks);

// ioctl(op, arg)
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);

    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT: {
            if(!sdcard_power_on(self))
            {
                return MP_OBJ_NEW_SMALL_INT(-1);  // Initialization failed
            }
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        case MP_BLOCKDEV_IOCTL_DEINIT:
        case MP_BLOCKDEV_IOCTL_SYNC: {
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT: {
            return MP_OBJ_NEW_SMALL_INT(self->block_count);
        }
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE: {
            return MP_OBJ_NEW_SMALL_INT(self->block_len);
        }
        default: // unknown command
        {
            return MP_OBJ_NEW_SMALL_INT(-1); // error
        }
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_ioctl_obj, machine_sdcard_ioctl);

STATIC const mp_rom_map_elem_t sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),        MP_ROM_PTR(&machine_sdcard_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),      MP_ROM_PTR(&machine_sdcard_deinit_obj) },
    // block device protocol
    { MP_ROM_QSTR(MP_QSTR_readblocks),  MP_ROM_PTR(&machine_sdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&machine_sdcard_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl),       MP_ROM_PTR(&machine_sdcard_ioctl_obj) },
};
STATIC MP_DEFINE_CONST_DICT(sdcard_locals_dict, sdcard_locals_dict_table);

const mp_obj_type_t machine_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = sdcard_obj_make_new,
    .locals_dict = (mp_obj_dict_t *)&sdcard_locals_dict,
};

void machine_sdcard_init0(void) {
    return;
}


void USDHC1_IRQHandler() {

}