import struct

def parse_tcp_frame(data: bytes):
    """
    Parse a TCP frame from raw bytes.
    :param data: Raw TCP segment (bytes)
    :return: Dictionary with parsed TCP header fields and payload
    """
    # TCP header is at least 20 bytes
    if len(data) < 20:
        raise ValueError("Data too short to be a valid TCP frame")

    # Unpack first 20 bytes of TCP header
    # ! = network byte order (big-endian)
    # H = unsigned short (2 bytes), L = unsigned long (4 bytes), B = unsigned char (1 byte)
    src_port, dst_port, seq, ack, offset_reserved_flags, window, checksum, urg_ptr = struct.unpack(
        '!HHLLHHHH', data[:20]
    )

    # Data offset (upper 4 bits of offset_reserved_flags) * 4 = header length in bytes
    data_offset = (offset_reserved_flags >> 12) * 4

    # Extract TCP flags (lower 9 bits)
    flags = offset_reserved_flags & 0x01FF
    flag_names = {
        'NS': (flags >> 8) & 1,
        'CWR': (flags >> 7) & 1,
        'ECE': (flags >> 6) & 1,
        'URG': (flags >> 5) & 1,
        'ACK': (flags >> 4) & 1,
        'PSH': (flags >> 3) & 1,
        'RST': (flags >> 2) & 1,
        'SYN': (flags >> 1) & 1,
        'FIN': flags & 1
    }

    # Extract payload
    payload = data[data_offset:]

    return {
        'src_port': src_port,
        'dst_port': dst_port,
        'sequence_number': seq,
        'acknowledgment_number': ack,
        'data_offset': data_offset,
        'flags': flag_names,
        'window_size': window,
        'checksum': checksum,
        'urgent_pointer': urg_ptr,
        'payload': payload
    }


# Example usage
if __name__ == "__main__":
    # Example raw TCP segment (20-byte header + payload)
    # This is just sample data for demonstration
    raw_tcp = b'\x45\x00\x00\x54\x4d\x83\x40\x00\x40\x01\xa3\x09\xc0\xa8\x64\x66\xc0\xa8\x64\x65\x08\x00\xf1\x97\x00\x01\x00\x01\xe6\xe5\x59\x69\xd9\x13\x02\x00\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37'  # Payload: "Hello"

    try:
        parsed = parse_tcp_frame(raw_tcp)
        print("Parsed TCP Frame:")
        for key, value in parsed.items():
            if key == 'payload':
                print(f"{key}: {value} ({value.decode(errors='ignore')})")
            else:
                print(f"{key}: {value}")
    except Exception as e:
        print(f"Error parsing TCP frame: {e}")
