
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


#define SDCARD_INIT_ARG_ID (0)

#define SDMMC_VOLTAGE_WINDOW_SD            0x80100000U
#define SDMMC_HIGH_CAPACITY                0x40000000U
#define SD_SWITCH_1_8V_CAPACITY            ((uint32_t)0x01000000U)
#define SDMMC_MAX_VOLT_TRIAL               ((uint32_t)0x0000FFFFU)
#define SDMMC_R1_CURRENT_STATE(x) (((x) & 0x00001E00U) >> 9U)

#define BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE (32U)
#define BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE (32U)
#define FSL_SDMMC_DEFAULT_BLOCK_SIZE (512U)

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    USDHC_Type *sdcard;
    uint32_t rca;
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
    uint32_t csd[4];
} csd_t;


typedef enum _sdmmc_r1_current_state
{
    kSDMMC_R1StateIdle        = 0U, /*!< R1: current state: idle */
    kSDMMC_R1StateReady       = 1U, /*!< R1: current state: ready */
    kSDMMC_R1StateIdentify    = 2U, /*!< R1: current state: identification */
    kSDMMC_R1StateStandby     = 3U, /*!< R1: current state: standby */
    kSDMMC_R1StateTransfer    = 4U, /*!< R1: current state: transfer */
    kSDMMC_R1StateSendData    = 5U, /*!< R1: current state: sending data */
    kSDMMC_R1StateReceiveData = 6U, /*!< R1: current state: receiving data */
    kSDMMC_R1StateProgram     = 7U, /*!< R1: current state: programming */
    kSDMMC_R1StateDisconnect  = 8U, /*!< R1: current state: disconnect */
} sdmmc_r1_current_state_t;

enum
{
    SdCmd_GoIdleState        = 0U,
    SdCmd_AllSendCid         = 2U,
    SdCmd_SendRelAddr        = 3U,
    SdCmd_SetDsr             = 4U,
    SdCmd_SelectCard         = 7U,
    SdCmd_SendIfCond         = 8U,
    SdCmd_SendCsd            = 9U,
    SdCmd_SendCid            = 10U,
    SdCmd_StopTransmission   = 12U,
    SdCmd_SendStatus         = 13U,
    SdCmd_GoInactiveState    = 15U,
    SdCmd_SetBlockLength     = 16U,
    SdCmd_ReadSingleBlock    = 17U,
    SdCmd_ReadMultipleBlock  = 18U,
    SdCmd_SetBlockCount      = 23U,
    SdCmd_WriteSingleBlock   = 24U,
    SdCmd_WriteMultipleBlock = 25U,
    SdCmd_ProgramCsd         = 27U,
    SdCmd_SetWriteProtect    = 28U,
    SdCmd_ClearWriteProtect  = 29U,
    SdCmd_SendWriteProtect   = 30U,
    SdCmd_Erase              = 38U,
    SdCmd_LockUnlock         = 42U,
    SdCmd_ApplicationCommand = 55U,
    SdCmd_GeneralCommand     = 56U,
    SdCmd_ReadOcr            = 58U,
    SdAcmd_SendOpCond        = 41U,
};

enum
{
    kSDMMC_R1OutOfRangeFlag                  = 31, /*!< Out of range status bit */
    kSDMMC_R1AddressErrorFlag                = 30, /*!< Address error status bit */
    kSDMMC_R1BlockLengthErrorFlag            = 29, /*!< Block length error status bit */
    kSDMMC_R1EraseSequenceErrorFlag          = 28, /*!< Erase sequence error status bit */
    kSDMMC_R1EraseParameterErrorFlag         = 27, /*!< Erase parameter error status bit */
    kSDMMC_R1WriteProtectViolationFlag       = 26, /*!< Write protection violation status bit */
    kSDMMC_R1CardIsLockedFlag                = 25, /*!< Card locked status bit */
    kSDMMC_R1LockUnlockFailedFlag            = 24, /*!< lock/unlock error status bit */
    kSDMMC_R1CommandCrcErrorFlag             = 23, /*!< CRC error status bit */
    kSDMMC_R1IllegalCommandFlag              = 22, /*!< Illegal command status bit */
    kSDMMC_R1CardEccFailedFlag               = 21, /*!< Card ecc error status bit */
    kSDMMC_R1CardControllerErrorFlag         = 20, /*!< Internal card controller error status bit */
    kSDMMC_R1ErrorFlag                       = 19, /*!< A general or an unknown error status bit */
    kSDMMC_R1CidCsdOverwriteFlag             = 16, /*!< Cid/csd overwrite status bit */
    kSDMMC_R1WriteProtectEraseSkipFlag       = 15, /*!< Write protection erase skip status bit */
    kSDMMC_R1CardEccDisabledFlag             = 14, /*!< Card ecc disabled status bit */
    kSDMMC_R1EraseResetFlag                  = 13, /*!< Erase reset status bit */
    kSDMMC_R1ReadyForDataFlag                = 8,  /*!< Ready for data status bit */
    kSDMMC_R1SwitchErrorFlag                 = 7,  /*!< Switch error status bit */
    kSDMMC_R1ApplicationCommandFlag          = 5,  /*!< Application command enabled status bit */
    kSDMMC_R1AuthenticationSequenceErrorFlag = 3,  /*!< error in the sequence of authentication process */
};

#define SDMMC_MASK(bit) (1U << (bit))

#define SDMMC_R1_ALL_ERROR_FLAG                                                                      \
    (SDMMC_MASK(kSDMMC_R1OutOfRangeFlag) | SDMMC_MASK(kSDMMC_R1AddressErrorFlag) |                   \
    SDMMC_MASK(kSDMMC_R1BlockLengthErrorFlag) | SDMMC_MASK(kSDMMC_R1EraseSequenceErrorFlag) |       \
    SDMMC_MASK(kSDMMC_R1EraseParameterErrorFlag) | SDMMC_MASK(kSDMMC_R1WriteProtectViolationFlag) | \
    SDMMC_MASK(kSDMMC_R1CardIsLockedFlag) | SDMMC_MASK(kSDMMC_R1LockUnlockFailedFlag) |             \
    SDMMC_MASK(kSDMMC_R1CommandCrcErrorFlag) | SDMMC_MASK(kSDMMC_R1IllegalCommandFlag) |            \
    SDMMC_MASK(kSDMMC_R1CardEccFailedFlag) | SDMMC_MASK(kSDMMC_R1CardControllerErrorFlag) |         \
    SDMMC_MASK(kSDMMC_R1ErrorFlag) | SDMMC_MASK(kSDMMC_R1CidCsdOverwriteFlag) |                     \
    SDMMC_MASK(kSDMMC_R1AuthenticationSequenceErrorFlag))


AT_NONCACHEABLE_SECTION_ALIGN(static uint32_t sdcard_read_buffer[FSL_SDMMC_DEFAULT_BLOCK_SIZE / 4U], 4U);
AT_NONCACHEABLE_SECTION_ALIGN(static uint32_t sdcard_write_buffer[FSL_SDMMC_DEFAULT_BLOCK_SIZE / 4U], 4U);
AT_NONCACHEABLE_SECTION_ALIGN(static uint32_t s_sdmmcHostDmaBuffer[BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE], 4U);

#if defined SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER && SDMMCHOST_ENABLE_CACHE_LINE_ALIGN_TRANSFER
/* two cache line length for sdmmc host driver maintain unalign transfer */
SDK_ALIGN(static uint8_t s_sdmmcCacheLineAlignBuffer[BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE * 2U], BOARD_SDMMC_DATA_BUFFER_ALIGN_SIZE);
#endif

STATIC mimxrt_sdcard_obj_t mimxrt_sdcard_objs[1] = {
    {
        .base.type = &machine_sdcard_type,
        .sdcard = USDHC1,
        .rca = 0x0UL,
    }
};

STATIC const mp_arg_t allowed_args[] = {
    [SDCARD_INIT_ARG_ID]     { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
};


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
        case kSDMMC_R1StateIdle: {
            mp_printf(&mp_plat_print, "Card State: StateIdle\n");
            break;
        }
        case kSDMMC_R1StateReady: {
            mp_printf(&mp_plat_print, "Card State: StateReady\n");
            break;
        }
        case kSDMMC_R1StateIdentify: {
            mp_printf(&mp_plat_print, "Card State: StateIdentify\n");
            break;
        }
        case kSDMMC_R1StateStandby: {
            mp_printf(&mp_plat_print, "Card State: StateStandby\n");
            break;
        }
        case kSDMMC_R1StateTransfer: {
            mp_printf(&mp_plat_print, "Card State: StateTransfer\n");
            break;
        }
        case kSDMMC_R1StateSendData: {
            mp_printf(&mp_plat_print, "Card State: StateSendData\n");
            break;
        }
        case kSDMMC_R1StateReceiveData: {
            mp_printf(&mp_plat_print, "Card State: StateReceiveData\n");
            break;
        }
        case kSDMMC_R1StateProgram: {
            mp_printf(&mp_plat_print, "Card State: StateProgram\n");
            break;
        }
        case kSDMMC_R1StateDisconnect: {
            mp_printf(&mp_plat_print, "Card State: StateDisconnect\n");
            break;
        }
        default: {
            mp_printf(&mp_plat_print, "Card State: INVALID!!!\n");
        }
    }
}

static void sdcard_cmd_go_idle_state(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SdCmd_GoIdleState,
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
    sdcard_check_status(status, SdCmd_GoIdleState);
}

static uint32_t sdcard_cmd_oper_cond(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SdCmd_SendIfCond,
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
    sdcard_check_status(status, SdCmd_SendIfCond);
    return command.response[0];
}

static uint32_t sdcard_cmd_app_cmd(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SdCmd_ApplicationCommand,
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
    sdcard_check_status(status, SdCmd_ApplicationCommand);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static uint32_t sdcard_cmd_sd_app_op_cond(USDHC_Type *base, uint32_t argument) {
    status_t status;
    usdhc_command_t command = {
        .index = SdAcmd_SendOpCond,
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
    sdcard_check_status(status, SdAcmd_SendOpCond);
    return command.response[0];
}

static cid_t sdcard_cmd_all_send_cid(USDHC_Type *base) {
    status_t status;
    cid_t cid;
    usdhc_command_t command = {
        .index = SdCmd_AllSendCid,
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
    sdcard_check_status(status, SdCmd_AllSendCid);
    // --
    cid.mdt = (uint16_t)((command.response[0] & 0xFFF00U) >> 8U);
    cid.psn = (uint32_t)(((command.response[1] & 0xFFFFFFU) << 8U) | ((command.response[0] & 0xFF000000U) >> 24U));
    cid.prv = (uint8_t)((command.response[1] & 0xFF000000U) >> 24U);
    cid.pnm[0] = (char)(command.response[2] & 0xFFU);
    cid.pnm[1] = (char)((command.response[2] & 0xFF00U) >> 8U);
    cid.pnm[2] = (char)((command.response[2] & 0xFF0000U) >> 16U);
    cid.pnm[3] = (char)((command.response[2] & 0xFF000000U) >> 24U);
    cid.pnm[4] = (char)(command.response[3] & 0xFFU);
    cid.pnm[5] = '\0';
    cid.oid = (uint16_t)((command.response[3] & 0xFFFF00U) >> 8U);
    cid.mid = (uint8_t)((command.response[3] & 0xFF000000U) >> 24U);
    // --
    return cid;
}

static cid_t sdcard_cmd_send_cid(USDHC_Type *base, uint32_t rca) {
    status_t status;
    cid_t cid;
    usdhc_command_t command = {
        .index = SdCmd_SendCid,
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
    sdcard_check_status(status, SdCmd_SendCid);
    // --
    cid.mdt = (uint16_t)((command.response[0] & 0xFFF00U) >> 8U);
    cid.psn = (uint32_t)(((command.response[1] & 0xFFFFFFU) << 8U) | ((command.response[0] & 0xFF000000U) >> 24U));
    cid.prv = (uint8_t)((command.response[1] & 0xFF000000U) >> 24U);
    cid.pnm[0] = (char)(command.response[2] & 0xFFU);
    cid.pnm[1] = (char)((command.response[2] & 0xFF00U) >> 8U);
    cid.pnm[2] = (char)((command.response[2] & 0xFF0000U) >> 16U);
    cid.pnm[3] = (char)((command.response[2] & 0xFF000000U) >> 24U);
    cid.pnm[4] = (char)(command.response[3] & 0xFFU);
    cid.pnm[5] = '\0';
    cid.oid = (uint16_t)((command.response[3] & 0xFFFF00U) >> 8U);
    cid.mid = (uint8_t)((command.response[3] & 0xFF000000U) >> 24U);
    // --
    return cid;
}

static uint32_t sdcard_cmd_set_rel_add(USDHC_Type *base) {
    status_t status;
    usdhc_command_t command = {
        .index = SdCmd_SendRelAddr,
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
    sdcard_check_status(status, SdCmd_SendRelAddr);
    return 0xFFFFFFFF & (command.response[0] >> 16);
}

static csd_t sdcard_cmd_send_csd(USDHC_Type *base, uint32_t rca) {
    status_t status;
    csd_t csd;
    usdhc_command_t command = {
        .index = SdCmd_SendCsd,
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
    sdcard_check_status(status, SdCmd_SendCsd);
    // --
    csd.csd[0] = command.response[0];
    csd.csd[1] = command.response[1];
    csd.csd[2] = command.response[2];
    csd.csd[3] = command.response[3];
    // --
    return csd;
}

static uint32_t sdcard_cmd_select_card(USDHC_Type *base, uint32_t rca) {
    status_t status;
    csd_t csd;
    usdhc_command_t command = {
        .index = SdCmd_SelectCard,
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
    sdcard_check_status(status, SdCmd_SelectCard);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static uint32_t sdcard_cmd_stop_transmission(USDHC_Type *base) {
    status_t status;
    csd_t csd;
    usdhc_command_t command = {
        .index = SdCmd_StopTransmission,
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
    sdcard_check_status(status, SdCmd_StopTransmission);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static uint32_t sdcard_cmd_set_blocklen(USDHC_Type *base, uint32_t block_size) {
    status_t status;
    csd_t csd;
    usdhc_command_t command = {
        .index = SdCmd_SetBlockLength,
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
    sdcard_check_status(status, SdCmd_SetBlockLength);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return command.response[0];
}

static status_t sdcard_read(USDHC_Type *base, uint32_t *buffer, uint32_t start_block, size_t block_size, uint32_t block_count) {

    // Todo: check card busy

    // Todo: check/read different behaviour for cards with high capacity for block_size checks !!

    usdhc_handle_t handle;


    usdhc_data_t data = {
        .enableAutoCommand12 = true,
        .enableAutoCommand23 = false,
        .enableIgnoreError = false,
        .dataType = kUSDHC_TransferDataNormal,
        .blockSize = block_size,
        .blockCount = block_count,
        .rxData = buffer,
        .txData = NULL,
    };

    usdhc_command_t command = {
        .index = (block_count == 1U) ? (uint32_t)SdCmd_ReadSingleBlock : (uint32_t)SdCmd_ReadMultipleBlock,
        .argument = 2,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };

    usdhc_transfer_t transfer = {
        .data = &data,
        .command = &command,
    };

    usdhc_adma_config_t dmaConfig = {
        .dmaMode = kUSDHC_DmaModeAdma2,
        #if !(defined(FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN) && FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN)
        .burstLen = kUSDHC_EnBurstLenForINCR,
        #endif
        .admaTable = s_sdmmcHostDmaBuffer,
        .admaTableWords = BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status_t status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, command.index);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return status;
}

static status_t sdcard_write(USDHC_Type *base, uint32_t *buffer, uint32_t start_block, size_t block_size, uint32_t block_count) {

    // Todo: check card busy

    // Todo: check/read different behaviour for cards with high capacity for block_size checks !!

    usdhc_handle_t handle;


    usdhc_data_t data = {
        .enableAutoCommand12 = true,
        .enableAutoCommand23 = false,
        .enableIgnoreError = false,
        .dataType = kUSDHC_TransferDataNormal,
        .blockSize = block_size,
        .blockCount = block_count,
        .rxData = NULL,
        .txData = buffer,
    };

    usdhc_command_t command = {
        .index = (block_count == 1U) ? (uint32_t)SdCmd_WriteSingleBlock : (uint32_t)SdCmd_WriteMultipleBlock,
        .argument = 2,
        .type = kCARD_CommandTypeNormal,
        .responseType = kCARD_ResponseTypeR1,
        .responseErrorFlags = SDMMC_R1_ALL_ERROR_FLAG,
    };

    usdhc_transfer_t transfer = {
        .data = &data,
        .command = &command,
    };

    usdhc_adma_config_t dmaConfig = {
        .dmaMode = kUSDHC_DmaModeAdma2,
        #if !(defined(FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN) && FSL_FEATURE_USDHC_HAS_NO_RW_BURST_LEN)
        .burstLen = kUSDHC_EnBurstLenForINCR,
        #endif
        .admaTable = s_sdmmcHostDmaBuffer,
        .admaTableWords = BOARD_SDMMC_HOST_DMA_DESCRIPTOR_BUFFER_SIZE,
    };

    mp_printf(&mp_plat_print, "CMD %d started\n", command.index);
    status_t status = USDHC_TransferBlocking(base, NULL, &transfer);
    sdcard_check_status(status, command.index);
    sdcard_card_status(SDMMC_R1_CURRENT_STATE(command.response[0]));
    return status;
}


static void sdcard_print_cid(cid_t cid) {
    mp_printf(&mp_plat_print, "\tManufacturer ID: %d\n", cid.mid);
    mp_printf(&mp_plat_print, "\tOEM/Application ID: %d\n", cid.oid);
    mp_printf(&mp_plat_print, "\tProduct name: %s\n", cid.pnm);
    mp_printf(&mp_plat_print, "\tProduct revision: 0x%02X\n", cid.prv);
    mp_printf(&mp_plat_print, "\tProduct serial number: 0x%08X\n", cid.psn);
    mp_printf(&mp_plat_print, "\tManufacturing date: %d.%d\n", (cid.mdt & 0xF), ((cid.mdt >> 4) & 0xFF) + 2000);
}

STATIC void machine_sdcard_init_helper(const mimxrt_sdcard_obj_t *self, const mp_arg_val_t *args) {
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
    uint32_t usdhc_clk = USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 300000UL);
    mp_printf(&mp_plat_print, "Actual USDHC clock: %dHz (%dHz)\n", usdhc_clk, 300000UL);
    USDHC_SetDataBusWidth(self->sdcard, kUSDHC_DataBusWidth1Bit);
}

STATIC mp_obj_t sdcard_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mimxrt_sdcard_obj_t *self = &mimxrt_sdcard_objs[(args[0].u_int - 1)];
    machine_sdcard_init_helper(self, args);
    return MP_OBJ_FROM_PTR(self);
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
    uint32_t block_start = mp_obj_get_int(_block_num) * FSL_SDMMC_DEFAULT_BLOCK_SIZE;
    mp_get_buffer_raise(_buf, &bufinfo, MP_BUFFER_WRITE);
    // ---
    uint32_t temp_block_size = FSL_SDMMC_DEFAULT_BLOCK_SIZE;
    uint32_t temp_block_start = 0U;  // SDHC and SDXC use block unit addressing (512 Bytes)!
    uint32_t temp_block_count = 1U;
    // ---
    sdcard_cmd_set_blocklen(self->sdcard, temp_block_size);
    status_t status = sdcard_read(self->sdcard, &sdcard_read_buffer[0], temp_block_start, temp_block_size, temp_block_count);

    for (int i = 0; i < 32; ++i)
    {
        mp_printf(&mp_plat_print, "0x%08X\n", sdcard_read_buffer[i]);
    }

    return MP_OBJ_NEW_SMALL_INT(status);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_readblocks_obj, machine_sdcard_readblocks);

// writeblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_writeblocks(mp_obj_t self_in, mp_obj_t _block_num, mp_obj_t _buf) {
    mp_buffer_info_t bufinfo;
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t block_start = mp_obj_get_int(_block_num) * FSL_SDMMC_DEFAULT_BLOCK_SIZE;
    mp_get_buffer_raise(_buf, &bufinfo, MP_BUFFER_WRITE);
    // ---
    uint32_t temp_block_size = FSL_SDMMC_DEFAULT_BLOCK_SIZE;
    uint32_t temp_block_start = 0U;  // SDHC and SDXC use block unit addressing (512 Bytes)!
    uint32_t temp_block_count = 1U;
    // ---
    for (int i = 0; i < 32; ++i)
    {
        sdcard_write_buffer[i] = 0xDEADBEEF;
    }
    // ---
    sdcard_cmd_set_blocklen(self->sdcard, temp_block_size);
    status_t status = sdcard_write(self->sdcard, &sdcard_write_buffer[0], temp_block_start, temp_block_size, temp_block_count);

    return MP_OBJ_NEW_SMALL_INT(status);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_writeblocks_obj, machine_sdcard_writeblocks);

// ioctl(op, arg)
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    // ---
    bool valid_voltage = false;
    uint32_t count = 0UL;
    uint32_t response;
    // ---
    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
            // Reset Card
            if (USDHC_Reset(self->sdcard, (USDHC_SYS_CTRL_RSTA_MASK | USDHC_SYS_CTRL_RSTC_MASK | USDHC_SYS_CTRL_RSTD_MASK), 2048) == false) {
                mp_printf(&mp_plat_print, "USDHC_Reset timed out!\n");
            } else {
                mp_printf(&mp_plat_print, "USDHC_Reset performed\n");
            }

            if (USDHC_SetCardActive(self->sdcard, 4096) == false) { // TODO: remove timeout check or change to calculated value based on CLK frequency
                mp_printf(&mp_plat_print, "USDHC_SetCardActive timed out!\n");
            } else {
                mp_printf(&mp_plat_print, "USDHC_SetCardActive performed\n");
            }


            sdcard_cmd_go_idle_state(self->sdcard);
            sdcard_cmd_oper_cond(self->sdcard);

            // Perform voltage validation
            while ((count < SDMMC_MAX_VOLT_TRIAL) && (valid_voltage == 0U)) {
                sdcard_cmd_app_cmd(self->sdcard);
                response = sdcard_cmd_sd_app_op_cond(self->sdcard, (uint32_t)(SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY));

                /* Get operating voltage*/
                valid_voltage = (((response >> 31U) == 1U) ? true : false);
                count++;

                if (!(count % 10000)) {
                    mp_printf(&mp_plat_print, "\t%d Voltage validation cycles executed\n", count);
                }
            }

            if (count >= SDMMC_MAX_VOLT_TRIAL) {
                mp_printf(&mp_plat_print, "Invalid voltage range!\n");
            } else {
                mp_printf(&mp_plat_print, "Voltage validation performed (%d cycles)\n", count);
            }

            cid_t cid_all = sdcard_cmd_all_send_cid(self->sdcard);
            // mp_printf(&mp_plat_print, "CID of all cards received:\n\t[3]0x%08X\n\t[2]0x%08X\n\t[1]0x%08X\n\t[0]0x%08X\n", cid_all.cid[3],cid_all.cid[2],cid_all.cid[1],cid_all.cid[0]);
            sdcard_print_cid(cid_all);

            self->rca = sdcard_cmd_set_rel_add(self->sdcard);
            mp_printf(&mp_plat_print, "RCA received - 0x%04X\n", self->rca);

            // Stand-by State (stby) reached
            uint32_t usdhc_clk = USDHC_SetSdClock(self->sdcard, CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) / 3, 20000000UL);
            mp_printf(&mp_plat_print, "Actual USDHC clock: %dHz (%dHz)\n", usdhc_clk, 20000000UL);

            cid_t cid = sdcard_cmd_send_cid(self->sdcard, self->rca);
            // mp_printf(&mp_plat_print, "CID of RCA referenced card received:\n\t[3]0x%08X\n\t[2]0x%08X\n\t[1]0x%08X\n\t[0]0x%08X\n", cid.cid[3],cid.cid[2],cid.cid[1],cid.cid[0]);
            sdcard_print_cid(cid);

            csd_t csd = sdcard_cmd_send_csd(self->sdcard, self->rca);
            mp_printf(&mp_plat_print, "CSD of RCA referenced card received:\n\t[3]0x%08X\n\t[2]0x%08X\n\t[1]0x%08X\n\t[0]0x%08X\n", csd.csd[3],csd.csd[2],csd.csd[1],csd.csd[0]);

            sdcard_cmd_select_card(self->sdcard, self->rca);
        case MP_BLOCKDEV_IOCTL_DEINIT:
        case MP_BLOCKDEV_IOCTL_SYNC:
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
        // TODO: Return number of bytes
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
        // TODO: Return size of block
        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
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