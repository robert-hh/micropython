#define MICROPY_HW_BOARD_NAME "i.MX RT1170 EVK"
#define MICROPY_HW_MCU_NAME   "MIMXRT1176DVL6A"

// MIMXRT1170_EVK has 2 user LEDs
#define MICROPY_HW_LED1_PIN (pin_GPIO_AD_04)
#define MICROPY_HW_LED2_PIN (pin_GPIO_AD_26)
#define MICROPY_HW_LED_ON(pin) (mp_hal_pin_low(pin))
#define MICROPY_HW_LED_OFF(pin) (mp_hal_pin_high(pin))

#define MICROPY_HW_NUM_PIN_IRQS (4 * 32 + 3)

// Define mapping hardware UART # to logical UART #
// LPUART2  on D0/D1    -> 1
// LPUART3  on D12/D11  -> 2
// LPUART5  on D10/D13  -> 3
// LPUART11 on D15/D14  -> 4
// LPUART12 on D25/D26  -> 5
// LPUART8  on D33/D34  -> 6
// LPUART7  on D35/D36  -> 7

#define MICROPY_HW_UART_NUM     (sizeof(uart_index_table) / sizeof(uart_index_table)[0])
#define MICROPY_HW_UART_INDEX   { 0, 2, 3, 5, 11, 12, 8, 7 }

#define IOMUX_TABLE_UART \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_DISP_B2_10_LPUART2_TXD }, { IOMUXC_GPIO_DISP_B2_11_LPUART2_RXD }, \
    { IOMUXC_GPIO_AD_30_LPUART3_TXD }, { IOMUXC_GPIO_AD_31_LPUART3_RXD }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_28_LPUART5_TXD }, { IOMUXC_GPIO_AD_29_LPUART5_RXD }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_AD_00_LPUART7_TXD }, { IOMUXC_GPIO_AD_01_LPUART7_RXD }, \
    { IOMUXC_GPIO_AD_02_LPUART8_TXD }, { IOMUXC_GPIO_AD_03_LPUART8_RXD }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_LPSR_04_LPUART11_TXD }, { IOMUXC_GPIO_LPSR_05_LPUART11_RXD }, \
    { IOMUXC_GPIO_LPSR_10_LPUART12_TXD }, { IOMUXC_GPIO_LPSR_11_LPUART12_RXD },

// Define the mapping hardware SPI # to logical SPI #
// SCK/CS/SDO/SDI  HW-SPI   Logical SPI
// D13/D10/D11/D12 LPSPI1  ->  0
// D26/D28/D25/D24 LPSPI6  ->  1
// D24/ - /D14/D15 LPSPI5  ->  2

#define MICROPY_HW_SPI_INDEX { 1, 6, 5 }

#define IOMUX_TABLE_SPI \
    { IOMUXC_GPIO_AD_28_LPSPI1_SCK }, { IOMUXC_GPIO_AD_29_LPSPI1_PCS0 }, \
    { IOMUXC_GPIO_AD_30_LPSPI1_SOUT }, { IOMUXC_GPIO_AD_31_LPSPI1_SIN }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_LPSR_12_LPSPI5_SCK }, { IOMUXC_GPIO_LPSR_13_LPSPI5_PCS0 }, \
    { IOMUXC_GPIO_LPSR_04_LPSPI5_SOUT }, { IOMUXC_GPIO_LPSR_05_LPSPI5_SIN }, \
    { IOMUXC_GPIO_LPSR_10_LPSPI6_SCK }, { IOMUXC_GPIO_LPSR_09_LPSPI6_PCS0 }, \
    { IOMUXC_GPIO_LPSR_11_LPSPI6_SOUT }, { IOMUXC_GPIO_LPSR_12_LPSPI6_SIN },


#define DMA_REQ_SRC_RX { 0, kDmaRequestMuxLPSPI1Rx, kDmaRequestMuxLPSPI2Rx, \
                         kDmaRequestMuxLPSPI3Rx, kDmaRequestMuxLPSPI4Rx }

#define DMA_REQ_SRC_TX { 0, kDmaRequestMuxLPSPI1Tx, kDmaRequestMuxLPSPI2Tx, \
                         kDmaRequestMuxLPSPI3Tx, kDmaRequestMuxLPSPI4Tx }

// Define the mapping hardware I2C # to logical I2C #
// SDA/SCL  HW-I2C    Logical I2C
// D14/D15  LPI2C5 ->    0
// D1/D0    LPI2C3 ->    1
// A4/A5    LPI2C1 ->    2
// D26/D25  LPI2C6 ->    3
// D19/D18  LPI2C2 ->    4

#define MICROPY_HW_I2C_INDEX   { 5, 3, 1, 6, 2 }

#define IOMUX_TABLE_I2C \
    { IOMUXC_GPIO_AD_08_LPI2C1_SCL }, { IOMUXC_GPIO_AD_09_LPI2C1_SDA }, \
    { IOMUXC_GPIO_AD_18_LPI2C2_SCL }, { IOMUXC_GPIO_AD_19_LPI2C2_SDA }, \
    { IOMUXC_GPIO_DISP_B2_10_LPI2C3_SCL }, { IOMUXC_GPIO_DISP_B2_11_LPI2C3_SDA }, \
    { 0 }, { 0 }, \
    { IOMUXC_GPIO_LPSR_05_LPI2C5_SCL }, { IOMUXC_GPIO_LPSR_04_LPI2C5_SDA }, \
    { IOMUXC_GPIO_LPSR_11_LPI2C6_SCL }, { IOMUXC_GPIO_LPSR_10_LPI2C6_SDA },

// USDHC1

#define USDHC_DUMMY_PIN NULL, 0
#define MICROPY_USDHC1 \
    { \
        .cmd = {GPIO_SD_B1_00_USDHC1_CMD}, \
        .clk = { GPIO_SD_B1_01_USDHC1_CLK }, \
        .cd_b = { USDHC_DUMMY_PIN }, \
        .data0 = { GPIO_SD_B1_02_USDHC1_DATA0 }, \
        .data1 = { GPIO_SD_B1_03_USDHC1_DATA1 }, \
        .data2 = { GPIO_SD_B1_04_USDHC1_DATA2 }, \
        .data3 = { GPIO_SD_B1_05_USDHC1_DATA3 }, \
    }

// Network definitions
// Transceiver Phy Address
#define ENET_PHY_ADDRESS    (2)
#define ENET_PHY            KSZ8081
#define ENET_PHY_OPS        phyksz8081_ops

// Etherner PIN definitions
#define ENET_RESET_PIN      pin_GPIO_LPSR_12
#define ENET_INT_PIN        pin_GPIO_AD_12

#define IOMUX_TABLE_ENET \
    { IOMUXC_GPIO_DISP_B2_06_ENET_RX_DATA00, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_07_ENET_RX_DATA01, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_08_ENET_RX_EN, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_02_ENET_TX_DATA00, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_03_ENET_TX_DATA01, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_04_ENET_TX_EN, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_DISP_B2_05_ENET_REF_CLK, 1, 0x71u }, \
    { IOMUXC_GPIO_DISP_B2_09_ENET_RX_ER, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_AD_33_ENET_MDIO, 0, 0xB0E9u }, \
    { IOMUXC_GPIO_AD_32_ENET_MDC, 0, 0xB0E9u },
