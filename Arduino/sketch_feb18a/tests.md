# Tests Draft
First version: isaac-silversat, 2023-07-31

This draft defines preliminary test precodures and code used to verify the 
radio code using the Adafruit® Metro (https://www.adafruit.com/product/3505).
These tests will likely differ when performed using a radio board
(https://github.com/silver-sat/Radio_Board).

# Data transfer test
This test verifies the code's ability to receive and transfer data between
computers. Two Adafruit Metros are linked by a common UART port; each are
connected to their own USB port on an Ubuntu 22.04 PC. Both ports should be monitored; at least one should be able to send data.

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
Setting `#define GPITEST` reads values from all GPIO pins and prints them to the USB port. `#define GPOTEST` flashes all digital GPIO pins.