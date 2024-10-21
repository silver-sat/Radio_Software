#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\radio.cpp"
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

Radio::Radio(int TX_RX_pin, int RX_TX_pin, int PAENABLE_pin, int SYSCLK_pin, int AX5043_DCLK_pin, int AX5043_DATA_pin, int PIN_LED_TX_pin, int IRQ_pin)
{
    _pin_TX_RX = TX_RX_pin;
    _pin_RX_TX = RX_TX_pin;
    _pin_PAENABLE = PAENABLE_pin;
    _pin_SYSCLK = SYSCLK_pin;
    _pin_AX5043_DCLK = AX5043_DCLK_pin;
    _pin_AX5043_DATA = AX5043_DATA_pin;
    _pin_TX_LED = PIN_LED_TX_pin;
    _pin_IRQ = IRQ_pin;

    // fill the ax5043 config array with zeros
    memset(&config, 0, sizeof(ax_config));
    // populate default modulation structure
    memset(&modulation, 0, sizeof(ax_modulation));
}

// this sets the mode for all the pins and their initial conditions.  It populates the config and modulation structure.
// and then sets it into receive mode.
void Radio::begin(void (*spi_transfer)(unsigned char *, uint8_t), FlashStorageClass<int> &operating_frequency)
{
    pinMode(_pin_TX_RX, OUTPUT);       // TX/ RX-bar
    pinMode(_pin_RX_TX, OUTPUT);       // RX/ TX-bar
    pinMode(_pin_PAENABLE, OUTPUT);    // enable the PA
    pinMode(_pin_SYSCLK, INPUT);       // AX5043 crystal oscillator clock output
    pinMode(_pin_AX5043_DCLK, INPUT);  // clock from the AX5043 when using wire mode
    pinMode(_pin_AX5043_DATA, OUTPUT); // data to the AX5043 when using wire mode
    pinMode(_pin_TX_LED, OUTPUT);
    pinMode(_pin_IRQ, INPUT);

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
    // config.synthesiser.vco_type = AX_VCO_INTERNAL_EXTERNAL_INDUCTOR;  //looks like radiolab is using this config 8/8/24
    //  config.synthesiser.A.frequency = constants::frequency;
    //  config.synthesiser.B.frequency = constants::frequency;
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

    //by default we're using gmsk with il2p, no crc added, and allowing other MSK/FSK type modes to be configured by modifying the structure
    modulation.modulation = AX_MODULATION_FSK;
    modulation.encoding = AX_ENC_NRZ;
    modulation.framing = AX_FRAMING_MODE_RAW_PATTERN_MATCH | AX_FRAMING_CRCMODE_OFF;
    modulation.shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
    modulation.bitrate = 9600;
    modulation.fec = 0;
    modulation.radiolab = 1;
    modulation.il2p_enabled = 1;
    modulation.power = constants::power;
    modulation.continuous = 0;
    modulation.fixed_packet_length = 0;
    modulation.parameters = {.fsk = {.modulation_index = 0.67}};
    modulation.max_delta_carrier = 0; // 0 sets it to the default, which is defined in constants.cpp
    modulation.par = {};

    ax_init(&config); // this does a reset, so needs to be first

    // load the RF parameters for the current config
    ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
    // I noticed this was never getting called, so trying it.  tkc 8/12/24
    ax_set_performance_tuning(&config, &modulation);

    //The end of the transmission may be determined by polling the register
    //RADIOSTATE until it indicates idle, or by enabling the
    //radio controller interrupt (bit IRQMRADIOCTRL) in
    //register IRQMASK0 and setting the radio controller to
    //signal an interrupt at the end of transmission (bit
    //REVMDONE of register RADIOEVENTMASK0).


    // parrot back what we set
    Log.verbose(F("config variable values:\r\n"));
    Log.verbose(F("tcxo frequency: %d\r\n"), int(config.f_xtal));
    Log.verbose(F("synthesizer A frequency: %d\r\n"), int(config.synthesiser.A.frequency));
    Log.verbose(F("synthesizer B frequency: %d\r\n"), int(config.synthesiser.B.frequency));
    Log.verbose(F("status: %X\r\n"), ax_hw_status());

    // set the IRQ for the radio control
    ax_SET_IRQMRADIOCTRL(&config);

    // turn on the receiver
    ax_rx_on(&config, &modulation);
    Log.trace(F("current selected synth for Tx: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    // for RF debugging
    //  printRegisters(config);
}

// setTransmit configures the radio for transmit..go figure
void Radio::setTransmit()
{
    //Log.verbose("enabling interrupts\r\n");
    //interrupts();
    ax_SET_SYNTH_A(&config);
    Log.trace(F("current selected synth for Tx: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    ax_force_quick_adjust_frequency_A(&config, config.synthesiser.A.frequency); // doppler compensation
    ax_set_pwrmode(&config, 0x05);                                              // see errata
    ax_set_pwrmode(&config, 0x07);                                              // see errata
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);
    //digitalWrite(_pin_PAENABLE, HIGH); // enable the PA BEFORE turning on the transmitter
    //delayMicroseconds(constants::pa_delay);
    ax_tx_on(&config, &modulation);         // turn on the radio in full tx mode
    ax_SET_SYNTH_A(&config);  //I think that the quick adjust is changing us to synth B
    digitalWrite(_pin_TX_LED, HIGH); 
    Log.verbose(F("PLLLOOP register: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    Log.verbose(F("getSynth: %i\r\n"), getSynth());
    if (getSynth() != 0) Log.error(F("LOOK! incorrect synth selected\r\n"));
}

// setReceive configures the radio for receive..go figure
void Radio::setReceive()
{
    //Log.verbose("disabling interrupts\r\n");
    //noInterrupts();
    digitalWrite(_pin_PAENABLE, LOW);       // cut the power to the PA
    digitalWrite(_pin_TX_LED, LOW);
    Log.trace(F("current selected synth for Rx: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    ax_force_quick_adjust_frequency_B(&config, config.synthesiser.B.frequency); // doppler compensation
    // go into full_RX mode -- does this cause a re-range of the synthesizer?
    delayMicroseconds(constants::pa_delay); // wait for it to turn off
    digitalWrite(_pin_TX_RX, LOW);          // set the TR state to receive
    digitalWrite(_pin_RX_TX, HIGH);
    Log.trace(F("turning on receiver\r\n"));
    ax_rx_on(&config, &modulation);
    ax_SET_SYNTH_B(&config);
    Log.verbose(F("PLLLOOP register: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
    Log.verbose(F("getSynth: %i\r\n"), getSynth());
    if (getSynth() != 1) Log.error(F("LOOK! incorrect synth selected\r\n"));
}

//set the radio transmitter frequency
int Radio::setTransmitFrequency(int frequency)
{
    config.synthesiser.A.frequency = frequency;
    int adjust_result = ax_adjust_frequency_A(&config, frequency);
    ax_SET_SYNTH_A(&config);     
    if (getSynth() != 0) Log.error(F("LOOK! incorrect synth selected\r\n"));
    return adjust_result;
}

//set the radio receive frequency
int Radio::setReceiveFrequency(int frequency)
{
    config.synthesiser.B.frequency = frequency;
    int adjust_result = ax_adjust_frequency_B(&config, frequency);
    ax_SET_SYNTH_B(&config);
    if (getSynth() != 1) Log.error(F("LOOK! incorrect synth selected\r\n"));
    return adjust_result;
}

int Radio::getTransmitFrequency()
{
    return config.synthesiser.A.frequency;
}

int Radio::getReceiveFrequency()
{
    return config.synthesiser.B.frequency;
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
void Radio::beaconMode()
{
    ax_off(&config);
    ax_init(&config);                 // do an init first
    ax_default_params(&config, &ask_modulation); // load the RF parameters
    digitalWrite(_pin_AX5043_DATA, LOW);

    _func = 0x84; // set for wire mode:
    // 0x84 => 1000 0100 => PUDATA=1 (DATA weak Pullup enable), PFDATA=4 => DATA input/output modem data
    ax_set_pinfunc_data(&config, _func); // remember to set this back when done!

    // set the RF switch to transmit
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);

    Log.trace(F("config variable values:\r\n"));
    Log.verbose(F("tcxo frequency: %d\r\n"), uint(config.f_xtal));
    Log.trace(F("synthesizer A frequency: %d\r\n"), uint(config.synthesiser.A.frequency));
    Log.trace(F("synthesizer B frequency: %d\r\n"), uint(config.synthesiser.B.frequency));
    Log.verbose(F("status: %X\r\n"), ax_hw_status());
    ax_SET_SYNTH_A(&config); // make sure we're using SYNTH A
    ax_tx_on(&config, &ask_modulation);
}

/* datamode is the normal operating mode for the AX5043
 * you also use it to take the chip out of CW or beacon mode
 */
void Radio::dataMode()
{
    digitalWrite(_pin_PAENABLE, LOW);
    digitalWrite(_pin_TX_LED, LOW);

    // and set the switch controls to the receive path.
    digitalWrite(_pin_TX_RX, LOW);
    digitalWrite(_pin_RX_TX, HIGH);

    _func = 2; // sets data pin to high impedance

    // drop out of wire mode
    ax_set_pinfunc_data(&config, _func);

    ax_off(&config); // turn the radio off
    ax_init(&config); // this does a reset, so probably needs to be first, this hopefully takes us out of wire mode too
    Log.trace(F("radio init\r\n"));
    // load the RF parameters
    ax_default_params(&config, &modulation); // ax_modes.c for RF parameters

    Log.trace(F("default params loaded\r\n"));
    ax_rx_on(&config, &modulation);
    Log.trace(F("current selected synth for Tx: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));

    Log.trace(F("receiver on\r\n"));
    Log.verbose(F("status: %X\r\n"), ax_hw_status());
    Log.notice(F("i'm done and back to receive\r\n"));
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
void Radio::cwMode(uint32_t duration, ExternalWatchdog &watchdog)
{
    ax_init(&config); // do an init first
    // modify the power to match what's in the modulation structure...make sure the modulation type matches
    // this keeps beacon at full power
    ask_modulation.power = modulation.power;

    Log.notice(F("ask power: %f\r\n"), ask_modulation.power); // check to make sure it was modified...but maybe it wasn't?

    ax_default_params(&config, &ask_modulation); // load the RF parameters

    pinfunc_t _func = 0x84;              // set for wire mode
    ax_set_pinfunc_data(&config, _func); // remember to set this back when done!

    // set the RF switch to transmit
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    ax_tx_on(&config, &ask_modulation); // turn on the transmitter

    // start transmitting
    Log.notice(F("output CW for %d seconds\r\n"), duration);
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);
    unsigned long duration_timer_start = millis();
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
    Log.notice(F("done\r\n"));

    // drop out of wire mode
    _func = 2;
    ax_set_pinfunc_data(&config, _func);

    // now put it back the way you found it.
    ax_init(&config);                 // do a reset
    ax_default_params(&config, &modulation); // ax_modes.c for RF parameters
    Log.trace(F("default params loaded\r\n"));
    ax_rx_on(&config, &modulation);
    Log.notice(F("receiver on\r\n"));
    Log.trace(F("current selected synth for Tx: %X\r\n"), ax_hw_read_register_8(&config, AX_REG_PLLLOOP));
}

size_t Radio::reportstatus(String &response, Efuse &efuse, bool fault)
{
    // create temperature sensor instance, only needed here
    Generic_LM75_10Bit tempsense(0x4B);
    //Log.verbose("response: %s\r\n", response);
    response = "Freq A:" + String(config.synthesiser.A.frequency, DEC);
    //Log.verbose("response: %s\r\n", response);
    response += "; Freq B:" + String(config.synthesiser.B.frequency, DEC);
    //Log.verbose("response: %s\r\n", response);
    response += "; Version:" + String(constants::version);
    //Log.verbose("response: %s\r\n", response);
    //response += "; Status:" + String(ax_hw_status(), HEX); // ax_hw_status is the FIFO status from the last transaction
    #ifdef SILVERSAT
        float patemp = tempsense.readTemperatureC();
        Log.verbose("response: %s\r\n", response);
        response += "; Temp:" + String(patemp, 1);
    #endif
    //Log.verbose("response: %s\r\n", response);
    response += "; Overcurrent:" + String(fault);
    //Log.verbose("response: %s\r\n", response);
    response += "; 5V Current:" + String(efuse.measure_current(), DEC);
    //Log.verbose("response: %s\r\n", response);
    response += "; 5V Current (Max): " + String(efuse.get_max_current());
    //Log.verbose("response: %s\r\n", response);
    response += "; Shape:" + String(modulation.shaping, HEX);
    //Log.verbose("response: %s\r\n", response);
    //response += "; FEC:" + String(modulation.fec, HEX);
    response += "' Baud rate:"+ String(modulation.bitrate);
    //Log.verbose("response: %s\r\n", response);
    response += "; il2p_enabled:" + String(modulation.il2p_enabled);
    //Log.verbose("response: %s\r\n", response);
    response += "; framing:" + String(modulation.framing & 0x0E);
    //Log.verbose("response: %s\r\n", response);
    response += "; CCA threshold:" + String(constants::clear_threshold);
    //Log.verbose("response: %s\r\n", response);
    //response += "; Bitrate:" + String(modulation.bitrate, DEC);
    //Log.verbose("response: %s\r\n", response);
    response += "; Pwr%:" + String(modulation.power, 3);
    //Log.verbose("response: %s\r\n", response);

    efuse.clear_max_current();
    return response.length();
}

// key() is a more generic version of dit and dah.  It may replace them entirely to abstract away morse code specifics
/* Before calling key, switch the AX5043 into WIRE mode using ASK modulation.
 * you do that by using the Radio::beaconmode() command.
 */

void Radio::key(int chips, Efuse &efuse)
{
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    delay(chips * constants::bit_time);
    efuse.measure_current(); // take a current measurement.  Store the max

    digitalWrite(_pin_AX5043_DATA, LOW);
    digitalWrite(_pin_PAENABLE, LOW); // turn off the PA
    digitalWrite(_pin_TX_LED, LOW);

    delay(constants::bit_time);
}

bool Radio::radioBusy()
{
  if (ax_RADIOSTATE(&config) == 0) return false;
  else return true;
}

uint8_t Radio::rssi()
{
  uint8_t rssi = ax_RSSI(&config);
  return rssi;
}

void Radio::transmit(byte* txqueue, int txbufflen)
{
digitalWrite(_pin_PAENABLE, HIGH);
digitalWrite(_pin_TX_LED, HIGH);
ax_tx_packet(&config, &modulation, txqueue, txbufflen);
}

bool Radio::receive()
{
    if (ax_rx_packet(&config, &rx_pkt, &modulation) == 1) return true;
    else return false;
}

void Radio::clear_Radio_FIFO()
{
    Log.trace(F("clearing the AX5043 FIFO\r\n")); // may be unnecessary...may have unintended consequences?
    //TODO: perhaps create a radio.reset function?  there is a procedure for it.
    ax_fifo_clear(&config);
}

void Radio::setSynthA()  //directly set the Tx synth to be active
{
    ax_SET_SYNTH_A(&config);
    Log.notice(F("Synth A set\r\n"));
} 

void Radio::setSynthB()  //directly set the Rx synth to be active
{
    ax_SET_SYNTH_B(&config);
    Log.notice(F("Synth B set\r\n"));
}

uint8_t Radio::getSynth()  //returns which synth is selected.  0 for Tx, 1 for Rx, 2 for error
{
  uint8_t selected_synth = ax_hw_read_register_8(&config, AX_REG_PLLLOOP);
  Log.trace(F("selected synth (register): %X\r\n"), selected_synth);
  Log.trace(F("A or B?: %X\r\n"), selected_synth & 0x80);
  if ((selected_synth & 0x80) == 0x80) return 1;  //it's 0x80, so it's set for B = Rx
  else return 0;  //it's a zero, so it's set for A = Tx
}  

uint16_t Radio::getRegValue(int register_int)
{
    return ax_hw_read_register_8(&config, register_int);
}

//this is for debugging so we can see the radio parameter settings
void Radio::printParamStruct()
{
    Log.verbose("rx_bandwidth: %X \r\n", modulation.par.rx_bandwidth);
    Log.verbose("f_baseband: %X \r\n", modulation.par.f_baseband);
    Log.verbose("if_frequency: %X \r\n", modulation.par.if_frequency);
    Log.verbose("iffreq: %X \r\n", modulation.par.iffreq);
    Log.verbose("decimation: %X \r\n", modulation.par.decimation);
    Log.verbose("ampl_filter: %X \r\n", modulation.par.ampl_filter);
    Log.verbose("match1_threashold: %X \r\n", modulation.par.match1_threashold);
    Log.verbose("match0_threashold: %X \r\n", modulation.par.match0_threashold);
    Log.verbose("pkt_misc_flags: %X \r\n", modulation.par.pkt_misc_flags);
    Log.verbose("tx_pll_boost_time: %X \r\n", modulation.par.tx_pll_boost_time);
    Log.verbose("tx_pll_settle_time: %X \r\n", modulation.par.tx_pll_settle_time);
    Log.verbose("rx_pll_boost_time: %X \r\n", modulation.par.rx_pll_boost_time);
    Log.verbose("rx_pll_settle_time: %X \r\n", modulation.par.rx_pll_settle_time);
    Log.verbose("rx_coarse_agc: %X \r\n", modulation.par.rx_coarse_agc);
    Log.verbose("rx_agc_settling: %X \r\n", modulation.par.rx_agc_settling);
    Log.verbose("rx_rssi_settling: %X \r\n", modulation.par.rx_rssi_settling);
    Log.verbose("preamble_1_timeout: %X \r\n", modulation.par.preamble_1_timeout);
    Log.verbose("preamble_1_timeout: %X \r\n", modulation.par.preamble_1_timeout);
    Log.verbose("rssi_abs_thr: %X \r\n", modulation.par.rssi_abs_thr);
    Log.verbose("perftuning_option: %X \r\n", modulation.par.perftuning_option);
}