# Packet error rate (PER) test
# Modified from packet_receive.py and SSRadioTests.py by isaac-silversat

import serial
import struct
from random import randbytes
from time import sleep

sequence_number = 0


# class RangeError(Exception):
#     # "Raised when value is out of range"
#     pass


# class SweepError(Exception):
#     # "Raised when Sweep is configured incorrectly"
#     pass


def kissenc(bytesequence):
    encbytes = b''
    for byte in bytesequence:
        print(byte.to_bytes(1, 'big'))
        if byte.to_bytes(1, 'big') == b'\xC0':
            encbytes += b'\xDB\xDC'
        elif byte.to_bytes(1, 'big') == b'\xDB':
            encbytes += b'\xDB\xDD'
        else:
            encbytes += byte.to_bytes(1, 'big')
    # print(encbytes)
    return encbytes


def main():
    tx_serial_port = serial.Serial('/dev/ttyACM0', 57600) # Port to send on. Leave it at /dev/ttyACM0 for now
    rx_serial_port = serial.Serial('/dev/ttyACM1', 57600) # Port to send on. Leave it at /dev/ttyACM1 for now
    quantity = 1024 # Leave it 1024 for now.

    # Dropped packets
    dropped_packets = 0
    malformed_packets = 0

    packet_start = b'\xC0\x00'
    packet_finish = b'\xC0'
    debug = False
    info = False # Print packet numbers, one-by-one
    inter_packet_delay = 0.2  # 200 mS between packets
    # packets are 200 (might vary) bytes long, plus one destination byte, 4 bytes for frame delimiter, and 9 0xAA's
    # and 2 CRC bytes
    # so figure 207 bytes at 9600 baud or about 180 mS.  The interface is running at 57600, so you don't want to overrun
    # the radio.  So if it spits out one every 200 mS, it should be okay...could look at this on a scope to get it finer
    # this begs the questions, is it possible for the RPi to overrun the UART when its running at 19200?
    packet_payload = randbytes(196)  # allow 4 bytes for sequence number (integer)
    debug and print(packet_payload)
    debug and print(len(packet_payload))
    kiss_packet_payload = kissenc(packet_payload)
    for seq_num in range(quantity):
        sequence_number_bytes = struct.pack(">I", seq_num)
        txpacket = b''.join([packet_start, sequence_number_bytes])
        txpacket = b''.join([txpacket, kiss_packet_payload])
        txpacket = b''.join([txpacket, packet_finish])
        debug and print(txpacket)
        tx_serial_port.write(txpacket)

        # Save the length of txpacket
        txpacket_length = len(txpacket)

        # Receive the packet just sent, through another serial port
        with rx_serial_port as ser:
            debug and print(ser.name)
            rxpacket = []
            while (len(rxpacket) > 0) & (len(rxpacket) <= txpacket_length):
                x = ser.read(1)  # looking for C0
                # print(x)
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
                            if txpacket != rxpacket:
                                malformed_packets += 1

                    debug and print(rxpacket)

                    debug and print(len(rxpacket))
                    sequence_number = rxpacket[:4]
                    debug and print(f'sequence number: {int.from_bytes(sequence_number, "big")}')
                    # info and print(f'seq_number = {sequence_number}')
                    rxpacket = []  # clear it
                    packet_count += 1
                    info and print(f'count = {packet_count}')
        sleep(inter_packet_delay)

    # Print the results
    print("Packets dropped:", dropped_packets)
    print("Packets with at least one byte error:", malformed_packets)
    bad_packets = dropped_packets + malformed_packets
    print("Total bad packets:", bad_packets)
    print("\nPackets expected:" quantity)
    print("Packet error rate:" bad_packets / quantity)

main()