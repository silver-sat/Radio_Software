// THIS VERSION USED FOR SILVERSAT MOCK GROUND STATION
// PINS ARE NOT COMPATIBLE WITH SILVERSAT RADIO BOARD
// PINS ASSIGNED TO MATCH AX5043 "HAT" and to assign extra signals for radio board to benign pins (no conflicts)
// LAST CHECKED BY TKC 08/04/2024

/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _VARIANT_ARDUINO_ZERO_
#define _VARIANT_ARDUINO_ZERO_


// The definitions here needs a SAMD core >=1.6.10
#define ARDUINO_SAMD_VARIANT_COMPLIANCE 10610

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

/** Frequency of the board main oscillator */
#define VARIANT_MAINOSC		(32768ul)

/** Master clock frequency */
#define VARIANT_MCK	(F_CPU)

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
#include "SERCOM.h"
#include "Uart.h"
#endif // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Number of pins defined in PinDescription array
#define PINS_COUNT           (26u) //should be 42?  is this used? 
#define NUM_DIGITAL_PINS     (20u) //should be 31?  was 20
#define NUM_ANALOG_INPUTS    (8u) //ADC Channels - at least this is consistent
#define NUM_ANALOG_OUTPUTS   (1u) //I assume it means AREF
#define analogInputToDigitalPin(p)  ((p < 6u) ? (p) + 14u : -1)

#define digitalPinToPort(P)        ( &(PORT->Group[g_APinDescription[P].ulPort]) )
#define digitalPinToBitMask(P)     ( 1 << g_APinDescription[P].ulPin )
//#define analogInPinToBit(P)        ( )
#define portOutputRegister(port)   ( &(port->OUT.reg) )
#define portInputRegister(port)    ( &(port->IN.reg) )
#define portModeRegister(port)     ( &(port->DIR.reg) )
#define digitalPinHasPWM(P)        ( g_APinDescription[P].ulPWMChannel != NOT_ON_PWM || g_APinDescription[P].ulTCChannel != NOT_ON_TIMER )

/*
 * digitalPinToTimer(..) is AVR-specific and is not defined for SAMD
 * architecture. If you need to check if a pin supports PWM you must
 * use digitalPinHasPWM(..).
 *
 * https://github.com/arduino/Arduino/issues/1833
 */
// #define digitalPinToTimer(P)

//mapping reference
/*
 (0u) = PA11 = Serial 1 (Metro D0)
 (1u) = PA10 = Serial 1 (Metro D1)
 (2u) = PA14 = SYSCLK (Metro D2)
 (3u) = PA09 = PAENABLE (Metro D3)
 (4u) = PA08 = OC5V (Metro D4)
 (5u) = PA15 = AX5043_DATA (Metro D5)
 (6u) = PA20 = AX5043_DCLK (Metro D6)
 (7u) = PA21 = WDTICK (Metro D7)
 (8u) = PA06 = SELBAR (Metro D8)
 (9u) = PA07 = Reset_5V  D9
(10u) = PA18 = **unused**  D10
(11u) = PA16 = Serial 0 (Metro D11)
(12u) = PA19 = IRQ (not completely sure it's connected on AX5043 HAT) (Metro D12)
(13u) = PA17 = Serial 0 (Metro D13)
(14u) = PA02 = Release_A (Metro A0)
(15u) = PB08 = Release_B (Metro A1)
(16u) = PB09 = EN0 (Metro A2)
(17u) = PA04 = EN1 (Metro A3)
(18u) = PA05 = Current_5V (Metro A4)
(19u) = PB02 = A5 - **unused**
(20u) = PA11 = A6 - but it's also D0
(21u) = PA10 = A7 - but it's also D1 
(22u) = PA08 = DUPLICATE to (4u)
(23u) = PA09 = DUPLICATE to (3u)
(24u) = PA06 = DUPLICATE to (8u)
(25u) = PA07 = DUPLICATE to (9u)
(26u) = PA22 = PIN_WIRE_SDA
(27u) = PA23 = PIN_WIRE_SCL
(28u) = PA12 = MISO
(29u) = PB10 = MOSI
(30u) = PB11 = SCK
(31u) = PA31 = RX_TX (Metro RXLED/SWDIO)
(32u) = PA27 = TX_RX (Metro TXLED)
(33u) = PA28 = USB Host Enable 
(34u) = PA24 = USB D-
(35u) = PA25 = USB D+
(36u) = PB03 = FLASH_MISO
(37u) = PB22 = FLASH_MOSI
(38u) = PB23 = FLASH SCK
(39u) = PA13 = FLASH_CS
(40u) = PA30 = NEOPIX
(41u) = PA03 = AREF
(42u) = PA02 = A0  DUPLICATE to (14)
*/

// LEDs
#define PIN_LED_13           (13u)
#define PIN_LED_RXL          (31u)
#define PIN_LED_TX           (32u)
#define PIN_LED              PIN_LED_13
#define PIN_LED2             PIN_LED_RXL
#define PIN_LED3             PIN_LED_TXL
#define LED_BUILTIN          PIN_LED_13
#define NEOPIXEL_BUILTIN     (40u)
#define PIN_NEOPIXEL         NEOPIXEL_BUILTIN

/*
 * GPIO pins
*/
#define GPIO0        (17u)
#define GPIO1        (18u)
#define GPIO2         (8u)
#define GPIO3         (7u)
#define GPIO4         (2u)
#define GPIO5         (5u)
#define GPIO6        (39u)
#define GPIO7        (10u)
#define GPIO8        (12u)
#define GPIO9        (19u)
#define GPIO10       (36u)
#define GPIO11       (37u)
#define GPIO12       (16u)
#define GPIO13       (15u)
#define GPIO14_A     (14u)

/*
 * GPIO pin meaning (board function)
 * Serial (debug) is USB serial, Serial0 (avionics) are Pins D11 (PA16)(11u) and D13 (PA17)(13u), Serial 1 are Pins D0 (PA11)(0u) and D1 (PA10)(1u)
*/
#define Release_B   (15u)  //this doesn't exist in ground station  PB08   A1 on Metro
#define Release_A   (14u)  //this doesn't exist in ground station  PA02   A0 on Metro
#define Current_5V  (18u)  //this doesn't exist in ground station  PA05  A4 on Metro.  This needs an ADC channel
#define TX_RX       (32u)  //this doesn't exist in ground station  PA27 == moved to TX LED
#define RX_TX       (31u)  //this doesn't exist in ground station  PA31  == moved to RX LED  Also SWDIO
#define PAENABLE    (3u)  // controls pwr switch to PA, but not present in dev ground station 
#define EN0         (16u)  //this doesn't exist in ground station  PB9 - A2 on Metro
#define EN1         (17u)  //this doesn't exist in ground station  PA04 - A3 on Metro
#define AX5043_DCLK (6u)   // output from AX5043  PA20 - shared with Serial2 - D6 on Metro  (serial 2 unused)
#define AX5043_DATA (5u)   // output from AX5043  PA15 - D5 on Metro
//#define OC3V3     (22u)  //this doesn't exist in ground station - also eliminated on Silversat board
#define OC5V        (4u)  //this doesn't exist in ground station  PA08 - input  D4 on Metro
#define SYSCLK      (2u)  // output from AX5043  PA14 - D2 on Metro
#define SELBAR      (8u)   // input to AX5043:  PA06 - D8 on Metro
#define IRQ         (12u)   // output from AX5043: PA19 -   D12 on Metro
#define WDTICK      (7u)  //flash sclk on the metro.  This doesn't exist in ground station. PA21 - D7
#define Reset_5V    (9u)  //this also doesn't exist in the ground station PA08  Also used for OC5V? also an input 

/*
 * Analog pins - None of these are used in the Silversat code
 */
#define PIN_A0               (14ul)
#define PIN_A1               (PIN_A0 + 1)
#define PIN_A2               (PIN_A0 + 2)
#define PIN_A3               (PIN_A0 + 3)
#define PIN_A4               (PIN_A0 + 4)
#define PIN_A5               (PIN_A0 + 5)
#define PIN_A6               (PIN_A0 + 6)
#define PIN_A7               (PIN_A0 + 7)
#define PIN_A8               (PIN_A0 + 8)
#define PIN_A9               (PIN_A0 + 9)
#define PIN_A10              (PIN_A0 + 10)
#define PIN_A11              (PIN_A0 + 11)
#define PIN_DAC0             (14ul)

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6 ;
static const uint8_t A7  = PIN_A7 ;
static const uint8_t A8  = PIN_A8 ;
static const uint8_t A9  = PIN_A9 ;
static const uint8_t A10 = PIN_A10 ;
static const uint8_t A11 = PIN_A11 ;

static const uint8_t DAC0 = PIN_DAC0;

#define ADC_RESOLUTION		12

// Other pins
#define PIN_ATN              (38ul)
static const uint8_t ATN = PIN_ATN;

// On-board SPI Flash
#define EXTERNAL_FLASH_DEVICES  GD25Q16C
#define EXTERNAL_FLASH_USE_SPI  SPI1
#define EXTERNAL_FLASH_USE_CS   SS1

/*
 * Serial interfaces
 */

// Serial1
#define PIN_SERIAL1_RX       (0ul)
#define PIN_SERIAL1_TX       (1ul)
#define PAD_SERIAL1_TX       (UART_TX_PAD_2)
#define PAD_SERIAL1_RX       (SERCOM_RX_PAD_3)

// Serial0
#define PIN_SERIAL0_RX       (11ul)
#define PIN_SERIAL0_TX       (13ul)
#define PAD_SERIAL0_TX       (UART_TX_PAD_0)
#define PAD_SERIAL0_RX       (SERCOM_RX_PAD_1)

// Serial2
#define PIN_SERIAL2_RX       (6ul)
#define PIN_SERIAL2_TX       (7ul)
#define PAD_SERIAL2_TX       (UART_TX_PAD_2)
#define PAD_SERIAL2_RX       (SERCOM_RX_PAD_3)


/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO         (28u)
#define PIN_SPI_MOSI         (29u)
#define PIN_SPI_SCK          (30u)
#define PERIPH_SPI           sercom4
#define PAD_SPI_TX           SPI_PAD_2_SCK_3
#define PAD_SPI_RX           SERCOM_RX_PAD_0

static const uint8_t SS	  = PIN_A2 ;	// SERCOM4 last PAD is present on A2 but HW SS isn't used. Set here only for reference.
static const uint8_t MOSI = PIN_SPI_MOSI ;
static const uint8_t MISO = PIN_SPI_MISO ;
static const uint8_t SCK  = PIN_SPI_SCK ;

/* Removing second SPI interface to make it available for UART
#define PIN_SPI1_MISO         (36u)
#define PIN_SPI1_MOSI         (37u)
#define PIN_SPI1_SCK          (38u)
#define PERIPH_SPI1           sercom5
#define PAD_SPI1_TX           SPI_PAD_2_SCK_3
#define PAD_SPI1_RX           SERCOM_RX_PAD_1

static const uint8_t SS1   = 39 ;	// HW SS isn't used. Set here only for reference.
static const uint8_t MOSI1 = PIN_SPI_MOSI ;
static const uint8_t MISO1 = PIN_SPI_MISO ;
static const uint8_t SCK1  = PIN_SPI_SCK ;
*/

/*
 * Wire Interfaces
 */
#define WIRE_INTERFACES_COUNT 2

#define PIN_WIRE_SDA         (26u)
#define PIN_WIRE_SCL         (27u)
#define PERIPH_WIRE          sercom3
#define WIRE_IT_HANDLER      SERCOM3_Handler

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

//define the second wire interface...see Wire.cpp
#define PIN_WIRE1_SDA        (4u)
#define PIN_WIRE1_SCL        (3u)
#define PERIPH_WIRE1         sercom2

static const uint8_t SDA1 = PIN_WIRE1_SDA;
static const uint8_t SCL1 = PIN_WIRE1_SCL;


/*
 * USB
 */
#define PIN_USB_HOST_ENABLE (33ul)
#define PIN_USB_DM          (34ul)
#define PIN_USB_DP          (35ul)

/*
 * I2S Interfaces
 */
#define I2S_INTERFACES_COUNT 0
/*
#define I2S_DEVICE          0
#define I2S_CLOCK_GENERATOR 3
#define PIN_I2S_SD          (9u)
#define PIN_I2S_SCK         (1u)
#define PIN_I2S_FS          (0u)
*/
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus

/*	=========================
 *	===== SERCOM DEFINITION
 *	=========================
*/
extern SERCOM sercom0;
extern SERCOM sercom1;
extern SERCOM sercom2;
extern SERCOM sercom3;
extern SERCOM sercom4;
extern SERCOM sercom5;

extern Uart Serial5;
extern Uart Serial2;
extern Uart Serial1;
extern Uart Serial0;

#endif

// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_USBVIRTUAL      Serial
#define SERIAL_PORT_MONITOR         Serial
// Serial has no physical pins broken out, so it's not listed as HARDWARE port
#define SERIAL_PORT_HARDWARE        Serial1
#define SERIAL_PORT_HARDWARE_OPEN   Serial1

#endif /* _VARIANT_ARDUINO_ZERO_ */

