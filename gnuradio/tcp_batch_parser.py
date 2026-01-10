import os
import struct
import time

INPUT_DIR = "c:/users/conra/onedrive/documents/parsed_output"
OUTPUT_PCAP = "c:/users/conra/onedrive/documents/output.pcap"

TCP_MAGIC = b"\x00\x00\x08\x00"   # prefix identifying TCP frames

def write_pcap_global_header(f):
    # PCAP Global Header (standard libpcap, Ethernet link type)
    f.write(struct.pack(
        "<IHHIIII",
        0xa1b2c3d4,  # magic number
        2, 4,        # version major, minor
        0, 0,        # timezone, accuracy
        65535,       # snapshot length
        1            # link-layer type: Ethernet
    ))

def write_pcap_packet(f, packet_bytes):
    ts = time.time()
    ts_sec = int(ts)
    ts_usec = int((ts - ts_sec) * 1_000_000)

    f.write(struct.pack(
        "<IIII",
        ts_sec,
        ts_usec,
        len(packet_bytes),
        len(packet_bytes)
    ))
    f.write(packet_bytes)

def main():
    files = sorted(os.listdir(INPUT_DIR))

    with open(OUTPUT_PCAP, "wb") as pcap:
        write_pcap_global_header(pcap)

        for filename in files:
            path = os.path.join(INPUT_DIR, filename)

            if not os.path.isfile(path):
                continue

            with open(path, "rb") as f:
                data = f.read()

            # Check prefix
            if not data.startswith(TCP_MAGIC):
                continue  # skip non-TCP files

            # Strip the 4-byte prefix
            frame = data[4:]

            # Write packet to PCAP
            write_pcap_packet(pcap, frame)

            print(f"Wrote TCP frame from {filename}")

    print(f"\nDone. PCAP written to: {OUTPUT_PCAP}")

if __name__ == "__main__":
    main()
