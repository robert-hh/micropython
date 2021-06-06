/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Philipp Ebensberger
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
#include "extmod/vfs.h"
#include "fsl_iomuxc.h"
#include "fsl_usdhc.h"

#include "modmachine.h"


#define SDCARD_INIT_ARG_ID (0)


typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    USDHC_Type *sdcard;
} mimxrt_sdcard_obj_t;


STATIC const mimxrt_sdcard_obj_t mimxrt_sdcard_objs[1] = {
    {
        .base.type = &machine_sdcard_type,
        .sdcard = USDHC1,
    }
};

STATIC const mp_arg_t allowed_args[] = {
    [SDCARD_INIT_ARG_ID]     { MP_QSTR_id, MP_ARG_INT, {.u_int = 0} },
};


STATIC void machine_sdcard_init_helper(const mimxrt_sdcard_obj_t *self, const mp_arg_val_t *args) {
    // Initialize pins

    // TODO: Implement APi for pin.c to initialize a GPIO for an ALT function

    // GPIO_SD_B0_00 | ALT0 | USDHC1_DATA2 | SD1_D2    | DAT2
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_00_USDHC1_DATA2,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_USDHC1_DATA2, 0x10B0u);

    // GPIO_SD_B0_01 | ALT0 | USDHC1_DATA3 | SD1_D3    | CD/DAT3
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_01_USDHC1_DATA3,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_USDHC1_DATA3, 0x10B0u);

    // GPIO_SD_B0_02 | ALT0 | USDHC1_CMD   | SD1_CMD   | CMD
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_02_USDHC1_CMD,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_USDHC1_CMD, 0x10B0u);

    // GPIO_SD_B0_03 | ALT0 | USDHC1_CLK   | SD1_CLK   | CLK
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_03_USDHC1_CLK,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_USDHC1_CLK, 0x10B0u);

    // GPIO_SD_B0_04 | ALT0 | USDHC1_DATA0 | SD1_D0    | DAT0
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA0,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_04_USDHC1_DATA0, 0x10B0u);

    // GPIO_SD_B0_05 | ALT0 | USDHC1_DATA1 | SD1_D1    | DAT1
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA1,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_05_USDHC1_DATA1, 0x10B0u);

    // GPIO_SD_B0_06 | ALT0 | USDHC1_CD_B  | SD_CD_SW  | DETECT
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_06_USDHC1_CD_B,0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_06_USDHC1_CD_B, 0x10B0u);

    // Initialize USDHC
    const usdhc_config_t config = {
        .endianMode = kUSDHC_EndianModeLittle,
        .dataTimeout = 0xFU,
        .readWatermarkLevel = 0x80U,
        .writeWatermarkLevel = 0x80U,
    };

    USDHC_Init(self->sdcard, &config);
}


STATIC mp_obj_t sdcard_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    const mimxrt_sdcard_obj_t *self = &mimxrt_sdcard_objs[(args[0].u_int - 1)];
    machine_sdcard_init_helper(self, args);
    return MP_OBJ_FROM_PTR(self);
}

// init()
STATIC mp_obj_t machine_sdcard_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args) - 1];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_sdcard_init_helper(pos_args[0], args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_sdcard_init_obj, 1, machine_sdcard_init);

// deinit()
STATIC mp_obj_t machine_sdcard_deinit(mp_obj_t self_in) {
    // TODO: Implement deinit function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_sdcard_deinit_obj, machine_sdcard_deinit);

// readblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_readblocks(mp_obj_t self_in, mp_obj_t block_num, mp_obj_t buf) {
    // TODO: Implement machine_sdcard_readblocks function

    const mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // usdhc_command_t command = {
    // uint32_t index;                          /*!< Command index */
    // uint32_t argument;                       /*!< Command argument */
    // usdhc_card_command_type_t type;          /*!< Command type */
    // usdhc_card_response_type_t responseType; /*!< Command response type */
    // uint32_t response[4U];                   /*!< Response for this command */
    // uint32_t responseErrorFlags;             /*!< response error flag, the flag which need to check
    //                                             the command reponse*/
    // uint32_t flags;                          /*!< Cmd flags */
    // };

    // USDHC_SendCommand(self->sdcard, &command);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_readblocks_obj, machine_sdcard_readblocks);

// writeblocks(block_num, buf, [offset])
STATIC mp_obj_t machine_sdcard_writeblocks(mp_obj_t self_in, mp_obj_t block_num, mp_obj_t buf) {
    // TODO: Implement machine_sdcard_writeblocks function
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_writeblocks_obj, machine_sdcard_writeblocks);

// ioctl(op, arg)
STATIC mp_obj_t machine_sdcard_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {
    // mimxrt_sdcard_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
        case MP_BLOCKDEV_IOCTL_DEINIT:
        case MP_BLOCKDEV_IOCTL_SYNC:
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
        // TODO: Return number of bytes
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
        // TODO: Return size of block
        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(machine_sdcard_ioctl_obj, machine_sdcard_ioctl);

STATIC const mp_rom_map_elem_t sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),        MP_ROM_PTR(&machine_sdcard_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),      MP_ROM_PTR(&machine_sdcard_deinit_obj) },
    // block device protocol
    { MP_ROM_QSTR(MP_QSTR_readblocks),  MP_ROM_PTR(&machine_sdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&machine_sdcard_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl),       MP_ROM_PTR(&machine_sdcard_ioctl_obj) },
};
STATIC MP_DEFINE_CONST_DICT(sdcard_locals_dict, sdcard_locals_dict_table);

const mp_obj_type_t machine_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = sdcard_obj_make_new,
    .locals_dict = (mp_obj_dict_t *)&sdcard_locals_dict,
};

void machine_sdcard_init0(void) {
    return;
}