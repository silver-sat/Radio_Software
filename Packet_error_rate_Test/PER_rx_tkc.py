# Packet error rate (PER) test
# Modified from packet_receive.py and SSRadioTests.py by isaac-silversat
# Alternate version by tkc.  Allows random restart and sync to packet capture.  Length of capture is arbitrary.
# Assumes all received packets are good (validated by CRC or RS decode check)

import serial

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
    rx_serial_port = serial.Serial('/dev/serial0', 19200) # Port to send on. Leave it at /dev/ttyACM0 for testing purposes

    # Dropped packets
    first = True # is this the first time running?
    packet_list = [] # this is a list of received packet sequence numbers
    first_packet = 0
    sequence_number = 0
    
    try:
        # first_packet is the sequence number of the first received packet
        # we keep watching the sequence_numbers until the difference between the current and first sequence number is equal to the quantity (could be off by one)
        # or there is an exception
        with rx_serial_port as ser:
            debug and print(ser.name)
            rxpacket = []        
            #need to synchronize to next packet, you might be starting in the middle, so start by looking for two 0xC0s
            #however, if we're just starting up, there aren't two in a row.
            x = b''
            while x != b'\xc0':
                x = ser.read(1)
            dest = ""
            while (len(rxpacket) >= 0) & (len(rxpacket) <= txpacket_length):
                x = ser.read(1)
                if x == b'\xc0':
                    pass
                else:
                    dest = x                
                    debug and print("dest = ", dest)                
                    rxpacket = bytearray(b'')
                    read_byte = None
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
                        x = b''
                            
                    sequence_number_bytes = rxpacket[:4]
                    sequence_number = int.from_bytes(sequence_number_bytes, "big")
                    
                
                    # stick the packet number in a list.  Assumption is that first and last packets are received...so do lots of packets
                    packet_list.append(sequence_number)
                    if first:
                        first_packet = sequence_number
                    first = False
                    rxpacket = []
                    print(sequence_number-first_packet)

    # Upon keyboard interrupt, print the results if packets were received
    except KeyboardInterrupt:
        debug and print(packet_list)
        #debug and print(len(packet_list))
        # quantity transmitted is a guess on how many were sent based on the first and last received sequence numbers
        quantity_transmitted = packet_list[-1] - first_packet + 1 
        print("first packet: ", first_packet)
        print("last packet: ", packet_list[-1])
        print("Packets dropped:", quantity_transmitted - len(packet_list))
        print("Quantity transmitted?: ", quantity_transmitted)
        print("Packet error rate:", (quantity_transmitted - len(packet_list))/quantity_transmitted)

main()