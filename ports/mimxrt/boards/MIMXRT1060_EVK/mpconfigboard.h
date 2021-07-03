#define MICROPY_HW_BOARD_NAME "i.MX RT1060 EVK"
#define MICROPY_HW_MCU_NAME   "MIMXRT1062DVJ6A"

#define BOARD_FLASH_SIZE (8 * 1024 * 1024)

// MIMXRT1060_EVK has 1 user LED
#define MICROPY_HW_LED1_PIN (pin_GPIO_AD_B0_09)
#define MICROPY_HW_LED_ON(pin) (mp_hal_pin_low(pin))
#define MICROPY_HW_LED_OFF(pin) (mp_hal_pin_high(pin))
#define BOARD_FLASH_CONFIG_HEADER_H "evkmimxrt1060_flexspi_nor_config.h"

#define MICROPY_HW_NUM_PIN_IRQS (4 * 32 + 3)

// Define mapping logical UART # to hardware UART #
// LPUART3 on D0/D1  -> 1
// LPUART2 on D7/D6  -> 2
// LPUART6 on D8/D9  -> 3
// LPUART8 on A1/A0  -> 4

#define MICROPY_HW_UART_NUM     (sizeof(uart_index_table) / sizeof(uart_index_table)[0])
#define MICROPY_HW_UART_INDEX   { 0, 3, 2, 6, 8 }

#define IOMUX_TABLE_UART \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_B1_02_LPUART2_TX }, { IOMUXC_GPIO_AD_B1_03_LPUART2_RX }, \
    { IOMUXC_GPIO_AD_B1_06_LPUART3_TX }, { IOMUXC_GPIO_AD_B1_07_LPUART3_RX }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_B0_02_LPUART6_TX }, { IOMUXC_GPIO_AD_B0_03_LPUART6_RX }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_B1_10_LPUART8_TX }, { IOMUXC_GPIO_AD_B1_11_LPUART8_RX },

#define MICROPY_HW_SPI_INDEX { 1 }

#define IOMUX_TABLE_SPI \
    { IOMUXC_GPIO_SD_B0_00_LPSPI1_SCK }, { IOMUXC_GPIO_SD_B0_01_LPSPI1_PCS0 }, \
    { IOMUXC_GPIO_SD_B0_02_LPSPI1_SDO }, { IOMUXC_GPIO_SD_B0_03_LPSPI1_SDI },

#define DMA_REQ_SRC_RX { 0, kDmaRequestMuxLPSPI1Rx, kDmaRequestMuxLPSPI2Rx, \
                            kDmaRequestMuxLPSPI3Rx, kDmaRequestMuxLPSPI4Rx }

#define DMA_REQ_SRC_TX { 0, kDmaRequestMuxLPSPI1Tx, kDmaRequestMuxLPSPI2Tx, \
                            kDmaRequestMuxLPSPI3Tx, kDmaRequestMuxLPSPI4Tx } 

// Define the mapping hardware I2C # to logical I2C #
// SDA/SCL  HW-I2C    Logical I2C
// D14/D15  LPI2C1 ->    0
// D1/D0    LPI2C3 ->    1

#define MICROPY_HW_I2C_INDEX   { 1, 3 }

#define IOMUX_TABLE_I2C \
    { IOMUXC_GPIO_AD_B1_00_LPI2C1_SCL }, { IOMUXC_GPIO_AD_B1_01_LPI2C1_SDA }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_B1_07_LPI2C3_SCL }, { IOMUXC_GPIO_AD_B1_06_LPI2C3_SDA },

// Network definitions
#define MICROPY_PY_NETWORK                  (1)
#define MICROPY_PY_USOCKET                  (1)
#define MICROPY_PY_LWIP                     (1)
#define MICROPY_HW_ETH_MDC                  (1)
#define MICROPY_PY_LWIP_SOCK_RAW            (MICROPY_PY_LWIP)

// Prevent the "LWIP task" from running.
#define MICROPY_PY_LWIP_ENTER   MICROPY_PY_PENDSV_ENTER
#define MICROPY_PY_LWIP_REENTER MICROPY_PY_PENDSV_REENTER
#define MICROPY_PY_LWIP_EXIT    MICROPY_PY_PENDSV_EXIT

// Etherner PIN definitions
#define ENET_RESET_PIN      pin_GPIO_AD_B0_09
#define ENET_INT_PIN        pin_GPIO_AD_B0_10

#define IOMUX_TABLE_ENET \
    { IOMUXC_GPIO_B1_10_ENET_TX_CLK }, \
    { IOMUXC_GPIO_B1_04_ENET_RX_DATA00 }, \
    { IOMUXC_GPIO_B1_05_ENET_RX_DATA01 }, \
    { IOMUXC_GPIO_B1_06_ENET_RX_EN }, \
    { IOMUXC_GPIO_B1_07_ENET_TX_DATA00 }, \
    { IOMUXC_GPIO_B1_08_ENET_TX_DATA01 }, \
    { IOMUXC_GPIO_B1_09_ENET_TX_EN }, \
    { IOMUXC_GPIO_B1_11_ENET_RX_ER }, \
    { IOMUXC_GPIO_EMC_41_ENET_MDIO }, \
    { IOMUXC_GPIO_EMC_40_ENET_MDC },
