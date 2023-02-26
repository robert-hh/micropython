/*****************************************************************************
*
* File Name : main.c
*
* Description: main
*
* Copyright (c) 2019 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
*****************************************************************************/
#include <string.h>
#include "wm_include.h"
#include "wm_mem.h"
#include "wm_crypto_hard.h"
#include "wm_rtc.h"
#include "wm_watchdog.h"

#include "py/stackctrl.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"

#include "mpthreadport.h"

OS_STK mpy_task_stk[MPY_STACK_LEN];

void uart_init(void);
void machine_pins_init(void);
void timer_init0(void);
void tls_sys_reset(void);
void machine_pwm_deinit_all(void);
void check_esc_on_boot(void);

static inline void *get_sp(void) {
    void *sp;
    asm volatile ("mov %0, sp;" : "=r" (sp));
    return sp;
}

static void mpy_task(void *param) {
    volatile uint32_t sp = (uint32_t)get_sp();

    #if MICROPY_PY_THREAD
    // Register the MPY main task with it's stack in the thread list
    mp_thread_init(mpy_task_stk, MPY_STACK_SIZE);
    #endif

    uart_init();

    // mp_printf(MP_PYTHON_PRINTER, "Stack base = %p, SP = %p\n", mpy_task_stk, sp);
    // mp_printf(MP_PYTHON_PRINTER, "avail_heap_size = %u\n", tls_mem_get_avail_heapsize());
    // Allocate the uPy heap using malloc and get the largest available region
    #if MICROPY_SSL_MBEDTLS
    u32 mp_task_heap_size = (tls_mem_get_avail_heapsize() * 8) / 10; // 80%
    #else
    u32 mp_task_heap_size = (tls_mem_get_avail_heapsize() * 8) / 10; // 80%
    #endif
    void *mp_task_heap = tls_mem_alloc(mp_task_heap_size);

    // mp_printf(MP_PYTHON_PRINTER, "heap_address = %p, heap_size = %u\n", mp_task_heap, mp_task_heap_size);

    check_esc_on_boot();

soft_reset:
    // initialise the stack pointer for the main thread
    mp_stack_set_top((void *)sp);
    mp_stack_set_limit(MPY_STACK_SIZE - 1024);
    gc_init(mp_task_heap, mp_task_heap + mp_task_heap_size);
    mp_init();
    readline_init0();

    machine_pins_init();
    // start rtc initial
    struct tm tblock;
    memset(&tblock, 0, sizeof(tblock));
    tblock.tm_mon = 1;
    tblock.tm_mday = 1;
    tls_set_rtc(&tblock);
    // start ticks_xx timer
    timer_init0();

    #if MICROPY_USE_FROZEN_SCRIPT
    // run boot-up scripts
    pyexec_frozen_module("_boot.py", false);
    // Execute user scripts.
    int ret = pyexec_file_if_exists("boot.py");
    if (ret & PYEXEC_FORCED_EXIT) {
        goto soft_reset_exit;
    }
    // Do not execute main.py if boot.py failed
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL && ret != 0) {
        ret = pyexec_file_if_exists("main.py");
        if (ret & PYEXEC_FORCED_EXIT) {
            goto soft_reset_exit;
        }
    }
    #endif

    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    int coldboot;
soft_reset_exit:
    coldboot = 0;
    #if MICROPY_PY_THREAD
    coldboot = mp_thread_deinit();
    #endif

    mp_hal_stdout_tx_str("MPY: soft reboot\r\n");

    // Deinit devices
    machine_pwm_deinit_all();

    gc_sweep_all();

    mp_deinit();

    if (coldboot) {
        tls_sys_reset();
    } else {
        goto soft_reset;
    }
}

void UserMain(void) {
    tls_os_task_create(NULL, "w60xmpy", mpy_task, NULL,
        (void *)mpy_task_stk, MPY_STACK_SIZE,
        MPY_TASK_PRIO, 0);
}

void nlr_jump_fail(void *val) {
    tls_sys_reset();
    for (;;) {; // Just to silence the compiler warning
    }
}
