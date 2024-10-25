# il2p decoder

# this takes in a file containing silversat il2p packets (delimited) and returns a new file with KISS encoded/delimited
# packets
# the intent is to make the output look like what comes out of the Silversat Radio

import reedsolo as rs
import mmap
from dataclasses import dataclass
import sys


# this is the generic set of information you would need to contruct/deconstruct an IL2P or an AX.25 packet
@dataclass
class Il2pPacket:
    header: bytearray = b''
    header_parity: bytearray = b''  # two bytes
    header_corrections: int = 0
    payload: bytearray = b''
    payload_parity: bytearray = b''  # 16 bytes
    payload_corrections: int = 0
    encoded_crc: bytearray = b''  # 4 bytes while encoded
    crc_success: bool = False  # set true when verified
    payload_byte_count: int = 0  # zero by default

class Il2pHeader:
    def __init__(self):
        self.header = b''
        self.destination_callsign = b'KC3VVW'
        self.source_callsign = b'KC3VVW'
        self.destination_ssid = b'\x01'
        self.source_ssid = b'\x00'
        self.UI = b'\x01'  # really on bit
        self.PID = b'\xF0'  # in il2p this is a nibble (upper), in ax.25 this is a byte
        self.control = b'\x03'
        self.HDR = b'\x01'  # also only one bit
        self.payload_byte_count = 0

    def deconstruct(self, header):
        print("deconstruct the header here")
        print(self.header)


"""
# given an Il2pHeader instance and a payload block, construct an AX.25 packet
class AX25Packet:
    def __init__(self):
        self.payload = b''
        self.
"""


# this should be modified to allow 1 bit error in the delimiter, there are 32 bits, so that means we're matching against
# 32 potential bit flips.  The potential of one of those showing up in the data is 32/(2**32)...small.
def extract_packet_locations(filename):
    print("processing: ", filename)
    delimiter = b'\xAA\xF1\x5E\x48'
    locations = []
    with open(filename, "r") as file:
        with mmap.mmap(file.fileno(), length=0, access=mmap.ACCESS_READ) as mmap_obj:
            index = mmap_obj.find(delimiter)
            locations.append(index)
            # print(locations)
            while index != -1:
                index = mmap_obj.find(delimiter, index + 1)
                locations.append(index)
                # print(locations)
            return locations


# input block is a list of integers (but really bytes)
def descramble(input_block, block_len):
    state = 0x1F0  # this isn't changing...
    output = [0] * block_len
    # print(input_block)
    # print("initial state: ", state)
    value = 0
    for b in range(0, block_len):
        for i in range(0, 8):
            m = 0x80 >> i
            if input_block[b] & m != 0:
                in_bit = 1
            else:
                in_bit = 0
            value, state = descramble_bit(in_bit, state)
            # print("state: ", state)
            # print("b: ", b)
            # print("m: ", m)
            if value == 1:
                output[b] |= m
            # print("output: ", output)
            hex_out = [hex(n) for n in output]
            # print("output(hex): ", hex_out)
            output_array = bytearray(output)
            # print(output_array)
    return output_array


# state is a bytearray
def descramble_bit(byte, state):
    out = (byte ^ state) & 1
    state = ((state >> 1) | ((byte & 1) << 8)) ^ ((byte & 1) << 3)
    return out, state


def main():
    rs.init_tables(0x11d, generator=1)
    rsc_2 = rs.RSCodec(nsym=2, generator=1, fcr=0)
    rsc_16 = rs.RSCodec(nsym=16, generator=1, fcr=0)

    filename = sys.argv[1]
    locations = extract_packet_locations(filename)
    print(locations)
    packet = []
    with open(filename, "r") as file:
        with mmap.mmap(file.fileno(), length=0, access=mmap.ACCESS_READ) as mmap_obj:
            for i, j in zip(locations, locations[1:]):
                # print(i, j)
                if j != -1:
                    packet.append(mmap_obj[i+4:j])
                else:
                    packet.append(mmap_obj[i+4:])
    # print(packet)
    packet_objects = []
    for pack in packet:
        packet_length = len(pack)
        il2p_pack = Il2pPacket()
        # process the header
        encoded_il2p_header = pack[:13]
        header_list = [byte for byte in encoded_il2p_header]
        il2p_pack.header = descramble(header_list, 13)
        il2p_pack.header_parity = pack[13:15]
        # process the payload
        payload_length = packet_length-20-15-1  # (crc + rs) + header + 1
        encoded_il2p_payload = pack[15:packet_length-20]
        payload_list = [byte for byte in encoded_il2p_payload]
        il2p_pack.payload = descramble(payload_list, payload_length)
        il2p_pack.payload_parity = pack[packet_length-19:packet_length-4]
        # process the CRC
        il2p_pack.encoded_crc = pack[packet_length-4:]

        print("the header is: ", il2p_pack.header.hex())
        print("the header parity is: ", il2p_pack.header_parity.hex())
        print("the payload is: ", il2p_pack.payload)
        print("the payload parity is: ", il2p_pack.payload_parity)
        print("the encoded CRC is: ", il2p_pack.encoded_crc)

        packet_objects.append(il2p_pack)
    print(len(packet_objects))

    # now let's first check the parity
    erasures = []
    message = packet_objects[0].header+packet_objects[0].header_parity
    message_2 = packet_objects[0].header
    print("message to check: ", message)
    emes = rsc_2.encode(message_2)
    print("encoded message: ", emes)
    print("encoded parity: ", emes[13:])
    try:
        rmes = rsc_2.decode(message)[0]

        print("rmes: ", rmes)

    except rs.ReedSolomonError as err:
        print(err)

    print(rs.rs_check(message, 2))


if __name__ == "__main__":
    main()
