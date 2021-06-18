Port of MicroPython to NXP iMX RT 10xx
======================================

Currently supports Teensy 4.0,  Teensy 4.1, i.MX RT1010, i.MX RT1020,
i.MX RT1050, i.MX RT1060 and i.MX RT1064 EVK board.

Features:
  - REPL over USB VCP
  - VSF flash file system
  - machine module with the classes Pin, ADC, RTC, Timer, SoftI2C, SoftSPI, UART, Signal
  - Onewire
  - DS18X20
  - math and cmath
  - uasyncio
  - most MicroPython core classes and methods

TODO:
  - Enable TCM
  - Peripherals (LED, Timers, etc)
