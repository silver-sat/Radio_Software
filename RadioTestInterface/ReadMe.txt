This is a simple interface to test the Silversat Radio.

You will need to install PySimpleGUI and PySerial

use

python -m pip install PySimpleGui
python -m pip install pyserial

Run the SSRadioTests.py for the GUI
Run main.py for the "avionics simulator".  That just looks for a packet and sends out a response.  This is only needed
for the commands that require a response.

Tested in Windows 10, Windows 11 (ARM), and Raspberry Pi Buster.
