#define MICROPY_HW_BOARD_NAME "ItsyBitsy M4 Express"
#define MICROPY_HW_MCU_NAME   "SAMD51G19A"

#define MICROPY_HW_DFLL_USB_SYNC    (1)

#define MICROPY_HW_QSPIFLASH        GD25Q16C

// Bluetooth config.
#define MICROPY_HW_BLE_UART_ID          (3)
#define MICROPY_HW_BLE_UART_RX          (16)
#define MICROPY_HW_BLE_UART_TX          (17)
// MOSI
#define MICROPY_HW_BLE_UART_RTS         (0)
#define MICROPY_HW_BLE_UART_BAUDRATE    (115200)
#define MICROPY_HW_BLE_UART_BAUDRATE_SECONDARY (460800)

// WiFi config.

#define MICROPY_HW_WIFI_SPI_ID          (1)
#define MICROPY_HW_WIFI_SPI_BAUDRATE    (8000000)

#define MICROPY_HW_WIFI_SPI_CS          (22)
#define MICROPY_HW_WIFI_SPI_MOSI        (0)
#define MICROPY_HW_WIFI_SPI_MISO        (55)
#define MICROPY_HW_WIFI_SPI_SCK         (1)

#define MICROPY_HW_WIFI_DATAREADY       (20)
#define MICROPY_HW_WIFI_HANDSHAKE       (21)
#define MICROPY_HW_WIFI_IRQ_PIN         (MICROPY_HW_WIFI_DATAREADY)

// ESP hosted control pins
#define MICROPY_HW_ESP_HOSTED_RESET     (23)
#define MICROPY_HW_ESP_HOSTED_GPIO0     (20)

#define MICROPY_HW_ESP_HOSTED_SHARED_PINS (1)
