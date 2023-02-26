/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Robert Hammelrath, Yichen from Winner Micro
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
 */


#include "wm_type_def.h"
#include "wm_watchdog.h"

#include "py/runtime.h"
#include "mphalport.h"
#include "py/stream.h"
#include "bootloader_helper.h"

extern uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags);

int mp_hal_stdin_rx_chr(void);

void bootloader_helper(void) {

    *(uint32_t *)BOOTLOADER_MAGIC_ADDRESS = BOOTLOADER_MAGIC_NUMBER;
    tls_sys_reset();
}

#define ESC_LIMIT  10
#define TIMEOUT_INITIAL  100
#define TIMEOUT_SERIES   20

void check_esc_on_boot(void) {
    uint32_t time = mp_hal_ticks_ms();
    uint32_t timeout = TIMEOUT_INITIAL;
    int esc_count = 0;
    int chr;

    do {
        chr = 0;
        do {
            if (mp_hal_stdio_poll(MP_STREAM_POLL_RD) != 0) {
                chr = mp_hal_stdin_rx_chr();
                break;
            }
        } while ((mp_hal_ticks_ms() - time) < timeout);
        if (chr != 0x1b) {
            return;
        }
        esc_count += 1;
        timeout = TIMEOUT_SERIES;
        time = mp_hal_ticks_ms();
    } while (esc_count < ESC_LIMIT);

    bootloader_helper();

    return;
}
