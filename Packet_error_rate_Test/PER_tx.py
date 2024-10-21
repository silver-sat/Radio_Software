# Packet error rate (PER) test
# Modified from packet_receive.py and SSRadioTests.py by isaac-silversat

import serial
import struct
from random import randbytes
from time import sleep

sequence_number = 0

debug = False
info = False # Print packet numbers, one-by-one

def kissenc(bytesequence):
    encbytes = b''
    for byte in bytesequence:
        debug and print(byte.to_bytes(1, 'big'))
        if byte.to_bytes(1, 'big') == b'\xC0':
            encbytes += b'\xDB\xDC'
        elif byte.to_bytes(1, 'big') == b'\xDB':
            encbytes += b'\xDB\xDD'
        else:
            encbytes += byte.to_bytes(1, 'big')
    debug and print(encbytes)
    return encbytes


def main():
    tx_serial_port = serial.Serial('/dev/ttyACM0', 57600) # Port to send on. Leave it at /dev/ttyACM0 for now
    quantity = 1024 # Leave it 1024 for now.

    packet_start = b'\xC0\x00'
    packet_finish = b'\xC0'
    inter_packet_delay = 0.2  # 200 mS between packets
    # packets are 200 (might vary) bytes long, plus one destination byte, 4 bytes for frame delimiter, and 9 0xAA's
    # and 2 CRC bytes
    # so figure 207 bytes at 9600 baud or about 180 mS.  The interface is running at 57600, so you don't want to overrun
    # the radio.  So if it spits out one every 200 mS, it should be okay...could look at this on a scope to get it finer
    # this begs the questions, is it possible for the RPi to overrun the UART when its running at 19200?
    packet_payload = b".;\xc4Uz\xcfy\xb2G\xf4\xd2\xca?\xd2G\xb5\x8b\xa9\xd5\xee\xefC\x1d\x04#\x1b\xb5+\xc2\x1d\x8c\xaet\x86}\xfc\x10E\x9d\xbc\xcf\x9e\xfd\xe1\xa5V\x06\xca\xd9\xe2\x98^\xe9\xfc\xb9\xb6\xc8w\xcc=\x1a\xb0\xb1e\x81>#y0X\xed\xb3\xbc\xdd\xaa\xc9\xa8\xd5u\xce=\x920\x90DQ5\xab\x98|wd\xef\xac\rz\xa4\xec\xdd7R\x90\x14\xec.s<\x96\xd4\xa1E\xd2x3\xf7\xd2\x12/\xc0\xa8\x8a\x88\xab\xd1l\x84E\xa0\x9c\xe2\xbf\xd4\x19\xb0-\xa0Q\x05\xd0wA6\x04\xe2\x06D\x85\x98\x1a(\x9f]\xce\x07\xab]5'\xca5\x14\x0eE\x0f\xdc\xc2\r\x12\x7f\x13\xa2+\xa8\xe1_\x9a$A\xe5c\xfdT\xec<\x93p\xe8\xf9\xfb\x89\xcdzK\xce\xb6D"  # allow 4 bytes for sequence number (integer)
    # debug and print(packet_payload)
    # debug and print(len(packet_payload))
    kiss_packet_payload = kissenc(packet_payload)
    for seq_num in range(quantity):
        sequence_number_bytes = struct.pack(">I", seq_num)
        txpacket = b''.join([packet_start, kissenc(sequence_number_bytes)])
        txpacket = b''.join([txpacket, kiss_packet_payload])
        txpacket = b''.join([txpacket, packet_finish])
        debug and print(txpacket)
        tx_serial_port.write(txpacket)

        sleep(inter_packet_delay)

main()