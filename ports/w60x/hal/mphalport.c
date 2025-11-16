/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 * Copyright (c) 2023 Robert Hammelrath
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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "wm_osal.h"
#include "wm_cpu.h"
#include "wm_regs.h"
#include "wm_watchdog.h"
#include "wm_irq.h"
#include "wm_rtc.h"
#include "modmachine.h"

#include "py/obj.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "shared/runtime/pyexec.h"
#include "shared/timeutils/timeutils.h"
#include "mphalport.h"

#define CNT_START_VALUE (0xffffffff)
volatile uint64_t wdg_ticks_total;
mp_uint_t wdg_ticks_reload_value = CNT_START_VALUE;
static mp_uint_t ticks_per_us = 40;

extern uint64_t get_wdg_counter_value(void);

void timer_init0() {

    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    ticks_per_us = sysclk.apbclk;
    wdg_ticks_total = 0;

    tls_reg_write32(HR_WDG_LOAD_VALUE, wdg_ticks_reload_value);
    tls_reg_write32(HR_WDG_CTRL, 0x1);              /* enable irq */
    tls_reg_write32(HR_WDG_INT_CLR, 0x01);

    tls_irq_enable(WATCHDOG_INT);
}

void tls_sys_reset(void) {
    tls_reg_write32(HR_WDG_LOCK, 0x1ACCE551);
    tls_reg_write32(HR_WDG_LOAD_VALUE, 0x100);
    tls_reg_write32(HR_WDG_CTRL, 0x3);
    tls_reg_write32(HR_WDG_LOCK, 1);
}

void mp_hal_wdg_enable(mp_uint_t usec) {
    // Adjust the wdg_ticks_total counter
    wdg_ticks_total += wdg_ticks_reload_value - tls_reg_read32(HR_WDG_CUR_VALUE);
    // Load the new value
    wdg_ticks_reload_value = ticks_per_us * usec;
    // Enable the Watchdog
    tls_reg_write32(HR_WDG_LOCK, 0x1ACCE551); // For changing the timeout
    tls_reg_write32(HR_WDG_LOAD_VALUE, wdg_ticks_reload_value);
    tls_reg_write32(HR_WDG_INT_CLR, 0x01);
    tls_reg_write32(HR_WDG_CTRL, 0x3);      /* enable irq & reset */
    tls_reg_write32(HR_WDG_LOCK, 1);
}

void mp_hal_wdg_feed(void) {
    // Adjust the wdg_ticks_total counter
    wdg_ticks_total += wdg_ticks_reload_value - tls_reg_read32(HR_WDG_CUR_VALUE);
    // Reset the watchdog
    tls_reg_write32(HR_WDG_LOCK, 0x1ACCE551);
    tls_reg_write32(HR_WDG_INT_CLR, 0x01);
    tls_reg_write32(HR_WDG_LOCK, 1);
}

// simplified division by the constant 40. Adapted by @rkompass,
// according to Henry S. Warren JR.: "Hackers Delight" 2nd Ed., ISBN-13: 978-0-321-84268-8
// Even if it does not look simple, the algorithm takes a constant time of ~300 ns
// The 64 bit straight division a/40 in contrast takes 2-11 µs.

uint64_t mp_hal_ticks_us64(void) {
    uint64_t t = get_wdg_counter_value();
    uint64_t t0 = get_wdg_counter_value();
    // If the t > t0, a counter roll-over may have happened
    // during reading of t, which is the too large by wdg_ticks_reload_value.
    if (t > t0) {
        t -= wdg_ticks_reload_value;
    }
    t >>= 3;  // divide by 8
    uint64_t x = t & 0xffffffffull;    // lower 32 bits
    uint64_t y = t >> 32;   // upper 32 bits
    uint64_t xc = x * 0xccccccccull;
    uint64_t yc = y * 0x33333333ull;
    uint64_t r = (xc + x) >> 32;
    r += xc + y;
    r >>= 2;
    r += yc;
    r >>= 32;
    r += yc;
    return r;
}

mp_uint_t mp_hal_ticks_ms(void) {
    return mp_hal_ticks_us64() / 1000;
}

mp_uint_t mp_hal_ticks_us(void) {
    return mp_hal_ticks_us64();
}

static inline void delay(mp_uint_t us) {
    if (us < 20) {
        volatile int32_t loops = ((int32_t)us - 2) * 6 + us / 10 * 6;
        while (loops > 0) {
            loops--;
        }
    } else {
        mp_uint_t start = tls_reg_read32(HR_WDG_CUR_VALUE);
        mp_uint_t diff = (us - 4) * ticks_per_us;
        while ((start - tls_reg_read32(HR_WDG_CUR_VALUE)) < diff) {
            ;
        }
    }
}

static void __mp_hal_delay_ms(mp_uint_t ms) {
    if (ms / (1000 / HZ) > 0) {
        tls_os_time_delay(ms / (1000 / HZ));
    } else {
        delay(ms * 1000);
    }
}

void mp_hal_delay_ms(mp_uint_t ms) {
    if (!tls_get_isr_count()) {
        // IRQs enabled, so can use systick counter to do the delay
        mp_uint_t start = tls_os_get_time();
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while ((tls_os_get_time() - start) < (ms / (1000 / HZ))) {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        }

        if (ms < 2) {
            MICROPY_EVENT_POLL_HOOK
        }
    } else {
        __mp_hal_delay_ms(ms);
    }
}

void mp_hal_delay_us(mp_uint_t us) {
    if ((us / 1000) > 3) {
        mp_hal_delay_ms(us / 1000);
        delay(us % 1000);
    } else {
        delay(us);
    }
}

void mp_hal_delay_us_fast(mp_uint_t us) {
    delay(us);
}

mp_uint_t mp_hal_get_cpu_freq(void) {
    tls_sys_clk sysclk;
    tls_sys_clk_get(&sysclk);
    return sysclk.cpuclk * 1000000;
}

uint64_t mp_hal_time_ns(void) {
    uint64_t ns = 0;
    // Get current according to the RTC, which has only seconds resolution
    struct tm tblock;
    tls_get_rtc(&tblock);
    ns = timeutils_seconds_since_epoch(
        W600_YEAR_BASE + tblock.tm_year, tblock.tm_mon, tblock.tm_mday,
        tblock.tm_hour, tblock.tm_min, tblock.tm_sec);
    ns *= 1000000000ULL;
    return ns;
}
