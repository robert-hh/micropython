/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Damien P. George
 * Copyright (c) 2025 Robert Hammelrath
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

// This file is never compiled standalone, it's included directly from
// extmod/machine_i2c_target.c via MICROPY_PY_MACHINE_I2C_TARGET_INCLUDEFILE.

#include "fsl_lpi2c.h"
#include "machine_i2c.h"
#include CLOCK_CONFIG_H

static machine_i2c_target_data_t i2c_target_data[MICROPY_HW_I2C_NUM_MAX];

typedef struct _machine_i2c_target_memory_obj_t {
    mp_obj_base_t base;
    LPI2C_Type *i2c_inst;
    uint8_t i2c_id;
    uint8_t addr;
    lpi2c_slave_config_t *slave_config;
    lpi2c_slave_handle_t handle;
} machine_i2c_target_memory_obj_t;


MP_REGISTER_ROOT_POINTER(mp_obj_t pyb_i2cslave_mem[MICROPY_HW_I2C_NUM_MAX]);

static void lpi2c_slave_mem_callback(LPI2C_Type *base, lpi2c_slave_transfer_t *xfer, void *param) {
    machine_i2c_target_data_t *data = (machine_i2c_target_data_t *)param;

    switch (xfer->event)
    {
        //  Transmit request
        case kLPI2C_SlaveTransmitEvent:
            //  Update information for transmit process
            base->STDR = machine_i2c_target_data_read(data);
            break;

        //  Receive request
        case kLPI2C_SlaveReceiveEvent:
            machine_i2c_target_data_write(data, (uint8_t)base->SRDR);
            break;

        //  Transfer done
        case kLPI2C_SlaveCompletionEvent:
            machine_i2c_target_data_reset(data);
            break;

        default:
            break;
    }
}

static mp_obj_t mp_machine_i2c_target_memory_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // TODO: reconsider order of arguments
    enum { ARG_id, ARG_addr, ARG_mem, ARG_drive };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = DEFAULT_I2C_ID} },
        { MP_QSTR_addr, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_mem, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_drive, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DEFAULT_I2C_DRIVE} },
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus.
    int i2c_id = args[ARG_id].u_int;
    if (i2c_id < 0 || i2c_id >= micropy_hw_i2c_num || i2c_index_table[i2c_id] == 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }
    int i2c_hw_id = i2c_index_table[i2c_id];  // the hw i2c number 1..n

    // Get I2C Object.
    machine_i2c_target_memory_obj_t *self = mp_obj_malloc(machine_i2c_target_memory_obj_t, &machine_i2c_target_memory_type);
    self->i2c_id = i2c_id;
    self->i2c_inst = i2c_base_ptr_table[i2c_hw_id];
    uint8_t drive = args[ARG_drive].u_int;
    if (drive < 1 || drive > 7) {
        drive = DEFAULT_I2C_DRIVE;
    }
    // Set the target address.
    self->addr = args[ARG_addr].u_int;
    // Initialise data.
    MP_STATE_PORT(pyb_i2cslave_mem)[i2c_hw_id] = args[ARG_mem].u_obj;
    machine_i2c_target_data_t *data = &i2c_target_data[self->i2c_id];
    machine_i2c_target_data_init(data, args[ARG_mem].u_obj);

    // Initialise the GPIO pins
    lpi2c_set_iomux(i2c_hw_id, drive);
    // Initialise the I2C peripheral
    self->slave_config = m_new_obj(lpi2c_slave_config_t);
    LPI2C_SlaveGetDefaultConfig(self->slave_config);
    self->slave_config->address0 = self->addr;
    self->slave_config->sdaGlitchFilterWidth_ns = DEFAULT_I2C_FILTER_NS;
    self->slave_config->sclGlitchFilterWidth_ns = DEFAULT_I2C_FILTER_NS;
    LPI2C_SlaveInit(self->i2c_inst, self->slave_config, BOARD_BOOTCLOCKRUN_LPI2C_CLK_ROOT);
    // Create the LPI2C handle for the non-blocking transfer
    LPI2C_SlaveTransferCreateHandle(self->i2c_inst, &self->handle, lpi2c_slave_mem_callback, data);
    // Start accepting I2C transfers on the LPI2C slave peripheral
    status_t reVal = LPI2C_SlaveTransferNonBlocking(self->i2c_inst, &self->handle,
        kLPI2C_SlaveCompletionEvent |
        kLPI2C_SlaveTransmitEvent | kLPI2C_SlaveReceiveEvent);
    if (reVal != kStatus_Success) {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot start I2C"));
    }
    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_i2c_target_memory_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_i2c_target_memory_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2CTargetMemory(%u, addr=%u)",
        self->i2c_id, self->addr);
}

// Stop the Slave transfer and free the memory objects.
static void mp_machine_i2c_target_memory_deinit(machine_i2c_target_memory_obj_t *self) {
    LPI2C_SlaveDeinit(self->i2c_inst);
    m_del_obj(lpi2c_slave_config_t, self->slave_config);
    MP_STATE_PORT(pyb_i2cslave_mem)[self->i2c_id] = NULL;
}
