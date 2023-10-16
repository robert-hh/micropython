/* Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 * Copyright (c) 2024 Robert Hammelrath
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

#include "wm_osal.h"
#include "wm_regs.h"
#include "wm_watchdog.h"

extern volatile uint64_t wdg_ticks_total;
extern uint32_t wdg_ticks_reload_value;

void WDG_IRQHandler(void *data) {
    wdg_ticks_total += wdg_ticks_reload_value;
    tls_reg_write32(HR_WDG_INT_CLR, 0x01);
}

uint64_t get_wdg_counter_value(void) {
    // The below sequence of statements just expresses the importance of
    // HR_WDG_CUR_VALUE being loaded before wdg_ticks_total. The compiler
    // will do that anyhow, even if the WDG register access is in the return
    // statement's expression.
    int32_t wdg_ticks_value_at_get = *(TLS_REG *)HR_WDG_CUR_VALUE;
    return wdg_ticks_total + (uint64_t)(wdg_ticks_reload_value - wdg_ticks_value_at_get);
}
