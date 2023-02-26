/*****************************************************************************
*
* File Name : wm_fwup.c
*
* Description: firmware update Module
*
* Copyright (c) 2014 Winner Micro Electronic Design Co., Ltd.
* All rights reserved.
*
* Author : dave
*
* Date : 2014-6-16
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wm_mem.h"
#include "wm_fwup.h"
#include "wm_flash_map.h"
#include "wm_internal_flash.h"

extern u32 flashtotalsize;

u32 tls_fwup_enter(enum tls_fwup_image_src image_src) {
    return 0;
}

/**Run-time image area size*/
unsigned int CODE_RUN_AREA_LEN = 0;

/**Area can be used by User in 1M position*/
unsigned int USER_ADDR_START = 0;
unsigned int TLS_FLASH_PARAM_DEFAULT = 0;
unsigned int USER_AREA_LEN = 0;
unsigned int USER_ADDR_END = 0;


/**Upgrade image header area & System parameter area */
unsigned int CODE_UPD_HEADER_ADDR = 0;
unsigned int TLS_FLASH_PARAM1_ADDR = 0;
unsigned int TLS_FLASH_PARAM2_ADDR = 0;
unsigned int TLS_FLASH_PARAM_RESTORE_ADDR = 0;

/**Upgrade image area*/
unsigned int CODE_UPD_START_ADDR = 0;
unsigned int CODE_UPD_AREA_LEN = 0;

unsigned int TLS_FLASH_END_ADDR = 0;

#ifndef MICROPY_HW_CODESIZE
#define MICROPY_HW_CODESIZE             0xC0000
#endif

#undef CODE_RUN_START_ADDR
#ifdef MICROPY_HW_CODEADDRESS
#define CODE_RUN_START_ADDR             MICROPY_HW_CODEADDRESS
#else
#define CODE_RUN_START_ADDR             0x8010100
#endif

void tls_fls_layout_init(void) {

    CODE_RUN_AREA_LEN = (MICROPY_HW_CODESIZE - 256);

    /** User & System parameter area */
    TLS_FLASH_PARAM_DEFAULT = (CODE_RUN_START_ADDR + CODE_RUN_AREA_LEN);
    TLS_FLASH_PARAM1_ADDR = (TLS_FLASH_PARAM_DEFAULT + 0x1000);
    TLS_FLASH_PARAM2_ADDR = (TLS_FLASH_PARAM1_ADDR + 0x1000);
    TLS_FLASH_PARAM_RESTORE_ADDR = (TLS_FLASH_PARAM2_ADDR + 0x1000);
    TLS_FLASH_END_ADDR = (TLS_FLASH_PARAM_RESTORE_ADDR + 0x1000 - 1);

    /**Area can be used by User*/
    USER_ADDR_START = (TLS_FLASH_END_ADDR + 1);
    USER_AREA_LEN = FLASH_BASE_ADDR + flashtotalsize - USER_ADDR_START;
    USER_ADDR_END = (USER_ADDR_START + USER_AREA_LEN - 1);

}
