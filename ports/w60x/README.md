MicroPython port to the W60X
=============================

W60X is an embedded Wi-Fi SoC chip which is complying with IEEE802.11b/g/n international
standard and which supports multi interface, multi protocol.
It can be easily applied to smart appliances, smart home, health care, smart toy, wireless audio & video,
industrial and other IoT fields.
This SoC integrates Cortex-M3 CPU, Flash, RF Transceiver, CMOS PA, BaseBand.
It applies multi interfaces such as SPI, UART, GPIO, I2C, PWM, I2S, 7816.
It applies multi encryption and decryption protocols such as PRNG/SHA1/MD5/RC4/DES/3DES/AES/CRC/RSA.

This is an experimental port of MicroPython to the WinnerMicro W60X microcontroller.  

Supported features
------------------------------------

- REPL (Python prompt) over UART0.
- 8k stack for the MicroPython task and 100k Python heap.
- Most of MicroPython's features are enabled: Unicode, arbitrary-precision integers,
  single-precision floats (30bit), frozen bytecode, native emitters (native, viper and arm_thumb),
  framebuffer, asyncio, as well as many of the internal modules.
- The machine module with mem8..mem32, GPIO, UART, SPI, I2C, PWM, WDT, ADC, RTC and Timer.
- The network module with WLAN (WiFi) support (including OneShot).
- Support of SSL using hardware encryption and decryption.
- Internal LFS2 filesystem using the flash (>=300 KB available, >= 1.3 MB on 2MB flash devices).
- Built-in FTP server for transfer of script files.

Setting up the cross toolchain and WM_SDK
-----------------------------------------

Supports direct compilation in Linux system and compilation in Cygwin environment in Windows system.

There are two main things to do here:
- Download the cross toolchain and add it to the environment variable
- Download WM_SDK and add to environment variables

The cross toolchain used is arm-none-eabi-gcc version where the download address is
[GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

You will need to update your `PATH` environment variable to include the cross toolchain. For example, you can issue the following commands on (at least) Linux:

    $ export PATH=$PATH:/opt/tools/arm-none-eabi-gcc/bin

You can put this command in your `.profile` or `.bash_login` (or `.bashrc` if using Github code-spaces).

64 bit versions of the GCC cross compiler as of 10.3, 11.2, and 12.2 have been verified to work. Building with gcc-arm-none-eabi version 13.2 fails.

The initial version of the WM_SDK is located at [W60X_SDK](http://www.winnermicro.com/en/html/1/156/158/497.html), under the Software Data tab. WM_SDK must be G3.01 and newer versions (G3.04 is latest as of end of 2022).
**Since then a few changes have been made to the SDK. The updated version is at https://github.com/robert-hh/WM_SDK_W60X.**

You will need to update your `PATH` environment variable to include the path of WM_SDK. For example, you can issue the following commands on (at least) Linux:

    $ export WMSDK_PATH=/home/username/WM_SDK

You can put this command in your `.profile` or `.bash_login` (or `.bashrc` if using Github code-spaces).

You also need to modify the build configuration file in WM_SDK, located at: `WM_SDK/Include/wm_config.h`

You can crop the component by modifying the macro switch, For example, 

    #define TLS_CONFIG_HOSTIF CFG_OFF

The recommended components that can be turned off are:

    #define TLS_CONFIG_HOSTIF      CFG_OFF
    #define TLS_CONFIG_RMMS        CFG_OFF
    #define TLS_CONFIG_HTTP_CLIENT CFG_OFF
    #define TLS_CONFIG_NTP         CFG_OFF

Building the firmware
---------------------

Clone the MicroPython and WM_SDK repositories and build MicroPython for a generic board:
```bash
git clone https://github.com/robert-hh/WM_SDK_W60X
export WMSDK_PATH=/your-path-to-the-current-directory/WM_SDK_W60X
git clone -b w60x https://github.com/robert-hh/micropython.git
cd micropython/mpy-cross
make
cd ../ports/w60x
make submodules
make BOARD=GENERIC
```
This will produce binary firmware images in the `build-GENERIC` subdirectory.
There are several options that can be modified in the Makefile.
They are described in the separate file: `Makefile_build_options.txt`.

Instead of BOARD=GENERIC another board may be selected.
Currently available selections are:

- GENERIC
- THINGSTURN_TB01
- W600_EVB_V2
- WAVGAT_AIR602
- WEMOS_W600
- WIS600

Required changes of the SDK
---------------------------

The option

#define INCLUDE_xTimerGetTimerDaemonTaskHandle  1

has to be set in FreeRTOS.h. Otherwise the firmware build fails.

Site specific definitions
-------------------------

If the build environments requires changes to the header files, these
can be placed into a file mpconfigsite.h, which will be included
at the top of mpconfigboard.h if it exists.

Makefile build options
----------------------

Make is called in the form:

make option1=value1 option2=value2 .... target

Options:

BOARD ?= GENERIC
    
    Specifies the board for which the binary is built. The only
    difference will be the Pin.board set of names.

SECBOOT ?= 1

    Controls the use of the secondary bootloader by the firmware and
    the bootloader. Without the secondary bootloader, 56k more space
    is available for the flash file system.

CODESIZE ?= 0xb0000

    The amount of flash reserved for the MicroPython code. Lowering
    it increases the size of the flash file system, and the opposite.

MICROPY_USE_FATFS ?= 0

    Whether FAT file system support is built-in. Only needed if SD cards
    are to be used.

MICROPY_PY_THREAD ?= 0

    Control threading support of the firmware. Threading needs more
    RAM & Flash.

MICROPY_PY_SSL ?= 1

    Control of SSL support using MBEDTLS.

FROZEN_MANIFEST ?= manifest.py

    Specifies the name of the manifest file.

MACHINE_HSPI ?= 0

    Controls whether the HSPI mode at SPI(1) is available. This HSPI
    works as slave device only, and it's use is not documented.
    So it's disabled by default.
    When both HSPI and DMA are not used for SPI, the MicroPython heap
    space is increased by 16 kByte.

All options can be set in the command line or by changing Makefile.

Flashing the Firmware
-----------------------

To upload the firmware to the target board, please use the command 
```bash
make flash
```
Some boards like the Wemos W600 require pushing reset at the start of the upload while the
upload tool waits for synchronisation with the target board.
Connecting pin PA0 to GND while pushing reset will ensure that the bootloader of the board
will start and go into synchronization mode to allow the upload.
Once the new Micropython firmware is on the board this grounding of PA0 is no longer necessary.
Then you also may execute `machine.bootloader` from within Micropython to start the bootloader.

Reference documents
-----------------------
Visit [WinnerMicro](http://www.winnermicro.com/en/html/1/156/158/497.html) for more documentation.

