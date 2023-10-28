# paths to WM_SDK
ifneq ($(WMSDK_PATH),)
TOPDIR = $(WMSDK_PATH)
else
$(info The WMSDK_PATH variable has not been set, please set it to the root of the WM_SDK.)
$(info See README.md for installation instructions.)
$(error WMSDK_PATH not set)
endif
# Select the board to build for:
ifdef BOARD_DIR
# Custom board path - remove trailing slash and get the final component of
# the path as the board name.
BOARD ?= $(notdir $(BOARD_DIR:/=))
else
# If not given on the command line, then default to GENERIC.
BOARD ?= GENERIC
BOARD_DIR ?= boards/$(BOARD)
endif

ifeq ($(wildcard $(BOARD_DIR)/.),)
$(error Invalid BOARD specified: $(BOARD_DIR))
endif

BUILD ?= build-$(BOARD)

# Select the cross compile prefix
CROSS_COMPILE ?= arm-none-eabi-

include ../../py/mkenv.mk
# include mpconfigboard.mk if it exists
ifneq (,$(wildcard $(BOARD_DIR)/mpconfigboard.mk))
include $(BOARD_DIR)/mpconfigboard.mk
endif
# qstr definitions (must come before including py.mk)

GIT_SUBMODULES = lib/mbedtls

QSTR_DEFS = qstrdefsport.h

MICROPY_USE_FATFS ?= 0
MICROPY_USE_LITTLEFS = 1

MICROPY_PY_THREAD ?= 1

MICROPY_PY_SSL ?= 1

ifeq ($(MICROPY_PY_SSL),1)
MICROPY_SSL_MBEDTLS = 1
else
MICROPY_SSL_MBEDTLS = 0
endif

SECBOOT ?= 1

CODESIZE ?= 0xc0000  # 768 k reserved for code

ifeq ($(SECBOOT),1)
CODEADDRESS = 0x8010100
else
CODEADDRESS = 0x8002100
endif

# FROZEN_MPY_DIR = scripts
FROZEN_MANIFEST ?= manifest.py

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I./include
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I$(BOARD_DIR)
INC += -I$(TOP)/shared/readline
INC += -I$(TOP)/shared/netutils
INC += -I$(TOP)/shared/timeutils

#TOPDIR = $(WMSDK_PATH)
#include $(TOPDIR)/Tools/rules.mk
#include $(TOPDIR)/Tools/toplevel.mk
export TOPDIR
include $(TOPDIR)/Tools/subdir.mk
COMPILER_OS=$(shell uname -o)
$(info COMPILER_OS: $(COMPILER_OS))
export COMPILER_OS
ifeq ($(COMPILER_OS),Cygwin)
COMPILER_OS_CYGWIN=1
else
COMPILER_OS_CYGWIN=0
endif
export COMPILER_OS_CYGWIN
include $(TOPDIR)/Tools/verbose.mk

ifeq ($(COMPILER_OS_CYGWIN),1)
$(shell mkdir $(BUILD))
$(shell ./tools/cygwin.sh $(PWD) $(WMSDK_PATH))
include $(BUILD)/cygwin.mk
endif

include $(TOPDIR)/Tools/toolchain.def

CFLAGS += -g  # always include debug info in the ELF

OUTPUT_PATH = $(BUILD)

PACKIMG = tools/create_img.sh
DOWNLOADIMG = tools/download_img.sh
DOWNLOADFLS = tools/download_fls.sh

#---------------------------------------------------------------------------
# Target definition (User)
#---------------------------------------------------------------------------
GOAL = wm_w600

#---------------------------------------------------------------------------
# Source section (User)
#---------------------------------------------------------------------------
# ASM_SRC := $(TOPDIR)/Platform/Boot/gcc/startup_ARMCM3.s
ASM_SRC := hal/startup_ARMCM3.s
C_SRC := $(TOPDIR)/Platform/Boot/gcc/retarget_gcc.c \
	 $(TOPDIR)/Platform/Boot/gcc/misc.c \

#---------------------------------------------------------------------------
# Linker options
#---------------------------------------------------------------------------
LDFLAGS	   += -static
LDFLAGS    += -nostartfiles
LDFLAGS    += -T./tools/link_w600.ld
LDFLAGS    += -mthumb
LDFLAGS    += -mcpu=cortex-m3
#---------------------------------------------------------------------------
# Link Library Section
#---------------------------------------------------------------------------
#LIB_SRC := $(LIB_DIR)/wmboot.a
LIB_SRC += \
	$(LIB_DIR)/libairkiss_log.a \
	$(LIB_DIR)/oneshot.a \
	$(LIB_DIR)/wmcJSON.a \
	$(LIB_DIR)/wmcmd.a \
	$(LIB_DIR)/wmcommon.a \
	$(LIB_DIR)/wmdemo.a \
	$(LIB_DIR)/wmdhcpserver.a \
	$(LIB_DIR)/wmdnsserver.a \
	$(LIB_DIR)/wmdriver.a \
	$(LIB_DIR)/wmhttpclient.a \
	$(LIB_DIR)/wmiperf.a \
	$(LIB_DIR)/wmlwip.a \
	$(LIB_DIR)/wmmain.a \
	$(LIB_DIR)/wmntp.a \
	$(LIB_DIR)/wmota.a \
	$(LIB_DIR)/wmping.a \
	$(LIB_DIR)/wmrtos.a \
	$(LIB_DIR)/wmssl.a \
	$(LIB_DIR)/wmsslserver.a \
	$(LIB_DIR)/wmweb.a \
	$(LIB_DIR)/wmwebsocket.a \

ifeq ($(COST_DOWN),1)
LIB_SRC += $(LIB_DIR)/wlan_costdown.a
else
LIB_SRC += $(LIB_DIR)/wlan.a
endif


#---------------------------------------------------------------------------
# mpy Source section
#---------------------------------------------------------------------------
MPY_SRC_C += \
	hal/bootloader_helper.c \
	hal/fatfs_port.c \
	hal/gccollect.c \
	hal/help.c \
	hal/math_port.c \
	hal/mphalport.c \
	hal/mpthreadport.c \
	hal/uart.c \
	hal/w600_flash.c \
	hal/wm_fwup.c \
	hal/wm_hostspi.c \
	hal/wm_main.c \
	hal/wm_uart.c \
	machine/machine_adc.c \
	machine/machine_bitstream.c \
	machine/machine_i2c.c \
	machine/machine_pin.c \
	machine/machine_rtc.c \
	machine/machine_spi.c \
	machine/machine_timer.c \
	main.c \
	modules/modhashlib.c \
	modules/modmachine.c \
	modules/modnetwork.c \
	modules/modsocket.c \
	modules/modw600.c \
	tools/ftpserver.c \

LIB_SRC_C += $(addprefix lib/,\
	libm/asinfacosf.c \
	libm/atan2f.c \
	libm/atanf.c \
	libm/ef_rem_pio2.c \
	libm/ef_sqrt.c \
	libm/fmodf.c \
	libm/kf_cos.c \
	libm/kf_rem_pio2.c \
	libm/kf_sin.c \
	libm/kf_tan.c \
	libm/math.c \
	libm/roundf.c \
	libm/sf_cos.c \
	libm/sf_frexp.c \
	libm/sf_ldexp.c \
	libm/sf_modf.c \
	libm/sf_sin.c \
	libm/sf_tan.c \
    libm/acoshf.c \
    libm/asinhf.c \
    libm/atanhf.c \
    libm/erf_lgamma.c \
    libm/log1pf.c \
    libm/nearbyintf.c \
    libm/sf_erf.c \
    libm/wf_lgamma.c \
    libm/wf_tgamma.c \
	)

LIB_SRC_C += $(addprefix shared/,\
	netutils/netutils.c \
	readline/readline.c \
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/sys_stdio_mphal.c \
	timeutils/timeutils.c \
	)

ifeq ($(MICROPY_USE_FATFS), 1)
CFLAGS += -DFFCONF_H='"lib/oofatfs/ffconf.h"'
LIB_SRC_C += \
	lib/oofatfs/ff.c \
	lib/oofatfs/ffunicode.c
endif

ifeq ($(MICROPY_USE_LITTLEFS), 1)
CFLAGS += -DLFS2_NO_DEBUG
# CFLAGS += -DLFS2_NO_DEBUG -DLFS2_NO_WARN -DLFS2_NO_ERROR -DLFS2_NO_ASSERT
LIB_SRC_C += \
	lib/littlefs/lfs2.c \
	lib/littlefs/lfs2_util.c
endif

DRIVERS_SRC_C += $(addprefix drivers/,\
	bus/softspi.c \
	dht/dht.c \
	)

ifeq ($(MICROPY_SSL_MBEDTLS),1)
CFLAGS_MOD += -DMBEDTLS_CONFIG_FILE='"mbedtls/mbedtls_config.h"'
INC += -I./mbedtls
INC += -I$(TOP)/lib/mbedtls/include
MBEDTLS_SRC_C = mbedtls/mbedtls_port.c
endif

SRC_O += \
	shared/runtime/gchelper_thumb2.o

OBJ_MP =
OBJ_MP += $(PY_O)
#OBJ_MP += $(PY_CORE_O)
OBJ_MP += $(addprefix $(BUILD)/, $(MPY_SRC_C:.c=.o))
OBJ_MP += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))
OBJ_MP += $(addprefix $(BUILD)/, $(DRIVERS_SRC_C:.c=.o))
OBJ_MP += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))
OBJ_MP += $(addprefix $(BUILD)/, $(SRC_O:.s=.o))
OBJ_MP += $(addprefix $(BUILD)/, $(ASM_SRC:.s=.o))

ifeq ($(MICROPY_SSL_MBEDTLS),1)
OBJ_MP += $(addprefix $(BUILD)/, $(MBEDTLS_SRC_C:.c=.o))
endif

OBJ_WM =
OBJ_WM += $(C_SRC:.c=.o)

OBJ_FILES	= $(OBJ_MP) $(OBJ_WM)
OBJ = $(OBJ_FILES)

# List of sources for qstr extraction
SRC_QSTR += $(MPY_SRC_C) $(EXTMOD_SRC_C) $(LIB_SRC_C) $(DRIVERS_SRC_C)
ifeq ($(MICROPY_SSL_MBEDTLS),1)
SRC_QSTR += $(MBEDTLS_SRC_C)
endif
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

CFLAGS += $(INCLUDES) $(INC) $(CFLAGS_MOD)

CFLAGS += -DMICROPY_HW_CODESIZE=$(CODESIZE)
CFLAGS += -DMICROPY_HW_SECBOOT=$(SECBOOT)
CFLAGS += -DMICROPY_HW_CODEADDRESS=$(CODEADDRESS)

ifeq ($(MICROPY_SSL_MBEDTLS), 1)
CFLAGS += -DMICROPY_PY_SSL=1 -DMICROPY_SSL_MBEDTLS=1
endif
ifeq ($(MICROPY_USE_FATFS), 1)
CFLAGS += -DMICROPY_VFS_FAT=1
endif
ifeq ($(MICROPY_USE_LITTLEFS), 1)
CFLAGS += -DMICROPY_VFS_LFS2=1
endif
ifdef MICROPY_PY_THREAD
CFLAGS += -DMICROPY_PY_THREAD=$(MICROPY_PY_THREAD)
endif

CFLAGS += -D__HEAP_SIZE=0x24000
CFLAGS += -Wno-unused-variable -Wno-unused-function -g
ifeq ($(DEBUG),1)
CFLAGS += -Og
else
CFLAGS += -Os -DNDEBUG
endif

# CFLAGS += -Werror -Wno-error=unused-function -Wno-error=unused-but-set-variable

_ENV_OS=$(subst /,-,$(COMPILER_OS))
ENV_OS=$(shell echo $(_ENV_OS) | tr '[A-Z]' '[a-z]')

MODULES := library

all: $(MODULES) $(GOAL)
	@./$(PACKIMG) $(BUILD) $(SECBOOT)

image: $(MODULES) $(GOAL)
	@./$(PACKIMG) $(BUILD) $(SECBOOT)
	@./$(DOWNLOADIMG) $(BUILD)

flash: $(MODULES) $(GOAL)
	@./$(PACKIMG) $(BUILD) $(SECBOOT)
	@./$(DOWNLOADFLS) $(BUILD)

$(MODULES):
	@for i in ${Src_App_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i || exit $?; done
	@for i in ${Src_Net_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w || exit $?; done
#	@for i in ${Src_Wlan_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w || exit $?; done
	@for i in ${Src_RTOS_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w || exit $?; done
	@for i in ${WM_Demo_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w || exit $?; done
	@for i in ${Platform_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w || exit $?; done
	@echo "finish MODULES"

$(GOAL): $(GOAL).elf

$(GOAL).elf: $(OBJ_FILES) $(LIB_SRC)
	$(CC) -o $(OUTPUT_PATH)/$(GOAL).elf $(OBJ_FILES) $(LDFLAGS) \
		-Wl,--gc-sections -Wl,--defsym=_codesize=$(CODESIZE) \
		-Wl,--defsym=_codeaddress=$(CODEADDRESS) \
		-Wl,--print-memory-usage \
		-Wl,--start-group $(LIB_SRC) -Wl,--end-group \
		-Wl,-Map,$(OUTPUT_PATH)/$(GOAL).map -Wl,--warn-common
	$(ELF) \
		--output-target=binary \
		-S -g -x -X -R .sbss -R .bss -R .reginfo -R .stack \
		$(OUTPUT_PATH)/$(GOAL).elf $(OUTPUT_PATH)/$(GOAL).bin
# 	$(Q)$(SIZE) $(BUILD)/$@


MAKE_PIN_DICT = boards/make-pin-dict.py
BOARD_PINS = $(BOARD_DIR)/pins.csv
GEN_PIN_DICT = pin_board_dict.h

machine/machine_pin.c: $(BUILD)/$(GEN_PIN_DICT)

$(BUILD)/$(GEN_PIN_DICT): $(BOARD_PINS) | $(HEADER_BUILD)
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(MAKE_PIN_DICT) --board $(BOARD_PINS) --table $(BUILD)/$(GEN_PIN_DICT)

include $(TOP)/py/mkrules.mk

clean:
	@for i in ${Src_App_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i clean || exit $?; done
	@for i in ${Src_Net_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w clean || exit $?; done
#	@for i in ${Src_Wlan_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w clean || exit $?; done
	@for i in ${Src_RTOS_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w clean || exit $?; done
	@for i in ${WM_Demo_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w clean || exit $?; done
	@for i in ${Platform_dirs} ; do [ ! -d $$i ] || $(MAKE) -C $$i -w clean || exit $?; done
	$(RM) -rf $(OUTPUT_PATH)
