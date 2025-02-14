MCU_SERIES = MIMXRT1064
MCU_VARIANT = MIMXRT1064DVL6A
CFLAGS += -DMIMXRT106x_SERIES

MICROPY_FLOAT_IMPL = double
MICROPY_HW_FLASH_TYPE = internal
MICROPY_HW_FLASH_SIZE = 0x400000  # 4MB
MICROPY_HW_FLASH_CLK = kFlexSpiSerialClk_100MHz
MICROPY_HW_FLASH_QE_CMD = 0x31
MICROPY_HW_FLASH_QE_ARG = 0x02

MICROPY_HW_SDRAM_AVAIL = 1
MICROPY_HW_SDRAM_SIZE  = 0x2000000  # 32MB

MICROPY_PY_LWIP = 1
MICROPY_PY_SSL = 1
MICROPY_SSL_MBEDTLS = 1

USE_UF2_BOOTLOADER = 1

FROZEN_MANIFEST ?= $(BOARD_DIR)/manifest.py

JLINK_PATH ?= /media/RT1064-EVK/

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
