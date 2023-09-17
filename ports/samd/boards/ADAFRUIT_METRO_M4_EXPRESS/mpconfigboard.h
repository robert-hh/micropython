#define MICROPY_HW_BOARD_NAME               "Metro M4 Express Airlift"
#define MICROPY_HW_MCU_NAME                 "SAMD51J19A"

#define MICROPY_HW_XOSC32K                  (1)
#define MICROPY_HW_QSPIFLASH                GD25Q16C

// WiFi config.
#define MICROPY_HW_WIFI_SPI_ID              (2)
#define MICROPY_HW_WIFI_SPI_BAUDRATE        (8000000)

#define MICROPY_HW_WIFI_SPI_MOSI            (12)
#define MICROPY_HW_WIFI_SPI_SCK             (13)
#define MICROPY_HW_WIFI_SPI_MISO            (14)
#define MICROPY_HW_WIFI_SPI_CS              (15)

#define MICROPY_HW_WIFI_DATAREADY           (33)
#define MICROPY_HW_WIFI_HANDSHAKE           (36)
#define MICROPY_HW_WIFI_IRQ_PIN             (MICROPY_HW_WIFI_DATAREADY)

// Bluetooth config.
#define MICROPY_HW_BLE_UART_ID              (0)
#define MICROPY_HW_BLE_UART_RX              (7)
#define MICROPY_HW_BLE_UART_TX              (4)
// MOSI
#define MICROPY_HW_BLE_UART_RTS             (55)
#define MICROPY_HW_BLE_UART_BAUDRATE        (460800)
#define MICROPY_HW_BLE_UART_BAUDRATE_SECONDARY (460800)


// ESP hosted control pins
#define MICROPY_HW_ESP_HOSTED_RESET         (37)
#define MICROPY_HW_ESP_HOSTED_GPIO0         (MICROPY_HW_WIFI_DATAREADY)

#define MICROPY_HW_ESP_HOSTED_SHARED_PINS   (1)
