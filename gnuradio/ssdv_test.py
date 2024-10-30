import serial
from time import sleep

packet_size = 195
debug = True

def kissenc(bytesequence):
    encbytes = b''
    for byte in bytesequence:
        #print(byte.to_bytes(1, 'big'))
        if byte.to_bytes(1, 'big') == b'\xC0':
            encbytes += b'\xDB\xDC'
        elif byte.to_bytes(1, 'big') == b'\xDB':
            encbytes += b'\xDB\xDD'
        else:
            encbytes += byte.to_bytes(1, 'big')
    #print(encbytes.hex())
    return encbytes

def packetsend(serial_port, payload):
    packet_start = b'\xC0\x00'
    packet_finish = b'\xC0'
    inter_packet_delay = 1.300  # 200 mS between packets
    
    #debug and print(payload)
    #debug and print(len(payload))
    
    if len(payload) > 0:    
        kiss_payload = kissenc(payload)      
        packet = b''.join([packet_start, kiss_payload])
        packet = b''.join([packet, packet_finish])
        #debug and print(packet)
        serial_port.write(packet)
        sleep(inter_packet_delay)
        
    return

    
    
if __name__ == '__main__':
    
    ser = serial.Serial('/dev/serial0', 9600, timeout=0, write_timeout=2)
    #payloads = []
    #payload = ''
    # read in the file and convert it to a list
    with open('/home/tom/ssdv/output.bin', 'rb') as file:
        payload = file.read(packet_size)
        print(payload.hex())
        packetsend(ser, payload)
        while payload:
            payload = file.read(packet_size)
            print(payload.hex())
            #debug and print(''.join(f'{byte:02x}' for byte in payload))
            packetsend(ser, payload)