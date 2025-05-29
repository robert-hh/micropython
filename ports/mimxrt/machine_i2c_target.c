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
#include CLOCK_CONFIG_H

#define DEFAULT_I2C_ID          (0)
#define DEFAULT_I2C_DRIVE       (6)
// BUFFER_SIZE must be sufficiently large to store the longest expected
// message + offset. At 8 bit addressing this is 257 bytes. Still
// longer data may occur, at which point the offset in the first byte
// will be overwritten and the data will be corrupted.
// Since the amount of data is not known beforehand, there is no way to
// prevent that situation.
#define BUFFER_SIZE             (257)

// These data objects and functions are defined in machine_i2c.c.
extern const uint8_t i2c_index_table[];
extern LPI2C_Type *i2c_base_ptr_table[];
extern bool lpi2c_set_iomux(int8_t hw_i2c, uint8_t drive);
#define MICROPY_HW_I2C_NUM      (6)
extern const uint8_t micropy_hw_i2c_num;

static machine_i2c_target_data_t i2c_target_data[MICROPY_HW_I2C_NUM];

typedef struct _machine_i2c_target_memory_obj_t {
    mp_obj_base_t base;
    LPI2C_Type *i2c_inst;
    uint8_t i2c_id;
    uint8_t addr;
    bool receive_event;
    uint32_t offset;
    lpi2c_slave_config_t *slave_config;
    lpi2c_slave_handle_t handle;
    uint8_t *buffer;
} machine_i2c_target_memory_obj_t;


MP_REGISTER_ROOT_POINTER(mp_obj_t pyb_i2cslave_mem[MICROPY_HW_I2C_NUM]);

static void lpi2c_slave_mem_callback(LPI2C_Type *base, lpi2c_slave_transfer_t *xfer, void *param) {
    machine_i2c_target_memory_obj_t *self = (machine_i2c_target_memory_obj_t *)param;
    machine_i2c_target_data_t *data = &i2c_target_data[self->i2c_id];

    switch (xfer->event)
    {
        // Repeated address event. Happens when switching the direction
        // at readfrom_mem
        case kLPI2C_SlaveRepeatedStartEvent:
            if (self->receive_event) {
                self->offset = self->buffer[0];
            }
            self->receive_event = false;
            break;
        //  Transmit request
        case kLPI2C_SlaveTransmitEvent:
            //  Update information for transmit process
            if (self->offset <= 0) {
                xfer->data = data->mem;
            } else {
                // If the offset is != 0, a shifted copy of the data has
                // to be created in the buffer to get the wrapping.
                uint8_t len = data->len + 1 - self->offset;
                memcpy(self->buffer, data->mem + self->offset, len);
                memcpy(self->buffer + len, data->mem, self->offset);
                xfer->data = self->buffer;
            }
            xfer->dataSize = data->len + 1;
            break;

        //  Receive request
        case kLPI2C_SlaveReceiveEvent:
            //  Update information for receive process
            xfer->data = self->buffer;
            xfer->dataSize = BUFFER_SIZE;
            machine_i2c_target_data_reset(data);
            self->receive_event = true;
            break;

        /*  Transfer done */
        case kLPI2C_SlaveCompletionEvent:
            xfer->data = NULL;
            xfer->dataSize = 0;
            if (self->receive_event) {
                self->offset = self->buffer[0];
                for (int i = 0; i < self->handle.transferredCount; i++) {
                    machine_i2c_target_data_write(data, self->buffer[i]);
                }
            }
            self->receive_event = false;
            self->handle.transferredCount = 0;
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
    // A temporary buffer is needed to store the transferred data, since
    // the callback is called once before and after a full transfer, not for
    // every byte.
    self->buffer = m_new(uint8_t, BUFFER_SIZE);
    self->buffer[0] = 0;

    // Initialise the GPIO pins
    lpi2c_set_iomux(i2c_hw_id, drive);
    // Initialise the I2C peripheral
    self->slave_config = m_new_obj(lpi2c_slave_config_t);
    LPI2C_SlaveGetDefaultConfig(self->slave_config);
    self->slave_config->address0 = self->addr;
    LPI2C_SlaveInit(self->i2c_inst, self->slave_config, BOARD_BOOTCLOCKRUN_LPI2C_CLK_ROOT);
    // Create the LPI2C handle for the non-blocking transfer
    LPI2C_SlaveTransferCreateHandle(self->i2c_inst, &self->handle, lpi2c_slave_mem_callback, self);
    // Start accepting I2C transfers on the LPI2C slave peripheral
    status_t reVal = LPI2C_SlaveTransferNonBlocking(self->i2c_inst, &self->handle,
        kLPI2C_SlaveCompletionEvent | kLPI2C_SlaveRepeatedStartEvent |
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
    machine_i2c_target_data_t *data = &i2c_target_data[self->i2c_id];
    LPI2C_SlaveDeinit(self->i2c_inst);
    m_del(uint8_t, self->buffer, BUFFER_SIZE);
    self->buffer = NULL;
    m_del_obj(lpi2c_slave_config_t, self->slave_config);
    // Dummy call to silence unused function compiler warning
    // These functions are not used.
    machine_i2c_target_data_read(data);
    MP_STATE_PORT(pyb_i2cslave_mem)[self->i2c_id] = NULL;
}
