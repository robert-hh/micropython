MCU_SERIES = MIMXRT1062
MCU_VARIANT = MIMXRT1062DVJ6A

MICROPY_FLOAT_IMPL = double
MICROPY_PY_LWIP = 1

SRC_ETH_C += \
	hal/phy/device/phydp83825/fsl_phydp83825.c

deploy: $(BUILD)/firmware.hex
	teensy_loader_cli --mcu=imxrt1062 -v -w $<
