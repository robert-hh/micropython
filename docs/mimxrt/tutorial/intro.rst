.. _mimxrt_intro:

Getting started with MicroPython on the i.MXRT
==============================================

Using MicroPython is a great way to get the most of your i.MXRT board.  And
vice versa, the i.MXRT chip is a great platform for using MicroPython.  This
tutorial will guide you through setting up MicroPython, getting a prompt, using
the hardware peripherals, and controlling some external components.

Let's get started!

Requirements
------------

The first thing you need is a board with an i.MXRT chip.  The MicroPython
software supports the i.MXRT chip itself and any board should work.  The main
characteristic of a board is how the GPIO pins are connected to the outside
world, and whether it includes a built-in USB-serial convertor to make the
UART available to your PC.

Names of pins will be given in this tutorial using the chip names (eg GPIO2)
and it should be straightforward to find which pin this corresponds to on your
particular board.

Powering the board
------------------

If your board has a USB connector on it then most likely it is powered through
this when connected to your PC.  Otherwise you will need to power it directly.
Please refer to the documentation for your board for further details.

Getting the firmware
--------------------

At the moment the MicroPython web site does not offer pre-built packages. So
you have to build the firmware from the sources, as detailed in the README.md
files of MicroPyton. The make procedure for the respecitive board will create
the firmware.bin or firmware.hex file.

Once firmware versions are provided by MicroPyhgton, you can download the
most recent MicroPython firmware .hex or .bin file to load onto your
i.MXRT device. You can download it from the 
`MicroPython downloads page <https://micropython.org/download/all>`_.
From here, you have 2 main choices:

* Stable firmware builds
* Daily firmware builds

If you are just starting with MicroPython, the best bet is to go for the Stable
firmware builds. If you are an advanced, experienced MicroPython i.MXRT user
who would like to follow development closely and help with testing new
features, there are daily builds.

Deploying the firmware
----------------------

Once you have the MicroPython firmware you need to load it onto your i.MXRT device.
The exact procedure for these steps is highly dependent on the particular board
and you will need to refer to its documentation for details.

For Teensy 4.0 and 4.1 you have to use the built-in loader together with the PC
loader provided by PJRC. The built-in loader will be activated by pushing the
button on the board. Then you can uload the firmware with the command:

teensy_loader_cli --mcu=imxrt1062 -v -w firmware.hex

The IMXRT10xx-EVK have a second USB port. Connecting that to your it will register
a disk drive with the name of the board. Just copy the firmware.bin file to this
drive. That will start the flashing procedure. You will know that the flash
was complete, if that drive disappears and reappears.


Serial prompt
-------------

Once you have the firmware on the device you can access the REPL (Python prompt)
over USB.

From here you can now follow the i.MXRT tutorial.

Troubleshooting installation problems
-------------------------------------

If you experience problems during flashing or with running firmware immediately
after it, here are troubleshooting recommendations:

* Be aware of and try to exclude hardware problems.  There are 2 common
  problems: bad power source quality, and worn-out/defective FlashROM.
  Speaking of power source, not just raw amperage is important, but also low
  ripple and noise/EMI in general.  The most reliable and convenient power
  source is a USB port.
