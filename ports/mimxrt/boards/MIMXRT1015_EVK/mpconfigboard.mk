MCU_SERIES = MIMXRT1015
MCU_VARIANT = MIMXRT1015DAF5A
CFLAGS += -DMIMXRT101x_SERIES

MICROPY_FLOAT_IMPL = single
MICROPY_HW_FLASH_TYPE = qspi_nor_flash
MICROPY_HW_FLASH_SIZE = 0x1000000  # 16MB
MICROPY_HW_FLASH_CLK = kFlexSpiSerialClk_100MHz
MICROPY_HW_FLASH_QE_CMD = 0x31
MICROPY_HW_FLASH_QE_ARG = 0x02

MICROPY_BOOT_BUFFER_SIZE = (32 * 1024)

USE_UF2_BOOTLOADER = 1
