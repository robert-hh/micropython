include mcu/$(MCU_SERIES_LOWER)/mpconfigvariant_WLAN.mk

ifeq ($(MICROPY_HW_CODESIZE),$(filter $(MICROPY_HW_CODESIZE), 496K 1008K))
    MICROPY_PY_BLUETOOTH ?= 1
    MICROPY_BLUETOOTH_NIMBLE ?= 1
    SRC_C += \
        mpbthciport.c \
        mpnimbleport.c

    INC += \
        -I$(TOP)/extmod/nimble
endif
