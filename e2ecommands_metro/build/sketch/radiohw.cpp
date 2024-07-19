#line 1 "C:\\Users\\conra\\OneDrive\\Documents\\GitHub\\Radio_Software\\e2ecommands_metro\\radiohw.cpp"
/**
* @file radiohw.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing HW support for the Silversat Radio, especially the GPIO config
* @version 1.0.1
* @date 2023-7-17

radioHW.cpp - Library providing HW support for the Silversat Radio, especially the GPIO config
Created by Tom Conrad, July 17, 2024.
Released into the public domain.

*/

#include "radiohw.h"

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


void Radio::begin()
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
}

void Radio::setTransmit(ax_config& config, ax_modulation& mod)
{
    ax_force_quick_adjust_frequency_A(&config, config.synthesiser.A.frequency); // doppler compensation
    ax_set_pwrmode(&config, 0x05);                                            // see errata
    ax_set_pwrmode(&config, 0x07);                                            // see errata
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);
    digitalWrite(_pin_PAENABLE, HIGH); // enable the PA BEFORE turning on the transmitter
    delayMicroseconds(constants::pa_delay);
    ax_tx_on(&config, &mod); // turn on the radio in full tx mode
    digitalWrite(_pin_TX_LED, HIGH); // this line and the one in set_receive removed for metro version..should fix this
}

void Radio::setReceive(ax_config &config, ax_modulation &mod)
{
    ax_force_quick_adjust_frequency_B(&config, config.synthesiser.B.frequency); // doppler compensation
    ax_rx_on(&config, &mod);                                                  // go into full_RX mode -- does this cause a re-range of the synthesizer?
    digitalWrite(_pin_TX_LED, LOW);
    digitalWrite(_pin_PAENABLE, LOW);       // cut the power to the PA
    delayMicroseconds(constants::pa_delay); // wait for it to turn off
    digitalWrite(_pin_TX_RX, LOW);          // set the TR state to receive
    digitalWrite(_pin_RX_TX, HIGH);
}

void Radio::beaconMode(ax_config &config, ax_modulation &mod)
{
    ax_off(&config);
    ax_init(&config); // do an init first
    ax_default_params(&config, &mod); // load the RF parameters
    digitalWrite(_pin_AX5043_DATA, LOW);

    _func = 0x84;              // set for wire mode
    ax_set_pinfunc_data(&config, _func); // remember to set this back when done!

    // set the RF switch to transmit
    digitalWrite(_pin_TX_RX, HIGH);
    digitalWrite(_pin_RX_TX, LOW);

    debug_printf("config variable values: \r\n");
    debug_printf("tcxo frequency: %u \r\n", uint(config.f_xtal));
    debug_printf("synthesizer A frequency: %u \r\n", uint(config.synthesiser.A.frequency));
    debug_printf("synthesizer B frequency: %u \r\n", uint(config.synthesiser.B.frequency));
    debug_printf("status: %x \r\n", ax_hw_status());

    ax_tx_on(&config, &ask_modulation);
}

void Radio::dataMode(ax_config &config, ax_modulation &mod)
{
    digitalWrite(_pin_PAENABLE, LOW);
    digitalWrite(_pin_TX_LED, LOW);

    // and set the switch controls to the receive path.
    digitalWrite(_pin_TX_RX, LOW);
    digitalWrite(_pin_RX_TX, HIGH);

    _func = 2;

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

void Radio::cwMode(ax_config &config, ax_modulation &mod, int duration, ExternalWatchdog &watchdog)
{
    ax_init(&config); // do an init first
    // modify the power to match what's in the modulation structure...make sure the modulation type matches
    // this keeps beacon at full power
    ask_modulation.power = mod.power;

    debug_printf("ask power: %d \r\n", ask_modulation.power); //check to make sure it was modified...but maybe it wasn't?

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

size_t Radio::reportstatus(String &response, ax_config &config, ax_modulation &modulation, Efuse &efuse)
{
    // create temperature sensor instance, only needed here
    Generic_LM75_10Bit tempsense(0x4B);

    response = "Freq A:" + String(config.synthesiser.A.frequency, DEC);
    response += "Freq B:" + String(config.synthesiser.B.frequency, DEC);
    response += "; Status:" + String(ax_hw_status(), HEX); // ax_hw_status is the FIFO status from the last transaction
    float patemp{tempsense.readTemperatureC()};
    response += "; Temp: " + String(patemp, 1);
    // response += "; Overcurrent: " + String(efuse.overcurrent(true), HEX);
    response += "; Current: " + String(efuse.measure_current(), DEC);
    response += "; Shape:" + String(modulation.shaping, HEX);
    response += "; FEC:" + String(modulation.fec, HEX);
    response += "; Bitrate:" + String(modulation.bitrate, DEC);
    response += "; Pwr%:" + String(modulation.power, 3);

    // response = "generic response";
    return response.length();
}

/************************************************************************/
/** dah - sends a morse code "dah" using wire mode                    */
/************************************************************************/
void Radio::dah()
{
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    delay(3 * constants::bit_time);

    digitalWrite(_pin_AX5043_DATA, LOW);
    digitalWrite(_pin_PAENABLE, LOW); // turn off the PA
    digitalWrite(_pin_TX_LED, LOW);

    delay(constants::bit_time);
}

/************************************************************************/
/** dit() - sends a morse code "dit"                                    */
/************************************************************************/
void Radio::dit()
{
    digitalWrite(_pin_PAENABLE, HIGH);
    // delay(PAdelay); //let the pa bias stabilize
    digitalWrite(_pin_TX_LED, HIGH);
    digitalWrite(_pin_AX5043_DATA, HIGH);

    delay(constants::bit_time);

    digitalWrite(_pin_AX5043_DATA, LOW);
    digitalWrite(_pin_PAENABLE, LOW); // turn off the PA
    digitalWrite(_pin_TX_LED, LOW);

    delay(constants::bit_time);
}