# Radio_Software

This code is a port of the AX library available here:  https://github.com/richardeoin/ax/blob/master/sw/

The code has been modified to work with a custom shield (hacked together really) using an ONsemi 433 MHz eval board,
and an Adafruit Metro M0 Express board.

The AX5043 is connected via SPI on the 6 pin header.

Packetbuffer_rateconverter uses two back to back RPi/Metro combination boards.  The RPi to Metro connection is serial running at 115kbaud.  The two systems are cross-connected via serial running at 9600 baud.  The second serial port runs in half-duplex mode (only one side transmitting at a time).

The gnuradio flow graph is an FSK demod designed for the current modulation (FSK) in the AX test file.  Basic header detection has been implemented.
This requires an RTL-SDR to operate.  This file was designed using gnuradio 3.9.

Variants have been added that support the Adafruit Metro and Silversat Radio board.  Signals are defined by their schematic net name (or something close) to make code easier to read.
