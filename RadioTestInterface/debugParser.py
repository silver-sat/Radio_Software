


import PySimpleGUI as sg
import serial
from serial.tools import list_ports
from datetime import datetime
import os
import argparse

def main(args):
    sg.theme('Light Blue 2')

    ports = serial.tools.list_ports.comports(include_links=True)

    # iterate through ports until we have a good one
    validport = False
    for port in ports:
        print(port)
        print(port[0])
    while not validport:
        for port in ports:
            try:
                print("current port: ", port[0])
                ser = serial.Serial(port[0], baudrate=57600, timeout=0, write_timeout=2)
                validport = True
                currentportname = port
                currentport = port[0]
                break
            except serial.serialutil.SerialException:
                print('Serial port is in use')
                # window2['output'].print('Serial port is in use')

    now = datetime.now()
    date_time_string = now.strftime("%Y-%m-%dT%H_%M_%S")
    filename = currentport + "_" + date_time_string + ".txt"
    #print(filename)
    #cwd = os.getcwd()
    #print(cwd)
    file_path = os.path.join(logpath, filename)
    print(file_path)

    output_layout = [
        [[sg.Text('Serial Port', size=15), sg.Spin(ports, size=30, key='portname', enable_events=True, initial_value=currentportname)],
         [sg.Checkbox('RSSI', default=True, key='rssi_check', enable_events=True), sg.Checkbox('Offset', default=True, key='offset_check', enable_events=True), sg.Checkbox('Count', default=True, key='count_check', enable_events=True), sg.Button('Reset', key='reset_count', enable_events=True)],
         sg.Frame('Serial Output', [[sg.Multiline(default_text="Serial Output goes here \r\n", key='output',
                                                  write_only=True, size=(400, 400), autoscroll=True)]],
                  size=(440, 440))]]
    rssi_layout = [[sg.Text(font=("Calibri", 96), key='rssi', text_color='blue')]]

    offset_layout = [[sg.Text(font=("Calibri", 96), key='offset', text_color='blue')]]

    packetcount_layout = [[sg.Text(font=("Calibri", 96), key='packet_count', text_color='blue')]]


    window2 = sg.Window('Radio Output', output_layout, finalize=True, location = (650, 10))
    window = sg.Window('Last RSSI', rssi_layout, finalize=True, disable_close=True, location= (20,10), keep_on_top=True, no_titlebar=True)
    window3 = sg.Window('Last Offset', offset_layout, finalize=True, disable_close=True, location=(20,200), keep_on_top=True, no_titlebar=True)
    window4 = sg.Window('Packet Count', packetcount_layout, finalize=True, disable_close=True, location=(20,390), keep_on_top=True, no_titlebar=True)
    window['rssi'].update('RSSI: 0')
    window3['offset'].update('OFFSET: 0')
    window4['packet_count'].update('COUNT: 0')
    count = 0

    # event loop
    while True:
        with open(filename, 'a') as logfile:
            # integer inputs: duration, power, number of steps, dwell, frequency, start, stop
            # alpha inputs: beaconstring

            if validport:
                event2, values2 = window2.read(timeout=100)
                event, values = window.read(timeout=100)
                event3, values3 = window3.read(timeout=100)
                event4, value4 = window4.read(timeout=100)

                serinput = ser.read()
                while ser.in_waiting > 0:
                    serinput += ser.read()
                if serinput != b"":
                    window2['output'].print(serinput.decode('utf-8'))
                    logfile.write(serinput.decode('utf-8').replace('\r\n', '\r'))
                    # print(serinput)
                    parsedinput = serinput.decode('utf-8').split('N: ')
                    for strings in parsedinput:
                        if strings.split(' ')[0] == 'rssi':
                            rssi_string = 'RSSI: ' + strings.split(' ')[1]
                            window['rssi'].update(rssi_string)
                        if strings.split(' ')[0] == 'rf':
                            offset_string = 'OFFSET: ' + strings.split(' ')[2]
                            window3['offset'].update(offset_string)
                        if strings.split(' ')[0] == 'Success!':
                            count = count + 1
                            countstr = 'COUNT: ' + str(count)
                            window4['packet_count'].update(countstr)
                    #print(serinput.decode('utf-8')[3:12])
                    #if serinput.decode('utf-8')[3:7] == 'rssi':
                        # print(serinput.decode('utf-8')[8:12])

                    #if serinput.decode('utf-8')[3:12] == 'rf offset':
                    #    window3['offset'].update(serinput.decode('utf-8')[13:])
                # window.refresh()

            if event2 == 'reset_count':
                count = 0
                countstr = 'COUNT: ' + str(count)
                window4['packet_count'].update(countstr)

            # process button inputs
            if event2 == sg.WIN_CLOSED:
                if ser.is_open:
                    ser.close()
                logfile.close()
                window2.close()
                break  # exit cleanly
            # print(event)
            # print(values['portname'][0])
            # print(currentport)
            if event2 == 'portname':  # and values['portname'][0] != currentport:
                print("portevent")
                ser.close()
                try:
                    ser = serial.Serial(values['portname'][0], 57600, timeout=0, write_timeout=2)
                    validport = True  # okay that worked so the port is valid
                    window2['output'].print('Valid Port')
                    window2.refresh()
                    currentport = values['portname'][0]  # and update the current port

                except serial.serialutil.SerialException:
                    print('Serial port is in use')
                    validport = False  # that didn't work, so the current selected port isn't valid, buttons don't work
                    window2['output'].print('Serial port is in use')
            if event2 == 'rssi_check':
                if window2['rssi_check'].get() == True:
                    window.un_hide()
                    window.refresh()
                else:
                    window.hide()
            if event2 == 'offset_check':
                if window2['offset_check'].get() == True:
                    window3.un_hide()
                    window3.refresh()
                else:
                    window3.hide()
            if event2 == 'count_check':
                if window2['count_check'].get() == True:
                    window4.un_hide()
                    window4.refresh()
                else:
                    window4.hide()
    ser.close()
    logfile.close()
    window2.close()
    window.close()
    window3.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="debugParser")
    #parser.add_argument("-i", "--input", help="the file containing il2p packets")
    parser.add_argument("-o", "--logpath", help="logfile path")
    #parser.add_argument("-v", "--verbose", action="store_true", help="display parsed packets")
    args = parser.parse_args()

    if args.logpath == None:
        logpath = os.getcwd()
    else:
        logpath = args.logpath
    main(args)