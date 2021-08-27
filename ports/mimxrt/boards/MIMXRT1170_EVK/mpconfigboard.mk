MCU_SERIES = MIMXRT1176
MCU_VARIANT = MIMXRT1176DVL6B

MICROPY_FLOAT_IMPL = double

BOARD_FLASH_TYPE ?= qspi_nor
BOARD_FLASH_SIZE ?= 0x1000000  # 16MB

CFLAGS += -DCPU_MIMXRT1176AVM8A_cm7 \
			-DCFG_TUSB_MCU=OPT_MCU_MIMXRT11XX \
			-DCPU_HEADER_H='<$(MCU_SERIES)_cm7.h>'

SRC_SS = $(MCU_DIR)/gcc/startup_$(MCU_SERIES)_cm7.S
SRC_HAL_IMX_C += \
		$(MCU_DIR)/system_$(MCU_SERIES)_cm7.c \
		$(MCU_DIR)/drivers/cm7/fsl_cache.c \
		$(MCU_DIR)/drivers/fsl_pmu.c \
		$(MCU_DIR)/drivers/fsl_common_arm.c \
		$(MCU_DIR)/drivers/fsl_anatop_ai.c

INC += -I$(TOP)/$(MCU_DIR)/drivers/cm7

JLINK_PATH ?= /media/RT1170-EVKB/

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
