/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Philipp Ebensberger
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

#include "flash.h"

#include "fsl_cache.h"
#include "fsl_flexspi.h"

bool flash_busy_status_pol = 0;
bool flash_busy_status_offset = 0;

#if !defined(MIMXRT117x_SERIES) && !MICROPY_HW_HYPERFLASH
static uint16_t freq_table_mhz[] = {
    60,  // Entry 0 to 2 are out of range
    60,  // So it is set to 60
    60,  // as lowest possible MHz value
    60,
    75,
    80,
    100,
    133,
    166
};
#endif

__attribute__((section(".ram_functions"))) void flash_init(void) {
    // Make sure everything is flushed after updating the LUT.
    // Get the busy status flags from the flash config & store them locally.
    flash_busy_status_pol = qspiflash_config.memConfig.busyBitPolarity;
    flash_busy_status_offset = qspiflash_config.memConfig.busyOffset;
    // Update the LUT to make sure all entries are available. Copy the values to
    // memory first so that we don't read from the flash as we update the LUT.
    uint32_t lut_copy[64];
    memcpy(lut_copy, (const uint32_t *)&qspiflash_config.memConfig.lookupTable, 64 * sizeof(uint32_t));
    FLEXSPI_UpdateLUT(BOARD_FLEX_SPI, 0, lut_copy, 64);

    #if !defined(MIMXRT117x_SERIES) && !MICROPY_HW_HYPERFLASH
    // The PFD is 480 * 18 / PFD0_FRAC. We do / 18 which outputs 480 MHz.
    // We pre-calculate the divider here as long as the freq_table_mhz is accessible.
    uint32_t freq_divider = (480 + freq_table_mhz[MICROPY_HW_FLASH_CLK] / 2) / freq_table_mhz[MICROPY_HW_FLASH_CLK] - 1;
    __DSB();
    __ISB();

    __disable_irq();
    SCB_DisableDCache();
    while (!FLEXSPI_GetBusIdleStatus(FLEXSPI)) {
    }
    FLEXSPI_Enable(FLEXSPI, false);

    // Disable FlexSPI clock
    CCM->CCGR6 &= ~CCM_CCGR6_CG5_MASK;
    // Changing the clock is OK now.
    // The PFD is 480 * 18 / PFD0_FRAC. We do / 18 which outputs 480 MHz.
    CCM_ANALOG->PFD_480 = (CCM_ANALOG->PFD_480 & ~CCM_ANALOG_PFD_480_TOG_PFD0_FRAC_MASK) | CCM_ANALOG_PFD_480_TOG_PFD0_FRAC(17);
    // This divides down the 480 Mhz by PODF + 1. So 508 / (3 + 1) = 127 MHz, 508 / (4 + 1) = 101.7 MHz.
    CCM->CSCMR1 = (CCM->CSCMR1 & ~CCM_CSCMR1_FLEXSPI_PODF_MASK) | CCM_CSCMR1_FLEXSPI_PODF(freq_divider);
    // Re-enable FlexSPI
    CCM->CCGR6 |= CCM_CCGR6_CG5_MASK;

    FLEXSPI_Enable(FLEXSPI, true);
    FLEXSPI_SoftwareReset(FLEXSPI);
    while (!FLEXSPI_GetBusIdleStatus(FLEXSPI)) {
    }
    SCB_EnableDCache();
    __enable_irq();
    #endif

    // Configure FLEXSPI IP FIFO access.
    BOARD_FLEX_SPI->MCR0 &= ~(FLEXSPI_MCR0_ARDFEN_MASK);
    BOARD_FLEX_SPI->MCR0 &= ~(FLEXSPI_MCR0_ATDFEN_MASK);
    BOARD_FLEX_SPI->MCR0 |= FLEXSPI_MCR0_ARDFEN(0);
    BOARD_FLEX_SPI->MCR0 |= FLEXSPI_MCR0_ATDFEN(0);
}

// flash_erase_block(erase_addr)
// erases the block starting at addr. Block size according to the flash properties.
__attribute__((section(".ram_functions"))) status_t flash_erase_block(uint32_t erase_addr) {
    status_t status = kStatus_Fail;

    __disable_irq();
    SCB_DisableDCache();

    status = flexspi_nor_flash_erase_block(BOARD_FLEX_SPI, erase_addr);

    SCB_EnableDCache();
    __enable_irq();

    return status;
}

// flash_erase_sector(erase_addr_bytes)
// erases the sector starting at addr. Sector size according to the flash properties.
__attribute__((section(".ram_functions"))) status_t flash_erase_sector(uint32_t erase_addr) {
    status_t status = kStatus_Fail;

    __disable_irq();
    SCB_DisableDCache();

    status = flexspi_nor_flash_erase_sector(BOARD_FLEX_SPI, erase_addr);

    SCB_EnableDCache();
    __enable_irq();

    return status;
}

// flash_write_block(flash_dest_addr_bytes, data_source, length_bytes)
// writes length_byte data to the destination address
// the vfs driver takes care for erasing the sector if required
__attribute__((section(".ram_functions"))) status_t flash_write_block(uint32_t dest_addr, const uint8_t *src, uint32_t length) {
    status_t status = kStatus_Fail;
    uint32_t write_length;
    uint32_t next_addr;

    if (length == 0) {
        status = kStatus_Success;  // Nothing to do
    } else {


        // write data in chunks not crossing a page boundary
        do {
            next_addr = dest_addr - (dest_addr % PAGE_SIZE_BYTES) + PAGE_SIZE_BYTES; // next page boundary
            write_length = next_addr - dest_addr;  // calculate write length based on destination address and subsequent page boundary.
            if (write_length > length) { // compare possible write_length against remaining data length
                write_length = length;
            }

            __disable_irq();
            SCB_DisableDCache();

            status = flexspi_nor_flash_page_program(BOARD_FLEX_SPI, dest_addr, (uint32_t *)src, write_length);

            SCB_EnableDCache();
            __enable_irq();

            // Update remaining data length
            length -= write_length;

            // Move source and destination pointer
            src += write_length;
            dest_addr += write_length;
        } while ((length > 0) && (status == kStatus_Success));


    }
    return status;
}

// flash_read_block(flash_src_addr_bytes, data_dest, length_bytes)
// read length_byte data to the source address
// It is just a shim to provide the same structure for read_block and write_block.
__attribute__((section(".ram_functions"))) void flash_read_block(uint32_t src_addr, uint8_t *dest, uint32_t length) {
    memcpy(dest, (const uint8_t *)(BOARD_FLEX_SPI_ADDR_BASE + src_addr), length);
}
