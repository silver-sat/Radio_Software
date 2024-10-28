# il2p decoder

# this takes in a file containing silversat il2p packets (delimited) and returns a new file with KISS encoded/delimited
# packets
# the intent is to make the output look like what comes out of the Silversat Radio


# for my and future reference, my derived version of the RS codec used in Direwolf produces the same parity bytes
# as the U frame example in the IL2P spec.  So I believe that's working.
# setting generator to 1 seems to make the codec stop working.  It only outputs 0x00\0x01 regardless of other
# parameters

# the error I was chasing was two-fold.  It turns out that there's one extra byte in each of the parsed out packets.
# It might be easier just to include length byte after all.  That meant that the payload size was off by 1.
# And I was calculating the parity on the first packet object , while the temporary encoded_il2p_payload value
# had been updated to the sixth value!  That works on the header, it doesn't change, but not on payload!

import reedsolo as rs
import mmap
from dataclasses import dataclass
import sys
from PyCRC.CRCCCITT import CRCCCITT
from PyCRC.CRC16Kermit import CRC16Kermit


# this is the generic set of information you would need to construct/deconstruct an IL2P or an AX.25 packet
@dataclass
class Il2pPacket:
    packet_error_type: list
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
    state = 0x1F0  # initial state
    output = [0] * block_len
    value = 0
    for b in range(0, block_len):
        for i in range(0, 8):
            m = 0x80 >> i
            if input_block[b] & m != 0:
                in_bit = 1
            else:
                in_bit = 0
            value, state = descramble_bit(in_bit, state)
            if value == 1:
                output[b] |= m
            hex_out = [hex(n) for n in output]
            output_block = bytearray(output)
    return output_block


# state is a bytearray
def descramble_bit(byte, state):
    out = (byte ^ state) & 1
    state = ((state >> 1) | ((byte & 1) << 8)) ^ ((byte & 1) << 3)
    return out, state


def decode_received_crc(received_crc):
    decode_table = [
        0x0, 0x0, 0x0, 0x3, 0x0, 0x5, 0xe, 0x7,
        0x0, 0x9, 0xe, 0xb, 0xe, 0xd, 0xe, 0xe,
        0x0, 0x3, 0x3, 0x3, 0x4, 0xd, 0x6, 0x3,
        0x8, 0xd, 0xa, 0x3, 0xd, 0xd, 0xe, 0xd,
        0x0, 0x5, 0x2, 0xb, 0x5, 0x5, 0x6, 0x5,
        0x8, 0xb, 0xb, 0xb, 0xc, 0x5, 0xe, 0xb,
        0x8, 0x1, 0x6, 0x3, 0x6, 0x5, 0x6, 0x6,
        0x8, 0x8, 0x8, 0xb, 0x8, 0xd, 0x6, 0xf,
        0x0, 0x9, 0x2, 0x7, 0x4, 0x7, 0x7, 0x7,
        0x9, 0x9, 0xa, 0x9, 0xc, 0x9, 0xe, 0x7,
        0x4, 0x1, 0xa, 0x3, 0x4, 0x4, 0x4, 0x7,
        0xa, 0x9, 0xa, 0xa, 0x4, 0xd, 0xa, 0xf,
        0x2, 0x1, 0x2, 0x2, 0xc, 0x5, 0x2, 0x7,
        0xc, 0x9, 0x2, 0xb, 0xc, 0xc, 0xc, 0xf,
        0x1, 0x1, 0x2, 0x1, 0x4, 0x1, 0x6, 0xf,
        0x8, 0x1, 0xa, 0xf, 0xc, 0xf, 0xf, 0xf]

    received_crc_integer = int.from_bytes(received_crc, "big")
    # print(received_crc_integer)
    corrected_fourth_byte = decode_table[(received_crc_integer & 0xFF000000) >> 24]
    corrected_third_byte = decode_table[(received_crc_integer & 0x00FF0000) >> 16]
    corrected_second_byte = decode_table[(received_crc_integer & 0x0000FF00) >> 8]
    corrected_first_byte = decode_table[received_crc_integer & 0x000000FF]
    corrected_crc = (corrected_fourth_byte << 24) | (corrected_third_byte << 16) | (corrected_second_byte << 8) | \
        corrected_first_byte
    crc = (corrected_fourth_byte & 0x0F) << 12 | (corrected_third_byte & 0x0F) << 8 | \
          (corrected_second_byte & 0x0F) << 4 | (corrected_first_byte & 0x0F)
    return crc


def main():
    # constants
    generator = 2
    fcr = 0
    header_length = 13
    header_parity_length = 2
    payload_parity_length = 16
    encoded_crc_length = 4
    ax25_header = b'\x96\x86\x66\xAC\xAC\xAE\xE2\x96\x86\x66\xAC\xAC\xAE\x61\x03\xF0'

    filename = sys.argv[1]
    locations = extract_packet_locations(filename)
    print("packet locations: ", locations)
    packet = []
    with open(filename, "r") as file:
        with mmap.mmap(file.fileno(), length=0, access=mmap.ACCESS_READ) as mmap_obj:
            for i, j in zip(locations, locations[1:]):
                # print(i, j)
                if j != -1:
                    packet.append(mmap_obj[i + 4:j])
                else:
                    packet.append(mmap_obj[i + 4:])
    # print(packet)
    packet_objects = []

    for index, pack in enumerate(packet):
        try:
            packet_length = len(pack)
            # print("the packet length is: ", packet_length)
            il2p_pack = Il2pPacket(packet_error_type=[])
            # grab the encoded received header
            encoded_il2p_header = pack[:13]  # 0 to 12 is 13 bytes
            il2p_pack.header_parity = pack[13:15]

            # let's check the encoded header for errors
            # need to watch, RS code is on the scrambled header
            header_with_parity = encoded_il2p_header + il2p_pack.header_parity
            # print("received encoded header: ", header_with_parity)
            rsc_2 = rs.RSCodec(nsym=2, nsize=255, fcr=fcr, prim=0x11d, generator=generator, single_gen=False)
            if rsc_2.check(header_with_parity, nsym=2):
                corrected_header = rsc_2.decode(header_with_parity, nsym=2)
                # print("corrected encoded header: ", corrected_header[0])
                # descramble it
                header_list = [byte for byte in
                               corrected_header[0]]  # convert the header bytes into a list for processing
                il2p_pack.header = descramble(header_list, header_length)
            else:
                il2p_pack.packet_error_type.append("BAD HEADER")
                # if there's an error, just write the uncorrected packet to the header field
                il2p_pack.header = encoded_il2p_header

            # check the payload for errors
            rsc_16 = rs.RSCodec(nsym=16, nsize=255, fcr=fcr, prim=0x11d, generator=generator, single_gen=False)
            payload_length = packet_length - encoded_crc_length - payload_parity_length - header_length - \
                header_parity_length - 1  # (crc + rs) + header + 1
            encoded_il2p_payload = pack[15:15 + payload_length]  # grab the payload block
            il2p_pack.payload_parity = pack[payload_length + header_length + header_parity_length:
                                            payload_length + header_length + header_parity_length +
                                            payload_parity_length]

            if rsc_2.check(encoded_il2p_payload+il2p_pack.payload_parity, nsym=16):
                corrected_payload = rsc_16.decode(encoded_il2p_payload + il2p_pack.payload_parity, nsym=16)
                # print("corrected encoded payload", corrected_payload[0])

                # and descramble that
                payload_list = [byte for byte in corrected_payload[0]]
                il2p_pack.payload = descramble(payload_list, payload_length)
            else:
                il2p_pack.packet_error_type.append("BAD PAYLOAD")
                il2p_pack.payload = encoded_il2p_payload

            # process the CRC
            il2p_pack.encoded_crc = pack[payload_length + 15 + 16:payload_length + 15 + 16 + 4]
            # to decode the CRC you can just take the lower nibble of each byte
            received_crc = decode_received_crc(il2p_pack.encoded_crc)
            ax25_packet = ax25_header + il2p_pack.payload
            # silversat uses a precomputed AX.25 header since we are really doing a pt to pt link.
            # print(ax25_packet)
            # print(type(ax25_packet))
            computed_crc = CRCCCITT(version='FFFF').calculate(ax25_packet)
            if computed_crc != received_crc:
                il2p_pack.packet_error = il2p_pack.packet_error_type.append("BAD CRC")
            # print("CRC calcs")
            # print(il2p_pack.encoded_crc)
            # print(hex(received_crc))
            # print(hex(computed_crc))

            packet_objects.append(il2p_pack)

        except rs.ReedSolomonError as err:
            print("packet", index, "cannot be recovered")
            print("check to see if error is in header or payload")
            print(err)
            continue
    # print(len(packet_objects))

    for index, packet in enumerate(packet_objects):
        print("packet: ", index)
        print("here's the payload", packet.payload)


if __name__ == "__main__":
    main()
