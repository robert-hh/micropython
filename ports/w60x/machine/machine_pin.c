/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
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
#include <string.h>

#include "wm_include.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/gc.h"
#include "extmod/virtpin.h"

#include "mphalport.h"
#include "extmod/modmachine.h"
#include "mpthreadport.h"

enum mp_pin_mode {
    GPIO_MODE_INPUT = WM_GPIO_DIR_INPUT,
    GPIO_MODE_OUTPUT = WM_GPIO_DIR_OUTPUT,
    GPIO_MODE_OPEN_DRAIN = 3 // SDK has no matching WM_GPIO_DIR!
};

/*
 * WM_GPIO_IRQ_TRIG_RISING_EDGE  == GPIO_IRQ_RISING - 1
 * WM_GPIO_IRQ_TRIG_FALLING_EDGE == GPIO_IRQ_FALLING - 1
 * WM_GPIO_IRQ_TRIG_DOUBLE_EDGE  == (GPIO_IRQ_RISING | GPIO_IRQ_FALLING)
 * WM_GPIO_IRQ_TRIG_HIGH_LEVEL == GPIO_IRQ_HIGH_LEVEL - 1
 * WM_GPIO_IRQ_TRIG_HIGH_LEVEL == GPIO_IRQ_LOW_LEVEL - 1
 */
enum mp_pin_irq_mode {
    GPIO_IRQ_RISING = WM_GPIO_IRQ_TRIG_RISING_EDGE + 1,  // 1
    GPIO_IRQ_FALLING = WM_GPIO_IRQ_TRIG_FALLING_EDGE + 1, // 2
    GPIO_IRQ_HIGH_LEVEL = WM_GPIO_IRQ_TRIG_HIGH_LEVEL + 1, // 4
    GPIO_IRQ_LOW_LEVEL = WM_GPIO_IRQ_TRIG_LOW_LEVEL + 1, // 5
};

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    enum tls_io_name id;
    enum tls_gpio_attr gpio_attr;
    enum mp_pin_mode mode;
    bool mp_pin_irq_mode;
} machine_pin_obj_t;

typedef struct _machine_pin_irq_obj_t {
    mp_obj_base_t base;
    enum tls_io_name id;
} machine_pin_irq_obj_t;

STATIC machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, WM_IO_PA_00},
    {{&machine_pin_type}, WM_IO_PA_01},
    {{&machine_pin_type}, WM_IO_PA_02},
    {{&machine_pin_type}, WM_IO_PA_03},
    {{&machine_pin_type}, WM_IO_PA_04},
    {{&machine_pin_type}, WM_IO_PA_05},

    {{&machine_pin_type}, WM_IO_PA_06},
    {{&machine_pin_type}, WM_IO_PA_07},
    {{&machine_pin_type}, WM_IO_PA_08},
    {{&machine_pin_type}, WM_IO_PA_09},
    {{&machine_pin_type}, WM_IO_PA_10},
    {{&machine_pin_type}, WM_IO_PA_11},

    {{&machine_pin_type}, WM_IO_PA_12},
    {{&machine_pin_type}, WM_IO_PA_13},
    {{&machine_pin_type}, WM_IO_PA_14},
    {{&machine_pin_type}, WM_IO_PA_15},

    {{&machine_pin_type}, WM_IO_PB_00},
    {{&machine_pin_type}, WM_IO_PB_01},
    {{&machine_pin_type}, WM_IO_PB_02},
    {{&machine_pin_type}, WM_IO_PB_03},
    {{&machine_pin_type}, WM_IO_PB_04},
    {{&machine_pin_type}, WM_IO_PB_05},

    {{&machine_pin_type}, WM_IO_PB_06},
    {{&machine_pin_type}, WM_IO_PB_07},
    {{&machine_pin_type}, WM_IO_PB_08},
    {{&machine_pin_type}, WM_IO_PB_09},
    {{&machine_pin_type}, WM_IO_PB_10},
    {{&machine_pin_type}, WM_IO_PB_11},

    {{&machine_pin_type}, WM_IO_PB_12},
    {{&machine_pin_type}, WM_IO_PB_13},
    {{&machine_pin_type}, WM_IO_PB_14},
    {{&machine_pin_type}, WM_IO_PB_15},
    {{&machine_pin_type}, WM_IO_PB_16},
    {{&machine_pin_type}, WM_IO_PB_17},

    {{&machine_pin_type}, WM_IO_PB_18},
    {{&machine_pin_type}, WM_IO_PB_19},
    {{&machine_pin_type}, WM_IO_PB_20},
    {{&machine_pin_type}, WM_IO_PB_21},
    {{&machine_pin_type}, WM_IO_PB_22},
    {{&machine_pin_type}, WM_IO_PB_23},

    {{&machine_pin_type}, WM_IO_PB_24},
    {{&machine_pin_type}, WM_IO_PB_25},
    {{&machine_pin_type}, WM_IO_PB_26},
    {{&machine_pin_type}, WM_IO_PB_27},
    {{&machine_pin_type}, WM_IO_PB_28},
    {{&machine_pin_type}, WM_IO_PB_29},

    {{&machine_pin_type}, WM_IO_PB_30},
    {{&machine_pin_type}, WM_IO_PB_31},
};

STATIC const mp_rom_map_elem_t pin_cpu_pins_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_PA0), MP_ROM_PTR(&machine_pin_obj[0]) },
    { MP_ROM_QSTR(MP_QSTR_PA1), MP_ROM_PTR(&machine_pin_obj[1]) },
    { MP_ROM_QSTR(MP_QSTR_PA2), MP_ROM_PTR(&machine_pin_obj[2]) },
    { MP_ROM_QSTR(MP_QSTR_PA3), MP_ROM_PTR(&machine_pin_obj[3]) },
    { MP_ROM_QSTR(MP_QSTR_PA4), MP_ROM_PTR(&machine_pin_obj[4]) },
    { MP_ROM_QSTR(MP_QSTR_PA5), MP_ROM_PTR(&machine_pin_obj[5]) },
    { MP_ROM_QSTR(MP_QSTR_PA6), MP_ROM_PTR(&machine_pin_obj[6]) },
    { MP_ROM_QSTR(MP_QSTR_PA7), MP_ROM_PTR(&machine_pin_obj[7]) },
    { MP_ROM_QSTR(MP_QSTR_PA8), MP_ROM_PTR(&machine_pin_obj[8]) },
    { MP_ROM_QSTR(MP_QSTR_PA9), MP_ROM_PTR(&machine_pin_obj[9]) },
    { MP_ROM_QSTR(MP_QSTR_PA10), MP_ROM_PTR(&machine_pin_obj[10]) },
    { MP_ROM_QSTR(MP_QSTR_PA11), MP_ROM_PTR(&machine_pin_obj[11]) },
    { MP_ROM_QSTR(MP_QSTR_PA12), MP_ROM_PTR(&machine_pin_obj[12]) },
    { MP_ROM_QSTR(MP_QSTR_PA13), MP_ROM_PTR(&machine_pin_obj[13]) },
    { MP_ROM_QSTR(MP_QSTR_PA14), MP_ROM_PTR(&machine_pin_obj[14]) },
    { MP_ROM_QSTR(MP_QSTR_PA15), MP_ROM_PTR(&machine_pin_obj[15]) },
    { MP_ROM_QSTR(MP_QSTR_PB0), MP_ROM_PTR(&machine_pin_obj[16 + 0]) },
    { MP_ROM_QSTR(MP_QSTR_PB1), MP_ROM_PTR(&machine_pin_obj[16 + 1]) },
    { MP_ROM_QSTR(MP_QSTR_PB2), MP_ROM_PTR(&machine_pin_obj[16 + 2]) },
    { MP_ROM_QSTR(MP_QSTR_PB3), MP_ROM_PTR(&machine_pin_obj[16 + 3]) },
    { MP_ROM_QSTR(MP_QSTR_PB4), MP_ROM_PTR(&machine_pin_obj[16 + 4]) },
    { MP_ROM_QSTR(MP_QSTR_PB5), MP_ROM_PTR(&machine_pin_obj[16 + 5]) },
    { MP_ROM_QSTR(MP_QSTR_PB6), MP_ROM_PTR(&machine_pin_obj[16 + 6]) },
    { MP_ROM_QSTR(MP_QSTR_PB7), MP_ROM_PTR(&machine_pin_obj[16 + 7]) },
    { MP_ROM_QSTR(MP_QSTR_PB8), MP_ROM_PTR(&machine_pin_obj[16 + 8]) },
    { MP_ROM_QSTR(MP_QSTR_PB9), MP_ROM_PTR(&machine_pin_obj[16 + 9]) },
    { MP_ROM_QSTR(MP_QSTR_PB10), MP_ROM_PTR(&machine_pin_obj[16 + 10]) },
    { MP_ROM_QSTR(MP_QSTR_PB11), MP_ROM_PTR(&machine_pin_obj[16 + 11]) },
    { MP_ROM_QSTR(MP_QSTR_PB12), MP_ROM_PTR(&machine_pin_obj[16 + 12]) },
    { MP_ROM_QSTR(MP_QSTR_PB13), MP_ROM_PTR(&machine_pin_obj[16 + 13]) },
    { MP_ROM_QSTR(MP_QSTR_PB14), MP_ROM_PTR(&machine_pin_obj[16 + 14]) },
    { MP_ROM_QSTR(MP_QSTR_PB15), MP_ROM_PTR(&machine_pin_obj[16 + 15]) },
    { MP_ROM_QSTR(MP_QSTR_PB16), MP_ROM_PTR(&machine_pin_obj[16 + 16]) },
    { MP_ROM_QSTR(MP_QSTR_PB17), MP_ROM_PTR(&machine_pin_obj[16 + 17]) },
    { MP_ROM_QSTR(MP_QSTR_PB18), MP_ROM_PTR(&machine_pin_obj[16 + 18]) },
    { MP_ROM_QSTR(MP_QSTR_PB19), MP_ROM_PTR(&machine_pin_obj[16 + 19]) },
    { MP_ROM_QSTR(MP_QSTR_PB20), MP_ROM_PTR(&machine_pin_obj[16 + 20]) },
    { MP_ROM_QSTR(MP_QSTR_PB21), MP_ROM_PTR(&machine_pin_obj[16 + 21]) },
    { MP_ROM_QSTR(MP_QSTR_PB22), MP_ROM_PTR(&machine_pin_obj[16 + 22]) },
    { MP_ROM_QSTR(MP_QSTR_PB23), MP_ROM_PTR(&machine_pin_obj[16 + 23]) },
    { MP_ROM_QSTR(MP_QSTR_PB24), MP_ROM_PTR(&machine_pin_obj[16 + 24]) },
    { MP_ROM_QSTR(MP_QSTR_PB25), MP_ROM_PTR(&machine_pin_obj[16 + 25]) },
    { MP_ROM_QSTR(MP_QSTR_PB26), MP_ROM_PTR(&machine_pin_obj[16 + 26]) },
    { MP_ROM_QSTR(MP_QSTR_PB27), MP_ROM_PTR(&machine_pin_obj[16 + 27]) },
    { MP_ROM_QSTR(MP_QSTR_PB28), MP_ROM_PTR(&machine_pin_obj[16 + 28]) },
    { MP_ROM_QSTR(MP_QSTR_PB29), MP_ROM_PTR(&machine_pin_obj[16 + 29]) },
    { MP_ROM_QSTR(MP_QSTR_PB30), MP_ROM_PTR(&machine_pin_obj[16 + 30]) },
    { MP_ROM_QSTR(MP_QSTR_PB31), MP_ROM_PTR(&machine_pin_obj[16 + 31]) },
};
MP_DEFINE_CONST_DICT(machine_pin_cpu_pins_locals_dict, pin_cpu_pins_locals_dict_table);

// The Pin board dictionary has to be included a CPP macro and not as
// C file. pin_board_dict.h is created from $(BOARD)/pins.csv and
// defines PIN_BOARD_DICT.
#include "pin_board_dict.h"

PIN_BOARD_DICT

// Pin mapping dictionaries
MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_cpu_pins_obj_type,
    MP_QSTR_cpu,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_pin_cpu_pins_locals_dict
    );

// Pin mapping dictionaries
MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_board_pins_obj_type,
    MP_QSTR_board,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_pin_board_pins_locals_dict
    );

// forward declaration
STATIC const machine_pin_irq_obj_t machine_pin_irq_object[];

STATIC char *pin_mode_str[] = {"OUT", "IN", "", "OPEN_DRAIN"};

void machine_pins_init(void) {
    memset(&MP_STATE_PORT(machine_pin_irq_handler[0]), 0, sizeof(MP_STATE_PORT(machine_pin_irq_handler)));
}

STATIC void machine_pin_isr_handler(void *arg) {
    machine_pin_obj_t *self = arg;
    mp_obj_t handler = MP_STATE_PORT(machine_pin_irq_handler)[self->id];

    if (self->mp_pin_irq_mode == false) {
        mp_sched_schedule(handler, MP_OBJ_FROM_PTR(self));
    } else { // Hard blib
        mp_sched_lock();
        // When executing code within a handler we must lock the GC to prevent
        // any memory allocations.  We must also catch any exceptions.
        gc_lock();
        // save stack top and set temporary value to pass the stack check
        char *saved_stack_top = MP_STATE_THREAD(stack_top);
        MP_STATE_THREAD(stack_top) = (void *)&saved_stack_top + 1024; // kind of arbitrary
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_1(handler, MP_OBJ_FROM_PTR(self));
            nlr_pop();
        } else {
            // Uncaught exception; disable the callback so it doesn't run again.
            MP_STATE_PORT(machine_pin_irq_handler)[self->id] = MP_OBJ_NULL;
            tls_gpio_isr_register(self->id, 0, 0);
            tls_gpio_irq_disable(self->id);
            mp_printf(MICROPY_ERROR_PRINTER, "Uncaught exception in ExtInt interrupt handler GPIO %u\n", (unsigned int)self->id);
            mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
        }
        // restore the stack top value for stack checks and GC
        MP_STATE_THREAD(stack_top) = saved_stack_top;
        gc_unlock();
        mp_sched_unlock();
    }
}

STATIC inline machine_pin_obj_t *mp_obj_to_pin_obj(mp_obj_t pin) {
    if (mp_obj_get_type(pin) != &machine_pin_type) {
        mp_raise_ValueError("expecting a pin");
    }
    return MP_OBJ_TO_PTR(pin);
}

STATIC inline machine_pin_obj_t *pin_idx_to_pin_obj(mp_hal_pin_obj_t pin_index) {
    return (machine_pin_obj_t *)&machine_pin_obj[pin_index];
}

STATIC machine_pin_obj_t *pin_find_named_pin(const mp_obj_dict_t *named_pins, mp_obj_t name) {
    mp_map_t *named_map = mp_obj_dict_get_map((mp_obj_t)named_pins);
    mp_map_elem_t *named_elem = mp_map_lookup(named_map, name, MP_MAP_LOOKUP);
    if (named_elem != NULL && named_elem->value != NULL) {
        return named_elem->value;
    }
    return NULL;
}

machine_pin_obj_t *pin_find(mp_obj_t pin_in) {
    const machine_pin_obj_t *pin;

    // If a pin was provided, then use it
    if (mp_obj_is_type(pin_in, &machine_pin_type)) {
        return pin_in;
    }

    machine_pin_obj_t *self;
    // See if the pin name matches a board pin
    self = pin_find_named_pin(&machine_pin_board_pins_locals_dict, pin_in);
    if (self != NULL) {
        return self;
    }
    // See if the pin name matches a cpu/board pin
    self = pin_find_named_pin(&machine_pin_cpu_pins_locals_dict, pin_in);
    if (self != NULL) {
        return self;
    }
    // If pin is SMALL_INT
    if (mp_obj_is_small_int(pin_in)) {
        int wanted_pin = -1;
        wanted_pin = mp_obj_get_int(pin_in);
        if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(machine_pin_obj)) {
            return pin_idx_to_pin_obj(wanted_pin);
        }
    }
    mp_raise_ValueError("invalid pin");
}

mp_hal_pin_obj_t machine_pin_get_id(mp_obj_t pin_in) {
    return pin_find(pin_in)->id;
}

/*
 * NOTE: The WM60x does not have a native Pin.OPEN_DRAIN operation mode.
 * To get the behavior of Pin.OPEN_DRAIN, the pin must be configured to
 * WM_GPIO_DIR_OUTPUT when writing a 0 (pin pulled low) and to
 * WM_GPIO_DIR_INPUT when writing a 1 (pin in high impedance state).
 */

void machine_set_pin(machine_pin_obj_t *self, bool value) {
    if (GPIO_MODE_OPEN_DRAIN == self->mode) {
        tls_gpio_cfg(self->id, value ? WM_GPIO_DIR_INPUT : WM_GPIO_DIR_OUTPUT, self->gpio_attr);
    }
    tls_gpio_write(self->id, value);
}

void mp_hal_pin_input(mp_hal_pin_obj_t pin) {
    machine_pin_obj_t *self = pin_idx_to_pin_obj(pin);
    self->mode = GPIO_MODE_INPUT;
    tls_gpio_cfg(pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_FLOATING);
}

void mp_hal_pin_output(mp_hal_pin_obj_t pin) {
    machine_pin_obj_t *self = pin_idx_to_pin_obj(pin);
    self->mode = GPIO_MODE_OUTPUT;
    tls_gpio_cfg(pin, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_FLOATING);
}

void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin) {
    machine_pin_obj_t *self = pin_idx_to_pin_obj(pin);
    self->mode = GPIO_MODE_OPEN_DRAIN;
    tls_gpio_cfg(pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
}

void mp_hal_pin_od_low(mp_hal_pin_obj_t pin) {
    machine_pin_obj_t *self = pin_idx_to_pin_obj(pin);
    self->mode = GPIO_MODE_OPEN_DRAIN;
    tls_gpio_cfg(pin, WM_GPIO_DIR_OUTPUT, WM_GPIO_ATTR_PULLHIGH);
    tls_gpio_write(pin, 0);
}

void mp_hal_pin_od_high(mp_hal_pin_obj_t pin) {
    machine_pin_obj_t *self = pin_idx_to_pin_obj(pin);
    self->mode = GPIO_MODE_OPEN_DRAIN;
    tls_gpio_cfg(pin, WM_GPIO_DIR_INPUT, WM_GPIO_ATTR_PULLHIGH);
}

void mp_hal_pin_write(mp_hal_pin_obj_t pin, int value) {
    machine_set_pin(pin_idx_to_pin_obj(pin), value);
}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = mp_obj_to_pin_obj(self_in);
    mp_printf(print, "Pin(%u, mode=%s)", self->id, pin_mode_str[self->mode]);
}

// pin.init(direction, attribute, value)
STATIC mp_obj_t machine_pin_obj_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = WM_GPIO_DIR_INPUT} },
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = WM_GPIO_ATTR_FLOATING} },
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    bool has_value_arg = args[ARG_value].u_obj != MP_OBJ_NULL;
    bool value = has_value_arg ? mp_obj_is_true(args[ARG_value].u_obj) : true;
    int mode = args[ARG_mode].u_int;
    self->mode = mode;

    if (GPIO_MODE_OPEN_DRAIN == mode) {
        tls_gpio_cfg(self->id, value ? WM_GPIO_DIR_INPUT : WM_GPIO_DIR_OUTPUT, args[ARG_pull].u_int);
    } else {
        tls_gpio_cfg(self->id, mode, args[ARG_pull].u_int);
    }
    if (has_value_arg) {
        tls_gpio_write(self->id, value);
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    machine_pin_obj_t *self = pin_find(args[0]);

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = mp_obj_to_pin_obj(self_in);
    if (n_args == 0) {
        // get pin
        return MP_OBJ_NEW_SMALL_INT(tls_gpio_read(self->id));
    } else {
        // set pin
        machine_set_pin(self, mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.off() resp. pin.low()
STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_set_pin(mp_obj_to_pin_obj(self_in), false);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

// pin.on() resp. pin.high()
STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_set_pin(mp_obj_to_pin_obj(self_in), true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

STATIC void machine_pin_irq_callback(void *p) {
    machine_pin_obj_t *self = p;
    machine_pin_isr_handler(self);
    tls_clr_gpio_irq_status(self->id);
}

// pin.toggle()
STATIC mp_obj_t machine_pin_toggle(mp_obj_t self_in) {
    machine_pin_obj_t *self = mp_obj_to_pin_obj(self_in);
    machine_set_pin(self, 1 - tls_gpio_read(self->id));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_toggle_obj, machine_pin_toggle);

// pin.irq(trigger_mode)
STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_hard };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_hard, MP_ARG_BOOL, {.u_bool = false} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (n_args > 1 || kw_args->used != 0) {
        // configure irq
        mp_obj_t handler = args[ARG_handler].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        if (handler == mp_const_none) {
            handler = MP_OBJ_NULL;
        }
        self->mp_pin_irq_mode = args[ARG_hard].u_bool;

        MP_STATE_PORT(machine_pin_irq_handler)[self->id] = handler;
        if (handler != MP_OBJ_NULL) {
            tls_gpio_isr_register(self->id, machine_pin_irq_callback, self);
            tls_gpio_irq_enable(self->id, trigger - 1);
        } else {
            tls_gpio_irq_disable(self->id);
            tls_gpio_isr_register(self->id, 0, 0);
        }
    }

    // return the irq object
    return MP_OBJ_FROM_PTR(&machine_pin_irq_object[self->id]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_low), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_high), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&machine_pin_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_pin_irq_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(GPIO_MODE_OPEN_DRAIN) },

    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(GPIO_MODE_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(GPIO_MODE_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(WM_GPIO_ATTR_PULLHIGH) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(WM_GPIO_ATTR_PULLLOW) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(WM_GPIO_ATTR_FLOATING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(GPIO_IRQ_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(GPIO_IRQ_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_HIGH_LEVEL), MP_ROM_INT(GPIO_IRQ_HIGH_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_LOW_LEVEL), MP_ROM_INT(GPIO_IRQ_LOW_LEVEL) },

    { MP_ROM_QSTR(MP_QSTR_PA_00), MP_ROM_INT(WM_IO_PA_00) },
    { MP_ROM_QSTR(MP_QSTR_PA_01), MP_ROM_INT(WM_IO_PA_01) },
    { MP_ROM_QSTR(MP_QSTR_PA_02), MP_ROM_INT(WM_IO_PA_02) },
    { MP_ROM_QSTR(MP_QSTR_PA_03), MP_ROM_INT(WM_IO_PA_03) },
    { MP_ROM_QSTR(MP_QSTR_PA_04), MP_ROM_INT(WM_IO_PA_04) },

    { MP_ROM_QSTR(MP_QSTR_PA_05), MP_ROM_INT(WM_IO_PA_05) },
    { MP_ROM_QSTR(MP_QSTR_PA_06), MP_ROM_INT(WM_IO_PA_06) },
    { MP_ROM_QSTR(MP_QSTR_PA_07), MP_ROM_INT(WM_IO_PA_07) },
    { MP_ROM_QSTR(MP_QSTR_PA_08), MP_ROM_INT(WM_IO_PA_08) },
    { MP_ROM_QSTR(MP_QSTR_PA_09), MP_ROM_INT(WM_IO_PA_09) },

    { MP_ROM_QSTR(MP_QSTR_PA_10), MP_ROM_INT(WM_IO_PA_10) },
    { MP_ROM_QSTR(MP_QSTR_PA_11), MP_ROM_INT(WM_IO_PA_11) },
    { MP_ROM_QSTR(MP_QSTR_PA_12), MP_ROM_INT(WM_IO_PA_12) },
    { MP_ROM_QSTR(MP_QSTR_PA_13), MP_ROM_INT(WM_IO_PA_13) },
    { MP_ROM_QSTR(MP_QSTR_PA_14), MP_ROM_INT(WM_IO_PA_14) },

    { MP_ROM_QSTR(MP_QSTR_PA_15), MP_ROM_INT(WM_IO_PA_15) },

    { MP_ROM_QSTR(MP_QSTR_PB_00), MP_ROM_INT(WM_IO_PB_00) },
    { MP_ROM_QSTR(MP_QSTR_PB_01), MP_ROM_INT(WM_IO_PB_01) },
    { MP_ROM_QSTR(MP_QSTR_PB_02), MP_ROM_INT(WM_IO_PB_02) },
    { MP_ROM_QSTR(MP_QSTR_PB_03), MP_ROM_INT(WM_IO_PB_03) },
    { MP_ROM_QSTR(MP_QSTR_PB_04), MP_ROM_INT(WM_IO_PB_04) },

    { MP_ROM_QSTR(MP_QSTR_PB_05), MP_ROM_INT(WM_IO_PB_05) },
    { MP_ROM_QSTR(MP_QSTR_PB_06), MP_ROM_INT(WM_IO_PB_06) },
    { MP_ROM_QSTR(MP_QSTR_PB_07), MP_ROM_INT(WM_IO_PB_07) },
    { MP_ROM_QSTR(MP_QSTR_PB_08), MP_ROM_INT(WM_IO_PB_08) },
    { MP_ROM_QSTR(MP_QSTR_PB_09), MP_ROM_INT(WM_IO_PB_09) },

    { MP_ROM_QSTR(MP_QSTR_PB_10), MP_ROM_INT(WM_IO_PB_10) },
    { MP_ROM_QSTR(MP_QSTR_PB_11), MP_ROM_INT(WM_IO_PB_11) },
    { MP_ROM_QSTR(MP_QSTR_PB_12), MP_ROM_INT(WM_IO_PB_12) },
    { MP_ROM_QSTR(MP_QSTR_PB_13), MP_ROM_INT(WM_IO_PB_13) },
    { MP_ROM_QSTR(MP_QSTR_PB_14), MP_ROM_INT(WM_IO_PB_14) },

    { MP_ROM_QSTR(MP_QSTR_PB_15), MP_ROM_INT(WM_IO_PB_15) },
    { MP_ROM_QSTR(MP_QSTR_PB_16), MP_ROM_INT(WM_IO_PB_16) },
    { MP_ROM_QSTR(MP_QSTR_PB_17), MP_ROM_INT(WM_IO_PB_17) },
    { MP_ROM_QSTR(MP_QSTR_PB_18), MP_ROM_INT(WM_IO_PB_18) },
    { MP_ROM_QSTR(MP_QSTR_PB_19), MP_ROM_INT(WM_IO_PB_19) },

    { MP_ROM_QSTR(MP_QSTR_PB_20), MP_ROM_INT(WM_IO_PB_20) },
    { MP_ROM_QSTR(MP_QSTR_PB_21), MP_ROM_INT(WM_IO_PB_21) },
    { MP_ROM_QSTR(MP_QSTR_PB_22), MP_ROM_INT(WM_IO_PB_22) },
    { MP_ROM_QSTR(MP_QSTR_PB_23), MP_ROM_INT(WM_IO_PB_23) },
    { MP_ROM_QSTR(MP_QSTR_PB_24), MP_ROM_INT(WM_IO_PB_24) },

    { MP_ROM_QSTR(MP_QSTR_PB_25), MP_ROM_INT(WM_IO_PB_25) },
    { MP_ROM_QSTR(MP_QSTR_PB_26), MP_ROM_INT(WM_IO_PB_26) },
    { MP_ROM_QSTR(MP_QSTR_PB_27), MP_ROM_INT(WM_IO_PB_27) },
    { MP_ROM_QSTR(MP_QSTR_PB_28), MP_ROM_INT(WM_IO_PB_28) },
    { MP_ROM_QSTR(MP_QSTR_PB_29), MP_ROM_INT(WM_IO_PB_29) },

    { MP_ROM_QSTR(MP_QSTR_PB_30), MP_ROM_INT(WM_IO_PB_30) },
    { MP_ROM_QSTR(MP_QSTR_PB_31), MP_ROM_INT(WM_IO_PB_31) },

    { MP_ROM_QSTR(MP_QSTR_cpu), MP_ROM_PTR(&machine_pin_cpu_pins_obj_type) },
    { MP_ROM_QSTR(MP_QSTR_board), MP_ROM_PTR(&machine_pin_board_pins_obj_type) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
            return tls_gpio_read(self->id);
        }
        case MP_PIN_WRITE: {
            tls_gpio_write(self->id, arg);
            return 0;
        }
    }
    return -1;
}

STATIC const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, mp_pin_make_new,
    print, machine_pin_print,
    call, machine_pin_call,
    protocol, &pin_pin_p,
    locals_dict, &machine_pin_locals_dict
    );

/******************************************************************************/
// Pin IRQ object

const mp_obj_type_t machine_pin_irq_type;

STATIC const machine_pin_irq_obj_t machine_pin_irq_object[] = {
    {{&machine_pin_irq_type}, WM_IO_PA_00},
    {{&machine_pin_irq_type}, WM_IO_PA_01},
    {{&machine_pin_irq_type}, WM_IO_PA_02},
    {{&machine_pin_irq_type}, WM_IO_PA_03},
    {{&machine_pin_irq_type}, WM_IO_PA_04},
    {{&machine_pin_irq_type}, WM_IO_PA_05},

    {{&machine_pin_irq_type}, WM_IO_PA_06},
    {{&machine_pin_irq_type}, WM_IO_PA_07},
    {{&machine_pin_irq_type}, WM_IO_PA_08},
    {{&machine_pin_irq_type}, WM_IO_PA_09},
    {{&machine_pin_irq_type}, WM_IO_PA_10},
    {{&machine_pin_irq_type}, WM_IO_PA_11},

    {{&machine_pin_irq_type}, WM_IO_PA_12},
    {{&machine_pin_irq_type}, WM_IO_PA_13},
    {{&machine_pin_irq_type}, WM_IO_PA_14},
    {{&machine_pin_irq_type}, WM_IO_PA_15},


    {{&machine_pin_irq_type}, WM_IO_PB_00},
    {{&machine_pin_irq_type}, WM_IO_PB_01},
    {{&machine_pin_irq_type}, WM_IO_PB_02},
    {{&machine_pin_irq_type}, WM_IO_PB_03},
    {{&machine_pin_irq_type}, WM_IO_PB_04},
    {{&machine_pin_irq_type}, WM_IO_PB_05},

    {{&machine_pin_irq_type}, WM_IO_PB_06},
    {{&machine_pin_irq_type}, WM_IO_PB_07},
    {{&machine_pin_irq_type}, WM_IO_PB_08},
    {{&machine_pin_irq_type}, WM_IO_PB_09},
    {{&machine_pin_irq_type}, WM_IO_PB_10},
    {{&machine_pin_irq_type}, WM_IO_PB_11},

    {{&machine_pin_irq_type}, WM_IO_PB_12},
    {{&machine_pin_irq_type}, WM_IO_PB_13},
    {{&machine_pin_irq_type}, WM_IO_PB_14},
    {{&machine_pin_irq_type}, WM_IO_PB_15},
    {{&machine_pin_irq_type}, WM_IO_PB_16},
    {{&machine_pin_irq_type}, WM_IO_PB_17},

    {{&machine_pin_irq_type}, WM_IO_PB_18},
    {{&machine_pin_irq_type}, WM_IO_PB_19},
    {{&machine_pin_irq_type}, WM_IO_PB_20},
    {{&machine_pin_irq_type}, WM_IO_PB_21},
    {{&machine_pin_irq_type}, WM_IO_PB_22},
    {{&machine_pin_irq_type}, WM_IO_PB_23},

    {{&machine_pin_irq_type}, WM_IO_PB_24},
    {{&machine_pin_irq_type}, WM_IO_PB_25},
    {{&machine_pin_irq_type}, WM_IO_PB_26},
    {{&machine_pin_irq_type}, WM_IO_PB_27},
    {{&machine_pin_irq_type}, WM_IO_PB_28},
    {{&machine_pin_irq_type}, WM_IO_PB_29},

    {{&machine_pin_irq_type}, WM_IO_PB_30},
    {{&machine_pin_irq_type}, WM_IO_PB_31},
};

STATIC mp_obj_t machine_pin_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    machine_pin_isr_handler(self);
    return mp_const_none;
}

STATIC mp_obj_t machine_pin_irq_trigger(size_t n_args, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = args[0];

    if (n_args == 2) {
        // set trigger
        tls_gpio_irq_enable(self->id, mp_obj_get_int(args[1]) - 1);
    }
    // return irq status
    return MP_OBJ_NEW_SMALL_INT(tls_get_gpio_irq_status(self->id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_irq_trigger_obj, 1, 2, machine_pin_irq_trigger);

STATIC const mp_rom_map_elem_t machine_pin_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_trigger), MP_ROM_PTR(&machine_pin_irq_trigger_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_irq_locals_dict, machine_pin_irq_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_irq_type,
    MP_QSTR_GPIO_IRQ,
    MP_TYPE_FLAG_NONE,
    call, machine_pin_irq_call,
    locals_dict, &machine_pin_irq_locals_dict
    );

MP_REGISTER_ROOT_POINTER(mp_obj_t machine_pin_irq_handler[48]);
