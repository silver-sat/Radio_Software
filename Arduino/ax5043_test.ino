/*
    Name:       ax5043_test.ino
    Created:  3/30/2022 8:23:30 AM
    Author:     tom-PC\tom
*/

//this is derived from the ax5043_test example, written for RPi/Python, and ported for Arduino

#define _AX_TX_DIFF  //defines that we're using the differential tx output, not the single ended one 

//set this for transmit testing
//#define TX_ONLY

//set this to be the loopback receiver
//#define LOOPBACK

//set this to be the loopback transmitter
#define LOOPBACK_TRANSMITTER

// define DEBUG to enable debug output.  Code uses printf, so include LibPrintf to get that to output to Serial, or at least try
#define DEBUG

#include "ax.h"
#include "ax_fifo.h"
#include "ax_hw.h"
#include "ax_modes.h"
#include "ax_params.h"
#include "ax_reg.h"
#include "ax_reg_values.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <SPI.h>
#include <LibPrintf.h>

const int interrupt = 9;  //arduino pin for packet interrupt (for larger packets)
const int selectChip = 8; //SPI select pin

const int timeout = 1000;  //timeout on receive of packet response
const int testpackets = 10;  //number of test packets to send
const int loopdelay = 1;  //milliseconds to pause between packets; this is to throttle the data for testing

ax_packet rx_pkt;  //instance of packet structure
uint8_t tx_pkt[0x100];  //array of tx characters

ax_config config;

//these not used yet...
uint8_t ax25_frame[0x100];  //ax.25 frame array
#define AX25_CONTROL_WORD   0x03 /* Use Unnumbered Information (UI) frames */
#define AX25_PROTOCOL_ID    0xF0 /* No third level protocol */


// The setup() function runs once each time the micro-controller starts
void setup()
{
Serial.begin(57600);
#ifndef LOOPBACK

while (!Serial){ //trap so that I can watch packet setup over SPI...this can be removed later.
  ;
}

#endif

Serial.println("starting up");

//setup and start SPI
SPI.begin();
SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));  //these settings seem to work, but not optimized

//setup IO
pinMode(interrupt, INPUT); 
pinMode(selectChip, OUTPUT);

//create an array in memory to hold the ax5043 configuration
memset(&config, 0, sizeof(ax_config));

// ------- init ------- 
//the headings are the main parts of the structure, unless changed, they are loaded with defaults
  /* power mode */
//not needed to be set just yet! 

  /* spi transfer */
config.spi_transfer = wiring_spi_transfer;  //define the SPI handler

  /* external clock */
config.clock_source = AX_CLOCK_SOURCE_TCXO;  //our text board has a TCXO...we should too.
config.f_xtal = 48000000;

  /* synthesiser */
config.synthesiser.vco_type = AX_VCO_INTERNAL; //note: I added this to try to match the DVK
config.synthesiser.A.frequency = 433000000;
config.synthesiser.B.frequency = 433000000;
  
  /* transmit path */
//default is differential; needs to be single ended for external PA; NOTE: this is also a #define, this needs to be tested
  
  /* receive */
config.pkt_store_flags = AX_PKT_STORE_RSSI | AX_PKT_STORE_RF_OFFSET;  //search on "AX_PKT_STORE" for other options, only data rate offset is implemented
  
  /* wakeup */
//for WOR, we're not using
  
  /* digitial to analogue (DAC) channel */
//not needed 
  
  /* pll vco */
//frequency range of vco; see ax_set_pll_parameters 

ax_init(&config);  //this does a reset, so probably needs to be first

//load the RF parameters
ax_default_params(&config, &fsk_modulation);  //ax_modes.c for RF parameters

//ax_set_pinfunc_pwramp() // default should be okay "0110" on reset, which should be output pwr amp control

//parrot back what we set
//Serial.print("location of config: ");Serial.println((unsigned int)&config, HEX);  //note the cast to allow printing of pointer address! for library testing

Serial.println("config variable values: "); 
Serial.print("tcxo frequency: ");Serial.println(config.f_xtal);
Serial.print("synthesizer A frequency: "); Serial.println(config.synthesiser.A.frequency);
Serial.print("synthesizer B frequency: "); Serial.println(config.synthesiser.B.frequency);

Serial.print("status: ");Serial.println(ax_hw_status(), HEX);
}

// Add the main program code into the continuous loop() function
void loop()
{
	
#ifdef TX_ONLY
    /* -------- tx -------- */
	
    ax_tx_on(&config, &fsk_modulation);
    while (1) {
        strcpy((char*)tx_pkt, "1234567");
        ax_tx_packet(&config, &fsk_modulation, tx_pkt, 7);  //make sure to modify the length!!
        delay(2000);
        }
#endif
	
#ifdef LOOPBACK
	ax_rx_on(&config, &fsk_modulation);
	int loopbackcount = 0;
	while (1) {		
		while (ax_rx_packet(&config, &rx_pkt)){
			loopbackcount++;
			Serial.print("rx! "); Serial.println(loopbackcount);
			Serial.print("packet length: ");Serial.println(rx_pkt.length);
			for (int i=0; i < rx_pkt.length; i++){
				Serial.print(*(rx_pkt.data + i));	
			}
			Serial.println();
			ax_off(&config);  //turn the radio off to switch from rx to tx (and vice-versa)...also see errata
			ax_tx_on(&config, &fsk_modulation);
			//delay(5);  //guard interval to allow transmitter to switch modes
			ax_tx_packet(&config, &fsk_modulation, rx_pkt.data+1, rx_pkt.length-1);  //move the pointer up by one to drop the old length byte and drop the length back to the old length (so you're looping back the same data); watch for longer packets..assumption is that length is one byte
			ax_off(&config);
			ax_rx_on(&config, &fsk_modulation);
		}
	}
#endif

#ifdef LOOPBACK_TRANSMITTER
	int loopbacktimerstart = millis();
	int receivedbytes = 0;
	int transmittedbytes = 0;
	for (int packetcount = 0; packetcount < testpackets; packetcount++){
		int starttime = millis();
		ax_tx_on(&config, &fsk_modulation);
		strcpy((char*)tx_pkt, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");  //copy the c-string into tx_pkt
		uint8_t tx_pkt_length = strlen((char*)tx_pkt); //calculate the length; including length byte (+1)
		transmittedbytes += tx_pkt_length;
		/*
		uint8_t tx_comp[0x102]; 
		*tx_comp = tx_pkt_length;
		strcpy(tx_comp+1, (char*)tx_pkt);  //followed by the data
		*/
		ax_tx_packet(&config, &fsk_modulation, tx_pkt, tx_pkt_length);  //now send it
		int switchtime = millis();
		ax_off(&config);	//might need a guard interval, but right now assuming that all the state changing is enough; no return value (void)
		Serial.print("Time to transmit: "); Serial.print(millis() - starttime); Serial.println(" mS");
		starttime = millis();
		Serial.println("Switching to receive");
		ax_rx_on(&config, &fsk_modulation);  //turn on the receiver and listen
		Serial.print("Time to switch: "); Serial.print(millis() - switchtime); Serial.println(" mS");
		int responsetimer = millis();
		while ((ax_rx_packet(&config, &rx_pkt) != 1) && (millis() - responsetimer < timeout)) { //wait for a packet to be received or timer to be exceeded
		}
		if (millis() - responsetimer > timeout) {  //check which one happened
			Serial.println("timout exceeded...moving on");
		}
		else {  //must be a packet!
			Serial.println("rx!");
			/*
			Serial.print("packet length: ");Serial.println(rx_pkt.length);
			for (int i=0; i < rx_pkt.length; i++){
				Serial.print(*(rx_pkt.data + i));
			}
			Serial.println();
			Serial.println((char*)rx_pkt.data);
			Serial.println((char*)tx_pkt);
			Serial.println();
			for (int j=0; j<strlen((char*)tx_pkt);j++) {
				Serial.println(*(tx_pkt+j));
				Serial.println(*(rx_pkt.data+j+1));
			}
			Serial.println();
			*/
			Serial.print("Data length: "); Serial.println(rx_pkt.length);
			receivedbytes += rx_pkt.length; 
			//now compare the received data byte for byte
			for (int i=0; i< rx_pkt.length; i++) {
				if (*(rx_pkt.data+i+1) != *(tx_pkt+i)) {  //compare the data, dropping the length byte from the rx packet
					Serial.println("loopback error");
				}			
			}
			Serial.println("loopback check completed");
		}	
		Serial.println();
		Serial.println("Switching to transmit");
		ax_off(&config);
		Serial.print("Time to receive: "); Serial.print(millis() - starttime); Serial.println(" mS");
		delay(loopdelay);
	}
	int loopbacktimertotal = millis() - loopbacktimerstart;
	Serial.print("this test took "); Serial.print(loopbacktimertotal); Serial.println(" ms");
	Serial.print(receivedbytes); Serial.println(" bytes were received");
	Serial.print(transmittedbytes); Serial.println(" bytes were transmitted");
	Serial.print("the effective rate was: "); Serial.print(float((receivedbytes+transmittedbytes)*8*1000/loopbacktimertotal)); Serial.println(" bits/second");
	Serial.println("reset to run again");
	
#endif
	
    /* -------- rx -------- */    
    ax_rx_on(&config, &fsk_modulation);
	//int rx_total = 0;
    while (1) {
        while (ax_rx_packet(&config, &rx_pkt)) {
            Serial.println("rx!");
			for (int i=0; i < rx_pkt.length; i++){
				Serial.println(*(rx_pkt.data + i));
			}	
			Serial.println();
			//rx_total = rx_total + rx_pkt.length;
			//printf("received %u bytes in total", rx_total);
        }
    } 
}

void wiring_spi_transfer(unsigned char* data, uint8_t length)
{
  digitalWrite(selectChip, LOW);  //select
  SPI.transfer(data, length); //do the transfer
  digitalWrite(selectChip, HIGH);  //deselect
}

//callback not used yet
void rx_callback(unsigned char* data, uint8_t length)
{
    //Serial.println("Rx: %s\r\n", (char*)data);
    Serial.print("Rx: ");
	for (int i=0;i<length;i++)
		{
		Serial.print(char(*(data + i)));
		}
	Serial.println();
}


//aprs not used yet
int aprs(void) {
    char addresses[50];
    char information[50];
    uint32_t i = 0;
    uint16_t fcs;

    /* Encode the destination / source / path addresses */
    //uint32_t addresses_len = sprintf(addresses, "%-6s%c%-6s%c%-6s%c",
    uint32_t addresses_len = sprintf(addresses, "%-6s%c%-6s%c%-6s%c",
    "APRS", 0,
    "Q0QQQ", 2,
    "WIDE2", 1);
    uint32_t information_len = 5;
    strcpy(information, "HELLO");

    /* Process addresses */
    for (i = 0; i < addresses_len; i++) {

        if ((i % 7) == 6) {         /* Secondary Station ID */
            ax25_frame[i] = ((addresses[i] << 1) & 0x1F) | 0x60;
            } else {
            ax25_frame[i] = (addresses[i] << 1);
        }
    }
    ax25_frame[i-1] |= 0x1;     /* Set HLDC bit */

    ax25_frame[i++] = AX25_CONTROL_WORD;
    ax25_frame[i++] = AX25_PROTOCOL_ID;

    /* Process information */
    memcpy(ax25_frame+i, information, information_len);
    i += information_len;

    return i;
}
