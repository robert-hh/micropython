MICROPY_PY_NETWORK = 1
CFLAGS += -DMICROPY_PY_NETWORK=1
MICROPY_PY_NETWORK_ESP_HOSTED = 1
MICROPY_PY_LWIP = 1
MICROPY_PY_SSL = 1
MICROPY_SSL_MBEDTLS = 1

SRC_C += \
    esp_hosted_hal.c \
    mpbthciport.c \
    mpnimbleport.c \
    mpnetworkport.c \
    mbedtls/mbedtls_port.c

INC += \
    -Ilwip_inc
