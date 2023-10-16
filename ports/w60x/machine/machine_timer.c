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

#include "py/runtime.h"
#include "py/obj.h"

#include "wm_include.h"
#include "wm_timer.h"

typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;
    tls_os_timer_t *stimer;
    u32 timeout;
    mp_obj_t callback;
    u8 timerid;
    bool is_repeat;
    bool is_hwtimer;
} machine_timer_obj_t;

const mp_obj_type_t machine_timer_type;
static mp_obj_t machine_timer_del(mp_obj_t self_in);

static void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;

    mp_printf(print, "Timer(%d, ", self->is_hwtimer ? self->timerid - 1 : -1);
    mp_printf(print, "mode=%q, ", self->is_repeat ? MP_QSTR_PERIODIC : MP_QSTR_ONE_SHOT);
    mp_printf(print, "period=%u)", self->timeout);
}

static void machine_timer_callback(void *arg) {
    machine_timer_obj_t *self = (machine_timer_obj_t *)arg;

    if (self->callback) {
        mp_sched_schedule(self->callback, self);
    }
}

static void machine_soft_timer_callback(void *ptmr, void *parg) {
    machine_timer_obj_t *self = (machine_timer_obj_t *)parg;

    if (self->callback) {
        mp_sched_schedule(self->callback, self);
    }
}

static void mp_machine_timer_init_helper(machine_timer_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    struct tls_timer_cfg timercfg;
    enum { ARG_mode, ARG_period, ARG_callback, ARG_tick_hz, ARG_freq };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_INT, {.u_int = true} },
        { MP_QSTR_period,       MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_callback,     MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_tick_hz,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_freq,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint64_t delta_ms = self->timeout;
    if (n_args > 0 || kw_args->used > 0) {
        self->is_repeat = args[ARG_mode].u_int;
        if (args[ARG_freq].u_obj != mp_const_none) {
            // Frequency specified in Hz
            delta_ms = (uint32_t)(MICROPY_FLOAT_CONST(1000.0) / mp_obj_get_float(args[ARG_freq].u_obj));
        } else if (args[ARG_period].u_int != 0xffffffff) {
            delta_ms = args[ARG_period].u_int * 1000 / args[ARG_tick_hz].u_int;
        }
        if (delta_ms < 1) {
            delta_ms = 1;
        } else if (delta_ms >= 0x40000000) {
            mp_raise_ValueError(MP_ERROR_TEXT("period too large"));
        }
        self->timeout = delta_ms;
        if (mp_obj_is_callable(args[ARG_callback].u_obj)) {
            self->callback = args[ARG_callback].u_obj;
        } else if (args[ARG_callback].u_obj == mp_const_none) {
            self->callback = NULL;
        } else {
            mp_raise_ValueError("callback must be a function");
        }

        // Delete the timer before creating a new one.
        machine_timer_del(self);

        if (self->is_hwtimer) {
            memset(&timercfg, 0, sizeof(timercfg));
            timercfg.unit = TLS_TIMER_UNIT_MS;
            timercfg.is_repeat = self->is_repeat;
            timercfg.timeout = self->timeout;
            timercfg.callback = machine_timer_callback;
            timercfg.arg = self;

            self->timerid = tls_timer_create(&timercfg);
            if (WM_TIMER_ID_INVALID != self->timerid) {
                tls_timer_start(self->timerid);
            }
        } else {
            tls_os_timer_create(&self->stimer,
                machine_soft_timer_callback,
                self, self->timeout / (1000 / HZ), self->is_repeat, NULL);
            tls_os_timer_start(self->stimer);
        }
    }
}

static mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_machine_timer_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);

static mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_timer_obj_t *self = mp_obj_malloc_with_finaliser(machine_timer_obj_t, &machine_timer_type);
    self->stimer = NULL;
    self->timerid = WM_TIMER_ID_INVALID;
    self->is_repeat = TRUE;
    if (-1 == mp_obj_get_int(args[0])) {
        self->is_hwtimer = FALSE;
        self->timeout = HZ;
    } else {
        self->is_hwtimer = TRUE;
        self->timeout = 1000;
    }

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_timer_init_helper(self, n_args - 1, args + 1, &kw_args);

    return self;
}

static mp_obj_t machine_timer_del(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    if (NULL != self->stimer) {
        tls_os_timer_delete(self->stimer);
        self->stimer = NULL;
    }
    if (WM_TIMER_ID_INVALID != self->timerid) {
        tls_timer_destroy(self->timerid);
        self->timerid = WM_TIMER_ID_INVALID;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_del_obj, machine_timer_del);

static mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    if (NULL != self->stimer) {
        tls_os_timer_stop(self->stimer);
    }
    if (WM_TIMER_ID_INVALID != self->timerid) {
        tls_timer_stop(self->timerid);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

static const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};
static MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, machine_timer_make_new,
    print, machine_timer_print,
    locals_dict, &machine_timer_locals_dict
    );
