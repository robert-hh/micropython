CFLAGS_MCU += -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS_MCU += -DCFG_TUSB_MCU=OPT_MCU_SAMD51

MPY_CROSS_MCU_ARCH = armv7m

MICROPY_HW_CODESIZE ?= 368K

MICROPY_VFS_LFS2 ?= 1
MICROPY_VFS_FAT ?= 1
FROZEN_MANIFEST ?= mcu/$(MCU_SERIES_LOWER)/manifest.py

SRC_S += shared/runtime/gchelper_thumb2.s

SRC_C += \
	fatfs_port.c \

ifneq ($(BOARD_VARIANT),)
    MICROPY_PY_NETWORK = 1
    CFLAGS += -DMICROPY_PY_NETWORK=1
    MICROPY_PY_NETWORK_ESP_HOSTED = 1

    MICROPY_PY_LWIP = 1
    MICROPY_PY_SSL = 1
    MICROPY_SSL_MBEDTLS = 1

    ifeq ($(BOARD_VARIANT), WLAN_BLE)
        MICROPY_PY_BLUETOOTH = 1
        CFLAGS += -DMICROPY_PY_BLUETOOTH=$(MICROPY_PY_BLUETOOTH)
        MICROPY_BLUETOOTH_NIMBLE = 1
        MICROPY_BLUETOOTH_BTSTACK = 0
    endif

    SRC_C += \
        esp_hosted_hal.c \
        mpbthciport.c \
        mpnimbleport.c \
        mpnetworkport.c \
        mbedtls/mbedtls_port.c

    INC += \
        -Ilwip_inc
endif
