/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2023 Damien P. George
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
#include "wm_internal_flash.h"
#include "wm_flash_map.h"

#include "py/runtime.h"

#include "extmod/vfs.h"

#define INTERNAL_FLS_BASE               (USER_ADDR_START)
#define INTERNAL_FLS_LEN                (USER_AREA_LEN)
#define INTERNAL_FLS_FS_PAGE_SIZE       (INSIDE_FLS_PAGE_SIZE)
#define INTERNAL_FLS_FS_SECTOR_SIZE     (INSIDE_FLS_SECTOR_SIZE)

/******************************************************************************/
// MicroPython bindings to expose the internal flash as an object with the
// block protocol.

// there is a singleton Flash object
extern const mp_obj_type_t w600_flash_type;
STATIC const mp_obj_base_t w600_flash_obj = {&w600_flash_type};

STATIC mp_obj_t w600_flash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return singleton object
    return (mp_obj_t)&w600_flash_obj;
}

// Function for writeblocks
// The tlf_fls_write() function erase a sector if required. Thus, erase is not
// required. The function & call is kept for visibility
STATIC mp_obj_t eraseblock(uint32_t addr_in) {

    // Destination address aligned with page start to be erased.
    // uint32_t sector = addr_in / INTERNAL_FLS_FS_SECTOR_SIZE; // Number of page to be erased.
    // mp_printf(MP_PYTHON_PRINTER, "erase_block %x -> %d\n", addr_in, sector);
    // tls_fls_erase(sector);

    return mp_const_none;
}

STATIC mp_obj_t w600_flash_readblocks(size_t n_args, const mp_obj_t *args) {
    uint32_t offset = (mp_obj_get_int(args[1]) * INTERNAL_FLS_FS_SECTOR_SIZE) + INTERNAL_FLS_BASE;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    if (n_args == 4) {
        offset += mp_obj_get_int(args[3]);
    }
    // mp_printf(MP_PYTHON_PRINTER, "readblocks %x -> %x\n", mp_obj_get_int(args[1]), offset);

    int result = tls_fls_read(offset, bufinfo.buf, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(w600_flash_readblocks_obj, 3, 4, w600_flash_readblocks);

STATIC mp_obj_t w600_flash_writeblocks(size_t n_args, const mp_obj_t *args) {
    uint32_t offset = (mp_obj_get_int(args[1]) * INTERNAL_FLS_FS_SECTOR_SIZE) + INTERNAL_FLS_BASE;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    if (n_args == 3) {
        eraseblock(offset);
        // TODO check return value
    } else {
        offset += mp_obj_get_int(args[3]);
    }
    // mp_printf(MP_PYTHON_PRINTER, "writeblocks %x -> %x\n", mp_obj_get_int(args[1]), offset);

    int result = tls_fls_write(offset, bufinfo.buf, bufinfo.len);
    // TODO check return value
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(w600_flash_writeblocks_obj, 3, 4, w600_flash_writeblocks);

STATIC mp_obj_t w600_flash_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);

    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_DEINIT:
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_SYNC:
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
            return MP_OBJ_NEW_SMALL_INT(INTERNAL_FLS_LEN / INTERNAL_FLS_FS_SECTOR_SIZE);
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
            return MP_OBJ_NEW_SMALL_INT(INTERNAL_FLS_FS_SECTOR_SIZE);
        case MP_BLOCKDEV_IOCTL_BLOCK_ERASE: {
            // The tlf_fls_write() function erase a sector if required. Thus, erase is not
            // required. The function & call is kept for visibility

            // int sector = mp_obj_get_int(arg_in) + INTERNAL_FLS_BASE / INTERNAL_FLS_FS_SECTOR_SIZE;
            // mp_printf(MP_PYTHON_PRINTER, "erase_ioctl %d -> %d\n", mp_obj_get_int(arg_in), sector);
            // tls_fls_erase(sector);
        }
            // TODO check return value
            return MP_OBJ_NEW_SMALL_INT(0);
        default:
            return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(w600_flash_ioctl_obj, w600_flash_ioctl);

STATIC const mp_rom_map_elem_t w600_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&w600_flash_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&w600_flash_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&w600_flash_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(w600_flash_locals_dict, w600_flash_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    w600_flash_type,
    MP_QSTR_Flash,
    MP_TYPE_FLAG_NONE,
    make_new, w600_flash_make_new,
    locals_dict, &w600_flash_locals_dict
    );
