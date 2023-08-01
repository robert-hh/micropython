/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Arduino SA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ESP-Hosted WiFi HAL.
 */

#include "py/mphal.h"

#if MICROPY_PY_NETWORK_ESP_HOSTED

#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "modmachine.h"
#include "extmod/modmachine.h"
#include "pin_af.h"
#include "mpconfigboard.h"

#include "esp_hosted_hal.h"
#include "esp_hosted_wifi.h"

extern void mod_network_poll_events(void);

static mp_obj_t esp_hosted_pin_irq_callback(mp_obj_t self_in) {
    mod_network_poll_events();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(esp_hosted_pin_irq_callback_obj, esp_hosted_pin_irq_callback);

int esp_hosted_hal_init(uint32_t mode) {

    // Perform a hard reset and set pins to their defaults.
    esp_hosted_hal_deinit();

    if (mode == ESP_HOSTED_MODE_BT) {
        // For Bluetooth mode, init is done.
        return 0;
    }

    mp_hal_pin_input(MICROPY_HW_WIFI_HANDSHAKE);
    mp_hal_pin_input(MICROPY_HW_WIFI_DATAREADY);

    // Initialize SPI.
    mp_obj_t spi_args[] = {
        MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_SPI_ID),
        MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_SPI_BAUDRATE),
        MP_OBJ_NEW_QSTR(MP_QSTR_phase), MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_QSTR(MP_QSTR_polarity), MP_OBJ_NEW_SMALL_INT(1),
        MP_ROM_QSTR(MP_QSTR_sck), MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_SPI_SCK),
        MP_ROM_QSTR(MP_QSTR_miso), MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_SPI_MISO),
        MP_ROM_QSTR(MP_QSTR_mosi), MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_SPI_MOSI),
    };

    MP_STATE_PORT(mp_wifi_spi) =
        MP_OBJ_TYPE_GET_SLOT(&machine_spi_type, make_new)((mp_obj_t)&machine_spi_type, 2, 5, spi_args);

    // SPI might change the direction/mode of CS pin,
    // set it to GPIO again just in case.
    mp_hal_pin_output(MICROPY_HW_WIFI_SPI_CS);
    mp_hal_pin_write(MICROPY_HW_WIFI_SPI_CS, 1);
    return 0;
}

void esp_hosted_hal_irq_enable(bool enable) {
    #ifdef MICROPY_HW_WIFI_IRQ_PIN
    machine_pin_obj_t *micropy_hw_wifi_irq_pin = (machine_pin_obj_t *)pin_find(MP_OBJ_NEW_SMALL_INT(MICROPY_HW_WIFI_IRQ_PIN));

    // Disable Pin-IRQ for MICROPY_HW_WIFI_IRQ_PIN
    mp_obj_t pin_args[] = {
        NULL, // Method pointer
        (mp_obj_t)micropy_hw_wifi_irq_pin,   // Pin object
        mp_const_none  // Set to None
    };
    mp_load_method_maybe((mp_obj_t)micropy_hw_wifi_irq_pin, MP_QSTR_irq, pin_args);
    if (pin_args[0] && pin_args[1]) {
        mp_call_method_n_kw(1, 0, pin_args);
    }

    if (enable) {
        // Enable Pin-IRQ for the handshake PIN to call esp_hosted_wifi_poll()
        mp_obj_t irq_rising_attr[2];
        mp_load_method_maybe((mp_obj_t)micropy_hw_wifi_irq_pin, MP_QSTR_IRQ_RISING, irq_rising_attr);

        if (irq_rising_attr[0] != MP_OBJ_NULL && irq_rising_attr[1] == MP_OBJ_NULL) {
            // value for IRQ rising found
            mp_obj_t pin_args[] = {
                NULL, // Method pointer
                (mp_obj_t)micropy_hw_wifi_irq_pin,   // Pin object
                (mp_obj_t)&esp_hosted_pin_irq_callback_obj,   // Callback function object
                NULL,  // The Rising edge value is set below.
                mp_const_true,  // Hard IRQ, since the actual polling is scheduled.
            };
            pin_args[3] = irq_rising_attr[0];
            mp_load_method_maybe((mp_obj_t)micropy_hw_wifi_irq_pin, MP_QSTR_irq, pin_args);
            if (pin_args[0] != MP_OBJ_NULL && pin_args[1] != MP_OBJ_NULL) {
                mp_call_method_n_kw(3, 0, pin_args);
            }
        }
    }
    #endif
}

#endif // MICROPY_PY_NETWORK_ESP_HOSTED
