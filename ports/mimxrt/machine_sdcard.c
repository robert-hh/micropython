
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
#include "pin.h"
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
    (SDMMC_MASK(SDCARD_STATUS_OUT_OF_RANGE_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_ADDRESS_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_BLOCK_LEN_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_ERASE_SEQ_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_ERASE_PARAM_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_WP_VIOLATION_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_LOCK_UNLOCK_FAILED_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_COM_CRC_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_ILLEGAL_COMMAND_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_CARD_ECC_FAILED_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_CC_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_ERROR_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_CSD_OVERWRITE_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_WP_ERASE_SKIP_SHIFT)) | \
    (SDMMC_MASK(SDCARD_STATUS_AUTH_SEQ_ERR_SHIFT))

#define SDMMC_R1_CURRENT_STATE(x) (((x) & 0x00001E00U) >> 9U)


typedef struct _mimxrt_sdcard_pin_t {
    const machine_pin_obj_t *pin;
    uint8_t af_idx;
} mimxrt_sdcard_pin_t;

typedef struct _mimxrt_sdcard_obj_t {
    mp_obj_base_t base;
    USDHC_Type *sdcard;
    bool initialized;
    bool inserted;
    uint32_t rca;
    uint16_t block_len;
    uint32_t block_count;
    struct {
        mimxrt_sdcard_pin_t cmd;
        mimxrt_sdcard_pin_t clk;
        mimxrt_sdcard_pin_t cd_b;
        mimxrt_sdcard_pin_t data0;
        mimxrt_sdcard_pin_t data1;
        mimxrt_sdcard_pin_t data2;
        mimxrt_sdcard_pin_t data3;
    } pins;
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
    uint32_t data[4];
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



STATIC mimxrt_sdcard_obj_t mimxrt_sdcard_objs[] = {

    #if defined MICROPY_USDHC1 && USDHC1_AVAIL
    {
        .base.type = &machine_sdcard_type,
        .sdcard = USDHC1,
        .initialized = false,
        .inserted = true,
        .rca = 0x0UL,
        .block_len = SDMMC_DEFAULT_BLOCK_SIZE,
        .block_count = 0UL,
        .pins = MICROPY_USDHC1,
    }
    #endif
};

STATIC const mp_arg_t allowed_args[] = {
    [SDCARD_INIT_ARG_ID]     { MP_QSTR_id, MP_ARG_INT, {.u_int = 1} },
};


static status_t sdcard_transfer_blocking(USDHC_Type *base, usdhc_adma_config_t *dmaConfig, usdhc_transfer_t *transfer, uint32_t timeout_ms) {
    status_t status = USDHC_TransferBlocking(base, dmaConfig, transfer);

    if (status == kStatus_Success) {
        for (int i = 0; i < timeout_ms * 100; i++) {
            if (!(USDHC_GetPresentStatusFlags(base) & (kUSDHC_DataInhibitFlag | kUSDHC_CommandInhibitFlag))) {
                return kStatus_Success;
            }
            ticks_delay_us64(10);
        }
        return kStatus_Timeout;
    } else {
        return status;
    }
}

static void sdcard_decode_csd(mimxrt_sdcard_obj_t *sdcard, csd_t *csd) {
    uint8_t csd_structure = 0x3 & (csd->data[3] >> 30);

    uint8_t read_bl_len;
    uint32_t c_size;
    uint8_t c_size_mult;

    switch (csd_structure)
    {
        case (0): {
            read_bl_len = 0xF & (csd->data[2] >> 16);
            c_size = ((0x3FF & csd->data[2]) << 30) | (0x3 & (csd->data[1] >> 30));
            c_size_mult = 0x7 & (csd->data[1] >> 15);

            sdcard->block_len = (1U << (read_bl_len));
            sdcard->block_count = ((c_size + 1U) << (c_size_mult + 2U));

            if (sdcard->block_len != SDMMC_DEFAULT_BLOCK_SIZE) {
                sdcard->block_count = (sdcard->block_count * sdcard->block_len);
                sdcard->block_len = SDMMC_DEFAULT_BLOCK_SIZE;
                sdcard->block_count = (sdcard->block_count / sdcard->block_len);
            }
            break;
        }
        case (1): {
            read_bl_len = 0xF & (csd->data[2] >> 16);
            c_size = ((0x3F & csd->data[2]) << 16) | (0xFFFF & (csd->data[1] >> 16));

            sdcard->block_len = (1U << (read_bl_len));
            sdcard->block_count = (uint32_t)(((uint64_t)(c_size + 1U) * (uint64_t)524288) / (uint64_t)sdcard->block_len);
            break;
        }
        case (2): {
            read_bl_len = 0xF & (csd->data[2] >> 16);
            c_size = ((0xFF & csd->data[2]) << 16) | (0xFFFF & (csd->data[1] >> 16));

            sdcard->block_len = (1U << (read_bl_len));
            sdcard->block_count = (uint32_t)(((uint64_t)(c_size + 1U) * (uint64_t)524288) / (uint64_t)sdcard->block_len);
            break;
        }
        default: {
            break;
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        return true;
    } else {
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        *oper_cond = command.response[0];
        return true;
    } else {
        return false;
    }
}

static bool sdcard_cmd_app_cmd(USDHC_Type *base) {
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        return true;
    } else {
        return false;
    }
}

static bool sdcard_cmd_sd_app_op_cond(USDHC_Type *base, uint32_t argument, uint32_t *oper_cond) {
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

    status = sdcard_transfer_blocking(base, NULL, &transfer, 250);

    if (status == kStatus_Success) {
        *oper_cond = command.response[0];
        return true;
    } else {
        return false;
    }
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
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
    } else {
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
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
    } else {
        return false;
    }
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        *rca = 0xFFFFFFFF & (command.response[0] >> 16);
        return true;
    } else {
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

    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        csd->data[0] = command.response[0];
        csd->data[1] = command.response[1];
        csd->data[2] = command.response[2];
        csd->data[3] = command.response[3];
        return true;
    } else {
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
    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        return true;
    } else {
        return false;
    }
}

static bool sdcard_cmd_set_blocklen(USDHC_Type *base, uint32_t block_size) {
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
    status = USDHC_TransferBlocking(base, NULL, &transfer);

    if (status == kStatus_Success) {
        return true;
    } else {
        return false;
    }
}

static bool sdcard_read(mimxrt_sdcard_obj_t *sdcard, uint8_t *buffer, uint32_t block_num, uint32_t block_count) {
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


    status_t status = sdcard_transfer_blocking(sdcard->sdcard, NULL, &transfer, 500);

    if (status == kStatus_Success) {
        return true;
    } else {
        return false;
    }
}

static bool sdcard_write(mimxrt_sdcard_obj_t *sdcard, uint8_t *buffer, uint32_t block_num, uint32_t block_count) {
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


    status_t status = sdcard_transfer_blocking(sdcard->sdcard, NULL, &transfer, 500);

    if (status == kStatus_Success) {
        return true;
    } else {
        return false;
    }
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
        if (!sdcard_cmd_app_cmd(base)) {
            return false;
        }

        if (!sdcard_cmd_sd_app_op_cond(base, (uint32_t)(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY), &response)) {
            return false;
        }

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
    if (self->initialized) {
        return true;
    }

    // Start initialization process
    status = sdcard_reset(self->sdcard);
    if (!status) {
        return false;
    }

    status = sdcard_set_active(self->sdcard);
    if (!status) {
        return false;
    }

    status = sdcard_cmd_go_idle_state(self->sdcard);
    if (!status) {
        return false;
    }

    uint32_t oper_cond;  // Todo: evaluate operation conditions
    status = sdcard_cmd_oper_cond(self->sdcard, &oper_cond);
    if (!status) {
        return false;
    }

    status = sdcard_volt_validation(self->sdcard);
    if (!status) {
        return false;
    }

    // ===
    // Ready State
    // ===
    cid_t cid_all;
    status = sdcard_cmd_all_send_cid(self->sdcard, &cid_all);
    if (!status) {
        return false;
    }

    // ===
    // Identification State
    // ===
    status = sdcard_cmd_set_rel_add(self->sdcard, &self->rca);
    if (!status) {
        return false;
    }

    // ===
    // Standby State
    // ===
    (void)USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 50000000UL);

    csd_t csd;
    status = sdcard_cmd_send_csd(self->sdcard, self->rca, &csd);
    if (!status) {
        return false;
    }
    sdcard_decode_csd(self, &csd);

    cid_t cid;
    status = sdcard_cmd_send_cid(self->sdcard, self->rca, &cid);
    if (!status) {
        return false;
    }

    // ===
    // Transfer State
    // ===
    status = sdcard_cmd_select_card(self->sdcard, self->rca);
    if (!status) {
        return false;
    } else {
        self->initialized = true;
        return true;
    }
}

static bool sdcard_power_off(mimxrt_sdcard_obj_t *self) {
    return true;
}

static bool sdcard_detect(mimxrt_sdcard_obj_t *self) {
    if (self->pins.cd_b.pin) {
        return USDHC_DetectCardInsert(self->sdcard);
    } else {
        USDHC_CardDetectByData3(self->sdcard, true);
        return (USDHC_GetPresentStatusFlags(self->sdcard) & USDHC_PRES_STATE_DLSL(8)) != 0;
    }
}


STATIC bool machine_sdcard_init_helper(mimxrt_sdcard_obj_t *self, const mp_arg_val_t *args) {
    if (self->initialized) {
        return 1;
    }

    // Initialize pins
    // GPIO_SD_B0_03 | ALT0 | USDHC1_CLK   | SD1_CLK   | CLK
    IOMUXC_SetPinMux(self->pins.clk.pin->muxRegister, self->pins.clk.af_idx, 0U, 0U, self->pins.clk.pin->configRegister, 0U);
    IOMUXC_SetPinConfig(self->pins.clk.pin->muxRegister, self->pins.clk.af_idx, 0U, 0U, self->pins.clk.pin->configRegister, 0x017089u);

    // GPIO_SD_B0_02 | ALT0 | USDHC1_CMD   | SD1_CMD   | CMD
    IOMUXC_SetPinMux(self->pins.cmd.pin->muxRegister, self->pins.cmd.af_idx, 0U, 0U, self->pins.cmd.pin->configRegister, 0U);
    IOMUXC_SetPinConfig(self->pins.cmd.pin->muxRegister, self->pins.cmd.af_idx, 0U, 0U, self->pins.cmd.pin->configRegister, 0x017089u);

    // GPIO_SD_B0_04 | ALT0 | USDHC1_DATA0 | SD1_D0    | DAT0
    IOMUXC_SetPinMux(self->pins.data0.pin->muxRegister, self->pins.data0.af_idx, 0U, 0U, self->pins.data0.pin->configRegister, 0U);
    IOMUXC_SetPinConfig(self->pins.data0.pin->muxRegister, self->pins.data0.af_idx, 0U, 0U, self->pins.data0.pin->configRegister, 0x017089u);

    // GPIO_SD_B0_05 | ALT0 | USDHC1_DATA1 | SD1_D1    | DAT1
    IOMUXC_SetPinMux(self->pins.data1.pin->muxRegister, self->pins.data1.af_idx, 0U, 0U, self->pins.data1.pin->configRegister, 0U);
    IOMUXC_SetPinConfig(self->pins.data1.pin->muxRegister, self->pins.data1.af_idx, 0U, 0U, self->pins.data1.pin->configRegister, 0x017089u);

    // GPIO_SD_B0_00 | ALT0 | USDHC1_DATA2 | SD1_D2    | DAT2
    IOMUXC_SetPinMux(self->pins.data2.pin->muxRegister, self->pins.data2.af_idx, 0U, 0U, self->pins.data2.pin->configRegister, 0U);
    IOMUXC_SetPinConfig(self->pins.data2.pin->muxRegister, self->pins.data2.af_idx, 0U, 0U, self->pins.data2.pin->configRegister, 0x017089u);

    if (self->pins.cd_b.pin) {  // Have card detect pin?
        // GPIO_SD_B0_06 | ALT0 | USDHC1_CD_B  | SD_CD_SW  | DETECT
        IOMUXC_SetPinMux(self->pins.cd_b.pin->muxRegister, self->pins.cd_b.af_idx, 0U, 0U, self->pins.cd_b.pin->configRegister, 0U);
        IOMUXC_SetPinConfig(self->pins.cd_b.pin->muxRegister, self->pins.cd_b.af_idx, 0U, 0U, self->pins.cd_b.pin->configRegister, 0x017089u);

        // GPIO_SD_B0_01 | ALT0 | USDHC1_DATA3 | SD1_D3    | CD/DAT3
        IOMUXC_SetPinMux(self->pins.data3.pin->muxRegister, self->pins.data3.af_idx, 0U, 0U, self->pins.data3.pin->configRegister, 0U);
        IOMUXC_SetPinConfig(self->pins.data3.pin->muxRegister, self->pins.data3.af_idx, 0U, 0U, self->pins.data3.pin->configRegister, 0x017089u);
    } else { // No, use data3, which has to be pulled down then.
        // GPIO_SD_B0_01 | ALT0 | USDHC1_DATA3 | SD1_D3    | CD/DAT3
        IOMUXC_SetPinMux(self->pins.data3.pin->muxRegister, self->pins.data3.af_idx, 0U, 0U, self->pins.data3.pin->configRegister, 0U);
        IOMUXC_SetPinConfig(self->pins.data3.pin->muxRegister, self->pins.data3.af_idx, 0U, 0U, self->pins.data3.pin->configRegister, 0x013089u);
    }

    // Initialize USDHC
    const usdhc_config_t config = {
        .endianMode = kUSDHC_EndianModeLittle,
        .dataTimeout = 0xFU,
        .readWatermarkLevel = 128U,
        .writeWatermarkLevel = 128U,
    };

    USDHC_Init(self->sdcard, &config);
    if (sdcard_detect(self)) {
        (void)USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 300000UL);
        USDHC_SetDataBusWidth(self->sdcard, kUSDHC_DataBusWidth1Bit);
        self->inserted = true;
        return true;
    } else {
        return false;
    }
}

STATIC mp_obj_t sdcard_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Extract arguments
    mp_int_t sdcard_id = args[SDCARD_INIT_ARG_ID].u_int;

    if (!(1 <= sdcard_id && sdcard_id <= MP_ARRAY_SIZE(mimxrt_sdcard_objs))) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "SDCard(%d) doesn't exist", sdcard_id));
    }

    mimxrt_sdcard_obj_t *self = &mimxrt_sdcard_objs[(sdcard_id - 1)];

    // Initialize SDCard Host
    if (machine_sdcard_init_helper(self, args)) {
        return MP_OBJ_FROM_PTR(self);
    } else {
        return mp_const_none;
    }
}

// init()
STATIC mp_obj_t machine_sdcard_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args) - 1];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Todo: make sure that there is a valid reason that a call to init would not change anything for an already initialized hsot
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
    if (sdcard_cmd_set_blocklen(self->sdcard, SDMMC_DEFAULT_BLOCK_SIZE)) {
        if (sdcard_read(self, bufinfo.buf, mp_obj_get_int(_block_num), bufinfo.len / SDMMC_DEFAULT_BLOCK_SIZE)) {
            return MP_OBJ_NEW_SMALL_INT(0);
        } else {
            return MP_OBJ_NEW_SMALL_INT(-1);  // readblocks failed
        }
    } else {
        return MP_OBJ_NEW_SMALL_INT(-1);  // readblocks failed
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_readblocks_obj, machine_sdcard_readblocks);

// writeblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_writeblocks(mp_obj_t self_in, mp_obj_t _block_num, mp_obj_t _buf) {
    mp_buffer_info_t bufinfo;
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_get_buffer_raise(_buf, &bufinfo, MP_BUFFER_WRITE);
    // ---
    if (sdcard_cmd_set_blocklen(self->sdcard, SDMMC_DEFAULT_BLOCK_SIZE)) {
        if (sdcard_write(self, bufinfo.buf, mp_obj_get_int(_block_num), bufinfo.len / SDMMC_DEFAULT_BLOCK_SIZE)) {
            return MP_OBJ_NEW_SMALL_INT(0);
        } else {
            return MP_OBJ_NEW_SMALL_INT(-1);  // writeblocks failed
        }
    } else {
        return MP_OBJ_NEW_SMALL_INT(-1);  // writeblocks failed
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_writeblocks_obj, machine_sdcard_writeblocks);

// ioctl(op, arg)
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);

    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT: {
            if (!sdcard_power_on(self)) {
                return MP_OBJ_NEW_SMALL_INT(-1);  // Initialization failed
            }
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        case MP_BLOCKDEV_IOCTL_DEINIT: {
            if (!sdcard_power_off(self)) {
                return MP_OBJ_NEW_SMALL_INT(-1);  // Deinitialization failed
            }
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        case MP_BLOCKDEV_IOCTL_SYNC: {
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT: {
            if (!self->initialized) {
                return MP_OBJ_NEW_SMALL_INT(-1);  // Card not initialized
            } else {
                return MP_OBJ_NEW_SMALL_INT(self->block_count);
            }
        }
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE: {
            if (!self->initialized) {
                return MP_OBJ_NEW_SMALL_INT(-1);  // Card not initialized
            } else {
                return MP_OBJ_NEW_SMALL_INT(self->block_len);
            }
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