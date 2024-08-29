/**
* @file radio.cpp
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing HW support for the Silversat Radio, especially the GPIO config
* @version 1.0.1
* @date 2024-7-24

radio.cpp - Library providing HW support for the Silversat Radio, especially the GPIO config
Created by Tom Conrad, July 17, 2024.
Released into the public domain.

*/

#include "radio.h"

Radio::Radio(int TX_RX_pin, int RX_TX_pin, int PAENABLE_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin)
{
    _pin_TX_RX = TX_RX_pin;
    _pin_RX_TX = RX_TX_pin;
    _pin_PAENABLE = PAENABLE_pin;
    _pin_SYSCLK = SYSCLK_pin;
    _pin_AX5043_DCLK = AX5043_DCLK_pin;
    _pin_AX5043_DATA = AX5043_DATA_pin;
    _pin_TX_LED = PIN_LED_TX_pin;
}

//this sets the mode for all the pins and their initial conditions.  It populates the config and modulation structure.
//and then sets it into receive mode.
void Radio::begin(ax_config &config, ax_modulation &mod, void (*spi_transfer)(unsigned char *, uint8_t), FlashStorageClass<int> &operating_frequency)
{
    pinMode(_pin_TX_RX, OUTPUT); // TX/ RX-bar
    pinMode(_pin_RX_TX, OUTPUT); // RX/ TX-bar
    pinMode(_pin_PAENABLE, OUTPUT); // enable the PA
    pinMode(_pin_SYSCLK, INPUT);    // AX5043 crystal oscillator clock output
    pinMode(_pin_AX5043_DCLK, INPUT); // clock from the AX5043 when using wire mode
    pinMode(_pin_AX5043_DATA, OUTPUT); // data to the AX5043 when using wire mode
    pinMode(_pin_TX_LED, OUTPUT);

    // set the default state (Receiver on, PA off)
    digitalWrite(_pin_TX_RX, LOW);
    digitalWrite(_pin_RX_TX, HIGH);
    digitalWrite(_pin_PAENABLE, LOW);
    digitalWrite(_pin_TX_LED, LOW); // outputs a high while in transmit mode

    // set the data pin for wire mode into the AX5043 low, NOT transmitting
    digitalWrite(_pin_AX5043_DATA, LOW);

    // ------- initialize radio structures -------
    // the headings below corresponds to the main parts of the config structure. Unless changed they are loaded with defaults
    /* power mode */
    // generally handled internally, so consider it a variable handled by a private function

    /* synthesiser */
    config.synthesiser.vco_type = AX_VCO_INTERNAL; // note: I added this to try to match the DVK, this means that the external inductor is not used
    //config.synthesiser.vco_type = AX_VCO_INTERNAL_EXTERNAL_INDUCTOR;  //looks like radiolab is using this config 8/8/24
    // config.synthesiser.A.frequency = constants::frequency;
    // config.synthesiser.B.frequency = constants::frequency;
    config.synthesiser.A.frequency = operating_frequency.read();
    config.synthesiser.B.frequency = operating_frequency.read();

    /* external clock */
    config.clock_source = AX_CLOCK_SOURCE_TCXO;
    config.f_xtal = 48000000;

    /* transmit path */
    // default is differential; needs to be single ended for external PA; NOTE: there is a command to change the path
    config.transmit_power_limit = 1;

    /* SPI transfer */
    config.spi_transfer = spi_transfer; // define the SPI handler

    /* receive */
    // config.pkt_store_flags = AX_PKT_STORE_RSSI | AX_PKT_STORE_RF_OFFSET;  //search on "AX_PKT_STORE" for other options, only data rate offset is implemented
    //  config.pkt_accept_flags =     // code sets accept residue, accept bad address, accept packets that span fifo chunks.  We DON'T want to accept residues, or packets that span more than one

    /* wakeup */
    // for WOR, we're not using

    /* digital to analogue (DAC) channel */
    // not needed

    /* PLL VCO */
    // frequency range of vco; see ax_set_pll_parameters
    //  ------- end init -------

    // modulation = gmsk_modulation;  //by default we're using gmsk, and allowing other MSK/FSK type modes to be configured by modifying the structure
    mod.modulation = AX_MODULATION_FSK;
    mod.encoding = AX_ENC_NRZI;
    mod.framing = AX_FRAMING_MODE_HDLC | AX_FRAMING_CRCMODE_CCITT;
    mod.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
    mod.bitrate = 9600;
    mod.fec = 0;
    mod.rs_enabled = 0;
    mod.power = constants::power;
    mod.continuous = 0;
    mod.fixed_packet_length = 0;
    mod.parameters = {.fsk = {.modulation_index = 0.5}};
    mod.max_delta_carrier = 0; // 0 sets it to the default, which is defined in constants.cpp
    mod.par = {};

    ax_init(&config); // this does a reset, so needs to be first

    // load the RF parameters for the current config
    ax_default_params(&config, &mod); // ax_modes.c for RF parameters
    // I noticed this was never getting called, so trying it.  tkc 8/12/24
    ax_set_performance_tuning(&config, &mod);

        // parrot back what we set
    debug_printf("config variable values: \r\n");
    debug_printf("tcxo frequency: %d \r\n", int(config.f_xtal));
    debug_printf("synthesizer A frequency: %d \r\n", int(config.synthesiser.A.frequency));
    debug_printf("synthesizer B frequency: %d \r\n", int(config.synthesiser.B.frequency));
    debug_printf("status: %x \r\n", ax_hw_status());

    // turn on the receiver
    ax_rx_on(&config, &mod);

    // for RF debugging
    //  printRegisters(config);
}

// setTransmit configures the radio for transmit..go figure
void Radio::setTransmit(ax_config &config, ax_modulation &mod)
{
    ax_SET_SYNTH_A(&config);  //TODO: how does the right one get selected in the first place?
    debug_printf("current selected synth for Tx: %x \r\n", ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    ax_force_quick_adjust_frequency_A(&config, config.synthesiser.A.frequency); // doppler compensation
    ax_set_pwrmode(&config, 0x05);                                            // see errata
    ax_set_pwrmode(&config, 0x07);                                            // see errata
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);
    digitalWrite(_pin_PAENABLE, HIGH); // enable the PA BEFORE turning on the transmitter
    delayMicroseconds(constants::pa_delay);
    ax_tx_on(&config, &mod); // turn on the radio in full tx mode
    digitalWrite(_pin_TX_LED, HIGH); // this line and the one in set_receive removed for metro version..should fix this
    debug_printf("synth after tx_on: %x \r\n", ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
}

// setReceive configures the radio for receive..go figure
void Radio::setReceive(ax_config &config, ax_modulation &mod)
{
    debug_printf("current selected synth for Rx: %x \r\n", ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    ax_force_quick_adjust_frequency_B(&config, config.synthesiser.B.frequency); // doppler compensation
    // go into full_RX mode -- does this cause a re-range of the synthesizer?
    digitalWrite(_pin_TX_LED, LOW);
    digitalWrite(_pin_PAENABLE, LOW);       // cut the power to the PA
    delayMicroseconds(constants::pa_delay); // wait for it to turn off
    digitalWrite(_pin_TX_RX, LOW);          // set the TR state to receive
    digitalWrite(_pin_RX_TX, HIGH);
    ax_rx_on(&config, &mod);  
    ax_SET_SYNTH_B(&config);
    debug_printf("synth after rx_on and switch: %x \r\n", ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
}

/* beacon mode is entered by putting the AX5043 in Wire mode and setting the modulation for ASK.
 * When in WIRE mode the data rate can instead be thought of as sampling rate.
 * So, if you have a data rate of 100 bps, then the smallest time sample is 10 mSeconds.
 * Since you're in wire mode, you're clocking out a 1 if data is high, every
 * 1/datarate seconds.
 *
 * To enter wire mode write 0x84 to PINFUNCDATA (there's a library call for
 *  that).
 *
 * THIS IS IMPORTANT: The PA will go kablooey if you turn on the RF signal
 *  before the power is applied.  I had three (at ~$20 each) PA's blow up this
 *  way.  Make sure the output switch is set to the load (TX/~RX high, ~TX/RX
 *  low), and that a load is attached.  You must also switch the TX path to
 *  single ended.  Only then can you safely turn on the RF signal from the
 *  AX5043.
 */
void Radio::beaconMode(ax_config &config, ax_modulation &mod)
{
    ax_off(&config);
    ax_init(&config); // do an init first
    ax_default_params(&config, &mod); // load the RF parameters
    digitalWrite(_pin_AX5043_DATA, LOW);

    _func = 0x84;              // set for wire mode: 
    //0x84 => 1000 0100 => PUDATA=1 (DATA weak Pullup enable), PFDATA=4 => DATA input/output modem data
    ax_set_pinfunc_data(&config, _func); // remember to set this back when done!

    // set the RF switch to transmit
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);

    debug_printf("config variable values: \r\n");
    debug_printf("tcxo frequency: %u \r\n", uint(config.f_xtal));
    debug_printf("synthesizer A frequency: %u \r\n", uint(config.synthesiser.A.frequency));
    debug_printf("synthesizer B frequency: %u \r\n", uint(config.synthesiser.B.frequency));
    debug_printf("status: %x \r\n", ax_hw_status());
    ax_SET_SYNTH_A(&config); //make sure we're using SYNTH A
    ax_tx_on(&config, &ask_modulation);
}

/* datamode is the normal operating mode for the AX5043 
 * you also use it to take the chip out of CW or beacon mode
*/
void Radio::dataMode(ax_config &config, ax_modulation &mod)
{
    digitalWrite(_pin_PAENABLE, LOW);
    digitalWrite(_pin_TX_LED, LOW);

    // and set the switch controls to the receive path.
    digitalWrite(_pin_TX_RX, LOW);
    digitalWrite(_pin_RX_TX, HIGH);

    _func = 2;  //sets data pin to high impedance

    // drop out of wire mode
    ax_set_pinfunc_data(&config, _func);

    ax_off(&config); // turn the radio off
    // debug_printf("radio off \r\n");
    ax_init(&config); // this does a reset, so probably needs to be first, this hopefully takes us out of wire mode too
    debug_printf("radio init \r\n");
    // load the RF parameters
    ax_default_params(&config, &mod); // ax_modes.c for RF parameters

    debug_printf("default params loaded \r\n");
    ax_rx_on(&config, &mod);

    debug_printf("receiver on \r\n");
    debug_printf("status: %x \r\n", ax_hw_status());
    debug_printf("i'm done and back to receive \r\n");
}


/* cW mode is entered by putting the AX5043 in Wire mode and setting the modulation for ASK.
 * When in WIRE mode the data rate can instead be thought of as sampling rate.
 * So, if you have a data rate of 100 bps, then the smallest time sample is 10 mSeconds.
 * Since you're in wire mode, you're clocking out a 1 if data is high, every
 * 1/datarate seconds.
 *
 * To enter wire mode write 0x84 to PINFUNCDATA (there's a library call for
 *  that).
 *
 * THIS IS IMPORTANT: The PA will go kablooey if you turn on the RF signal
 *  before the power is applied.  I had three (at ~$20 each) PA's blow up this
 *  way.  Make sure the output switch is set to the load (TX/~RX high, ~TX/RX
 *  low), and that a load is attached.  You must also switch the TX path to
 *  single ended.  Only then can you safely turn on the RF signal from the
 *  AX5043.
 */
void Radio::cwMode(ax_config &config, ax_modulation &mod, int duration, ExternalWatchdog &watchdog)
{
    ax_init(&config); // do an init first
    // modify the power to match what's in the modulation structure...make sure the modulation type matches
    // this keeps beacon at full power
    ask_modulation.power = mod.power;

    debug_printf("ask power: %f \r\n", ask_modulation.power); //check to make sure it was modified...but maybe it wasn't?

    ax_default_params(&config, &ask_modulation); // load the RF parameters

    pinfunc_t _func = 0x84;              // set for wire mode
    ax_set_pinfunc_data(&config, _func); // remember to set this back when done!

    // set the RF switch to transmit
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    ax_tx_on(&config, &ask_modulation); // turn on the transmitter

    // start transmitting
    debug_printf("output CW for %u seconds \r\n", duration);
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);
    int duration_timer_start = millis();
    while (millis() - duration_timer_start < duration * 1000)
    {
        watchdog.trigger();
    }

    // stop transmitting
    digitalWrite(_pin_AX5043_DATA, LOW);
    digitalWrite(_pin_TX_RX, LOW); // put the switch back to receive
    digitalWrite(_pin_RX_TX, HIGH);
    digitalWrite(_pin_PAENABLE, LOW); // turn off the PA
    digitalWrite(_pin_TX_LED, LOW);
    debug_printf("done \r\n");

    // drop out of wire mode
    _func = 2;
    ax_set_pinfunc_data(&config, _func);

    // now put it back the way you found it.
    ax_init(&config);                        // do a reset
    ax_default_params(&config, &mod); // ax_modes.c for RF parameters
    debug_printf("default params loaded \r\n");
    // Serial.println("default params loaded \r\n");
    ax_rx_on(&config, &mod);
    debug_printf("receiver on \r\n");
    // Serial.println("receiver on \r\n");
}

size_t Radio::reportstatus(String &response, ax_config &config, ax_modulation &modulation, Efuse &efuse, bool fault)
{
    // create temperature sensor instance, only needed here
    Generic_LM75_10Bit tempsense(0x4B);

    response = "Freq A:" + String(config.synthesiser.A.frequency, DEC);
    response += "; Freq B:" + String(config.synthesiser.B.frequency, DEC);
    response += "; Version:" + constants::version;
    response += "; Status:" + String(ax_hw_status(), HEX); // ax_hw_status is the FIFO status from the last transaction
    float patemp{tempsense.readTemperatureC()};
    response += "; Temp:" + String(patemp, 1); 
    response += "; Overcurrent:" + String(fault);
    response += "; 5V Current:" + String(efuse.measure_current(), DEC);
    response += "; 5V Current (Max): " + String(efuse.get_max_current());
    response += "; Shape:" + String(modulation.shaping, HEX);
    response += "; FEC:" + String(modulation.fec, HEX);
    response += "; RS_enabled: " + String(modulation.rs_enabled);
    response += "; Bitrate:" + String(modulation.bitrate, DEC);
    response += "; Pwr%:" + String(modulation.power, 3);

    // response = "generic response";
    return response.length();
}

// key() is a more generic version of dit and dah.  It may replace them entirely to abstract away morse code specifics
/* Before calling key, switch the AX5043 into WIRE mode using ASK modulation.
 * you do that by using the Radio::beaconmode() command.  
 */

void Radio::key(int chips)
{
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    delay(chips * constants::bit_time);

    digitalWrite(_pin_AX5043_DATA, LOW);
    digitalWrite(_pin_PAENABLE, LOW); // turn off the PA
    digitalWrite(_pin_TX_LED, LOW);

    delay(constants::bit_time);
}