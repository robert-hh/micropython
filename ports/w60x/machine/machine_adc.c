/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/modmachine.h"

#include "wm_include.h"
#include "wm_gpio_afsel.h"
#include "wm_adc.h"

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    u8 adc_channel;
} machine_adc_obj_t;

// The ADC class doesn't have any constants for this port.
#define MICROPY_PY_MACHINE_ADC_CLASS_CONSTANTS

static void mp_machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC Channel (%hhu)", self->adc_channel);
}

static mp_obj_t mp_machine_adc_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_int_t chn = mp_obj_get_int(args[0]);

    switch (chn) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            break;
        default:
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "ADC channel (%d) doesn't exist", chn));
    }

    // create ADC object from the given channel id
    machine_adc_obj_t *self = m_new_obj(machine_adc_obj_t);
    self->base.type = &machine_adc_type;
    self->adc_channel = chn;

    wm_adc_config(chn);

    return MP_OBJ_FROM_PTR(self);
}

// read_u16()
static mp_int_t mp_machine_adc_read_u16(machine_adc_obj_t *self) {
    uint32_t value = adc_get_inputVolt(self->adc_channel);
    return value * 65535 / 1024;
}

// Legacy method
static mp_int_t mp_machine_adc_read(machine_adc_obj_t *self) {
    return adc_get_inputVolt(self->adc_channel);
}
