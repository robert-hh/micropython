MCU_SERIES = MIMXRT1011
MCU_VARIANT = MIMXRT1011DAE5A

MICROPY_FLOAT_IMPL = single
MICROPY_HW_FLASH_TYPE = qspi_nor_flash
MICROPY_HW_FLASH_SIZE = 0x200000  # 2MB
MICROPY_HW_FLASH_RESERVED ?= 0x1000  # 4KB
MICROPY_HW_FLASH_CLK = kFlexSpiSerialClk_100MHz
MICROPY_HW_FLASH_QE_CMD = 0x01
MICROPY_HW_FLASH_QE_ARG = 0x40

USE_UF2_BOOTLOADER = 1

CFLAGS += -DMICROPY_HW_FLASH_DQS=kFlexSPIReadSampleClk_LoopbackInternally

SRC_C += \
	hal/flexspi_nor_flash.c \

MICROPY_PY_NETWORK_NINAW10 ?= 1
MICROPY_PY_SSL ?= 1
MICROPY_SSL_MBEDTLS ?= 1

MICROPY_PY_BLUETOOTH ?= 1
MICROPY_BLUETOOTH_NIMBLE ?= 1
