# Radio_Software

This code is a port of the AX library available here:  https://github.com/richardeoin/ax/blob/master/sw/

The code has been modified to work with a custom shield (hacked together really) using an ONsemi 433 MHz eval board,
and an Adafruit Metro M0 Express board.

The AX5043 is connected via SPI on the 6 pin header.

The gnuradio flow graph is an FSK demod designed for the current modulation (FSK) in the AX test file.  Basic header detection has been implemented.
This requires an RTL-SDR to operate.  This file was designed using gnuradio 3.9.
