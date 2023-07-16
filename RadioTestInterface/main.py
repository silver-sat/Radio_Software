# Command tests - sends predefined strings out the serial port.  Need to check the port prior to running.

import serial

datacmdresponse = b'\xC0\x00\x41\x4F\x4B\xC0' #don't forget this is hex!!
beaconcmd = b'\xC0\x07\x41\x42\x43\x44\xC0'
deployantennacmd = b'\xC0\x08\xC0'
statuscmd = b'\xC0\x09\xC0'
haltcmd = b'\xC0\x0A\xC0'
badcmd = b'\xC0\x0B\xC0'

def main():
    with serial.Serial('COM7', 57600) as ser:
        print(ser.name)
        #ser.write(datacmd)
        packet=[]
        while True:
            x = ser.read()  #looking for C0
            print(x)
            if x == b'\xc0':
                #len = ser.read() # read the length
                #print("length = ", len)
                dest = ser.read() # read the destination (temp)
                print("dest = ", dest)
                data = ser.read()
                while data != b'\xc0':
                    packet.append(data)
                    data = ser.read()
                    if data == b'\xc0':
                        print(packet)
                        packet = []  # clear it
                        ser.write(datacmdresponse) # send a response



            #y = x.decode("utf-8", "backslashreplace").encode('unicode_escape')
            #print(y.decode("utf-8", "ignore"), end="")
            #if x == b'\\xc0':
            #    print('yup!')
            #    len = ser.read().decode("utf-8", "backslashreplace").encode('unicode_escape')
            #    print(len.decode("utf-8"), "ignore")
            #    for i in range(len-2):
            #        packet.append(ser.read().decode("utf-8", "backslashreplace").encode('unicode_escape'))
            #print(packet, end="")


            #print(x.decode("utf-8", "backslashreplace"), end="")




if __name__ == '__main__':
    main()