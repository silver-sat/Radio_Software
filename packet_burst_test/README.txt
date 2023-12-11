These two python programs test the sensitivity of the radio by sending a configurable number of packets from the ground to the satellite.

The packets contain a 4 byte sequence number.  They are 207 bytes long.  There are 200 bytes in the payload, 4 bytes for the frame delimiter, 2 for the CRC and a 1 byte destination.  All have to be present for a packet to be received correctly.

The payload data is a random string that is generated each time you run the code.

It is not limited to ASCII (just like the real payload does), so the input and output must be KISS encoded/decoded.

There is also a 200mS inter-packet delay.  This is needed because the Ground Station 'avionics' interface runs at 57600, whereas the radio only transmits at 9600 baud.  So, it's very easy to overrun the radio packet buffer.

