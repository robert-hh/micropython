MCU_SERIES = MIMXRT1176
MCU_VARIANT = MIMXRT1176DVL6B

MICROPY_FLOAT_IMPL = double

SRC_C += \
	hal/flexspi_nor_flash.c \

JLINK_PATH ?= /media/RT1170-EVKB/

deploy: $(BUILD)/firmware.bin
	cp $< $(JLINK_PATH)
