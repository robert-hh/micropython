/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Damien P. George
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

#include "py/runtime.h"
#include "modmimxrt.h"
#include "py/mperrno.h"
#include "hal/flexspi_nor_flash.h"

extern status_t flash_erase_block(uint32_t erase_addr);
extern status_t flash_write_block(uint32_t dest_addr, const uint8_t *src, uint32_t length);
extern uint8_t __vfs_start;
extern uint8_t __vfs_end;
extern uint8_t __flash_start;
// BOARD_FLASH_SIZE is defined in mpconfigport.h
#define SECTOR_SIZE_BYTES (qspiflash_config.sectorSize)
#define PAGE_SIZE_BYTES (qspiflash_config.pageSize)


STATIC mp_obj_t mimxrt_flash_read(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    memcpy(bufinfo.buf, (uint8_t *)(FlexSPI_AMBA_BASE + offset), bufinfo.len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mimxrt_flash_read_obj, mimxrt_flash_read);

STATIC mp_obj_t mimxrt_flash_write(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    // address and length must be a multiple of the page size
    if (offset % PAGE_SIZE_BYTES || bufinfo.len % PAGE_SIZE_BYTES) {
        mp_raise_OSError(MP_EINVAL);
    }
    status_t status = flash_write_block(offset, bufinfo.buf, bufinfo.len);
    if (status != kStatus_Success) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mimxrt_flash_write_obj, mimxrt_flash_write);

STATIC mp_obj_t mimxrt_flash_erase(mp_obj_t sector_in) {
    mp_int_t sector = mp_obj_get_int(sector_in);
    status_t status = flash_erase_block(sector * SECTOR_SIZE_BYTES);
    if (status != kStatus_Success) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mimxrt_flash_erase_obj, mimxrt_flash_erase);

STATIC mp_obj_t mimxrt_flash_size(void) {
    return mp_obj_new_int_from_uint(BOARD_FLASH_SIZE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mimxrt_flash_size_obj, mimxrt_flash_size);

STATIC mp_obj_t mimxrt_flash_user_start(void) {
    return mp_obj_new_int_from_uint((uint32_t)&__vfs_start - FlexSPI_AMBA_BASE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mimxrt_flash_user_start_obj, mimxrt_flash_user_start);


STATIC const mp_rom_map_elem_t mimxrt_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_mimxrt) },
    { MP_ROM_QSTR(MP_QSTR_Flash),               MP_ROM_PTR(&mimxrt_flash_type) },

    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&mimxrt_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&mimxrt_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&mimxrt_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&mimxrt_flash_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_PTR(&mimxrt_flash_user_start_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mimxrt_module_globals, mimxrt_module_globals_table);

const mp_obj_module_t mp_module_mimxrt = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mimxrt_module_globals,
};
