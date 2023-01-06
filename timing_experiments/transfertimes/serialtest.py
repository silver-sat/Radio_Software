import serial

def main():
    with serial.Serial('COM11', 57600) as ser:
        print(ser.name)
        for i in range(349):
            ser.write(b'\x31')
        #ser.write(datacmd)
        #while True:
            #x = ser.read().decode("utf-8", "backslashreplace").encode('unicode_escape')
            #print(x.decode("utf-8", "backslashreplace"), end="")
            #print(ser.read(), end="")
    print("750 bytes written to serial port")

if __name__ == '__main__':
    main()