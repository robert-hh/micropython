/*
 * Copyright 2019 NXP.
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include BOARD_FLASH_CONFIG_HEADER_H

/* Component ID definition, used by tools. */

#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.xip_board"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if defined(XIP_BOOT_HEADER_ENABLE) && (XIP_BOOT_HEADER_ENABLE == 1)
#if defined(__ARMCC_VERSION) || defined(__GNUC__)
__attribute__((section(".boot_hdr.conf")))
#elif defined(__ICCARM__)
#pragma location = ".boot_hdr.conf"
#endif

// Config for W25Q16DVSSIG with QSPI routed.
const flexspi_nor_config_t qspiflash_config = {
    .memConfig =
    {
        .tag = FLEXSPI_CFG_BLK_TAG,
        .version = FLEXSPI_CFG_BLK_VERSION,
        .readSampleClkSrc = kFLEXSPIReadSampleClk_LoopbackFromDqsPad,
        .csHoldTime = 3u,
        .csSetupTime = 3u,
        .busyOffset = FLASH_BUSY_STATUS_OFFSET,         // Status bit 0 indicates busy.
        .busyBitPolarity = FLASH_BUSY_STATUS_POL,       // Busy when the bit is 1.
        .deviceModeCfgEnable = 1u,
        .deviceModeType = kDeviceConfigCmdType_QuadEnable,
        .deviceModeSeq = {
            .seqId = 3u,
            .seqNum = 2u,
        },
        // The high order byte is written first, but the 0x02 must be in the
        // second byte. So just write 0x02 to both S1 and S2. In S1 that
        // bit is the WEL, which is readonly, so writing does not harm.
        .deviceModeArg = 0x0202,
        .deviceType = kFLEXSPIDeviceType_SerialNOR,
        .sflashPadType = kSerialFlash_4Pads,
        .serialClkFreq = kFLEXSPISerialClk_100MHz,
        .sflashA1Size = MICROPY_HW_FLASH_SIZE,
        .lookupTable =
        {
            // 0 Read LUTs 0
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0xEB, RADDR_SDR, FLEXSPI_4PAD, 24),
            FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 0x06, READ_SDR, FLEXSPI_4PAD, 0x04),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 1 Read status register
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x05, READ_SDR, FLEXSPI_1PAD, 0x01),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 2 Empty: Read extend parameters is not supported
            EMPTY_LUT

            // 3 Write Enable
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x06, STOP, FLEXSPI_1PAD, 0),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 4 Write Status Reg 1 and 2
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x01, WRITE_SDR, FLEXSPI_1PAD, 0x02),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 5 Erase Sector
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x20, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 6 Fast read quad output mode - SDR
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x6B, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_4PAD, 0x08, READ_SDR, FLEXSPI_4PAD, 0x04),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 7 Read ID
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x90, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(READ_SDR, FLEXSPI_1PAD, 0x00, 0, 0, 0),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 8 Erase Block (32k)
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x52, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 9 Page Program - single mode
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x02, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_1PAD, 0, 0, 0, 0),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 10 Page Program - quad mode
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x32, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(WRITE_SDR, FLEXSPI_4PAD, 0x04, STOP, FLEXSPI_1PAD, 0),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 11 Erase Chip
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x60, STOP, FLEXSPI_1PAD, 0),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler

            // 12 Empty LUT
            EMPTY_LUT

            // 13 READ SDFP
            FLEXSPI_LUT_SEQ(CMD_SDR, FLEXSPI_1PAD, 0x5A, RADDR_SDR, FLEXSPI_1PAD, 24),
            FLEXSPI_LUT_SEQ(DUMMY_SDR, FLEXSPI_1PAD, 8, READ_SDR, FLEXSPI_1PAD, 0x04),
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
            FLEXSPI_LUT_SEQ(0, 0, 0, 0, 0, 0),         // Filler
        },
    },
    .pageSize = 256u,
    .sectorSize = 4u * 1024u,
    .ipcmdSerialClkFreq = kFLEXSPISerialClk_30MHz,
    .blockSize = 32u * 1024u,
    .isUniformBlockSize = false,
};
#endif /* XIP_BOOT_HEADER_ENABLE */
