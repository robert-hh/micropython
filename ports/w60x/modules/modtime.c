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
#include "wm_rtc.h"
#include "py/obj.h"
#include "shared/timeutils/timeutils.h"
#include "modmachine.h"

// Return the localtime as an 8-tuple.
static void mp_time_localtime_get(timeutils_struct_time_t *tm) {
    struct tm tm_rtc;
    tls_get_rtc(&tm_rtc);

    tm->tm_year  = tm_rtc.tm_year + W600_YEAR_BASE;
    tm->tm_mon = tm_rtc.tm_mon;
    tm->tm_mday = tm_rtc.tm_mday;
    tm->tm_hour = tm_rtc.tm_hour;
    tm->tm_min = tm_rtc.tm_min;
    tm->tm_sec = tm_rtc.tm_sec;
    tm->tm_wday = timeutils_calc_weekday(tm_rtc.tm_year + W600_YEAR_BASE, tm_rtc.tm_mon, tm_rtc.tm_mday);
    tm->tm_yday = timeutils_year_day(tm_rtc.tm_year, tm_rtc.tm_mon, tm_rtc.tm_mday);
}

// Returns the number of seconds, as an integer, since the Epoch.
static mp_obj_t mp_time_time_get(void) {
    struct tm tblock;
    tls_get_rtc(&tblock);
    return mp_obj_new_int_from_uint(timeutils_mktime(tblock.tm_year + W600_YEAR_BASE,
        tblock.tm_mon, tblock.tm_mday, tblock.tm_hour, tblock.tm_min, tblock.tm_sec));
}
