MCU_SERIES = MIMXRT1062
MCU_VARIANT = MIMXRT1062DVJ6A

MICROPY_FLOAT_IMPL = double

BOARD_FLASH_TYPE ?= qspi_nor
BOARD_FLASH_SIZE ?= 0x800000  # 8MB
BOARD_FLASH_RESERVED ?= 0x1000  # 4KB

CFLAGS += -DCPU_HEADER_H='<$(MCU_SERIES).h>' \
			-DCFG_TUSB_MCU=OPT_MCU_MIMXRT10XX

SRC_SS = $(MCU_DIR)/gcc/startup_$(MCU_SERIES).S
SRC_C += \
	hal/flexspi_nor_flash.c

SRC_HAL_IMX_C += \
		$(MCU_DIR)/system_$(MCU_SERIES).c \
		$(MCU_DIR)/drivers/fsl_adc.c \
		$(MCU_DIR)/drivers/fsl_cache.c \
		$(MCU_DIR)/drivers/fsl_trng.c

deploy: $(BUILD)/firmware.hex
	teensy_loader_cli --mcu=imxrt1062 -v -w $<
