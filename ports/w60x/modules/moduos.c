/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
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

#include <string.h>

#include "wm_include.h"
#include "wm_adc.h"
#include "wm_crypto_hard.h"
#include "mphalport.h"

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#if MICROPY_PY_UOS_URANDOM
u32 w600_adc_get_allch_4bit_rst(void)
{
extern u8 adc_irq_flag;
	u32 average = 0;
	int i = 0;

	for (i = 0; i < 8; i++)
	{
		adc_get_offset();
		tls_adc_init(0, 0);
		tls_adc_reference_sel(ADC_REFERENCE_EXTERNAL);
		adc_irq_flag = 0;
		tls_adc_start_with_cpu(i);
		tls_os_time_delay(2);
        while(1)
        {
            if(adc_irq_flag)
            {
                adc_irq_flag = 0;
                break;
            }
        }
		average = (average << 4) | (tls_read_adc_result() & 0xF);
		tls_adc_stop(0);
	}

	return average;
}

STATIC mp_obj_t mp_uos_urandom(mp_obj_t num) {
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    static bool seeded = false; // Seed the RNG on the first call to uos.urandom
    if (seeded == false) {
        /* adc floating in the air can get the seed of true random number */
        tls_crypto_random_init(w600_adc_get_allch_4bit_rst(), CRYPTO_RNG_SWITCH_32); 
        seeded = true;
    }
    vstr_init_len(&vstr, n);
    tls_crypto_random_bytes((unsigned char *)vstr.buf, n);
    return mp_obj_new_bytes_from_vstr(&vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_uos_urandom_obj, mp_uos_urandom);
#endif

#if MICROPY_PY_OS_DUPTERM
STATIC mp_obj_t mp_uos_dupterm_notify(mp_obj_t obj_in) {
    (void)obj_in;
    for (;;) {
        int c = mp_uos_dupterm_rx_chr();
        if (c < 0) {
            break;
        }
        ringbuf_put(&stdin_ringbuf, c);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_uos_dupterm_notify_obj, mp_uos_dupterm_notify);
#endif
