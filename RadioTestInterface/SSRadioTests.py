# Test GUI

import PySimpleGUI as sg
import serial
from serial.tools import list_ports
import struct
from random import randbytes
from time import sleep

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


def packetsend(serial_port, quantity):
    packet_start = b'\xC0\xAA'
    packet_finish = b'\xC0'
    debug = False
    inter_packet_delay = 0.45  # 200 mS between packets
    # packets are 200 (might vary) bytes long, plus one destination byte, 4 bytes for frame delimiter, and 9 0xAA's
    # and 2 CRC bytes
    # so figure 207 bytes at 9600 baud or about 180 mS.  The interface is running at 57600, so you don't want to overrun
    # the radio.  So if it spits out one every 200 mS, it should be okay...could look at this on a scope to get it finer
    # this begs the questions, is it possible for the RPi to overrun the UART when its running at 19200?
    packet_payload = randbytes(18)  # allow 4 bytes for sequence number (integer)
    debug and print(packet_payload)
    debug and print(len(packet_payload))

    for seq_num in range(quantity):
        sequence_number_bytes = struct.pack(">I", seq_num)
        sequenced_packet_payload = b''.join([sequence_number_bytes, packet_payload])
        kiss_packet_payload = kissenc(sequenced_packet_payload)
        packet = b''.join([packet_start, kiss_packet_payload])
        packet = b''.join([packet, packet_finish])
        debug and print(packet)
        serial_port.write(packet)
        sleep(inter_packet_delay)


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
                ser = serial.Serial(port[0], baudrate=19200, timeout=0, write_timeout=2)
                validport = True
                currentportname = port
                currentport = port[0]
                break
            except serial.serialutil.SerialException:
                print('Serial port is in use')
                # window2['output'].print('Serial port is in use')

    modulation_mode_layout = [[sg.Radio("GMSK 4800 IL2P", "RADIO1", key='fsk'),
                               sg.Radio("GMSK 9600 HDLC", "RADIO1", key='gmsk'),
                               sg.Radio("GMSK 9600 IL2P", "RADIO1", key='fec', default=True)]]

    radio_config_layout = [[sg.Text('Serial Port', size=15),
                            sg.Spin(ports, size=30, key='portname',
                                    enable_events=True, initial_value=currentportname)],
                           [sg.Text('Frequency (TX) (Hz)', size=15),
                            sg.InputText("437175000", key='frequency', size=10, justification='center')],
                           [sg.Text('Frequency (RX) (Hz)', size=15),
                            sg.InputText("437175000", key='frequency2', size=10, justification='center')],
                           [sg.Text('Power level (0-10)', size=15),
                            sg.InputText("5", key='power', size=4, justification='center')],
                           [sg.Text('CCA Threshold', size=15),
                            sg.InputText("180", key='threshold', size=4, justification='center')],
                           [sg.Push(), sg.Frame('Modulation Mode', modulation_mode_layout), sg.Push()]]

    radio_config_button_layout = [[sg.Button('Modify Frequency', size=30)],
                                  [sg.Button('Apply Doppler Offset', size=30)],
                                  [sg.Button('Adjust Output Power', size=30)],
                                  [sg.Button('Modify Mode', size=30)],
                                  [sg.Button('Modify CCA Threshold', size=30)],
                                  [sg.Button('Request Status', size=30, button_color='red')]]

    antenna_release_layout = [[sg.Radio("Release_A", "RADIO2", key='relA', default=True),
                               sg.Push(), sg.Radio("Release_B", "RADIO2", key='relB'),
                               sg.Push(), sg.Radio("Both", "RADIO2", key='both')]]

    functional_test_layout = [[sg.Text('Beacon String', size=22),
                               sg.InputText("ABC", key='beaconstring', size=6, justification='center')],
                              [sg.Push(), sg.Frame('Antenna Release Select', antenna_release_layout), sg.Push()]
                              ]

    functional_test_button_layout = [[sg.Button('Send Beacon', size=30)],
                                     [sg.Button('Deploy Antenna', size=30)],
                                     [sg.Button('Send Callsign', size=30)],
                                     [sg.Button('RESET', size=30)]]

    packet_test_layout = [[sg.Text('Packet Quantity (10000 max)', size=22),
                           sg.InputText("10", key='packet_quantity', size=6, justification='center')]]

    packet_test_button_layout = [[sg.Button('Send Bad Command', size=30)],
                                 [sg.Button('Send Test Data Packet Command', size=30)],
                                 [sg.Button('Send Test Remote Command', size=30)],
                                 [sg.Button('Send random-string packets', size=30)],
                                 [sg.Button('Print Stats', size=30)]]
                                 #[sg.Button('Send File via FTP', size=30)]]

    radio_test_layout = [[sg.Text('Tx Duration (Seconds)', size=22), sg.Push(),
                          sg.InputText("10", key='duration', size=3, justification='center')],
                         [sg.Text('Rx Integration Time (Seconds)', size=22), sg.Push(),
                          sg.InputText("30", key='integrate', size=3, justification='center')],
                         [sg.Text('Start Frequency (Hz)', size=20), sg.Push(),
                          sg.InputText("430000000", size=10, key='start', justification='center')],
                         [sg.Text('Stop Frequency (Hz)', size=20), sg.Push(),
                          sg.InputText("440000000", size=10, key='stop', justification='center')],
                         [sg.Text('Number of Steps', size=20), sg.Push(),
                          sg.InputText("10", size=5, key='step', justification='center')],
                         [sg.Text('Dwell Time (mSec)', size=20), sg.Push(),
                          sg.InputText("999", size=6, key='dwell', justification='center')],
                         [sg.Text('Register Address', size=22), sg.Push(),
                          sg.InputText("000", key='register', size=6, justification='center')]]

    radio_test_button_layout = [[sg.Button('Transmit Dead Carrier', size=30)],
                                       [sg.Button('Measure Current RSSI', size=30)],
                                       [sg.Button('Measure Background RSSI', size=30)],
                                       [sg.Button('Sweep Transmitter', size=30)],
                                       [sg.Button('Sweep Receiver', size=30)],
                                       [sg.Button('Query AX5043 Register', size=30)]]

    output_layout = [
        [sg.Frame('Serial Output', [[sg.Multiline(default_text="Serial Output goes here \r\n", key='output',
                  write_only=True, size=(400, 400), autoscroll=True)]], size=(440, 440))]]

    radio_layout = [radio_config_layout, radio_config_button_layout]
    radio_test = [radio_test_layout, radio_test_button_layout]
    function_layout = [functional_test_layout, functional_test_button_layout]
    packet_Layout = [packet_test_layout, packet_test_button_layout]

    main_layout = [[sg.Frame('Radio Config', radio_config_layout, expand_x=True, expand_y=True),
                     sg.Frame('Radio Config Commands', radio_config_button_layout)],
                    [sg.Frame('Radio Test Config', radio_test_layout, expand_x=True, expand_y=True),
                     sg.Push(),
                     sg.Frame('Radio Tests', radio_test_button_layout)
                     ]]

    secondary_layout = [[sg.Frame('Function Config', functional_test_layout),
                         sg.Frame('Function Tests', functional_test_button_layout)],
                     [sg.Frame('Packet Test Config', packet_test_layout),
                      sg.Push(),
                      sg.Frame('Packet Interface Tests', packet_test_button_layout)
                     ]]

    window = sg.Window('Silversat Radio Tests', main_layout, finalize=True, location=(25, 50))
    window3 = sg.Window('Silversat Radio Functional Tests', secondary_layout, finalize=True, location=(775, 50))
    window2 = sg.Window('Radio Output', output_layout, finalize=True, disable_close=True)

    window2.move(window.current_location()[0] + 775, window.current_location()[1])

    # defaults
    #qty_value = 10

    # event loop
    while True:
        # integer inputs: duration, power, number of steps, dwell, frequency, start, stop
        # alpha inputs: beaconstring

        if validport:
            event, values = window.read(timeout=100)  # timeout=10 removed
            event2, values2 = window2.read(timeout=100)
            event3, values3 = window3.read(timeout=100)
        else:
            event, values = window.read()
            event3, value3 = window3.read()

        formvalid = True  # don't execute command unless form is valid

        # check the config values, unless event is close
        if event != sg.WIN_CLOSED and event != '__TIMEOUT__':
            #print("validating windows")
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
                
            value = values['threshold']
            try:
                intvalue = int(value)
                if intvalue < 1 or intvalue > 255:
                    raise RangeError
            except ValueError:
                window2['output'].print('Threshold level must be a valid integer between > 1')
                values['threshold'] = 180
                window['threshold'].update(values['threshold'])
                formvalid = False
            except RangeError:
                window2['output'].print('Power level must be between 1 and 255')
                window['threshold'].update(values['threshold'])
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

            value = values['frequency2']
            try:
                intvalue = int(value)
                if intvalue < 400000000 or intvalue > 525000000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Frequency must be a valid integer: setting to safe value')
                values['frequency2'] = 430000000
                window['frequency2'].update(values['frequency2'])
                formvalid = False
            except RangeError:
                window2['output'].print('Frequency is OUT OF RANGE (400 to 525 MHz): setting to safe value')
                values['frequency2'] = 430000000
                window['frequency2'].update(values['frequency2'])
                formvalid = False

            value = values['power']
            try:
                intvalue = int(value)
                if intvalue < 1 or intvalue > 10:
                    raise RangeError
            except ValueError:
                window2['output'].print('Power level must be a valid integer between 1 and 10')
                values['power'] = 10
                window['power'].update(values['power'])
                formvalid = False
            except RangeError:
                window2['output'].print('Power level must be between 1 and 10')
                window['power'].update(values['power'])
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

            value = values3['beaconstring']
            try:
                if not value.isalpha():
                    raise ValueError
                if len(value) != 3:
                    raise ValueError
            except ValueError:
                window2['output'].print('Beaconstring must be 3 Alpha characters: setting to default value')
                values['beaconstring'] = 'ABC'
                window3['beaconstring'].update(values['beaconstring'])
                formvalid = False

            value = values3['packet_quantity']
            print(value)
            try:
                qty_value = int(value)
                if qty_value < 1 or qty_value > 10000:
                    raise RangeError
            except ValueError:
                window2['output'].print('Quantity must be a valid integer')
                values3['packet_quantity'] = 1
                window3['packet_quantity'].update(values3['packet_quantity'])
                formvalid = False
            except RangeError:
                window2['output'].print('Quantity must be between 1 and 10000')
                window3['packet_quantity'].update(values3['packet_quantity'])
                formvalid = False

            value = values['register']
            try:
                register = int(value, 16)
                if register < 0 or register > 0xFFF:
                    raise RangeError
            except ValueError:
                window2['output'].print('Quantity must be a valid integer')
                values['register'] = 0
                window['register'].update(values['register'])
                formvalid = False
            except RangeError:
                window2['output'].print('Register must be between 0x000 and 0xFFF')
                window['register'].update(values['register'])
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
                if event3 == 'Send Beacon':
                    window2['output'].print('Beacon sent')
                    beaconcmd = b'\xC0\x07' + bytes(values3['beaconstring'], 'utf-8') + b'\xC0'
                    window2['output'].print(beaconcmd)
                    ser.write(beaconcmd)
                elif event3 == 'Deploy Antenna':
                    window2['output'].print('Antenna deployment started')
                    if values3['relA']:
                        deployantennacmd = b'\xC0\x08\x41\xC0'
                    if values3['relB']:
                        deployantennacmd = b'\xC0\x08\x42\xC0'
                    if values3['both']:
                        deployantennacmd = b'\xC0\x08\x43\xC0'
                    window2['output'].print(deployantennacmd)
                    ser.write(deployantennacmd)
                elif event == "Modify CCA Threshold":
                    window2['output'].print('Threshold modified, send twice to make permanent')
                    newthresh = values['threshold'].encode('utf-8')
                    threshcmd = b'\xC0\x1F' + newthresh + b'\xC0'
                    window2['output'].print(threshcmd)
                    ser.write(threshcmd)
                elif event == 'Request Status':
                    window2['output'].print('Status request sent')
                    statuscmd = b'\xC0\x09\xC0'
                    window2['output'].print(statuscmd)
                    ser.write(statuscmd)
                elif event3 == 'RESET':
                    window2['output'].print('Halt sequence started')
                    haltcmd = b'\xC0\x0A\xC0'
                    window2['output'].print(haltcmd)
                    ser.write(haltcmd)
                elif event3 == "Print Stats":
                    window2['output'].print('Stats printed to debug')
                    statscmd = b'\xC0\x1E\xC0'
                    window2['output'].print(statscmd)
                    ser.write(statscmd)
                elif event == 'Modify Frequency':
                    window2['output'].print('Permanently changing to specified frequency')
                    newfreq = values['frequency'].encode('utf-8')
                    modfreqcmd = b'\xC0\x0B' + newfreq + b'\xC0'
                    window2['output'].print(modfreqcmd)
                    ser.write(modfreqcmd)
                elif event == 'Modify Mode':
                    window2['output'].print('Changing to specified mode')
                    if values['fsk']:
                        modmodecmd = b'\xC0\x0C\x00\xC0'
                    elif values['gmsk']:
                        modmodecmd = b'\xC0\x0C\x01\xC0'
                    elif values['fec']:
                        modmodecmd = b'\xC0\x0C\x02\xC0'
                    window2['output'].print(modmodecmd)
                    ser.write(modmodecmd)
                elif event == 'Apply Doppler Offset':
                    window2['output'].print('Applying doppler compensation')
                    txfreq = values['frequency'].encode('utf-8')
                    rxfreq = values['frequency2'].encode('utf-8')
                    dopplercmd = b'\xC0\x0D' + txfreq + b'\x20' + rxfreq + b'\xC0'
                    window2['output'].print(dopplercmd)
                    ser.write(dopplercmd)
                elif event3 == 'Send Callsign':
                    window2['output'].print('Sending the Call Sign')
                    sendcallsigncmd = b'\xC0\x0E\xC0'
                    window2['output'].print(sendcallsigncmd)
                    ser.write(sendcallsigncmd)
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
                                 b'\x20' + values['stop'].encode('utf-8') + \
                                 b'\x20' + values['step'].rjust(3, "0").encode('utf-8') + \
                                 b'\x20' + values['dwell'].rjust(3, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(sweeptxcmd)
                    ser.write(sweeptxcmd)
                elif event == 'Sweep Receiver':
                    window2['output'].print('Starting Rx Sweep')
                    sweeprxcmd = b'\xC0\x1B' + values['start'].encode('utf-8') + \
                                 b'\x20' + values['stop'].encode('utf-8') + \
                                 b'\x20' + values['step'].rjust(3, "0").encode('utf-8') + \
                                 b'\x20' + values['dwell'].rjust(3, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(sweeprxcmd)
                    ser.write(sweeprxcmd)
                elif event3 == 'Send Bad Command':
                    window2['output'].print('Sending Bad (unsupported) Command')
                    badcmd = b'\xC0\x05\xC0'
                    window2['output'].print(badcmd)
                    ser.write(badcmd)
                elif event3 == 'Send Test Data Packet Command':
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
                elif event3 == 'Send Test Remote Command':
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
                elif event == 'Adjust Output Power':
                    window2['output'].print('Adjusting Output Power')
                    decimal_integer = int(values['power'])
                    print(decimal_integer)
                    hex_decimal_integer = hex(decimal_integer)
                    print(type(hex_decimal_integer))
                    adjpwrcmd = b'\xC0\x1D' + decimal_integer.to_bytes(1,'big') + b'\xC0'
                    window2['output'].print(adjpwrcmd)
                    ser.write(adjpwrcmd)
                elif event3 == 'Send random-string packets':
                    # this differs from other commands in that a command string is not being sent, just packets
                    # qty_value=1
                    window2['output'].print('Sending a bunch of packets')
                    qty_value = int(values3['packet_quantity'])
                    window2['output'].print(qty_value)
                    packetsend(ser, qty_value)
                    window2['output'].print('Send complete')
                elif event == 'Query AX5043 Register':
                    window2['output'].print('Looking up register value')
                    print(register)
                    register_string = b'\xC0\x1C' + values['register'].rjust(3, "0").encode('utf-8') + b'\xC0'
                    window2['output'].print(register_string)
                    ser.write(register_string)

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
