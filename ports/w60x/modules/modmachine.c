/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "wm_include.h"
#include "wm_cpu.h"
#include "wm_watchdog.h"
#include "tls_wireless.h"
#include "task.h"

#include "py/runtime.h"
#include "extmod/modmachine.h"
#include "modmachine.h"

extern uint8_t *wpa_supplicant_get_mac(void);
extern void bootloader_helper(void);
extern void timer_init0(void);

#if MICROPY_PY_MACHINE

typedef enum {
    MP_PWRON_RESET = 0,
    MP_DEEPSLEEP_RESET
} reset_reason_t;

typedef enum {
    MP_PS_SLEEP = 0,
    MP_DEEP_SLEEP
} sleep_type_t;

typedef enum {
    MP_OP_PS_WAKEUP = 1,
    MP_OP_GPIO_WAKEUP = 0,
    MP_OP_PS_SLEEP = 0,
    MP_OP_TIMER_WAKEUP = 1,
} sleep_mode_t;


#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_Pin),                 MP_ROM_PTR(&machine_pin_type) }, \
    { MP_ROM_QSTR(MP_QSTR_Timer),               MP_ROM_PTR(&machine_timer_type) }, \
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&machine_sleep_obj) }, \
    { MP_ROM_QSTR(MP_QSTR_RTC),                 MP_ROM_PTR(&machine_rtc_type) }, \
    \
    /* Class constants. */ \
    /* Use numerical constants instead of the symbolic names, */ \
    /* since the names differ between SAMD21 and SAMD51. */ \
    { MP_ROM_QSTR(MP_QSTR_PSWAKEUP),            MP_ROM_INT(MP_OP_PS_WAKEUP) }, \
    { MP_ROM_QSTR(MP_QSTR_PSLEEP),              MP_ROM_INT(MP_OP_PS_SLEEP) }, \
    { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(MP_PWRON_RESET) }, \
    { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP_RESET),     MP_ROM_INT(MP_DEEPSLEEP_RESET) }, \


static mp_obj_t mp_machine_get_freq(void) {
    // get
    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    return mp_obj_new_int(sysclk.cpuclk * 1000000);
}

static void mp_machine_set_freq(size_t n_args, const mp_obj_t *args) {
    mp_int_t freq = mp_obj_get_int(args[0]) / 1000000;
    if (freq != 40 && freq != 80) {
        mp_raise_ValueError("frequency can only be either 40MHz or 80MHz");
    }
    if (40 == freq) {
        tls_sys_clk_set(CPU_CLK_40M);
    } else {
        tls_sys_clk_set(CPU_CLK_80M);
    }
    tls_os_timer_init();
    timer_init0(); // Reset the counters for ticks.
}

static mp_obj_t machine_sleep(size_t n_args, const mp_obj_t *args) {
    mp_int_t mode;
    mode = MP_OP_PS_SLEEP;
    if (n_args > 0) {
        mode = mp_obj_get_int(args[0]);
    }
    tls_wl_if_ps(mode);// MP_OP_PS_WAKEUP,MP_OP_PS_SLEEP
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_sleep_obj, 0, 1, machine_sleep);

// Just a dummy function.
static void mp_machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    if (n_args > 0) {
        mp_int_t period = mp_obj_get_int(args[0]);
        if (period > 0) {
            mp_hal_delay_ms(period);
        }
    }
};

NORETURN static void mp_machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    mp_int_t period = 0x7f000000;  // Just a very long period
    if (n_args > 0) {
        period = mp_obj_get_int(args[0]);
        if (period > 0) {
            tls_wl_if_standby(MP_OP_TIMER_WAKEUP, 0, period);// MP_OP_GPIO_WAKEUP,MP_OP_TIMER_WAKEUP
        }
    } else {
        tls_wl_if_standby(MP_OP_GPIO_WAKEUP, 0, period);// MP_OP_GPIO_WAKEUP,MP_OP_TIMER_WAKEUP
    }
    while (true) {  // Just a dummy loop to silence a compiler warning
        ;
    }
}

static mp_int_t mp_machine_reset_cause(void) {
    if (tls_reg_read32(HR_PMU_INTERRUPT_SRC) & (1 << 8)) {
        return MP_DEEPSLEEP_RESET;
    } else {
        return MP_PWRON_RESET;
    }
}

NORETURN static void mp_machine_reset(void) {
    tls_sys_reset();
    while (true) {  // Just a dummy loop to silence a compiler warning
        ;
    }
}

NORETURN void mp_machine_bootloader(size_t n_args, const mp_obj_t *args) {
    bootloader_helper();
    while (true) {  // Just a dummy loop to silence a compiler warning
        ;
    }
}

static mp_obj_t mp_machine_unique_id(void) {
    uint8_t *chipid;
    chipid = wpa_supplicant_get_mac();
    return mp_obj_new_bytes(chipid, 6);
}

static void mp_machine_idle(void) {
    MICROPY_EVENT_POLL_HOOK
}

#endif // MICROPY_PY_MACHINE
