/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Blake W. Felt & Angus Gratton
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

#ifndef MICROPY_INCLUDED_SHARED_TINYUSB_MP_USBD_CDC_H
#define MICROPY_INCLUDED_SHARED_TINYUSB_MP_USBD_CDC_H

#ifndef MICROPY_HW_USB_CDC_TX_TIMEOUT
#define MICROPY_HW_USB_CDC_TX_TIMEOUT (500)
#endif

#endif

#if MICROPY_HW_USB_MSC_EXCLUSIVE_ACCESS
#include "tusb.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
static mp_sched_node_t mp_remount_sched_node;

STATIC void tud_msc_remount_task(mp_sched_node_t *node) {
    mp_vfs_mount_t *vfs = NULL;
    for (vfs = MP_STATE_VM(vfs_mount_table); vfs != NULL; vfs = vfs->next) {
        if (vfs->len == 1) {
            const char *path_str = "/";
            mp_obj_t path = mp_obj_new_str(path_str, strlen(path_str));
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_vfs_umount(vfs->obj);
                mp_vfs_mount_and_chdir_protected(vfs->obj, path);
                nlr_pop();
            }
            break;
        }
    }
}

void tud_msc_remount(void) {
    mp_sched_schedule_node(&mp_remount_sched_node, tud_msc_remount_task);
}
#endif
