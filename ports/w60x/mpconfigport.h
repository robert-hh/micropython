// Options to control how MicroPython is built for this port,
// overriding defaults in py/mpconfig.h.
// Board specific definitions
#include "mpconfigboard.h"

#define MICROPY_W600_VERSION    "B1.6"

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

// object representation and NLR handling
#define MICROPY_OBJ_REPR                    (MICROPY_OBJ_REPR_A)
#define MICROPY_NLR_SETJMP                  (1)

// memory allocation policies
#define MICROPY_ALLOC_PATH_MAX              (128)

// emitters
#define MICROPY_PERSISTENT_CODE_LOAD        (1)
#define MICROPY_EMIT_THUMB                  (1)
#define MICROPY_EMIT_INLINE_THUMB           (1)
#define MICROPY_EMIT_THUMB_ARMV7M           (1)

// optimisations
#define MICROPY_OPT_COMPUTED_GOTO           (1)

// Python internal features
#define MICROPY_READER_VFS                  (1)
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ERROR_REPORTING             (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_WARNINGS                    (1)
#define MICROPY_FLOAT_IMPL                  (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_USE_INTERNAL_ERRNO          (1)
#define MICROPY_USE_INTERNAL_PRINTF         (0)
#define MICROPY_SCHEDULER_DEPTH             (8)
#define MICROPY_VFS                         (1)
#define MICROPY_TRACKED_ALLOC               (MICROPY_SSL_MBEDTLS)

// control over Python builtins
#define MICROPY_PY_STR_BYTES_CMP_WARN       (1)
#define MICROPY_PY_BUILTINS_COMPILE         (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT       w600_help_text
#define MICROPY_PY_IO_BUFFEREDWRITER        (1)
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (1)
#define MICROPY_PY_TIME_TIME_TIME_NS (1)
#define MICROPY_PY_TIME_INCLUDEFILE "ports/w60x/modules/modtime.c"
#define MP_NEED_LOG2                        (1)

// extended modules
#define MICROPY_PY_HASHLIB                  (0)  // Use the W60x version
#define MICROPY_PY_HASHLIB_SHA256           (1)
#define MICROPY_PY_OS_INCLUDEFILE           "ports/w60x/modules/modos.c"
#define MICROPY_PY_OS_SYNC                  (1)
#define MICROPY_PY_OS_DUPTERM               (1)
#define MICROPY_PY_OS_DUPTERM_NOTIFY        (1)
#define MICROPY_PY_OS_UNAME                 (1)
#define MICROPY_PY_OS_URANDOM               (1)
#define MICROPY_PY_URANDOM_SEED_INIT_FUNC   (w600_adc_get_allch_4bit_rst())
#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW     mp_pin_make_new
#define MICROPY_PY_MACHINE_PULSE            (1)
#define MICROPY_PY_MACHINE_PWM              (1)
#define MICROPY_PY_MACHINE_PWM_INIT         (1)
#define MICROPY_PY_MACHINE_PWM_DUTY         (1)
#define MICROPY_PY_MACHINE_PWM_DUTY_U16_NS  (1)
#define MICROPY_PY_MACHINE_PWM_INCLUDEFILE "ports/w60x/machine/machine_pwm.c"
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
#ifndef MICROPY_PY_MACHINE_ADC
#define MICROPY_PY_MACHINE_ADC              (0)
#endif
#define MICROPY_PY_MACHINE_ADC_READ         (1)
#define MICROPY_PY_MACHINE_ADC_INCLUDEFILE  "ports/w60x/machine/machine_adc.c"
#ifndef MICROPY_PY_MACHINE_UART
#define MICROPY_PY_MACHINE_UART             (1)
#endif
#define MICROPY_PY_MACHINE_UART_INCLUDEFILE "ports/w60x/machine/machine_uart.c"
#define MICROPY_PY_CRYPTOLIB                (MICROPY_PY_SSL)
#define MICROPY_PY_WEBREPL                  (1)
#define MICROPY_PY_WEBSOCKET                (1)
#define MICROPY_PY_SOCKET_EVENTS            (1)
#define MICROPY_PY_ONEWIRE                  (1)
#define MICROPY_PY_MACHINE_BITSTREAM        (1)
#define FFCONF_H                            "lib/oofatfs/ffconf.h"
#define MICROPY_PY_SYS_PLATFORM             "w600"
#define MICROPY_PLATFORM_VERSION            "SDK G3.04.00"

// fatfs configuration
#define MICROPY_FATFS_ENABLE_LFN            (1)
#define MICROPY_FATFS_RPATH                 (2)
#define MICROPY_FATFS_MAX_SS                (512)
#define MICROPY_FATFS_LFN_CODE_PAGE          437 /* 1=SFN/ANSI 437=LFN/U.S.(OEM) */

#define MICROPY_USE_FROZEN_SCRIPT           (1)

#define MP_STATE_PORT MP_STATE_VM

// type definitions for the specific machine

#define BYTES_PER_WORD (4)
#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((uint32_t)(p) | 1))
#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)
#define MP_SSIZE_MAX (0x7fffffff)

// Note: these "critical nested" macros do not ensure cross-CPU exclusion,
// the only disable interrupts on the current CPU.  To full manage exclusion
// one should use portENTER_CRITICAL/portEXIT_CRITICAL instead.
#include "wm_osal.h"
#define MICROPY_BEGIN_ATOMIC_SECTION() tls_os_set_critical()
#define MICROPY_END_ATOMIC_SECTION(state) tls_os_release_critical(state)

#if MICROPY_PY_SOCKET_EVENTS
#define MICROPY_PY_SOCKET_EVENTS_HANDLER extern void socket_events_handler(void); socket_events_handler();
#else
#define MICROPY_PY_SOCKET_EVENTS_HANDLER
#endif

#if MICROPY_PY_THREAD
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool raise_exc); \
        mp_handle_pending(true); \
        MICROPY_PY_SOCKET_EVENTS_HANDLER \
        MP_THREAD_GIL_EXIT(); \
        tls_os_time_delay(1); \
        MP_THREAD_GIL_ENTER(); \
    } while (0);
#else
#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool raise_exc); \
        MICROPY_PY_SOCKET_EVENTS_HANDLER \
        mp_handle_pending(true); \
        tls_os_time_delay(1); \
    } while (0);
#endif

#define UINT_FMT "%u"
#define INT_FMT "%d"

typedef int32_t mp_int_t; // must be pointer size
typedef uint32_t mp_uint_t; // must be pointer size
typedef long mp_off_t;
