/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
 * Copyright (c) 2017 Pycom Limited
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

#ifndef MICROPY_INCLUDED_W600_MPTHREADPORT_H
#define MICROPY_INCLUDED_W600_MPTHREADPORT_H

#include "wm_osal.h"

#define MPY_FTPS_PRIO               32
#define MPY_TASK_PRIO               33
#define MPY_STACK_SIZE              (16 * 1024)
#define MPY_STACK_LEN               (MPY_STACK_SIZE / sizeof(OS_STK))

extern OS_STK mpy_task_stk[MPY_STACK_LEN];

typedef struct _mp_thread_mutex_t {
    tls_os_mutex_t *sem;
} mp_thread_mutex_t;

void mp_thread_init(void *stack, uint32_t stack_len);
void mp_thread_gc_others(void);
int mp_thread_deinit(void);

#endif // MICROPY_INCLUDED_W600_MPTHREADPORT_H
