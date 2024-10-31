import serial
from time import sleep

frequency1 = "433000000"
frequency2 = "433001000"

cmd = b'\xC0\x0D' + frequency1.encode('utf-8') + b'\x20' + frequency2.encode('utf-8') + b'\xC0'

cmd2 = b'\xC0\x0D'+ frequency2.encode('utf-8') + b'\x20' + frequency1.encode('utf-8') + b'\xC0'

try:
    ser = serial.Serial('/dev/ttyUSB0', 19200, timeout=0, write_timeout=2)
    while(1):
        #print(cmd)
        ser.write(cmd)
        sleep(0.5)
        #print(cmd2)
        ser.write(cmd2)
        sleep(0.5)
except KeyboardInterrupt:
    print('exiting')
    quit()
        

