MCU_SERIES = MIMXRT1052
MCU_VARIANT = MIMXRT1052DVL6B

MICROPY_FLOAT_IMPL = double

BOARD_FLASH_TYPE ?= hyperflash
BOARD_FLASH_SIZE ?= 0x4000000  # 64MB

CFLAGS += -DCPU_HEADER_H='<$(MCU_SERIES).h>' \
			-DCFG_TUSB_MCU=OPT_MCU_MIMXRT10XX

SRC_SS = $(MCU_DIR)/gcc/startup_$(MCU_SERIES).S
SRC_HAL_IMX_C += \
		$(MCU_DIR)/system_$(MCU_SERIES).c \
		$(MCU_DIR)/drivers/fsl_adc.c \
		$(MCU_DIR)/drivers/fsl_cache.c \
		$(MCU_DIR)/drivers/fsl_trng.c

JLINK_PATH ?= /media/RT1050-EVKB/

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
