# This script sends out a burst of packets of a fixed length

# The packet content


import serial
from random import randbytes
import struct
from time import sleep

packet_start = b'\xC0\x00'
packet_finish = b'\xC0'
test_length = 1000
debug = True
inter_packet_delay = 0.2  # 200 mS between packets
# packets are 200 (might vary) bytes long, plus one destination byte, 4 bytes for frame delimiter, and 9 0xAA's
# and 2 CRC bytes
# so figure 207 bytes at 9600 baud or about 180 mS.  The interface is running at 57600, so you don't want to overrun
# the radio.  So if it spits out one every 200 mS, it should be okay...could look at this on a scope to get it finer
# this begs the questions, is it possible for the RPi to overrun the UART when its running at 19200?


def kissenc(bytesequence, verbose=False):
    encbytes = b''
    for byte in bytesequence:
        verbose and print(byte.to_bytes(1, 'big'))
        if byte.to_bytes(1, 'big') == b'\xC0':
            encbytes += b'\xDB\xDC'
        elif byte.to_bytes(1, 'big') == b'\xDB':
            encbytes += b'\xDB\xDD'
        else:
            encbytes += byte.to_bytes(1, 'big')
    verbose and print(encbytes)
    return encbytes


packet_payload = randbytes(196)  # allow 4 bytes for sequence number (integer)
debug and print(packet_payload)
debug and print(len(packet_payload))
kiss_packet_payload = kissenc(packet_payload)

with serial.Serial('COM15', 57600) as ser:
    for sequence_number in range(test_length):
        sequence_number_bytes = struct.pack(">I", sequence_number)
        packet = b''.join([packet_start, sequence_number_bytes])
        packet = b''.join([packet, kiss_packet_payload])
        packet = b''.join([packet, packet_finish])
        debug and print(packet)
        ser.write(packet)
        sleep(inter_packet_delay)

