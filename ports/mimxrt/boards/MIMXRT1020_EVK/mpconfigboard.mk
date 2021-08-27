MCU_SERIES = MIMXRT1021
MCU_VARIANT = MIMXRT1021DAG5A

MICROPY_FLOAT_IMPL = double

BOARD_FLASH_TYPE ?= qspi_nor
BOARD_FLASH_SIZE ?= 0x800000  # 8MB

CFLAGS += -DCPU_HEADER_H='<$(MCU_SERIES).h>' \
			-DCFG_TUSB_MCU=OPT_MCU_MIMXRT10XX

SRC_SS = $(MCU_DIR)/gcc/startup_$(MCU_SERIES).S
SRC_HAL_IMX_C += \
		$(MCU_DIR)/system_$(MCU_SERIES).c \
		$(MCU_DIR)/drivers/fsl_adc.c \
		$(MCU_DIR)/drivers/fsl_cache.c \
		$(MCU_DIR)/drivers/fsl_trng.c

JLINK_PATH ?= /media/RT1020-EVK/
JLINK_COMMANDER_SCRIPT = $(BUILD)/script.jlink


ifdef JLINK_IP
JLINK_CONNECTION_SETTINGS = -IP $(JLINK_IP)
else
JLINK_CONNECTION_SETTINGS =
endif


deploy_jlink: $(BUILD)/firmware.hex
	$(ECHO) "ExitOnError 1" > $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "speed auto" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "r" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "st" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "loadfile \"$(realpath $(BUILD)/firmware.hex)\"" >> $(JLINK_COMMANDER_SCRIPT)
	$(ECHO) "qc" >> $(JLINK_COMMANDER_SCRIPT)
	$(JLINK_PATH)JLinkExe -device $(MCU_VARIANT) -if SWD $(JLINK_CONNECTION_SETTINGS) -CommanderScript $(JLINK_COMMANDER_SCRIPT)

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
