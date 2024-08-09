# Packet error rate (PER) test
# Modified from packet_receive.py and SSRadioTests.py by isaac-silversat

import serial
import struct
from random import randbytes
from time import sleep

# sequence_number = 0

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
    txpacket_length = 200 # See comment in PER_tx.py on length
    rx_serial_port = serial.Serial('/dev/ttyACM0', 57600) # Port to send on. Leave it at /dev/ttyACM0 for testing purposes
    quantity = 1024 # Leave it 1024 for now.
    packet_count = 0

    # Dropped packets
    dropped_packets = 0
    malformed_packets = 0

    try:
        while packet_count < quantity:
            # Receive the packet just sent, through another serial port
            with rx_serial_port as ser:
                debug and print(ser.name)
                rxpacket = []
                while (len(rxpacket) > 0) & (len(rxpacket) <= txpacket_length):
                    x = ser.read(1)  # looking for C0
                    debug and print(x)
                    if x == b'\xc0':  # start of a packet detected
                        dest = ser.read(1)  # read the destination (temp)
                        debug and print("dest = ", dest)
                        rxpacket = bytearray(b'')
                        read_byte = None
                        loop_iterations = 0
                        while read_byte != b'\xc0':  # keep reading bytes until next 0xc0 encountered
                            read_byte = ser.read(1)  # need to kiss decode, do it on the fly
                            if read_byte == b'\xDB':
                                read_next_byte = ser.read(1)
                                if read_next_byte == b'\xDC':
                                    rxpacket.extend(b'\xC0')
                                elif read_next_byte == b'\xDD':
                                    rxpacket.extend(b'\xDB')
                            elif read_byte == b'\xc0':
                                pass
                            else:
                                rxpacket.extend(read_byte)
                            
                            if (loop_iterations > txpacket_length) | (len(rxpacket) > txpacket_length):   # If more bytes were received than expected, drop the packet
                                dropped_packets = dropped_packets + 1
                                pass
                            else: # check if the received packet matches the sent packet
                                if rxpacket != b".;\xc4Uz\xcfy\xb2G\xf4\xd2\xca?\xd2G\xb5\x8b\xa9\xd5\xee\xefC\x1d\x04#\x1b\xb5+\xc2\x1d\x8c\xaet\x86}\xfc\x10E\x9d\xbc\xcf\x9e\xfd\xe1\xa5V\x06\xca\xd9\xe2\x98^\xe9\xfc\xb9\xb6\xc8w\xcc=\x1a\xb0\xb1e\x81>#y0X\xed\xb3\xbc\xdd\xaa\xc9\xa8\xd5u\xce=\x920\x90DQ5\xab\x98|wd\xef\xac\rz\xa4\xec\xdd7R\x90\x14\xec.s<\x96\xd4\xa1E\xd2x3\xf7\xd2\x12/\xc0\xa8\x8a\x88\xab\xd1l\x84E\xa0\x9c\xe2\xbf\xd4\x19\xb0-\xa0Q\x05\xd0wA6\x04\xe2\x06D\x85\x98\x1a(\x9f]\xce\x07\xab]5'\xca5\x14\x0eE\x0f\xdc\xc2\r\x12\x7f\x13\xa2+\xa8\xe1_\x9a$A\xe5c\xfdT\xec<\x93p\xe8\xf9\xfb\x89\xcdzK\xce\xb6D":
                                    malformed_packets += 1

                        debug and print(rxpacket)

                        debug and print(len(rxpacket))
                        sequence_number = rxpacket[:4]
                        debug and print(f'sequence number: {int.from_bytes(sequence_number, "big")}')
                        # info and print(f'seq_number = {sequence_number}')
                        rxpacket = []  # clear it
                        packet_count += 1
                        info and print(f'count = {packet_count}')

    # Upon keyboard interrupt, print the results if packets were received
    except KeyboardInterrupt:
        bad_packets = dropped_packets + malformed_packets
        if bad_packets < 1:
            raise KeyboardInterrupt
        else:
            # Print the results
            print("Packets dropped:", dropped_packets)
            print("Packets with at least one byte error:", malformed_packets)
            print("Total bad packets:", bad_packets)
            print("\nPackets expected:", quantity)
            print("Packet error rate:", bad_packets / quantity)

main()