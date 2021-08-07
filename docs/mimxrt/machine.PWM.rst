.. currentmodule:: machine
.. _mimxrt_machine.PWM:

class PWM -- pulse width modulation for i.MXRT MCUs
===================================================

This class provides pulse width modulation output.

Example usage::

    # Samples for Teensy
    #

    from machine import Pin, PWM

    pwm2 = PWM(Pin(2))      # create PWM object from a pin
    pwm2.freq()             # get current frequency
    pwm2.freq(1000)         # set frequency
    pwm2.duty_u16()         # get current duty cycle, range 0-65535
    pwm2.duty_u16(200)      # set duty cycle, range 0-65535
    pwm2.deinit()           # turn off PWM on the pin
    # create a complementary signal pair on Pin 2 and 3
    pwm2 = PWM((2, 3), freq=2000, duty_ns=20000)

    # Create a group of four synchronized signals.
    # Start with Pin(4) at submodule 0, which creates the sync pulse.
    pwm4 = PWM(Pin(4), freq=1000, align=PWM.HEAD)
    # Pins 5, 6, and 9 are pins at the same module
    pwm5 = PWM(Pin(5), freq=1000, duty_u16=10000, align=PWM.HEAD, sync=True)
    pwm6 = PWM(Pin(6), freq=1000, duty_u16=20000, align=PWM.HEAD, sync=True)
    pwm9 = PWM(Pin(9), freq=1000, duty_u16=30000, align=PWM.HEAD, sync=True)

    pwm3                    # show the PWM objects properties

Constructors
------------

.. class:: PWM(dest, freq, \*, duty_u16, duty_ns, keyword_arguments)
  :noindex:

    Construct and return a new PWM object using the following parameters:

      - *dest* is the entity on which the PWM is output, which is usually a
        :ref:`machine.Pin <machine.Pin>` object, but a port may allow other values,
        like integers or strings, which designate a Pin in the machine.PIN class.
        *dest* is either a single object or a two element object tuple.
        If the object tuple is specified, the two pins act in complementary
        mode. These two pins must be the A/B channels of the same submodule.
      - *freq* should be an integer which sets the frequency in Hz for the
        PWM cycle. The valid frequency range is 5 Hz - > 1 MHz.
      - *duty_u16* sets the duty cycle as a ratio ``duty_u16 / 65536``.
        The duty cycle of a X channel can only be changed, if the A and B channel
        of the respective submodule is not used. Otherwise the duty_16 value of the
        X channel is 32768 (50%).
      - *duty_ns* sets the pulse width in nanoseconds. The limitation for X channels
        apply as well.


    PWM objects are either provided by a FLEXPWM module or a QTMR module.
    The i.MXRT devices have either two or four FLEXPWM and QTMR modules.
    Each FLEXPWM module has four submodules with three channels, each,
    called A, B and X. Each QTMR module has four channels.
    Each FLEXPWM module or QTMR channel may be set to different parameters.
    Not every channel is routed to a board pin. Details are listed below.

    Setting *freq* affects the three channels of the same FLEXPWM submodule.
    Only one of *duty_u16* and *duty_ns* should be specified at a time.

    Keyword arguments:

      - *center*\=value. An integer sets the center of the pulse within the pulse period.
        The range is 0-65535. The resulting pulse will last from center - duty_u16/2 to
        center + duty_u16/2.
      - *align*\=value. Predefine values are PWM.MIDDLE (0), PWM.BEGIN (1) and PWM.END (2).
        These are shortcuts for the pulse center setting, causing the pulse either at
        the center of the frame, the leading edge at the begin or the trailing edge at the
        end of a pulse period.
      - *invert*\=True|False channel_mask. Setting a bit in the mask inverts the respective channel.
        Bit 0 inverts the first specified channel, bit 2 the second. The default is 0.
      - *sync*\=True|False. If a channel of a module's submodule 0 is already active, other
        submodules of the same module can be forced to be synchronous to submodule 0. Their
        pulse period start then at at same clock cycle. The default is False.
      - *start*\=True|False. Start the PWM clock immediately. If set to False, you can define
        a group of channels first and then start them using the PWM.start() method. The
        default is True.
      - *xor*\=0|1|2. If set to 1 or 2, the channel will output the XORed signal from channels
        A or B. If set to 1 on channel A or B, both A and B will show the same signal. If set
        to 2, A and B will show alternating signals. For details and an illustration, please
        refer to the MCU's reference manual, chapter "Double Switching PWMs".
      - *deadtime*\=time_ns. This setting affects complementary channels and defines a deadtime
        between an edge of a first channel and the edge of the next channel, in which both
        channels are set to low. That allows connected H-bridges to switch off one side
        of a push-pull driver before switch on the other side.

    *freq*, *duty_u16* and *duty_ns* may be given as keyword arguments as well. Only the
    *freq* argument is mandatory.


Methods
-------

.. method:: PWM.init(freq, \*, duty_u16, duty_ns, keyword_arguments)
  :noindex:

   Modify settings for the PWM object.  See the above constructor for details
   about the parameters.

.. method:: PWM.deinit([mask])
  :noindex:

   Stop the PWM of the respective FLEXPWM submodule or QTMR channel. If a mask is specified,
   the bits of it designate the respective submodules or channels.

.. method:: PWM.freq([value])
  :noindex:

   Get or set the current frequency of the PWM output.

   With no arguments the frequency in Hz is returned.

   With a single *value* argument the frequency is set to that value in Hz.  The
   method may raise a ``ValueError`` if the frequency is outside the valid range.

.. method:: PWM.duty_u16([value])
  :noindex:

   Get or set the current duty cycle of the PWM output, as an unsigned 16-bit
   value in the range 0 to 65535 inclusive.

   With no arguments the duty cycle is returned.

   With a single *value* argument the duty cycle is set to that value, measured
   as the ratio ``value / 65536``.

.. method:: PWM.duty_ns([value])
  :noindex:

   Get or set the current pulse width of the PWM output, as a value in nanoseconds.

   With no arguments the pulse width in nanoseconds is returned.

   With a single *value* argument the pulse width is set to that value.

.. method:: PWM.center([value])

   Get or set the current pulse center of the PWM output, as an unsigned 16-bit
   value in the range 0 to 65535 inclusive.

   With no arguments the center position is returned.

   With a single *value* argument the center position is set to that value.

.. method:: PWM.start([mask])

   Start the PWM of the respective FLEXPWM submodule or QTMR channel. If a mask is specified,
   the bits of it designate the respective submodules or channels.

.. method:: PWM.stop([mask])

   This is an alias to PWM.deinit.

Pin Assignment
--------------

Each FLEX submodule or QTMR module may run at different frequencies. The frequency range
is 5 Hz to >1 MHz, with increasing error for frequency an duty cycle at higher frequencies.
Pins are specified in the same way as for the Pin class.

The following table shows the assignment of the board Pins to PWM units:

===========   ======    ======    ==============
Pin/ MIMXRT   1010      1020      1050/1060/1064
===========   ======    ======    ==============
D0            -         F1/1/B    -
D1            -         F1/1/A    -
D2            F1/3/B    -         F1/3/B
D3            F1/3/A    F2/3/B    F4/0/A
D4            F1/3/A    Q2/1      F2/3/A
D5            F1/0/B    F2/3/A    F1/3/A
D6            -         F2/0/A    Q3/2
D7            -         F1/0/A    Q3/3
D8            F1/0/A    F1/0/B    F1/1/X
D9            F1/1/B    F2/0/B    F1/0/X
D10           F1/3/B    F2/2/B    F1/0/B (*)
D11           F1/2/A    F2/1/A    F1/1/A (*)
D12           F1/2/B    F2/1/B    F1/1/B (*)
D13           F1/3/A    F2/2/A    F1/0/A (*)
D14           F1/0/B    -         F2/3/B
D15           F1/0/A    -         F2/3/A
A0            -         F1/2/A    -
A1            F1/3/X    F1/2/B    -
A2            F1/2/X    F1/3/A    -
A3            -         F1/3/B    -
A4            -         -         Q3/1
A5            -         -         Q3/0
===========   ======    ======    ==============

Pins denoted with (*) are by default not wired at the board.


====   ==========  ====   ==========
Pin    Teensy 4.0  Pin    Teensy 4.1
====   ==========  ====   ==========
0      F1/1/X      0      F1/1/X
1      F1/0/X      1      F1/0/X
2      F4/2/A      2      F4/2/A
3      F4/2/B      3      F4/2/B
4      F2/0/A      4      F2/0/A
5      F2/1/A      5      F2/1/A
6      F2/2/A      6      F2/2/A
7      F1/3/B      7      F1/3/B
8      F1/3/A      8      F1/3/A
9      F2/2/B      9      F2/2/B
10     Q1/0        10     Q1/0
11     Q1/2        11     Q1/2
12     Q1/1        12     Q1/1
13     Q2/0        13     Q2/0
14     Q3/2        14     Q3/2
15     Q3/3        15     Q3/3
18     Q3/1        18     Q3/1
19     Q3/0        19     Q3/0
22     F4/0/A      22     F4/0/A
23     F4/1/A      23     F4/1/A
24     F1/2/X      24     F1/2/X
25     F1/3/X      25     F1/3/X
28     F3/1/B      28     F3/1/B
29     F3/1/A      29     F3/1/A
33     F2/0/B      33     F2/0/B
-      -           36     F2/3/A
-      -           37     F2/3/B
DAT1   F1/1/B      42     F1/1/B
DAT0   F1/1/A      43     F1/1/A
CLK    F1/0/B      44     F1/0/B
CMD    F1/0/A      45     F1/0/A
DAT2   F1/2/A      46     F1/2/A
DAT3   F1/2/B      47     F1/2/B
-      -           48     F1/0/B
-      -           49     F1/2/A
-      -           50     F1/2/B
-      -           51     F3/3/B
-      -           52     F1/1/B
-      -           53     F1/1/A
-      -           54     F3/0/A
====   ==========  ====   ==========

Legend:

* Qm/n:    QTMR module m, channel n
* Fm/n/l:  FLEXPWM module m, submodule n, channel l. The X-Channels duty cycle is not
  independent from the A and B channels of the same submodule. The pulse at an X channel
  is always aligned to the period start.

Pins without a PWM signal are not listed. A signal may be available at more than one Pin.
PWM may also be activated at CPU pins, which are not available at the board connectors.
Such a Pin may be used as synchronisation master.