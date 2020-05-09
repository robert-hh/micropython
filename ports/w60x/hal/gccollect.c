﻿/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#include <stdint.h>

#include "py/obj.h"
#include "py/gc.h"
#include "py/mpstate.h"
#include "py/mpthread.h"
#include "lib/utils/gchelper.h"

#define MPY_STACK_SIZE              (16 * 1024)
#define MPY_STACK_LEN               (MPY_STACK_SIZE / sizeof(OS_STK))

extern OS_STK mpy_task_stk[MPY_STACK_LEN];

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live [now] on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)MP_STATE_THREAD(stack_top) - sp) / sizeof(uint32_t));

    // trace root pointers from any threads
#if MICROPY_PY_THREAD
    mp_thread_gc_others();
#endif

    // end the GC
    gc_collect_end();
}
