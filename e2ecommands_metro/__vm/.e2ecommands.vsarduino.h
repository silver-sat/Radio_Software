/* 
	Editor: https://www.visualmicro.com/
			visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			the contents of the _vm sub folder can be deleted prior to publishing a project
			all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Silversat Radio Board (SAMD21), Platform=samd, Package=radio
*/

#define _VMDEBUG 1
#define ARDUINO 108019
#define ARDUINO_MAIN
#define F_CPU 48000000L
#define printf iprintf
#define __SAMD__
#define F_CPU 48000000L
#define ARDUINO 108019
#define ARDUINO_SAMD_ZERO
#define ARDUINO_ARCH_SAMD
#define ARDUINO_SAMD_ADAFRUIT
#define __SAMD21G18A__
#define RADIO_BOARD
#define ARM_MATH_CM0PLUS
#define USB_VID 0x239A
#define USB_PID 0x8013
#define USBCON
#define USB_CONFIG_POWER 100
#include "pins_arduino.h" 
#include "variant.h" 
#include "arduino.h"
#include "e2ecommands.ino"
