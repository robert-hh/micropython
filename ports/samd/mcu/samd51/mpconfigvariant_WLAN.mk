MICROPY_PY_NETWORK ?= 1
CFLAGS += -DMICROPY_PY_NETWORK=1
MICROPY_PY_NETWORK_NINAW10 ?= 1
CFLAGS += -DMICROPY_PY_NETWORK_NINAW10=1

INC += -I$(TOP)/drivers/ninaw10
SRC_C += \
    mbedtls/mbedtls_port.c\
    nina_wifi_bsp.c
SHARED_SRC_C += \
    shared/netutils/dhcpserver.c \
    shared/netutils/netutils.c \
    shared/netutils/trace.c
DRIVERS_SRC_C += \
    drivers/ninaw10/nina_bthci_uart.c \
    drivers/ninaw10/nina_wifi_drv.c \
    drivers/ninaw10/nina_wifi_bsp.c \
    drivers/ninaw10/machine_pin_nina.c

ifeq ($(MICROPY_HW_CODESIZE),$(filter $(MICROPY_HW_CODESIZE), 496K 1008K))
    MICROPY_PY_SSL ?= 1
    MICROPY_SSL_MBEDTLS ?= 1
endif

FROZEN_MANIFEST ?= mcu/$(MCU_SERIES_LOWER)/manifest_$(BOARD_VARIANT).py
