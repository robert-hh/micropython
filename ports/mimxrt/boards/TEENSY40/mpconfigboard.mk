MCU_SERIES = MIMXRT1062
MCU_VARIANT = MIMXRT1062DVJ6A

MICROPY_FLOAT_IMPL = double

BOARD_FLASH_TYPE ?= qspi_nor
BOARD_FLASH_SIZE ?= 0x200000  # 2MB
BOARD_FLASH_RESERVED ?= 0x1000  # 4KB

CFLAGS += -DCPU_HEADER_H='<$(MCU_SERIES).h>' \

SRC_C += \
	hal/flexspi_nor_flash.c \

deploy: $(BUILD)/firmware.hex
	teensy_loader_cli --mcu=imxrt1062 -v -w $<
