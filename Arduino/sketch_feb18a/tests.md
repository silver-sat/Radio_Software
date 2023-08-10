# Tests Draft
First version: isaac-silversat, 2023-07-31

This draft defines preliminary test precodures and code used to verify the 
radio code using the Adafruit® Metro (https://www.adafruit.com/product/3505).
These tests will likely differ when performed using a radio board
(https://github.com/silver-sat/Radio_Board).

# Data transfer test
This test verifies the code's ability to receive and transfer data between
computers. Two Adafruit Metros are linked by a common UART port; each are
connected to their own USB port on an Ubuntu 22.04 PC. Both ports should be 
monitored; at least one should be able to send data.

To verify any arbitrary data can cross, one USB port should send data to one
Metro's serial buffer. Further processing should remain off until it is ready
to be tested.

# KISS Packet Detection
In a private email sent 14 April 2023, tconrad26 indicated that the 
"packetfinder" function, added to the previous test, should be given test 
packets and "printed out the output to interpret rather than generating a 
pass/fail."

# Packet Encapsulator and Decapsulator Test
In addition to KISS Packet Detection, the KISS decapsulator, then the
encapsulator, are added.

When both functions are in use, the output of the decapsulator could be
mirrored back to the source USB port.

# Miscellaneous: GPIO Test
Setting `#define GPITEST` reads values from all GPIO pins and prints
them to the USB port. `#define GPOTEST` flashes all digital GPIO pins.

# Test Log

## 2023-08-01: Data Transfer test
The test data was "The quick brown fox jumps over the lazy dog."

The first two tests did not use the KISSPacket class. This was to test the
basic data transfer capability.

Upon the first test, no data was received. The problem was that the code was
configured to send data one-way to another serial monitor. This was 
corrected by adding a copy of the transfer code to send data from serial1 to
serial0. This correction allowed data to pass on the second test.

## 2023-08-04: Data Transfer test with KISSPacket class
Since the previous test, the main code was modified to use KISSPacket.rawdata
instead of a simple Circular Buffer. The code was modified to allow data to
return from the satellite Metro to the ground Metro.

Upon the first test of this new code, no data passed. To debug, the code was
modified such that all data transfer code uses the same buffer. Upon this test
("second test"), data did not pass if it was sent using the blue
"Send Ctrl + C" button in Visual Studio Code. However, it did pass when the
paper airplane button was used.

```
---- Opened the serial port /dev/ttyACM0 ----
//
// When the text was sent using the blue button:
---- Sent hex encoded message: "03" ----
//
// When the text was sent using the paper airplane button:
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----
```
Visual Studio Code serial monitor output. Comments are indicated with double
slashes, i.e. C++-style comments.

Thus, today's first test may not have shown any data because `0x03` is not
printable.

The code from the first test was uploaded and tested again:

```
---- Opened the serial port /dev/ttyACM0 ----
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----
```
```
---- Opened the serial port /dev/ttyACM1 ----
```

## 2023-08-10: Discussion opened

A discussion on the missing traffic was opened on 8 August at
https://github.com/silver-sat/Radio_Software/commit/c6936307088ad3da2a59de0e79eaf211fa92aea4.