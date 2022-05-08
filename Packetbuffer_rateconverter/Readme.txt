This Arduino sketch works on an Adafruit Metro Express, with one extra serial port (Serial2) defined as a variant.

The full system requires 2 Metros and 2 RPis.  The RPis are connected to the Metro via Serial1 (Pins 0 and 1) running at 115kbaud
The second serial port (Serial2) runs at 9600 baud.  The two systems are cross connected.

The code simulates a half duplex connection between the two systems.

The system was testing using tftp (UDP) and ftp (TCP).  100k files were transferred.  tftp took 149.3 seconds.  ftp took 225.

average transfer rate for tftp was 5.486 kb/sec, and for ftp it was 3.64 kb/sec.
