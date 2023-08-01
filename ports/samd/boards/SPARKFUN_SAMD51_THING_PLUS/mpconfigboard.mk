MCU_SERIES = SAMD51
CMSIS_MCU = SAMD51J20A
LD_FILES = boards/samd51x20a.ld sections.ld
TEXT0 = 0x4000

# The ?='s allow overriding in mpconfigboard.mk.
# MicroPython settings
MICROPY_HW_CODESIZE ?= 1008K

MICROPY_PY_LWIP = 1
MICROPY_PY_SSL = 1
MICROPY_SSL_MBEDTLS = 1

MICROPY_PY_BLUETOOTH = 1
CFLAGS += -DMICROPY_PY_BLUETOOTH=$(MICROPY_PY_BLUETOOTH)
MICROPY_BLUETOOTH_NIMBLE = 1
MICROPY_BLUETOOTH_BTSTACK = 0
MICROPY_PY_NETWORK = 1
MICROPY_PY_NETWORK_ESP_HOSTED = 1

MBEDTLS_CONFIG_FILE = '"$(BOARD_DIR)/mbedtls_config_board.h"'

SRC_C += \
    esp_hosted_hal.c \
    mpbthciport.c \
    mpnimbleport.c \
    mpnetworkport.c \
    mbedtls/mbedtls_port.c \

INC += \
    -Ilwip_inc \
	-I$(TOP)/extmod/nimble \
    -I$(TOP)/lib/mynewt-nimble/nimble/host/include \
    -I$(TOP)/lib/mynewt-nimble/nimble/include \
    -I$(TOP)/lib/mynewt-nimble/porting/nimble/include
