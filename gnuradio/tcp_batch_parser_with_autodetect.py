import os
import struct
import time

INPUT_DIR = "c:/users/conra/onedrive/documents/parsed_output"
OUTPUT_PCAP = "c:/users/conra/onedrive/documents/output2.pcap"

TCP_MAGIC = b"\x00\x00\x08\x00"

ETHERTYPE_IPV4 = 0x0800
ETHERTYPE_IPV6 = 0x86DD

def write_pcap_global_header(f):
    f.write(struct.pack(
        "<IHHIIII",
        0xa1b2c3d4,  # magic
        2, 4,        # version
        0, 0,        # timezone, accuracy
        65535,       # snaplen
        1            # DLT_EN10MB (Ethernet)
    ))

def write_pcap_packet(f, packet_bytes):
    ts = time.time()
    ts_sec = int(ts)
    ts_usec = int((ts - ts_sec) * 1_000_000)

    f.write(struct.pack("<IIII", ts_sec, ts_usec,
                        len(packet_bytes), len(packet_bytes)))
    f.write(packet_bytes)

def is_ipv4(pkt):
    if len(pkt) < 20:
        return False
    version = pkt[0] >> 4
    ihl = pkt[0] & 0x0F
    if version != 4:
        return False
    if ihl < 5:
        return False
    total_len = struct.unpack(">H", pkt[2:4])[0]
    if total_len < 20 or total_len > 65535:
        return False
    return True

def is_ipv6(pkt):
    if len(pkt) < 40:
        return False
    version = pkt[0] >> 4
    return version == 6

def wrap_ip_in_eth(ip_bytes, ethertype):
    # Synthetic Ethernet header: src/dst = zeros
    return (
        b"\x00\x00\x00\x00\x00\x00" +
        b"\x00\x00\x00\x00\x00\x00" +
        struct.pack(">H", ethertype) +
        ip_bytes
    )

def main():
    files = sorted(os.listdir(INPUT_DIR))

    with open(OUTPUT_PCAP, "wb") as pcap:
        write_pcap_global_header(pcap)

        for filename in files:
            path = os.path.join(INPUT_DIR, filename)
            if not os.path.isfile(path):
                continue

            data = open(path, "rb").read()

            # Identify TCP-framed files
            if not data.startswith(TCP_MAGIC):
                continue

            payload = data[4:]  # strip prefix

            # AUTO-DETECT FORMAT
            frame = None

            # Case 1: Looks like Ethernet (EtherType at bytes 12–13)
            if len(payload) >= 14:
                ethertype = struct.unpack(">H", payload[12:14])[0]
                if ethertype in (ETHERTYPE_IPV4, ETHERTYPE_IPV6):
                    frame = payload

            # Case 2: Raw IPv4
            if frame is None and is_ipv4(payload):
                frame = wrap_ip_in_eth(payload, ETHERTYPE_IPV4)

            # Case 3: Raw IPv6
            if frame is None and is_ipv6(payload):
                frame = wrap_ip_in_eth(payload, ETHERTYPE_IPV6)

            # Unknown format → skip
            if frame is None:
                print(f"Skipping {filename}: unknown format")
                continue

            write_pcap_packet(pcap, frame)
            print(f"Wrote packet from {filename}")

    print(f"\nDone. PCAP written to {OUTPUT_PCAP}")

if __name__ == "__main__":
    main()
