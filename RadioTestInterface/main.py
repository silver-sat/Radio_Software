# Command tests - sends predefined strings out the serial port.  Need to check the port prior to running.

import serial
dataresponsestart = b'\xC0\x00\x41\x4F\x4B'
dataresponsefinish = b'\xC0'
datacmdresponse = b'\xC0\x00\x41\x4F\x4B\xC0' #don't forget this is hex!!
beaconcmd = b'\xC0\x07\x41\x42\x43\x44\xC0'
deployantennacmd = b'\xC0\x08\xC0'
statuscmd = b'\xC0\x09\xC0'
haltcmd = b'\xC0\x0A\xC0'
badcmd = b'\xC0\x0B\xC0'

def main():
    with serial.Serial('COM8', 57600) as ser:
        print(ser.name)
        #ser.write(datacmd)
        packet = []
        while True:
            x = ser.read(1)  #looking for C0
            print(x)
            if x == b'\xc0':  # start of a packet detected
                dest = ser.read(1) # read the destination (temp)
                print("dest = ", dest)
                data = b''
                while data != b'\xc0':  # keep reading bytes until next 0xc0 encountered
                    packet.append(data)
                    data = ser.read(1)
                # if data == b'\xc0':
                print(packet)
                print(len(packet))
                sequence_number = packet[len(packet)-1]
                print(sequence_number)
                packet = []  # clear it
                dataresponse = b''.join([dataresponsestart, sequence_number])
                dataresponse = b''.join([dataresponse, dataresponsefinish])
                print(dataresponse)
                ser.write(dataresponse) # send a response
                x = b''  # clear out x and go back to looking


if __name__ == '__main__':
    main()