/*
 * This file is part of the MicroPython project, http://micropython.org/
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

#include "wm_include.h"
#include "shared/runtime/mpirq.h"

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    u16 uart_num;
    tls_uart_options_t uartcfg;
    uint16_t mp_irq_trigger;   // user IRQ trigger mask
    uint16_t mp_irq_flags;     // user IRQ active IRQ flags
    #if MICROPY_PY_MACHINE_IRQ_TIMESTAMP
    uint32_t mp_irq_timestamp;
    #endif
    mp_irq_obj_t *mp_irq_obj;  // user IRQ object
} machine_uart_obj_t;

int tls_uart_tx_due_len(u16 port_no);
int tls_uart_write_async(u16 uart_no, char *buf, u16 writesize);

#define IRQ_RX                  (4)
#define IRQ_RXIDLE              (8)
#define IRQ_BREAK               (32)
#define MP_UART_ALLOWED_FLAGS   (IRQ_RX | IRQ_RXIDLE | IRQ_BREAK)

#define MICROPY_PY_MACHINE_UART_CLASS_CONSTANTS \
    { MP_ROM_QSTR(MP_QSTR_IRQ_RX), MP_ROM_INT(IRQ_RX) }, \
    { MP_ROM_QSTR(MP_QSTR_IRQ_RXIDLE), MP_ROM_INT(IRQ_RXIDLE) }, \
    { MP_ROM_QSTR(MP_QSTR_IRQ_BREAK), MP_ROM_INT(IRQ_BREAK) }, \

static machine_uart_obj_t *uart_list[3];

/******************************************************************************/
// MicroPython bindings for UART

static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u)",
        self->uart_num, self->uartcfg.baudrate, self->uartcfg.charlength + 5,
        self->uartcfg.paritytype ? ((self->uartcfg.paritytype == 1) ? "1" : "0") : "None",
        self->uartcfg.stopbits + 1);
}

static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set baudrate
    if (args[ARG_baudrate].u_int > 0) {
        self->uartcfg.baudrate = args[ARG_baudrate].u_int;
    }

    // set data bits
    switch (args[ARG_bits].u_int) {
        case 0:
            break;
        case 5:
            self->uartcfg.charlength = TLS_UART_CHSIZE_5BIT;
            break;
        case 6:
            self->uartcfg.charlength = TLS_UART_CHSIZE_6BIT;
            break;
        case 7:
            self->uartcfg.charlength = TLS_UART_CHSIZE_7BIT;
            break;
        case 8:
            self->uartcfg.charlength = TLS_UART_CHSIZE_8BIT;
            break;
        default:
            mp_raise_ValueError("invalid data bits");
            break;
    }

    // set parity
    if (args[ARG_parity].u_obj != MP_OBJ_NULL) {
        if (args[ARG_parity].u_obj == mp_const_none) {
            self->uartcfg.paritytype = TLS_UART_PMODE_DISABLED;
        } else {
            mp_int_t parity = mp_obj_get_int(args[ARG_parity].u_obj);
            if (0 == parity) {
                self->uartcfg.paritytype = TLS_UART_PMODE_EVEN;
            } else if (1 == parity) {
                self->uartcfg.paritytype = TLS_UART_PMODE_ODD;
            }
        }
    }

    // set stop bits
    switch (args[ARG_stop].u_int) {
        case 0:
            break;
        case 1:
            self->uartcfg.stopbits = TLS_UART_ONE_STOPBITS;
            break;
        case 2:
            self->uartcfg.stopbits = TLS_UART_TWO_STOPBITS;
            break;
        default:
            mp_raise_ValueError("invalid stop bits");
            break;
    }

    tls_uart_port_init(self->uart_num, &self->uartcfg, 0);
}

static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_num = mp_obj_get_int(args[0]);
    if (uart_num < TLS_UART_0 || uart_num > TLS_UART_2) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_num));
    }

    // Attempts to use UART0 from Python has resulted in all sorts of fun errors.
    // FIXME: UART0 is disabled for now.
    if (uart_num == TLS_UART_0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) is disabled (dedicated to REPL)", uart_num));
    }

    // Defaults
    machine_uart_obj_t *self = mp_obj_malloc(machine_uart_obj_t, &machine_uart_type);
    self->uart_num = uart_num;
    self->uartcfg.charlength = TLS_UART_CHSIZE_8BIT;
    self->uartcfg.paritytype = TLS_UART_PMODE_DISABLED;
    self->uartcfg.stopbits = TLS_UART_ONE_STOPBITS;
    self->uartcfg.flow_ctrl = TLS_UART_FLOW_CTRL_NONE;
    self->mp_irq_obj = NULL;
    uart_list[self->uart_num] = self;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self) {
    size_t rxbufsize;
    return tls_uart_read_avail(self->uart_num);
}

static bool mp_machine_uart_txdone(machine_uart_obj_t *self) {
    return tls_uart_tx_due_len(self->uart_num) == 0;
}

// Deinit is not available in the tls_uart lib.
static void mp_machine_uart_deinit(machine_uart_obj_t *self) {
    if (self) {
        uart_list[self->uart_num] = NULL;
        self->mp_irq_trigger = 0;
        self->mp_irq_obj->handler = mp_const_none;
    }
}

void machine_uart_deinit_all(void) {
    mp_machine_uart_deinit(uart_list[1]);
    mp_machine_uart_deinit(uart_list[2]);
}

static s16 mp_irq_callback_1(u16 irq_flags) {
    machine_uart_obj_t *self = uart_list[1];
    if (self && self->mp_irq_obj && (self->mp_irq_trigger & irq_flags)) {
        self->mp_irq_flags = irq_flags;
        #if MICROPY_PY_MACHINE_IRQ_TIMESTAMP
        self->mp_irq_timestamp = mp_hal_ticks_us();
        #endif
        mp_irq_handler(self->mp_irq_obj);
    }
    return 0;
}

static s16 mp_irq_callback_2(u16 irq_flags) {
    machine_uart_obj_t *self = uart_list[2];
    if (self && self->mp_irq_obj && (self->mp_irq_trigger & irq_flags)) {
        self->mp_irq_flags = irq_flags;
        #if MICROPY_PY_MACHINE_IRQ_TIMESTAMP
        self->mp_irq_timestamp = mp_hal_ticks_us();
        #endif
        mp_irq_handler(self->mp_irq_obj);
    }
    return 0;
}

#if MICROPY_PY_MACHINE_IRQ_TIMESTAMP
static mp_uint_t uart_irq_timestamp(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->mp_irq_timestamp;
}
#endif

static mp_uint_t uart_irq_trigger(mp_obj_t self_in, mp_uint_t new_trigger) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->mp_irq_trigger = new_trigger;
    return 0;
}

static mp_uint_t uart_irq_info(mp_obj_t self_in, mp_uint_t info_type) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (info_type == MP_IRQ_INFO_FLAGS) {
        return self->mp_irq_flags;
    } else if (info_type == MP_IRQ_INFO_TRIGGERS) {
        return self->mp_irq_trigger;
    }
    return 0;
}

const mp_irq_methods_t uart_irq_methods = {
    .trigger = uart_irq_trigger,
    .info = uart_irq_info,
    #if MICROPY_PY_MACHINE_IRQ_TIMESTAMP
    .timestamp = uart_irq_timestamp,
    #endif
};

static mp_irq_obj_t *mp_machine_uart_irq(machine_uart_obj_t *self, bool any_args, mp_arg_val_t *args) {
    if (self->mp_irq_obj == NULL) {
        self->mp_irq_trigger = 0;
        self->mp_irq_obj = mp_irq_new(&uart_irq_methods, MP_OBJ_FROM_PTR(self));
    }

    if (any_args) {
        // Check the handler
        mp_obj_t handler = args[MP_IRQ_ARG_INIT_handler].u_obj;
        if (handler != mp_const_none && !mp_obj_is_callable(handler)) {
            mp_raise_ValueError(MP_ERROR_TEXT("handler must be None or callable"));
        }

        // Check the trigger
        mp_uint_t trigger = args[MP_IRQ_ARG_INIT_trigger].u_int;
        mp_uint_t not_supported = trigger & ~MP_UART_ALLOWED_FLAGS;
        if (trigger != 0 && not_supported) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("trigger 0x%04x unsupported"), not_supported);
        }

        self->mp_irq_obj->handler = handler;
        self->mp_irq_obj->ishard = false;  // hard=True is not supported
        self->mp_irq_trigger = trigger;
        if (self->uart_num == 1) {
            tls_uart_rx_callback_register(TLS_UART_1, handler != mp_const_none ? mp_irq_callback_1 : NULL);
        } else if (self->uart_num == 2) {
            tls_uart_rx_callback_register(TLS_UART_2, handler != mp_const_none ? mp_irq_callback_2 : NULL);
        }
    }

    return self->mp_irq_obj;
}

static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    int bytes_read = tls_uart_read(self->uart_num, buf_in, size);

    if (bytes_read <= 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    return bytes_read;
}

static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int bytes_written = tls_uart_write_async(self->uart_num, (char *)buf_in, size);

    if (bytes_written < 0) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // return number of bytes written
    return size;
}

static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        uintptr_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && tls_uart_read_avail(self->uart_num)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if (flags & MP_STREAM_POLL_WR) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else if (request == MP_STREAM_FLUSH) {
        ret = 0;
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}
