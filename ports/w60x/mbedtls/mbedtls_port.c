/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
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

#ifdef MICROPY_SSL_MBEDTLS

#include "mbedtls_config_port.h"
#include "wm_crypto_hard.h"
#if defined(MBEDTLS_HAVE_TIME) || defined(MBEDTLS_HAVE_TIME_DATE)
#include "wm_rtc.h"
#include "py/runtime.h"
#include "shared/timeutils/timeutils.h"
#include "mbedtls/platform_time.h"
#include "modmachine.h"
#endif

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    tls_crypto_random_init(tls_os_get_time(), CRYPTO_RNG_SWITCH_32);
    tls_crypto_random_bytes(output, len);
    tls_crypto_random_stop();
    *olen = len;
    return 0;
}

#if defined(MBEDTLS_HAVE_TIME)
time_t w60x_rtctime_seconds(time_t *timer) {
    struct tm tblock;
    tls_get_rtc(&tblock);
    return timeutils_seconds_since_epoch(tblock.tm_year + W600_YEAR_BASE,
        tblock.tm_mon, tblock.tm_mday, tblock.tm_hour, tblock.tm_min, tblock.tm_sec);
}

mbedtls_ms_time_t mbedtls_ms_time(void) {
    time_t *tv = NULL;
    mbedtls_ms_time_t current_ms;
    current_ms = w60x_rtctime_seconds(tv) * 1000;
    return current_ms;
}

#endif

#if defined(MBEDTLS_HAVE_TIME_DATE)
struct tm *gmtime(const time_t *timep) {
    static struct tm tm;
    timeutils_struct_time_t tm_buf = {0};
    timeutils_seconds_since_epoch_to_struct_time(*timep, &tm_buf);

    tm.tm_sec = tm_buf.tm_sec;
    tm.tm_min = tm_buf.tm_min;
    tm.tm_hour = tm_buf.tm_hour;
    tm.tm_mday = tm_buf.tm_mday;
    tm.tm_mon = tm_buf.tm_mon - 1;
    tm.tm_year = tm_buf.tm_year - 1900;
    tm.tm_wday = tm_buf.tm_wday;
    tm.tm_yday = tm_buf.tm_yday;
    tm.tm_isdst = -1;

    return &tm;
}
#endif

#endif
