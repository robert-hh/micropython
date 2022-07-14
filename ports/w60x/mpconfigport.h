// Options to control how MicroPython is built for this port,
// overriding defaults in py/mpconfig.h.

#include <stdint.h>
#include <alloca.h>

#define MP_NEED_LOG2                        (1)

// object representation and NLR handling
#define MICROPY_OBJ_REPR                    (MICROPY_OBJ_REPR_A)
#define MICROPY_NLR_SETJMP                  (1)

// memory allocation policies
#define MICROPY_ALLOC_PATH_MAX              (128)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD        (1)

// compiler configuration
#define MICROPY_COMP_MODULE_CONST           (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN    (1)

// optimisations
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_OPT_MPZ_BITWISE             (1)

// Python internal features
#define MICROPY_READER_VFS                  (1)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_ENABLE_FINALISER            (1)
#define MICROPY_STACK_CHECK                 (1)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_KBD_EXCEPTION               (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_REPL_EMACS_KEYS             (1)
#define MICROPY_REPL_AUTO_INDENT            (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ENABLE_SOURCE_LINE          (1)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_WARNINGS                    (1)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_PY_BUILTINS_COMPLEX         (1)
#define MICROPY_CPYTHON_COMPAT              (1)
#define MICROPY_STREAMS_NON_BLOCK           (1)
#define MICROPY_STREAMS_POSIX_API           (1)
#define MICROPY_MODULE_BUILTIN_INIT         (1)
#define MICROPY_MODULE_WEAK_LINKS           (1)
#define MICROPY_MODULE_FROZEN_STR           (0)
#define MICROPY_MODULE_FROZEN_MPY           (1)
#define MICROPY_QSTR_EXTRA_POOL             mp_qstr_frozen_const_pool
#define MICROPY_CAN_OVERRIDE_BUILTINS       (1)
#define MICROPY_MODULE_BUILTIN_INIT         (1)
#define MICROPY_USE_INTERNAL_ERRNO          (1)
#define MICROPY_USE_INTERNAL_PRINTF         (0)
#define MICROPY_ENABLE_SCHEDULER            (1)
#define MICROPY_SCHEDULER_DEPTH             (8)
#define MICROPY_VFS                         (1)
#ifdef W60X_USE_FATFS
#define MICROPY_VFS_FAT                     (1)
#endif
#ifdef W60X_USE_LITTLEFS
#define MICROPY_VFS_LFS2                    (1)
#endif

// control over Python builtins
#define MICROPY_PY_FUNCTION_ATTRS           (1)
#define MICROPY_PY_DESCRIPTORS              (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS      (1)
#define MICROPY_PY_STR_BYTES_CMP_WARN       (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE     (1)
#define MICROPY_PY_BUILTINS_STR_CENTER      (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION   (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES  (1)
#define MICROPY_PY_BUILTINS_BYTEARRAY       (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW      (1)
#define MICROPY_PY_BUILTINS_SET             (1)
#define MICROPY_PY_BUILTINS_SLICE           (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_FROZENSET       (1)
#define MICROPY_PY_BUILTINS_PROPERTY        (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS     (1)
#define MICROPY_PY_BUILTINS_ROUND_INT       (1)
#define MICROPY_PY_BUILTINS_TIMEOUTERROR    (1)
#define MICROPY_PY_ALL_SPECIAL_METHODS      (1)
#define MICROPY_PY_BUILTINS_COMPILE         (1)
#define MICROPY_PY_BUILTINS_ENUMERATE       (1)
#define MICROPY_PY_BUILTINS_EXECFILE        (1)
#define MICROPY_PY_BUILTINS_FILTER          (1)
#define MICROPY_PY_BUILTINS_REVERSED        (1)
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED  (1)
#define MICROPY_PY_BUILTINS_INPUT           (1)
#define MICROPY_PY_BUILTINS_MIN_MAX         (1)
#define MICROPY_PY_BUILTINS_POW3            (1)
#define MICROPY_PY_BUILTINS_HELP            (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       w600_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES    (1)
#define MICROPY_PY___FILE__                 (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO     (1)
#define MICROPY_PY_ARRAY                    (1)
#define MICROPY_PY_ARRAY_SLICE_ASSIGN       (1)
#define MICROPY_PY_ATTRTUPLE                (1)
#define MICROPY_PY_COLLECTIONS              (1)
#define MICROPY_PY_COLLECTIONS_DEQUE        (1)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT  (1)
#define MICROPY_PY_MATH                     (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS   (1)
#define MICROPY_PY_CMATH                    (1)
#define MICROPY_PY_GC                       (1)
#define MICROPY_PY_IO                       (1)
#define MICROPY_PY_IO_IOBASE                (1)
#define MICROPY_PY_IO_FILEIO                (1)
#define MICROPY_PY_IO_BYTESIO               (1)
#define MICROPY_PY_IO_BUFFEREDWRITER        (1)
#define MICROPY_PY_STRUCT                   (1)
#define MICROPY_PY_SYS                      (1)
#define MICROPY_PY_SYS_MAXSIZE              (1)
#define MICROPY_PY_SYS_MODULES              (1)
#define MICROPY_PY_SYS_EXIT                 (1)
#define MICROPY_PY_SYS_STDFILES             (1)
#define MICROPY_PY_SYS_STDIO_BUFFER         (1)
#define MICROPY_PY_UERRNO                   (1)
#define MICROPY_PY_USELECT                  (1)
#define MICROPY_PY_UTIME_MP_HAL             (1)
#define MICROPY_PY_THREAD                   (0)  // 1
#define MICROPY_PY_THREAD_GIL               (0)  // 1
#define MICROPY_PY_THREAD_GIL_VM_DIVISOR    (32)

// extended modules
#ifndef MICROPY_PY_UASYNCIO
#define MICROPY_PY_UASYNCIO         (1)
#endif
#define MICROPY_PY_UCTYPES                  (1)
#define MICROPY_PY_UZLIB                    (1)
#define MICROPY_PY_UJSON                    (1)
#define MICROPY_PY_URE                      (1)
#define MICROPY_PY_URE_SUB                  (1)
#define MICROPY_PY_UHEAPQ                   (1)
#define MICROPY_PY_UTIMEQ                   (1)
#define MICROPY_PY_UHASHLIB                 (0)
#define MICROPY_PY_UHASHLIB_SHA1            (0)
#define MICROPY_PY_UHASHLIB_SHA256          (1)
#define MICROPY_PY_UBINASCII                (1)
#define MICROPY_PY_UBINASCII_CRC32          (1)
#define MICROPY_PY_URANDOM                  (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS      (1)
#define MICROPY_PY_UOS                      (1)
#define MICROPY_PY_UOS_INCLUDEFILE          "ports/w60x/modules/moduos.c"
#define MICROPY_PY_OS_DUPTERM               (1)
#define MICROPY_PY_UOS_DUPTERM_NOTIFY       (1)
#define MICROPY_PY_UOS_UNAME                (1)
#define MICROPY_PY_UOS_URANDOM              (1)
#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW     mp_pin_make_new
#define MICROPY_PY_MACHINE_PULSE            (1)
#define MICROPY_PY_MACHINE_I2C              (1)
#define MICROPY_PY_MACHINE_SOFTI2C          (1)
#define MICROPY_PY_MACHINE_I2C_MAKE_NEW     machine_hard_i2c_make_new
#define MICROPY_PY_MACHINE_SPI              (1)
#define MICROPY_PY_MACHINE_SOFTSPI          (1)
#define MICROPY_PY_MACHINE_SPI_MSB          (0)
#define MICROPY_PY_MACHINE_SPI_LSB          (1)
#define MICROPY_PY_MACHINE_SPI_MAKE_NEW     machine_hard_spi_make_new
#define MICROPY_HW_SOFTSPI_MIN_DELAY        (0)
#define MICROPY_HW_SOFTSPI_MAX_BAUDRATE     (mp_hal_get_cpu_freq() / 200) // roughly
#define MICROPY_PY_UCRYPTOLIB               (MICROPY_PY_USSL)
#define MICROPY_PY_WEBREPL                  (0)
#define MICROPY_PY_UWEBSOCKET               (1)
#define MICROPY_PY_FRAMEBUF                 (1)
#define MICROPY_PY_ONEWIRE                  (1)
#define MICROPY_PY_MACHINE_BITSTREAM        (1)
#define FFCONF_H                            "lib/oofatfs/ffconf.h"

// fatfs configuration
#define MICROPY_FATFS_ENABLE_LFN            (1)
#define MICROPY_FATFS_RPATH                 (2)
#define MICROPY_FATFS_MAX_SS                (512)
#define MICROPY_FATFS_LFN_CODE_PAGE          437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */
#if MICROPY_VFS_FAT
#define mp_type_fileio                      mp_type_vfs_fat_fileio
#define mp_type_textio                      mp_type_vfs_fat_textio
#endif
#if MICROPY_VFS_LFS2
#define mp_type_fileio                      mp_type_vfs_lfs2_fileio
#define mp_type_textio                      mp_type_vfs_lfs2_textio
#endif

#define MICROPY_USE_INTERVAL_FLS_FS         (1)
#define MICROPY_USE_FROZEN_SCRIPT           (1)

#define MP_STATE_PORT MP_STATE_VM

// type definitions for the specific machine

#define BYTES_PER_WORD (4)
#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p)))
#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)
#define MP_SSIZE_MAX (0x7fffffff)

// Note: these "critical nested" macros do not ensure cross-CPU exclusion,
// the only disable interrupts on the current CPU.  To full manage exclusion
// one should use portENTER_CRITICAL/portEXIT_CRITICAL instead.
#include "wm_osal.h"
#define MICROPY_BEGIN_ATOMIC_SECTION() tls_os_set_critical()
#define MICROPY_END_ATOMIC_SECTION(state) tls_os_release_critical(state)

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool raise_exc); \
        mp_handle_pending(true); \
        MP_THREAD_GIL_EXIT(); \
        tls_os_time_delay(1); \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool raise_exc); \
        mp_handle_pending(true); \
        __asm volatile ("wfi"); \
    } while (0);
#endif

#define UINT_FMT "%u"
#define INT_FMT "%d"

typedef int32_t mp_int_t; // must be pointer size
typedef uint32_t mp_uint_t; // must be pointer size
typedef long mp_off_t;
// ssize_t, off_t as required by POSIX-signatured functions in stream.h
#include <sys/types.h>

u32 w600_adc_get_allch_4bit_rst(void);
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC   (w600_adc_get_allch_4bit_rst())

// board specifics
#define MICROPY_W600_VERSION    "B1.6"

#define MICROPY_HW_BOARD_NAME "WinnerMicro module"
#define MICROPY_HW_MCU_NAME "W600"
#define MICROPY_PY_SYS_PLATFORM "w600"

