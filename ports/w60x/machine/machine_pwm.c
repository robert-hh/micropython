/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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

#include "wm_include.h"
#include "wm_gpio_afsel.h"

#include "py/nlr.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "mphalport.h"

extern const mp_obj_type_t machine_pwm_type;

typedef struct _machine_pwm_obj_t {
    mp_obj_base_t base;
    mp_hal_pin_obj_t pin;
    bool defer_start;
    uint8_t channel;
    uint8_t duty;
    uint8_t pnum;
    bool invert;
    uint32_t freq;
} machine_pwm_obj_t;

STATIC void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t freq);
STATIC void mp_machine_pwm_duty_set(machine_pwm_obj_t *self, mp_int_t duty);
STATIC void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty);
STATIC void mp_machine_pwm_duty_set_ns(machine_pwm_obj_t *self, mp_int_t duty);

// Mapping of Pin number to PWM channels. -1: Pin not supported.
static char channel_pin_table[] = {
    0, 1, 2, 3, 4, 0, -1, 1, 2, 3, 4, -1, -1, -1, -1, -1, // PA00 - PA15
    -1, 4, 3, 2, 1, 0, 3, -1, 4, -1, -1, -1, -1, 1, 4, 3, // PB00 - PB15
    2, 1, 0, 0, 1, 2, 3, 4, -1, -1, -1, -1, -1, -1, 0, -1 // PB16 - PB31
};

/******************************************************************************/

// MicroPython bindings for PWM

STATIC void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "PWM(%d, freq=%d, duty=%d, invert=%d, pnum=%d, pin=%d)", self->channel,
        self->freq, self->duty, self->invert, self->pnum, self->pin);
}

STATIC int w600_pwm_multiplex_config(machine_pwm_obj_t *self) {
    switch (self->channel) {
        case 0:
            wm_pwm1_config(self->pin);
        case 1:
            wm_pwm2_config(self->pin);
        case 2:
            wm_pwm3_config(self->pin);
        case 3:
            wm_pwm4_config(self->pin);
        case 4:
            wm_pwm5_config(self->pin);
        default:
            return -1;
    }
}

void w600_pwm_start(machine_pwm_obj_t *self) {
    if (self->freq != -1 && self->duty != -1 && self->defer_start == false) {
        tls_pwm_stop(self->channel);
        w600_pwm_multiplex_config(self);
        tls_pwm_init(self->channel, self->freq, self->duty, self->pnum);
        tls_pwm_out_inverse_cmd(self->channel, self->invert);
        tls_pwm_start(self->channel);
    }
}

STATIC void mp_machine_pwm_init_helper(machine_pwm_obj_t *self,
    size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_channel, ARG_freq, ARG_duty, ARG_duty_u16, ARG_duty_ns, ARG_pnum, ARG_invert };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_duty, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_duty_u16, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_duty_ns, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_pnum, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_invert, MP_ARG_INT, {.u_int = -1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int tval;

    self->defer_start = true;
    if (args[ARG_pnum].u_int != -1) {
        tval = args[ARG_pnum].u_int;
        if ((tval < 0) || (tval > 255)) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
                "Bad period num %d", tval));
        }
        self->pnum = tval;
    }

    if (args[ARG_invert].u_int != -1) {
        self->invert = !!args[ARG_invert].u_int;
    }

    if (args[ARG_freq].u_int != -1) {
        mp_machine_pwm_freq_set(self, args[ARG_freq].u_int);
    }

    if (args[ARG_duty].u_int != -1) {
        mp_machine_pwm_duty_set(self, args[ARG_duty].u_int);
    }

    if (args[ARG_duty_u16].u_int != -1) {
        mp_machine_pwm_duty_set_u16(self, args[ARG_duty_u16].u_int);
    }

    if (args[ARG_duty_ns].u_int != -1) {
        mp_machine_pwm_duty_set_ns(self, args[ARG_duty_ns].u_int);
    }
    self->defer_start = false;
    w600_pwm_start(self);
}

STATIC mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // create PWM object from the given pin
    machine_pwm_obj_t *self = m_new_obj(machine_pwm_obj_t);
    self->base.type = &machine_pwm_type;
    self->pin = mp_hal_get_pin_obj(args[0]);
    self->channel = -1;
    self->freq = -1;
    self->duty = -1;
    self->pnum = 0;
    self->invert = 0;

    if (self->pin < sizeof(channel_pin_table)) {
        self->channel = channel_pin_table[self->pin];
    }
    if (self->channel == -1) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Bad pin %d", self->pin));
    }

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

void machine_pwm_deinit_all(void) {
    for (int ch = 0; ch < 4; ch++) {
        tls_pwm_stop(ch);
    }
}

STATIC void mp_machine_pwm_deinit(machine_pwm_obj_t *self) {
    tls_pwm_stop(self->channel);
}

STATIC mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(self->freq);
}

STATIC void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t tval) {

    if ((tval < 1) || (tval > 156250)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
            "Bad frequency %d", tval));
    }
    self->freq = tval;
    w600_pwm_start(self);
}

STATIC mp_obj_t mp_machine_pwm_duty_get(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(self->duty);
}

STATIC void mp_machine_pwm_duty_set(machine_pwm_obj_t *self, mp_int_t duty) {
    if ((duty < 0) || (duty > 255)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError,
            "Bad duty %d", duty));
    }
    self->duty = duty;
    w600_pwm_start(self);
}

STATIC mp_obj_t mp_machine_pwm_duty_get_u16(machine_pwm_obj_t *self) {
    return MP_OBJ_NEW_SMALL_INT(self->duty * 256);
}

STATIC void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty) {
    mp_machine_pwm_duty_set(self, duty / 256);
}

STATIC mp_obj_t mp_machine_pwm_duty_get_ns(machine_pwm_obj_t *self) {
    uint32_t duty = 0;
    if (self->freq > 0) {
        // 3906250 == 1E9 / 256
        duty = 3906250 * self->duty / self->freq;
    }
    return MP_OBJ_NEW_SMALL_INT(duty);
}

STATIC void mp_machine_pwm_duty_set_ns(machine_pwm_obj_t *self, mp_int_t duty_ns) {
    if (self->freq > 0) {
        uint32_t duty = duty_ns * self->freq / 3906250;
        mp_machine_pwm_duty_set(self, duty);
    }
}
