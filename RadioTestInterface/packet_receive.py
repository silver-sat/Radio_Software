# packet_receive works with packet_send to do a bulk packet transfer test


import serial

debug = True
info = True
packet_count = 0

with serial.Serial('COM15', 57600) as ser:
    debug and print(ser.name)
    packet = []
    while True:
        x = ser.read(1)  # looking for C0
        # print(x)
        if x == b'\xc0':  # start of a packet detected
            dest = ser.read(1)  # read the destination (temp)
            print("dest = ", dest)
            packet = bytearray(b'')
            read_byte = None
            while read_byte != b'\xc0':  # keep reading bytes until next 0xc0 encountered
                read_byte = ser.read(1)  # need to kiss decode, do it on the fly
                if read_byte == b'\xDB':
                    read_next_byte = ser.read(1)
                    if read_next_byte == b'\xDC':
                        packet.extend(b'\xC0')
                    elif read_next_byte == b'\xDD':
                        packet.extend(b'\xDB')
                elif read_byte == b'\xc0':
                    pass
                else:
                    packet.extend(read_byte)

            debug and print(packet)

            debug and print(len(packet))
            sequence_number = packet[:4]
            debug and print(f'sequence number: {int.from_bytes(sequence_number, "big")}')
            # info and print(f'seq_number = {sequence_number}')
            packet = []  # clear it
            packet_count += 1
            info and print(f'count = {packet_count}')