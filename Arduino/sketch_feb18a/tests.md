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

Note: This packetfinder functions is `packetsize()`

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

In this discussion, tconrad26 indicated that I forgot to change `serial0Buffer`
to `serial1Buffer` on line 89, causing the wrong buffer's size to be checked.
Although this was corrected, many errors remained in KISS.h:

```
[Starting] Verifying sketch 'Arduino/sketch_feb18a/sketch_feb18a.ino'
[Warning] Output path is not specified. Unable to reuse previously compiled files. Build will be slower. See README.
In file included from /home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/sketch_feb18a.ino:18:
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h: In member function 'void KISSPacket::decapsulate()':
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:178:13: error: invalid use of member function 'unsigned int KISSPacket::packetsize()' (did you forget the '()' ?)
  178 |         if (packetsize >= 4)
      |             ^~~~~~~~~~
      |                       ()
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h: At global scope:
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:195:43: error: 'PACKETSIZE' was not declared in this scope
  195 | void kissencapsulate(CircularBuffer<char, PACKETSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
      |                                           ^~~~~~~~~~
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:195:53: error: template argument 2 is invalid
  195 | void kissencapsulate(CircularBuffer<char, PACKETSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
      |                                                     ^
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:195:53: error: template argument 3 is invalid
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h: In function 'void kissencapsulate(int, CircularBuffer<char, 1024>&)':
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:204:24: error: request for member 'isEmpty' in 'inputdata', which is of non-class type 'int'
  204 |     while ((!inputdata.isEmpty()) && (!kisspackets.isFull()))
      |                        ^~~~~~~
/home/isaac/git_repos/Radio_Software/Arduino/sketch_feb18a/KISS.h:207:30: error: request for member 'shift' in 'inputdata', which is of non-class type 'int'
  207 |         databyte = inputdata.shift();
      |                              ^~~~~

Error during build: exit status 1
IntelliSense configuration already up to date. To manually rebuild your IntelliSense configuration run "Ctrl+Alt+I"
[Error] Verifying sketch 'Arduino/sketch_feb18a/sketch_feb18a.ino': Exit with code=1
```

A few quick fixes were implemented, as these functions are not yet in use.

```
diff --git a/Arduino/sketch_feb18a/KISS.h b/Arduino/sketch_feb18a/KISS.h
index 94a8ed9..100621d 100644
--- a/Arduino/sketch_feb18a/KISS.h
+++ b/Arduino/sketch_feb18a/KISS.h
@@ -65,8 +65,7 @@
 // Constants
 const unsigned int BUFFERSIZE{1024}; // bytes
 // const unsigned int RADIO_PACKETSIZE{256};
-
-// const unsigned int PACKETSIZE{BUFFERSIZE};     // Could be the AX5043 FIFO size
+const unsigned int RADIO_BUFFERSIZE{BUFFERSIZE};     // Could be the AX5043 FIFO size
 
 // Classes
 // This hold the data and command byte of an unencoded KISS packet
@@ -175,7 +174,7 @@ public:
         // Cut the first packet out of rawdata
 
         // Copy the packet to the packet buffer packet only if it has two FENDS, a command byte, and at least one byte of data
-        if (packetsize >= 4)
+        if (packetsize() >= 4)
         {
             // Copy data to packet.packet from back to front (to avoid shifting {CircularBuffer data}, for speed)
             for (unsigned int i = 0; i < nextfend; i++)
@@ -192,7 +191,7 @@ public:
 // TODO: (1) Separate to a packet size and encapsulator functions
 //       (2) Change this to first convert inputdata, then pass this to kisspackets
 // Note: tconrad26 keeps the KISS command between the boards
-void kissencapsulate(CircularBuffer<char, PACKETSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
+void kissencapsulate(CircularBuffer<char, RADIO_BUFFERSIZE> inputdata, CircularBuffer<char, BUFFERSIZE> &kisspackets)
 {
     // Declare variables
     char databyte;
```

```
diff --git a/Arduino/sketch_feb18a/sketch_feb18a.ino b/Arduino/sketch_feb18a/sketch_feb18a.ino
index 7ae2521..1eab208 100644
--- a/Arduino/sketch_feb18a/sketch_feb18a.ino
+++ b/Arduino/sketch_feb18a/sketch_feb18a.ino
@@ -72,7 +72,7 @@ void loop()
     // pass the buffer through a packet detector function here
 
     // (testing) Shift the buffer contents after a certain size threshold
-    if (serial0Buffer.serialbuffer.size() > 20) // Leave it 1 bytes for now
+    if (serial0Buffer.serialbuffer.size() > 0) // Leave it 1 bytes for now
     {
         Serial.write(serial0Buffer.serialbuffer.shift());
         // serial0Buffer.rawdata.debug();
@@ -86,7 +86,7 @@ void loop()
     // pass the buffer through a packet detector function here
 
     // (testing) Shift the buffer contents after a certain size threshold
-    if (serial0Buffer.serialbuffer.size() > 0) // Leave it 1 bytes for now
+    if (serial1Buffer.serialbuffer.size() > 0) // Leave it 1 bytes for now
     {
         Serial1.write(serial1Buffer.serialbuffer.shift());
     }
```

These fixes allwed `The quick brown fox` to pass.

```
---- Opened the serial port /dev/ttyACM0 ----
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----
The quick brown fox jumps over the lazy dog.
---- Closed the serial port /dev/ttyACM0 ----
```

```
---- Opened the serial port /dev/ttyACM1 ----
The quick brown fox jumps over the lazy dog.
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----
---- Sent utf8 encoded message: "" ----
---- Closed the serial port /dev/ttyACM1 ----
```

# 2023-08-10: Test of packetsize()

For this test, `serialbuffer.size()` was replaced with `packetsize()`, and
../../RadioTestInterface/SSRadioTests.py was used to send a KISS-encoded test
packet `b'\xc0\x00123456789\xc0'` ("Send Test Data Packet Command" button).
Upon `packetsize`'s first test, no data passed. The second test returned
`serial port error` in the terminal. SSRadioTests was quickly confirmed to be
be able to send the packet by using it with the last working test's code. A
quick fix was made by switching `[0]` in packetsize with `first()`, and
updating Visual Studio Code. This had little to no impact.

# 2023-08-17: Debug code added
To understand how packetsize interprets the buffer, a function which prints a
character as hex, decimal, and binary were added to `serialbuffer.shift()`,
displaying which character was deleted. This can be disabled by commenting out
`#define DEBUG`.

Sending The quick brown fox revealed no data was being shifted out:

Excrept:
```
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----


HEX     DEC     BIN     Char
0       0       0
```

The buffer remained blank throughout monitoring.

Although primarily to declutter the serial monitor, a buffer size check was
added before packetsize. Before packetsize runs, the possibility of a packet is
checked using `serial0Buffer.buffer.size > 3` (two FENDS, a command byte,
and a data byte). This could prevent byte loss before a full packet enters.

Note: In addition, `serialbuffer` was renamed to `buffer` for clarity.

The first four bytes of The quick brown fox were passed, but the remainder did
not.

```
---- Sent utf8 encoded message: "The quick brown fox jumps over the lazy dog." ----
HEX     DEC     BIN     Char
54      84      1010100 T

HEX     DEC     BIN     Char
68      104     1101000 h

HEX     DEC     BIN     Char
65      101     1100101 e

HEX     DEC     BIN     Char
20      32      100000   

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0

HEX     DEC     BIN     Char
0       0       0
…
```

# 2023-08-22: Continued debugging
The buffer size check was moved to within `packetsize`. Initially, this was
placed after data before the first FEND was stripped. The quick brown fox
did not appear in the serial monitor.

Next, this check was copied to enclose the entirety of `packetsize`'s
internal code. This required adding `return -1` before the function's final
closing bracket. This caused T to print repeatedly. Thus, these attempts were
discarded.

# 2023-08-24: Discussion with tconrad26
Two days ago, tconrad26 met isaac-silversat to discuss the issue. tconrad26
followed up with an email with recommended changes. Notably, CircularBuffers do
not clear any deleted bytes until the next overwrite operation. If the `[]`
operator reads beyond the end of the data, it will read the last byte stored at
the given index. In addition, many issues regarding data size (e.g. processing
empty buffers, reading deallocated bytes).

To fix the `shift` problem, a tiny private function 
`zero_unallocated_end_byte`, which overwrites the `[buffer.size()]` byte with a
zero. As of this writing, it has not yet been tested.