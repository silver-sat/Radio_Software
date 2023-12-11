# Test GUI

import PySimpleGUI as sg
import serial
from serial.tools import list_ports
import struct

sequence_number = 0


class RangeError(Exception):
    # "Raised when value is out of range"
    pass


class SweepError(Exception):
    # "Raised when Sweep is configured incorrectly"
    pass


def kissenc(bytesequence):
    encbytes = b''
    for byte in bytesequence:
        print(byte.to_bytes(1, 'big'))
        if byte.to_bytes(1, 'big') == b'\xC0':
            encbytes += b'\xDB\xDC'
        elif byte.to_bytes(1, 'big') == b'\xDB':
            encbytes += b'\xDB\xDD'
        else:
            encbytes += byte.to_bytes(1, 'big')
    print(encbytes)
    return encbytes


if __name__ == '__main__':
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
                ser = serial.Serial(port[0], 57600, timeout=0, write_timeout=2)
                validport = True
                currentportname = port
                currentport = port[0]
                break
            except serial.serialutil.SerialException:
                print('Serial port is in use')
                # window2['output'].print('Serial port is in use')

    standard_layout = [[sg.Button('Send Beacon', size=30)], [sg.Button('Deploy Antenna', size=30)],
                       [sg.Button('Request Status', size=30)],
                       [sg.Button('Halt', size=30)], [sg.Button('Modify Frequency', size=30)],
                       [sg.Button('Modify Mode', size=30)], [sg.Button('Adjust Frequency', size=30)]]

    config_layout = [
        [sg.Text('Beacon String', size=22), sg.InputText("ABCD", key='beaconstring', size=6, justification='center')],
        [sg.Text('Tx Duration (Seconds)', size=22), sg.InputText("30", key='duration', size=3, justification='center')],
        [sg.Text('Rx Integration Time (Seconds)', size=22),
         sg.InputText("30", key='integrate', size=3, justification='center')],
        [sg.Text('Frequency (Hz)', size=22),
         sg.InputText("433000000", key='frequency', size=10, justification='center')],
        [sg.Text('Serial Port', size=22),
         sg.Spin(ports, size=35, key='portname', enable_events=True, initial_value=currentportname)]]

    # [sg.Text('Power (%)', size=22), sg.InputText("100", key='power', size=3, justification='center')],

    test_layout = [[sg.Button('Transmit Dead Carrier', size=30)],
                   [sg.Button('Measure Current RSSI', size=30)],
                   [sg.Button('Measure Background RSSI', size=30)],
                   [sg.Button('Sweep Transmitter', size=30)],
                   [sg.Button('Sweep Receiver', size=30)],
                   [sg.Button('Send Bad Command', size=30)],
                   [sg.Button('Send Test Data Packet Command', size=30)],
                   [sg.Button('Send Test Remote Command', size=30)]  # ,
                   # [sg.Button('Query AX5043 Register' size=30)]
                   ]

    sweep_layout = [[sg.Text('Start Frequency (Hz)', size=20),
                     sg.InputText("420000000", size=10, key='start', justification='center')],
                    [sg.Text('Stop Frequency (Hz)', size=20),
                     sg.InputText("440000000", size=10, key='stop', justification='center')],
                    [sg.Text('Number of Steps', size=20),
                     sg.InputText("10", size=5, key='step', justification='center')],
                    [sg.Text('Dwell Time (mSec)', size=20),
                     sg.InputText("100", size=6, key='dwell', justification='center')]]

    col2 = sg.Column(
        [
            [sg.Frame('Sweep Configuration', sweep_layout)],
            [sg.Frame('Test Commands', test_layout)],
            [sg.VPush()]
        ])

    col1 = sg.Column(
        [
            [sg.Frame('Configuration', config_layout)],

            [sg.Frame('Operational Commands', standard_layout)],
            [sg.VPush()]
        ])

    layout = [[col1, col2]]
    output_layout = [
        [sg.Frame('Serial Output', [[sg.Multiline(default_text="Serial Output goes here \r\n", key='output',
                                                  write_only=True, size=(400, 400), autoscroll=True)]],
                  size=(440, 440))]]

    window = sg.Window('Silversat Radio Tests', layout, finalize=True)
    window2 = sg.Window('Radio Output', output_layout, finalize=True, disable_close=True)

    window2.move(window.current_location()[0] + 400, window.current_location()[1])

    # event loop
    while True:
        # integer inputs: duration, power, number of steps, dwell, frequency, start, stop
        # alpha inputs: beaconstring

        if validport:
            event, values = window.read(timeout=100)  # timeout=10 removed
            event2, values2 = window2.read(timeout=100)
        else:
            event, values = window.read()

        formvalid = True  # don't execute command unless form is valid

        # check the config values, unless event is close
        if event != sg.WIN_CLOSED and event != '__TIMEOUT__':

            """
            value = values['power']
            try:
                intvalue = int(value)
                if intvalue < 10 or intvalue > 100:
                    raise RangeError
            except ValueError:
                window2['output'].print('Power must be a valid integer: setting to safe value')
                values['power'] = 10
                window['power'].update(values['power'])
                formvalid = False
            except RangeError:
                window2['output'].print('Power is OUT OF RANGE (10 to 100): setting to safe value')
                values['power'] = 10
                window['power'].update(values['power'])
                formvalid = False
            """

            value = values['duration']
            try:
                intvalue = int(value)
                if intvalue < 1 or intvalue > 60:
                    raise RangeError
            except ValueError:
                window2['output'].print('Duration must be a valid integer: setting to safe value')
                values['duration'] = 1
                window['duration'].update(values['duration'])
                formvalid = False
            except RangeError:
                window2['output'].print('Duration is OUT OF RANGE (1 to 60): setting to safe value')
                values['duration'] = 1
                window['duration'].update(values['duration'])
                formvalid = False

            value = values['integrate']
            try:
                intvalue = int(value)
                if intvalue < 1 or intvalue > 60:
                    raise RangeError
            except ValueError:
                window2['output'].print('Duration must be a valid integer: setting to safe value')
                values['integrate'] = 1
                window['integrate'].update(values['integrate'])
                formvalid = False
            except RangeError:
                window2['output'].print('Duration is OUT OF RANGE (1 to 60): setting to safe value')
                values['integrate'] = 1
                window['integrate'].update(values['integrate'])
                formvalid = False

            # worst case 1000 steps at 1 sec each (max dwell)
            value = values['step']
            try:
                intvalue = int(value)
                if intvalue < 10 or intvalue > 1000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Step Size must be a valid integer: setting to safe value')
                values['step'] = 10
                window['step'].update(values['duration'])
                formvalid = False
            except RangeError:
                window2['output'].print('Step Size is OUT OF RANGE (10 to 1000): setting to safe value')
                values['step'] = 10
                window['step'].update(values['step'])
                formvalid = False

            value = values['dwell']
            try:
                intvalue = int(value)
                if intvalue < 10 or intvalue > 1000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Dwell Time must be a valid integer: setting to safe value')
                values['dwell'] = 10
                window['dwell'].update(values['dwell'])
                formvalid = False
            except RangeError:
                window2['output'].print('Dwell Time is OUT OF RANGE (10 to 1000): setting to safe value')
                values['dwell'] = 10
                window['dwell'].update(values['dwell'])
                formvalid = False

            value = values['frequency']
            try:
                intvalue = int(value)
                if intvalue < 400000000 or intvalue > 525000000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Frequency must be a valid integer: setting to safe value')
                values['frequency'] = 430000000
                window['frequency'].update(values['frequency'])
                formvalid = False
            except RangeError:
                window2['output'].print('Frequency is OUT OF RANGE (400 to 525 MHz): setting to safe value')
                values['frequency'] = 430000000
                window['frequency'].update(values['frequency'])
                formvalid = False

            value = values['start']
            try:
                intvalue = int(value)
                if intvalue < 400000000 or intvalue > 525000000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Start Frequency must be a valid integer: setting to safe value')
                values['start'] = 430000000
                window['start'].update(values['start'])
                formvalid = False
            except RangeError:
                window2['output'].print('Start Frequency is OUT OF RANGE (400 to 525 MHz): setting to safe value')
                values['start'] = 430000000
                window['start'].update(values['start'])
                formvalid = False

            value = values['stop']
            try:
                intvalue = int(value)
                if intvalue < 400000000 or intvalue > 525000000:
                    raise RangeError
                if intvalue < int(values['start']):
                    raise SweepError
            except ValueError:
                window2['output'].print('Stop Frequency must be a valid integer: setting to safe value')
                values['stop'] = 430000000
                window['stop'].update(values['stop'])
                formvalid = False
            except RangeError:
                window2['output'].print('Stop Frequency is OUT OF RANGE (400 to 525 MHz): setting to safe value')
                values['stop'] = 430000000
                window['stop'].update(values['stop'])
                formvalid = False
            except SweepError:
                window2['output'].print('Stop Frequency must be larger than Start Frequency')
                values['stop'] = values['start'] + 100000
                window['stop'].update(values['stop'])
                formvalid = False

            value = values['beaconstring']
            try:
                if not value.isalpha():
                    raise ValueError
            except ValueError:
                window2['output'].print('Beaconstring must be 4 Alpha characters: setting to default value')
                values['beaconstring'] = 'ABCD'
                window['beaconstring'].update(values['beaconstring'])
                formvalid = False

        # print('formvalid: ', formvalid)

        # process button inputs
        if event == sg.WIN_CLOSED:
            if ser.is_open:
                ser.close()
            window2.close()
            break  # exit cleanly
        # print(event)
        # print(values['portname'][0])
        # print(currentport)
        if event == 'portname':  # and values['portname'][0] != currentport:
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

        if formvalid is True and validport is True:
            try:
                if event == 'Send Beacon':
                    window2['output'].print('Beacon sent')
                    beaconcmd = b'\xC0\x07' + bytes(values['beaconstring'], 'utf-8') + b'\xC0'
                    window2['output'].print(beaconcmd)
                    ser.write(beaconcmd)
                elif event == 'Deploy Antenna':
                    window2['output'].print('Antenna deployment started')
                    deployantennacmd = b'\xC0\x08\xC0'
                    window2['output'].print(deployantennacmd)
                    ser.write(deployantennacmd)
                elif event == 'Request Status':
                    window2['output'].print('Status request sent')
                    statuscmd = b'\xC0\x09\xC0'
                    window2['output'].print(statuscmd)
                    ser.write(statuscmd)
                elif event == 'Halt':
                    window2['output'].print('Halt sequence started')
                    haltcmd = b'\xC0\x0A\xC0'
                    window2['output'].print(haltcmd)
                    ser.write(haltcmd)
                elif event == 'Modify Frequency':
                    window2['output'].print('Permanently changing to specified frequency')
                    newfreq = values['frequency'].encode('utf-8')
                    modfreqcmd = b'\xC0\x0B' + newfreq + b'\xC0'
                    window2['output'].print(modfreqcmd)
                    ser.write(modfreqcmd)
                elif event == 'Modify Mode':
                    window2['output'].print('Permanently changing to specified mode index')
                    modmodecmd = b'\xC0\x0C\xC0'
                    window2['output'].print(modmodecmd)
                    ser.write(modmodecmd)
                elif event == 'Adjust Frequency':
                    window2['output'].print('Temporarily changing to specified frequency')
                    adjfreqcmd = b'\xC0\x0D' + values['frequency'].encode('utf-8') + b'\xC0'
                    window2['output'].print(adjfreqcmd)
                    ser.write(adjfreqcmd)
                elif event == 'Transmit Dead Carrier':
                    window2['output'].print('Dead Carrier for ' + values['duration'] + ' seconds')
                    carriercmd = b'\xC0\x17' + values['duration'].rjust(2, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(carriercmd)
                    ser.write(carriercmd)
                elif event == 'Measure Background RSSI':
                    window2['output'].print('Measuring background noise')
                    brssicmd = b'\xC0\x18' + values['integrate'].rjust(2, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(brssicmd)
                    ser.write(brssicmd)
                elif event == 'Measure Current RSSI':
                    window2['output'].print('Measuring current RSSI')
                    rssicmd = b'\xC0\x19\xC0'
                    window2['output'].print(rssicmd)
                    ser.write(rssicmd)
                elif event == 'Sweep Transmitter':
                    window2['output'].print('Starting Tx Sweep')
                    sweeptxcmd = b'\xC0\x1A' + values['start'].encode('utf-8') + \
                                 values['stop'].encode('utf-8') + \
                                 values['step'].rjust(3, "0").encode('utf-8') + \
                                 values['dwell'].rjust(3, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(sweeptxcmd)
                    ser.write(sweeptxcmd)
                elif event == 'Sweep Receiver':
                    window2['output'].print('Starting Rx Sweep')
                    sweeprxcmd = b'\xC0\x1B' + values['start'].encode('utf-8') + \
                                 values['stop'].encode('utf-8') + \
                                 values['step'].rjust(3, "0").encode('utf-8') + \
                                 values['dwell'].rjust(3, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(sweeprxcmd)
                    ser.write(sweeprxcmd)
                elif event == 'Send Bad Command':
                    window2['output'].print('Sending Bad (unsupported) Command')
                    badcmd = b'\xC0\x29\xC0'
                    window2['output'].print(badcmd)
                    ser.write(badcmd)
                elif event == 'Send Test Data Packet Command':
                    window2['output'].print('Sending Test Data Packet')
                    sequence_number += 1
                    if sequence_number == 256:
                        sequence_number = 0
                    sequence_byte = struct.pack('B', sequence_number)
                    print(sequence_byte)
                    # print(type(sequence_byte))
                    data = b''.join([b'\xC0\xAA\x31\x32\x33\x34\x35\x36\x37\x38\x39', sequence_byte])
                    data_cmd = b''.join([data, b'\xC0'])
                    window2['output'].print(data_cmd)
                    ser.write(data_cmd)
                elif event == 'Send Test Remote Command':
                    window2['output'].print('Sending Test Remote Command')
                    # don't forget this is hex! (0x50 = 'P');
                    # also added in AA to stand in for command/data (ax5043) address
                    sequence_byte = b''
                    sequence_number += 1
                    if sequence_number == 256:
                        sequence_number = 0
                    sequence_byte = struct.pack('B', sequence_number)
                    print(sequence_byte)
                    remote_bytes = b'\xC0\xAA\x41\x42\x43\x44\x45\x46\x47\x48\x49\x50'
                    remote_bytes = b''.join([remote_bytes, sequence_byte])
                    remote_cmd = b''.join([remote_bytes, b'\xC0'])
                    window2['output'].print(remote_cmd)
                    ser.write(remote_cmd)

            except serial.serialutil.SerialException:
                print('serial port error')

        if validport:
            serinput = ser.read()
            while ser.in_waiting > 0:
                serinput += ser.read()
            if serinput != b"":
                window2['output'].print(serinput)
            # window.refresh()
    ser.close()
    window.close()
    window2.close()
