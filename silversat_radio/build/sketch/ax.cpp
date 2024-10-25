#line 1 "C:\\GitHub\\Radio_Software\\silversat_radio\\ax.cpp"
/**
 * @file ax.cpp
 * @author Richard Meadows <richardeoin>
 * @brief Functions for controlling ax radio
 * @version 1.0
 * @date 2016
 *
 * Functions for controlling ax radios
 * Copyright (C) 2016  Richard Meadows <richardeoin>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ax.h"
// the following deal with a circular dependency
// #include "ax_params.h"
// #include "ax_hw.h"

#define MIN(a, b) ((a < b) ? (a) : (b))

// void ax_set_tx_power(ax_config* config, float power);  //never used? --tkc  doesn't exist in ax.h

void ax_set_synthesiser_parameters(ax_config *config,
                                   ax_synthesiser_parameters *params,
                                   ax_synthesiser *synth,
                                   enum ax_vco_type vco_type);

pinfunc_t _pinfunc_sysclk = 1; // SYSCLK Output '1'
pinfunc_t _pinfunc_dclk = 4;   // DCLK Output Modem Data Clock Output
pinfunc_t _pinfunc_data = 2;   // DATA Output 'Z'
pinfunc_t _pinfunc_antsel = 1; // ANTSEL Output '1'
pinfunc_t _pinfunc_pwramp = 7; // PWRAMP output External TCXO Enable (not used)

/**
 * FIFO -----------------------------------------------------
 */

/**
 * Clears the FIFO
 */
void ax_fifo_clear(ax_config *config)
{
    ax_hw_write_register_8(config, AX_REG_FIFOSTAT,
                           AX_FIFOCMD_CLEAR_FIFO_DATA_AND_FLAGS);

    ax_hw_write_register_16(config, AX_REG_FIFOTHRESH, 0xC8);
}

/**
 * Commits data written to the fifo
 */
void ax_fifo_commit(ax_config *config)
{
    ax_hw_write_register_8(config, AX_REG_FIFOSTAT,
                           AX_FIFOCMD_COMMIT);
}

/**
 * write tx 1k zeros
 */
void ax_fifo_tx_1k_zeros(ax_config *config)
{
    uint8_t header[4];
    uint8_t fifocount;

    /* wait for enough space to contain command */
    do
    {
        fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
    } while (fifocount > (256 - 4));

    /* preamble */
    header[0] = AX_FIFO_CHUNK_REPEATDATA;
    header[1] = AX_FIFO_TXDATA_NOCRC;
    header[2] = 125; /* 1000/8 = 125 */
    header[3] = 0;
    ax_hw_write_fifo(config, header, 4);
    ax_fifo_commit(config); /* commit */
}

/**
 * write tx data
 */
void ax_fifo_tx_data(ax_config *config, ax_modulation *mod,
                     uint8_t *data, uint16_t length)
{
    uint8_t header[8];
    uint16_t fifocount;
    uint8_t chunk_length;
    uint16_t rem_length;
    uint8_t pkt_end = 0;
    uint8_t pkt_max_chunk = 239;  //max size before splitting up chunks.  I changed it to 239 to account for flags byte

    /* send remainder first */
    chunk_length = length % pkt_max_chunk;  //if length = pkt_max_chunk -> 0
    rem_length = length - chunk_length; 
    Log.trace(F("chunk length = %d\r\n"), chunk_length);
    Log.trace(F("rem length = %d\r\n"), rem_length);

    if (length <= pkt_max_chunk)  //why not 240?, I changed that. tkc - 10/18/24
    { /* all in one go */
        pkt_end = AX_FIFO_TXDATA_PKTEND;
    }

    fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
    Log.verbose("%X bytes in the FIFO\r\n", fifocount);

    /* wait for enough space to contain both the preamble and chunk */
    do
    {
        fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT); // fifocount is current number of committed words.  So, free space is 256 - fifocount
        Log.verbose("+");
    } while (fifocount > (256 - (chunk_length + 17)));
    Log.verbose("\r\n");

    // where does 20 come from?  I'm still not sure why.  Chunk length is the amount of bytes over the 200.
    // preamble takes 4 bytes
    // sync bytes are another 7
    // Add 4 more for data header assuming length byte is included.  15 total.  That's not 20...
    // however a 240 byte chunk only leaves 15 bytes for everything else, so I'm guessing that's what limits it.
    // you can write up to 240 bytes, but it needs to be broken into 200 + 40 chunks by the code.
    // it does the smaller first so it clears out, once the preamble and framing is sent

    /* write preamble */
    switch (mod->framing & 0xE)
    {
    case AX_FRAMING_MODE_HDLC:
        /* preamble */
        header[0] = AX_FIFO_CHUNK_REPEATDATA;                                         // three byte payload (hdr1,2,3)
        header[1] = AX_FIFO_TXDATA_UNENC | AX_FIFO_TXDATA_RAW | AX_FIFO_TXDATA_NOCRC; // see table 10 in programming manual
        header[2] = constants::preamble_length;                                       // repeat count      was 9
        if (mod->fec == 1)
        {
            header[3] = 0x7E; // FEC requires 0x7E preambles
        }
        else
        {
            header[3] = 0xAA; // data, HDLC has no sync word, don't use 7E, clock wont sync unless FEC enabled
        }
        ax_hw_write_fifo(config, header, 4);
        break;

    default:
        /* preamble */
        header[0] = AX_FIFO_CHUNK_REPEATDATA;                                         // three byte payload (hdr1,2,3)
        header[1] = AX_FIFO_TXDATA_UNENC | AX_FIFO_TXDATA_RAW | AX_FIFO_TXDATA_NOCRC; // see table 10 in programming manual
        header[2] = constants::preamble_length;                                                                // repeat count
        header[3] = 0xAA;                                                             // data
        ax_hw_write_fifo(config, header, 4);
        
        /* sync word */
        header[0] = AX_FIFO_CHUNK_DATA;
        header[1] = 4 + 1; /* incl flags */
        header[2] = AX_FIFO_TXDATA_RAW | AX_FIFO_TXDATA_NOCRC;
        header[3] = 0x33;  
        header[4] = 0x55;
        header[5] = 0x33;
        header[6] = 0x55;
        ax_hw_write_fifo(config, header, header[1] + 2);
        break;
    }

    /* write first data */
    if (((mod->framing & 0xE) == AX_FRAMING_MODE_HDLC) || /* hdlc */
        (mod->fixed_packet_length) ||                     /* or fixed length */
        (length >= 255)                                   /* or can't include length byte anyhow */
    )
    {
        /* no length byte */
        header[0] = AX_FIFO_CHUNK_DATA;
        header[1] = 1 + chunk_length; /* incl flags */
        header[2] = AX_FIFO_TXDATA_PKTSTART | pkt_end;
        ax_hw_write_fifo(config, header, 3);
    }
    else if (mod->il2p_enabled == 1)
    {
        /* include length byte */
        header[0] = AX_FIFO_CHUNK_DATA;   // 0xE1
        header[1] = 1 + chunk_length + 1; /* incl flags */
        header[2] = AX_FIFO_TXDATA_PKTSTART | pkt_end | AX_FIFO_TXDATA_NOCRC;
        header[3] = length + 1; /* incl length byte */
        ax_hw_write_fifo(config, header, 4);
    }
    else 
    {
        /* include length byte */
        header[0] = AX_FIFO_CHUNK_DATA;   // 0xE1
        header[1] = 1 + chunk_length + 1; /* incl flags */
        header[2] = AX_FIFO_TXDATA_PKTSTART | pkt_end;
        header[3] = length + 1; /* incl length byte */
        ax_hw_write_fifo(config, header, 4);
    }
    /*
    // not checking that there's enough room in the FIFO? well, I am now.
    do
    {
        fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
        Log.verbose(".");

    } while (fifocount > (256 - (chunk_length + 12)));
    Log.verbose("\r\n");
    */
    ax_hw_write_fifo(config, data, (uint8_t)chunk_length);
    Log.trace("First data written to FIFO\r\n");
    for(int i=0; i<chunk_length; i++) Log.verbose("index: %i, data: %X\r\n", i, *(data+i));
    data += chunk_length;
    int chunk_start = chunk_length; //for debug, printing out later
    ax_fifo_commit(config); /* commit */

    /* write subsequent data */
    while (rem_length)
    {
        if (rem_length > pkt_max_chunk)
        { /* send 200 bytes */
            chunk_length = pkt_max_chunk;
            rem_length -= pkt_max_chunk;
        }
        else
        { /* finish off */
            chunk_length = rem_length;  //only do this if rem_length < 200 (it fits into an 8-bit number)
            rem_length = 0;
            pkt_end = AX_FIFO_TXDATA_PKTEND;
        }

        /* wait for enough space for chunk */
        do
        {
            fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
            Log.verbose("-");
        } while (fifocount > (256 - (chunk_length + 10)));  //3 for the chunk overhead
        Log.verbose("\r\n");

        /* write chunk */
        header[0] = AX_FIFO_CHUNK_DATA;
        header[1] = chunk_length + 1; /* incl flags */
        header[2] = pkt_end;
        ax_hw_write_fifo(config, header, 3);
        ax_hw_write_fifo(config, data, (uint8_t)chunk_length);
        Log.trace("Next data written to FIFO\r\n");
        for(int i=0; i<chunk_length; i++) Log.verbose("index: %i, data: %X\r\n", rem_length + i, *(data+i));
        data += chunk_length;
        ax_fifo_commit(config); /* commit */
    }
}

/**
 * read rx data
 */
uint16_t ax_fifo_rx_data(ax_config *config, ax_rx_chunk *chunk)
{
    uint8_t ptr[3];
    uint32_t scratch;

    uint8_t fifostat = ax_hw_read_register_8(config, AX_REG_FIFOSTAT);
    //if (fifostat != 0x21) Log.warning(F("fifostat: %X \r\n"), fifostat);
    uint16_t fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
    
    if (fifocount == 0)
    {
        return 0; /* nothing to read */
    }

    // check for fifo overruns, underruns, and full
    if (fifostat & 0x08){Log.error(F("fifo over \r\n"));}
    if (fifostat & 0x04){Log.error(F("fifo under \r\n"));}
    if (fifostat & 0x02){Log.error(F("fifo full \r\n"));}

    Log.trace(F("got something. fifocount = %X\r\n"), fifocount); // was %d...tryin somethin ; looks like this variable is otherwise unused.  Repeating packet is size 226

    chunk->chunk_t = ax_hw_read_register_8(config, AX_REG_FIFODATA);
    Log.trace(F("chunk: %X \r\n"), chunk->chunk_t); // what kind of chunk did we receive?

    switch (chunk->chunk_t)
    {
    case AX_FIFO_CHUNK_DATA:
        ax_hw_read_register_bytes(config, AX_REG_FIFODATA, ptr, 2);

        chunk->chunk.data.length = ptr[0] - 1; /* not including flags here */
        chunk->chunk.data.flags = ptr[1];
    
        /* read buffer */
        ax_hw_read_fifo(config,
                        chunk->chunk.data.data,
                        chunk->chunk.data.length + 1);

        //for (int i=0; i< fifocount; i++) Log.verbose(F("fifo data %d: %X\r\n"), i, chunk->chunk.data.data[i]);
        Log.verbose("fifocount: %X \r\n", fifocount);
        Log.verbose("chunk.data.length: %X \r\n", chunk->chunk.data.length);

        return 3 + chunk->chunk.data.length;
        

        /* RSSI */
    case AX_FIFO_CHUNK_RSSI:
        /* 8-bit register value is always negative */
        chunk->chunk.rssi = 0xFF00 | ax_hw_read_register_8(config, AX_REG_FIFODATA);
        return 2;
        /* FREQOFFS */
    case AX_FIFO_CHUNK_FREQOFFS:
        chunk->chunk.freqoffs = ax_hw_read_register_16(config, AX_REG_FIFODATA);
        return 2;
        /* ANTRSSI 2 */
    case AX_FIFO_CHUNK_ANTRSSI2:
        ax_hw_read_register_bytes(config, AX_REG_FIFODATA, ptr, 2);

        chunk->chunk.antrssi2.rssi = ptr[0];
        chunk->chunk.antrssi2.bgndnoise = ptr[1];
        return 3;
        /* TIMER */
    case AX_FIFO_CHUNK_TIMER:
        chunk->chunk.timer = ax_hw_read_register_24(config, AX_REG_FIFODATA);
        return 4;
        /* RFFREQOFFS */
    case AX_FIFO_CHUNK_RFFREQOFFS:
        scratch = ax_hw_read_register_24(config, AX_REG_FIFODATA);
        /* sign extend 24 -> 32 */
        chunk->chunk.rffreqoffs = (scratch & 0x800000) ? (0xFF000000 | scratch) : scratch;
        return 4;
        /* DATARATE */
    case AX_FIFO_CHUNK_DATARATE:
        chunk->chunk.datarate = ax_hw_read_register_24(config, AX_REG_FIFODATA);
        return 4;
        /* ANTRSSI3 */
    case AX_FIFO_CHUNK_ANTRSSI3:
        ax_hw_read_register_bytes(config, AX_REG_FIFODATA, ptr, 3);

        chunk->chunk.antrssi3.ant0rssi = ptr[0];
        chunk->chunk.antrssi3.ant1rssi = ptr[1];
        chunk->chunk.antrssi3.bgndnoise = ptr[2];
        return 4;
        /* default */
    default:
        return 1;
    }
}

/**
 * UTILITY FUNCTIONS ----------------------------------------
 */

/**
 * Wait for oscillator running and stable
 */
void ax_wait_for_oscillator(ax_config *config)
{
    int i = 0;
    while (!(ax_hw_read_register_8(config, AX_REG_XTALSTATUS) & 1))
    {
        i++;
    }

    Log.trace(F("osc stable in %d cycles\r\n"), i);
}

/**
 * Converts a value to 4-bit mantissa and 4-bit exponent
 */
static uint8_t ax_value_to_mantissa_exp_4_4(uint32_t value)
{
    uint8_t exp = 0;

    while (value > 15 && exp < 15)
    {
        value >>= 1;
        exp++;
    }

    return ((value & 0xF) << 4) | exp; /* mantissa, exponent */
}

/**
 * Converts a value to 3-bit exponent and 5-bit mantissa
 */
static uint8_t ax_value_to_exp_mantissa_3_5(uint32_t value)
{
    uint8_t exp = 0;

    while (value > 31 && exp < 7)
    {
        value >>= 1;
        exp++;
    }

    return ((exp & 0x7) << 5) | value; /* exponent, mantissa */
}

/**
 * REGISTERS -----------------------------------------------
 */

/**
 * 5.1 revision and interface probing
 */
uint8_t ax_silicon_revision(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_SILICONREVISION);
}

/**
 * 5.1 revision and interface probing
 */
uint8_t ax_scratch(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_SCRATCH);
}

/**
 * 5.2 set operating mode
 */
void ax_set_pwrmode(ax_config *config, uint8_t pwrmode)
{
    config->pwrmode = pwrmode;
    ax_hw_write_register_8(config, AX_REG_PWRMODE, 0x60 | pwrmode); /* TODO R-m-w */
}

/**
 * 5.5 - 5.6 set modulation and fec parameters
 */
void ax_set_modulation_parameters(ax_config *config, ax_modulation *mod)
{
    /* modulation */
    ax_hw_write_register_8(config, AX_REG_MODULATION, mod->modulation);

    /* encoding (inv, diff, scram, manch..) */
    if ((mod->encoding & AX_ENC_INV) && mod->fec)
    {
        /* FEC doesn't play with inversion */
        Log.warning(F("WARNING: Inversion is not supported in FEC! NOT INVERTING\r\n"));
        mod->encoding &= ~AX_ENC_INV; /* clear inv bit */
    }
    ax_hw_write_register_8(config, AX_REG_ENCODING, mod->encoding);

    /* framing */
    if (mod->fec && ((mod->framing & 0xE) != AX_FRAMING_MODE_HDLC))
    {
        /* FEC needs HDLC framing */
        Log.warning(F("WARNING: FEC needs HDLC! Forcing HDLC framing..\r\n"));
        mod->framing &= ~0xE;
        mod->framing |= AX_FRAMING_MODE_HDLC;
    }
    ax_hw_write_register_8(config, AX_REG_FRAMING, mod->framing);

    if ((mod->framing & 0xE) == AX_FRAMING_MODE_RAW_SOFT_BITS)
    {
        /* See 5.26 Performance Tuning */
        ax_hw_write_register_8(config, 0xF72, 0x06);
    }
    else
    {
        ax_hw_write_register_8(config, 0xF72, 0x00);
    }

    /* fec */
    if (mod->fec)
    {
        /* positive interleaver sync, 1/2 soft rx */
        ax_hw_write_register_8(config, AX_REG_FEC,
                               AX_FEC_POS | AX_FEC_ENA | (1 << 1));
        ax_hw_write_register_8(config, AX_REG_FECSYNC, 98);
    }
}

/**
 * 5.8 pin configuration
 */
void ax_set_pin_configuration(ax_config *config)
{
    ax_hw_write_register_8(config, AX_REG_PINFUNCSYSCLK, _pinfunc_sysclk);
    ax_hw_write_register_8(config, AX_REG_PINFUNCDCLK, _pinfunc_dclk);
    ax_hw_write_register_8(config, AX_REG_PINFUNCDATA, _pinfunc_data);
    ax_hw_write_register_8(config, AX_REG_PINFUNCANTSEL, _pinfunc_antsel);
    ax_hw_write_register_8(config, AX_REG_PINFUNCPWRAMP, _pinfunc_pwramp);
}

/**
 * Sets a PLL to a given frequency.
 *
 * returns the register value written
 */
uint32_t ax_set_freq_register(ax_config *config,
                              uint8_t reg, uint32_t frequency)
{
    uint32_t freq;

    /* we choose to always set the LSB to avoid spectral tones */
    freq = (uint32_t)(((double)frequency * (1 << 23)) /
                      (float)config->f_xtal);
    freq = (freq << 1) | 1;
    ax_hw_write_register_32(config, reg, freq);

    Log.trace(F("freq %d = %X\r\n"), (int)frequency, (unsigned int)freq);

    return freq;
}

/**
 * 5.10 set synthesiser frequencies
 */
void ax_set_synthesiser_frequencies(ax_config *config)
{
    if (config->synthesiser.A.frequency)
    {
        /* FREQA */
        config->synthesiser.A.register_value =
            ax_set_freq_register(config,
                                 AX_REG_FREQA, config->synthesiser.A.frequency);
    }
    if (config->synthesiser.B.frequency)
    {
        /* FREQB */
        config->synthesiser.B.register_value =
            ax_set_freq_register(config,
                                 AX_REG_FREQB, config->synthesiser.B.frequency);
    }
}

/**
 * Synthesiser parameters for ranging
 */
ax_synthesiser_parameters synth_ranging = {
    /* Internal Loop Filter 100kHz */
    .loop = AX_PLLLOOP_FILTER_DIRECT | AX_PLLLOOP_INTERNAL_FILTER_BW_100_KHZ,
    /* Charge Pump I = 68uA */
    .charge_pump_current = 8,
};

/**
 * Synthesiser parameters for operation
 */
ax_synthesiser_parameters synth_operation = {
    /* Internal Loop Filter 500kHz */
    .loop = AX_PLLLOOP_FILTER_DIRECT | AX_PLLLOOP_INTERNAL_FILTER_BW_500_KHZ,
    /* Charge Pump I = 136uA */
    .charge_pump_current = 16,
};

/**
 *  Synthesizer parameters for Tx
 *  ToDo: From Radiolab?
 */
ax_synthesiser_parameters synth_transmit = {
    .loop = AX_PLLLOOP_FILTER_DIRECT | AX_PLLLOOP_INTERNAL_FILTER_BW_100_KHZ,
    /* Charge Pump I = 17uA */
    .charge_pump_current = 2,
};

/**
 *  Synthesizer parameters for Rx
 */
ax_synthesiser_parameters synth_receive = {
    .loop = AX_PLLLOOP_FILTER_DIRECT | AX_PLLLOOP_INTERNAL_FILTER_BW_500_KHZ,
    /* Charge Pump I = 272uA */
    .charge_pump_current = 16,
};

/**
 * 5.10 set synthesiser parameters
 */
void ax_set_synthesiser_parameters(ax_config *config,
                                   ax_synthesiser_parameters *params,
                                   ax_synthesiser *synth,
                                   enum ax_vco_type vco_type)
{
    /* rfdiv */
    uint8_t vco_parameters =
        (synth->rfdiv == AX_RFDIV_1) ? AX_PLLVCODIV_RF_DIVIDER_DIV_TWO : 0;

    /* vco type */
    switch (vco_type)
    {
    case AX_VCO_INTERNAL_EXTERNAL_INDUCTOR:
        vco_parameters |= AX_PLLVCODIV_RF_INTERNAL_VCO_EXTERNAL_INDUCTOR;
        break;
    case AX_VCO_EXTERNAL:
        vco_parameters |= AX_PLLVCODIV_RF_EXTERNAL_VCO;
        break;
    default:
        vco_parameters |= AX_PLLVCODIV_RF_INTERNAL_VCO;
        break;
    }

    /* refdiv */
    // right now just setting it to the value we're using for 48 MHz TCXO
    vco_parameters |= AX_PLLVCODIV_DIVIDE_2;

    /* set registers */
    ax_hw_write_register_8(config, AX_REG_PLLLOOP, params->loop);
    ax_hw_write_register_8(config, AX_REG_PLLCPI, params->charge_pump_current);
    ax_hw_write_register_8(config, AX_REG_PLLVCODIV, vco_parameters);

    /* f34 (See 5.26) */
    if (vco_parameters & AX_PLLVCODIV_RF_DIVIDER_DIV_TWO)
    {
        ax_hw_write_register_8(config, 0xF34, 0x28);
    }
    else
    {
        ax_hw_write_register_8(config, 0xF34, 0x08);
    }
}

/**
 * 5.14 wakeup timer
 */
void ax_set_wakeup_timer(ax_config *config, ax_wakeup_config *wakeup_config)
{
    uint32_t period, xoearly;

    /* Assume the LPOSC is running at 640Hz (default) */

    if (wakeup_config)
    { /* program wakeup */
        period = (uint32_t)(config->wakeup_period_ms * 0.64);
        xoearly = (uint32_t)(config->wakeup_xo_early_ms * 0.64);
        if (period == 0)
        {
            period = 1;
        }
        if (xoearly == 0)
        {
            xoearly = 1;
        }

        ax_hw_write_register_8(config, AX_REG_WAKEUPFREQ, period);
    }
    else
    { /* always program this */
        xoearly = 1;
    }

    ax_hw_write_register_8(config, AX_REG_WAKEUPXOEARLY, xoearly);
}

/**
 * 5.15.8 - 5.15.10 set afsk receiver parameters
 */
void ax_set_afsk_rx_parameters(ax_config *config, ax_modulation *mod)
{
    uint16_t mark = mod->parameters.afsk.mark;
    uint16_t space = mod->parameters.afsk.space;
    uint16_t afskmark, afskspace;

    /* Mark */
    afskmark = (uint16_t)((((float)mark * (1 << 16) *
                            mod->par.decimation * config->f_xtaldiv) /
                           (float)config->f_xtal) +
                          0.5);
    ax_hw_write_register_16(config, AX_REG_AFSKMARK, afskmark);

    Log.trace(F("afskmark (rx) %d = %X\r\n"), mark, afskmark);

    /* Space */
    afskspace = (uint16_t)((((float)space * (1 << 16) *
                             mod->par.decimation * config->f_xtaldiv) /
                            (float)config->f_xtal) +
                           0.5);
    ax_hw_write_register_16(config, AX_REG_AFSKSPACE, afskspace);

    Log.trace(F("afskspace (rx) %d = %X\r\n"), space, afskspace);

    /* Detector Bandwidth */
    ax_hw_write_register_16(config, AX_REG_AFSKCTRL, mod->par.afskshift);
}

/**
 * 5.15 set receiver parameters
 */
void ax_set_rx_parameters(ax_config *config, ax_modulation *mod)
{
    /* IF Frequency */
    ax_hw_write_register_16(config, AX_REG_IFFREQ, mod->par.iffreq);

    Log.trace(F("WRITE IFFREQ %d\r\n"), (int)mod->par.iffreq);

    /* Decimation */
    ax_hw_write_register_8(config, AX_REG_DECIMATION, mod->par.decimation);

    /* RX Data Rate */
    ax_hw_write_register_24(config, AX_REG_RXDATARATE, mod->par.rx_data_rate);

    /* Max Data Rate offset */
    ax_hw_write_register_24(config, AX_REG_MAXDROFFSET, 0x0);
    /* 0. Therefore < 1% */

    /* Max RF offset - Correct offset at first LO */
    ax_hw_write_register_24(config, AX_REG_MAXRFOFFSET,
                            (AX_MAXRFOFFSET_FREQOFFSCORR_FIRST_LO |
                             mod->par.max_rf_offset));

    /* Maximum deviation of FSK Demodulator */
    switch (mod->modulation & 0xf)
    {
    case AX_MODULATION_FSK:
    case AX_MODULATION_MSK:
    case AX_MODULATION_AFSK:
        ax_hw_write_register_16(config, AX_REG_FSKDMAX, mod->par.fskd & 0xFFFF);
        ax_hw_write_register_16(config, AX_REG_FSKDMIN, ~mod->par.fskd & 0xFFFF);
        break;
    }

    /* Amplitude Lowpass filter */
    ax_hw_write_register_8(config, AX_REG_AMPLFILTER, mod->par.ampl_filter);

    // try setting the RSSI reference value...set to what Radiolab uses..this register is not documented
    // does not seem to allow writes, but radio lab expert settings lists it.
    ax_hw_write_register_8(config, 0x250, 0XF7);
}

/**
 * 5.15.15+ rx parameter sets
 */
void ax_set_rx_parameter_set(ax_config *config,
                             uint16_t ps, ax_rx_param_set *pars)
{
    uint8_t agcgain;
    uint8_t timegain, drgain;

    /* AGC Gain Attack/Decay */
    agcgain = ((pars->agc_decay & 0xF) << 4) | (pars->agc_attack & 0xF);
    ax_hw_write_register_8(config, ps + AX_RX_AGCGAIN, agcgain);

    /* AGC target value */
    /**
     * Always set to 132, which gives target output of 304 from 1023 counts
     */
    ax_hw_write_register_8(config, ps + AX_RX_AGCTARGET, 0x84);

    /* AGC digital threashold range */
    /**
     * Always set to zero, the analogue ADC always follows immediately
     */
    ax_hw_write_register_8(config, ps + AX_RX_AGCAHYST, 0x00);

    /* AGC minmax */
    /**
     * Always set to zero, this is probably best.
     */
    ax_hw_write_register_8(config, ps + AX_RX_AGCMINMAX, 0x00);

    /* Gain of timing recovery loop */
    timegain = ax_value_to_mantissa_exp_4_4(pars->time_gain);
    ax_hw_write_register_8(config, ps + AX_RX_TIMEGAIN, timegain);

    /* Gain of datarate recovery loop */
    drgain = ax_value_to_mantissa_exp_4_4(pars->dr_gain);
    ax_hw_write_register_8(config, ps + AX_RX_DRGAIN, drgain);

    /* Gain of phase recovery loop / decimation filter fractional b/w */
    ax_hw_write_register_8(config, ps + AX_RX_PHASEGAIN,
                           ((pars->filter_idx & 0x3) << 6) |
                               (pars->phase_gain & 0xF));

    /* Gain of baseband frequency recovery loop */
    ax_hw_write_register_8(config, ps + AX_RX_FREQUENCYGAINA,
                           pars->baseband_rg_phase_det);
    ax_hw_write_register_8(config, ps + AX_RX_FREQUENCYGAINB,
                           pars->baseband_rg_freq_det);

    /* Gain of RF frequency recovery loop */
    ax_hw_write_register_8(config, ps + AX_RX_FREQUENCYGAINC,
                           pars->rffreq_rg_phase_det);
    ax_hw_write_register_8(config, ps + AX_RX_FREQUENCYGAIND,
                           pars->rffreq_rg_freq_det);

    /* Amplitude Recovery Loop */
    ax_hw_write_register_8(config, ps + AX_RX_AMPLITUDEGAIN,
                           pars->amplflags | pars->amplgain);

    /* FSK Receiver Frequency Deviation */
    ax_hw_write_register_16(config, ps + AX_RX_FREQDEV, pars->freq_dev);

    /* TODO FOUR FSK */
    ax_hw_write_register_8(config, ps + AX_RX_FOURFSK, 0x16);

    /* BB Gain Block Offset Compensation Resistors */
    /**
     * Always 0x00
     */
    ax_hw_write_register_8(config, ps + AX_RX_BBOFFSRES, 0x00);
}

/**
 * 5.15.8 - 5.15.9 set afsk transmit parameters
 */
void ax_set_afsk_tx_parameters(ax_config *config, ax_modulation *mod)
{
    uint16_t mark = mod->parameters.afsk.mark;
    uint16_t space = mod->parameters.afsk.space;
    uint16_t afskmark, afskspace;

    /* Mark */
    afskmark = (uint16_t)((((float)mark * (1 << 18)) /
                           (float)config->f_xtal) +
                          0.5);
    ax_hw_write_register_16(config, AX_REG_AFSKMARK, afskmark);

    Log.trace(F("afskmark (tx) %d = %X\r\n"), mark, afskmark);

    /* Space */
    afskspace = (uint16_t)((((float)space * (1 << 18)) /
                            (float)config->f_xtal) +
                           0.5);
    ax_hw_write_register_16(config, AX_REG_AFSKSPACE, afskspace);

    Log.trace(F("afskspace (tx) %d = %X\r\n"), space, afskspace);
}

/**
 * helper function (5.16)
 *
 * return the lower two bits of modcfga for tx path.
 * will return TXSE only when _AX_TX_SE is defined;
 * will retrun TXDIFF only when _AX_TX_DIFF is defined;
 * #errors if neither is defined
 */
uint8_t ax_modcfga_tx_parameters_tx_path(enum ax_transmit_path path)
{
#ifndef _AX_TX_SE
#ifndef _AX_TX_DIFF
#error "You must define either _AX_TX_DIFF or _AX_TX_SE to build! Check your hw"
#endif
#endif

    /* main switch statement */
    switch (path)
    {
    case AX_TRANSMIT_PATH_SE:
#ifdef _AX_TX_SE
        return AX_MODCFGA_TXSE;
#else
        //Log.trace(F("Single ended transmit path NOT set!\r\n"));
        //Log.trace(F("Check this is okay on your hardware, and define _AX_TX_SE to enable.\r\n"));
        //Log.trace(F("Setting differential transmit path instead...\r\n"));
        return AX_MODCFGA_TXDIFF;
#endif
    case AX_TRANSMIT_PATH_DIFF:
#ifdef _AX_TX_DIFF
        return AX_MODCFGA_TXDIFF;
#else
        Log.trace(F("Differential transmit path NOT set!\r\n"));
        Log.trace(F("Check this is okay on your hardware, and define _AX_TX_DIFF to enable.\r\n"));
        Log.trace(F("Setting single ended transmit path instead...\r\n"));
        return AX_MODCFGA_TXSE;
#endif
    default:
        Log.error(F("Unknown transmit path!\r\n"));
#ifdef _AX_TX_DIFF
        return AX_MODCFGA_TXDIFF;
#else
        return AX_MODCFGA_TXSE;
#endif
    }
}

/**
 * 5.16 set transmitter parameters
 */
void ax_set_tx_parameters(ax_config *config, ax_modulation *mod)
{
    uint8_t modcfga;
    float p;
    uint16_t pwr;
    uint32_t deviation;
    uint32_t fskdev, txrate;

    /* frequency shaping mode of transmitter */
    ax_hw_write_register_8(config, AX_REG_MODCFGF, mod->shaping & 0x3);

    /* transmit path */
    modcfga = ax_modcfga_tx_parameters_tx_path(config->transmit_path);
    /* amplitude shaping mode of transmitter */
    switch (mod->modulation & 0xf)
    {
    default:
        modcfga |= AX_MODCFGA_AMPLSHAPE_RAISED_COSINE; // temporarily removed
        // modcfga |= 0x10;  // ramp timing
        break;
    }
    ax_hw_write_register_8(config, AX_REG_MODCFGA, modcfga);

    /* TX deviation */
    switch (mod->modulation & 0xf)
    {
    default:
    case AX_MODULATION_PSK: /* PSK */
        fskdev = 0;
        break;
    case AX_MODULATION_MSK: /* MSK */
    case AX_MODULATION_FSK: /* FSK */

        deviation = (mod->par.m * 0.5 * mod->bitrate);

        fskdev = (uint32_t)((((float)deviation * (1 << 24)) /
                             (float)config->f_xtal) +
                            0.5);
        break;
    case AX_MODULATION_AFSK: /* AFSK */

        deviation = mod->parameters.afsk.deviation;

        fskdev = (uint32_t)((((float)deviation * (1 << 24) * 0.858785) /
                             (float)config->f_xtal) +
                            0.5);

        break;
    }
    ax_hw_write_register_24(config, AX_REG_FSKDEV, fskdev);
    Log.trace(F("fskdev %d = %X\r\n"), (int)deviation, (unsigned int)fskdev);

    /* TX bitrate. We assume bitrate < f_xtal */
    txrate = (uint32_t)((((float)mod->bitrate * (1 << 24)) /
                         (float)config->f_xtal) +
                        0.5);
    ax_hw_write_register_24(config, AX_REG_TXRATE, txrate);

    Log.trace(F("bitrate %d = %X\r\n"), (int)mod->bitrate, (unsigned int)txrate);

    /* check bitrate for asynchronous wire mode */
    if (1 && mod->bitrate >= config->f_xtal / 32)
    {
        Log.warning(F("for asynchronous wire mode, bitrate must be less than f_xtal/32\r\n"));
    }

    /* TX power */
    if (config->transmit_power_limit > 0)
    {
        p = MIN(mod->power, config->transmit_power_limit);
    }
    else
    {
        p = mod->power;
    }
    pwr = (uint16_t)((p * (1 << 12)) + 0.5);
    pwr = (pwr > 0xFFF) ? 0xFFF : pwr; /* max 0xFFF */
    Log.trace(F("power value: %X\r\n"), pwr);
    ax_hw_write_register_16(config, AX_REG_TXPWRCOEFFB, pwr);

    Log.trace(F("power %f = %X\r\n"), mod->power, pwr);
}

/**
 * 5.17 set PLL parameters
 */
void ax_set_pll_parameters(ax_config *config)
{
    uint8_t pllrngclk_div;

    /* VCO Current - 1250 uA VCO1, 250 uA VCO2 */
    ax_hw_write_register_8(config, AX_REG_PLLVCOI,
                           AX_PLLVCOI_ENABLE_MANUAL | 25);

    /* PLL Ranging Clock */
    pllrngclk_div = AX_PLLRNGCLK_DIV_8192; // this was originallly 2048.  I bumped it up to 8192 to match radiolab. this value makes more sense with a 48MHz clock - tkc.
    ax_hw_write_register_8(config, AX_REG_PLLRNGCLK, pllrngclk_div);
    /* approx 8kHz for 16MHz clock */
    config->f_pllrng = config->f_xtal / (1 << (8 + pllrngclk_div));
    /* NOTE: config->f_pllrng should be less than 1/10 of the loop filter b/w */
    /* 8kHz is fine, as minimum loop filter b/w is 100kHz */
    Log.trace(F("Ranging clock f_pllrng %d Hz\r\n"), (int)config->f_pllrng);
}

/**
 * 5.18 set xtal parameters
 */
void ax_set_xtal_parameters(ax_config *config)
{
    uint8_t xtalcap;
    uint8_t xtalosc;
    uint8_t xtalampl;
    uint8_t f35;

    /* Load Capacitance */
    if ((config->clock_source == AX_CLOCK_SOURCE_CRYSTAL) &&
        (config->load_capacitance != 0))
    {

        if (config->load_capacitance == 3)
        {
            xtalcap = 0;
        }
        else if (config->load_capacitance == 8)
        {
            xtalcap = 1;
        }
        else if ((config->load_capacitance >= 9) &&
                 (config->load_capacitance <= 39))
        {
            xtalcap = (config->load_capacitance - 8) << 1;
        }
        else
        {
            Log.trace(F("xtal load capacitance %d not supported\r\n"),
                         config->load_capacitance);
            xtalcap = 0;
        }

        ax_hw_write_register_8(config, AX_REG_XTALCAP, xtalcap);
    }

    /* Crystal Oscillator Control */
    // I modified this code to correct logical error.  TCXO should take precedence over frequency.  this is untested.
    if (config->clock_source == AX_CLOCK_SOURCE_TCXO)
    {
        xtalosc = 0x04; /* TCXO */
    }
    else if (config->f_xtal > 43 * 1000 * 1000)
    {
        xtalosc = 0x0D; /* > 43 MHz */
    }
    else
    {
        xtalosc = 0x03; /*  */
    }
    ax_hw_write_register_8(config, AX_REG_XTALOSC, xtalosc);

    /* Crystal Oscillator Amplitude Control */
    if (config->clock_source == AX_CLOCK_SOURCE_TCXO)
    {
        xtalampl = 0x00;
    }
    else
    {
        xtalampl = 0x07;
    }
    ax_hw_write_register_8(config, AX_REG_XTALAMPL, xtalampl);

    /* F35 */
    if (config->f_xtal < 24800 * 1000)
    {
        f35 = 0x10; /* < 24.8 MHz */
        config->f_xtaldiv = 1;
    }
    else
    {
        f35 = 0x11;
        config->f_xtaldiv = 2;
    }
    ax_hw_write_register_8(config, 0xF35, f35);
}

/**
 * 5.19 set baseband parameters
 */
void ax_set_baseband_parameters(ax_config *config)
{
    ax_hw_write_register_8(config, AX_REG_BBTUNE, 0x0F);
    /* Baseband tuning value 0xF */

    ax_hw_write_register_8(config, AX_REG_BBOFFSCAP, 0x77);
    /* Offset capacitors all ones */
}

/**
 * 5.20 set packet format parameters
 */
void ax_set_packet_parameters(ax_config *config, ax_modulation *mod)
{
    if (mod->il2p_enabled)
    {
        ax_hw_write_register_8(config, AX_REG_PKTADDRCFG, 
        ((mod->par.fec_sync_dis & 1) << 5) | 0x80); 
        //address at position 1 - was 0x01 doesn't matter, adddress not used.  0x80 = MSB first
    }
    else
    {
        ax_hw_write_register_8(config, AX_REG_PKTADDRCFG,
                           ((mod->par.fec_sync_dis & 1) << 5) |
                               0x00); 
        // address at position 1 - was 0x01
    }

    if (mod->fixed_packet_length)
    { /* fixed packet length */
        /* use pktlencfg as fixed length value */
        ax_hw_write_register_8(config, AX_REG_PKTLENCFG, 0x00);
        /* fixed length */
        ax_hw_write_register_8(config, AX_REG_PKTLENOFFSET,
                               mod->fixed_packet_length);
    }
    else
    { /* variable packet length */
        /* 8 significant bits on length byte */
        ax_hw_write_register_8(config, AX_REG_PKTLENCFG, 0x80); //was 80
        // 0x80 => 1000 0000 = 8 significant bits in length byte (255), length byte in position 0
        // should be F0 for arbitrary length packets, 
        // but doesn't matter for HDLC (they can be any length and a delimited)
        // zero offset on length byte
        ax_hw_write_register_8(config, AX_REG_PKTLENOFFSET, 0x00);
    }

    // Maximum packet length - 255 bytes  //part of setting for arbitray length packets
    ax_hw_write_register_8(config, AX_REG_PKTMAXLEN, 0xFF);
}

/**
 * 5.21 pattern match
 */
void ax_set_pattern_match_parameters(ax_config *config, ax_modulation *mod)
{
    switch (mod->framing & 0xE)
    {
    case AX_FRAMING_MODE_HDLC: /* HDLC */
        ax_hw_write_register_16(config, AX_REG_MATCH1PAT, 0x5555);
        // 16 byte write...MATCH1PAT0 & MATCH1PAT1
        // note that inversion is not ignored in the preamble, so we get 55's instead of AA's

        // match uses Raw received bits, 11-bit pattern, pattern match length is "A" + 1, "8" means it matches on raw received bits
        // MATCH1 is a 16 bit match (it's the first preamble match...MATCH0 is the second, go figure...)
        ax_hw_write_register_8(config, AX_REG_MATCH1LEN, 0x8A);

        // the length of the pattern.
        /* signal a match if received bitstream matches for more than n bits */
        ax_hw_write_register_8(config, AX_REG_MATCH1MAX, mod->par.match1_threashold);

        // AX_REG_MATCH1MIN means the receiver signals a match if it's less than MATCH1MIN positions.
        // this is for detecting an inverse pattern
        ax_hw_write_register_8(config, AX_REG_MATCH1MIN, 0); // added by tkc 7/30/24...setting just to be safe

        // experimental
        /* Match 0 - preamble 2 */
        // ax_hw_write_register_32(config, AX_REG_MATCH0PAT, 0x81818181); //a 32 bit run of 0x81
        // ax_hw_write_register_32(config, AX_REG_MATCH0PAT, 0xAACCAACC); //match 0xAACCAACC
        ax_hw_write_register_32(config, AX_REG_MATCH0PAT, 0x55555555); // match 0xAACCAACC

        // decoded bits, 32-bit pattern
        // in radiolab, it looks like it turns MATCH0 off...I'm trying to turn it back on -- tkc 8/12/24
        //ax_hw_write_register_8(config, AX_REG_MATCH0LEN, 0); // TODO: I put it back for a test
         ax_hw_write_register_8(config, AX_REG_MATCH0LEN, 0x9E); //length of match is 31+1 (32 bits), match on raw data (1001 1110)
        /* signal a match if recevied bitstream matches for more than 28 bits */
        ax_hw_write_register_8(config, AX_REG_MATCH0MAX, mod->par.match0_threashold);
        ax_hw_write_register_8(config, AX_REG_MATCH0MIN, 0); // added by tkc 7/30/24
        break;

    default: /* Preamble and Sync Vector */
        /* Match 1 - initial preamble */
        ax_hw_write_register_16(config, AX_REG_MATCH1PAT, 0x5555);
        /* Raw received bits, 11-bit pattern */
        ax_hw_write_register_8(config, AX_REG_MATCH1LEN, 0x8A);
        /* signal a match if recevied bitstream matches for more than 10 bits */
        ax_hw_write_register_8(config, AX_REG_MATCH1MAX, mod->par.match1_threashold);
        ax_hw_write_register_8(config, AX_REG_MATCH1MIN, 1);

        /* Match 0 - sync vector */
        //ax_hw_write_register_32(config, AX_REG_MATCH0PAT, 0xCCAACCAA);
        ax_hw_write_register_32(config, AX_REG_MATCH0PAT, 0x55335533);
        /* decoded bits, 32-bit pattern */
        ax_hw_write_register_8(config, AX_REG_MATCH0LEN, 0x1F);
        /* signal a match if recevied bitstream matches for more than 28 bits */
        ax_hw_write_register_8(config, AX_REG_MATCH0MAX, mod->par.match0_threashold);
        ax_hw_write_register_8(config, AX_REG_MATCH0MIN, 1);
        break;
    }
}

/**
 * 5.22 packet controller parameters
 */
void ax_set_packet_controller_parameters(ax_config *config, ax_modulation *mod,
                                         ax_wakeup_config *wakeup_config)
{
    /* tx pll boost time */
    ax_hw_write_register_8(config, AX_REG_TMGTXBOOST,
                           ax_value_to_exp_mantissa_3_5(mod->par.tx_pll_boost_time));

    /* tx pll settle time */
    ax_hw_write_register_8(config, AX_REG_TMGTXSETTLE,
                           ax_value_to_exp_mantissa_3_5(mod->par.tx_pll_settle_time));

    /* rx pll boost time */
    ax_hw_write_register_8(config, AX_REG_TMGRXBOOST,
                           ax_value_to_exp_mantissa_3_5(mod->par.rx_pll_boost_time));

    /* rx pll settle time */
    ax_hw_write_register_8(config, AX_REG_TMGRXSETTLE,
                           ax_value_to_exp_mantissa_3_5(mod->par.rx_pll_settle_time));

    /* 0us bb dc offset aquis tim */
    ax_hw_write_register_8(config, AX_REG_TMGRXOFFSACQ, 0x00);

    /* rx agc coarse*/
    ax_hw_write_register_8(config, AX_REG_TMGRXCOARSEAGC,
                           ax_value_to_exp_mantissa_3_5(mod->par.rx_coarse_agc));

    /* rx agc settling time */
    ax_hw_write_register_8(config, AX_REG_TMGRXAGC,
                           ax_value_to_exp_mantissa_3_5(mod->par.rx_agc_settling));

    /* rx rssi settling time */
    ax_hw_write_register_8(config, AX_REG_TMGRXRSSI,
                           ax_value_to_exp_mantissa_3_5(mod->par.rx_rssi_settling));

    if (wakeup_config)
    { /* wakeup */
        /* preamble 1 timeout */
        ax_hw_write_register_8(config, AX_REG_TMGRXPREAMBLE1,
                               ax_value_to_exp_mantissa_3_5(wakeup_config->wakeup_duration_bits));
    }
    else
    {
        ax_hw_write_register_8(config, AX_REG_TMGRXPREAMBLE1,
                               ax_value_to_exp_mantissa_3_5(mod->par.preamble_1_timeout));
    }

    /* preamble 2 timeout */
    ax_hw_write_register_8(config, AX_REG_TMGRXPREAMBLE2,
                           ax_value_to_exp_mantissa_3_5(mod->par.preamble_2_timeout));

    /* rssi threashold */
    if (wakeup_config)
    { /* wakeup */
        ax_hw_write_register_8(config, AX_REG_RSSIABSTHR, wakeup_config->rssi_abs_thr);
    }

    /* 0 - don't detect busy channel */
    ax_hw_write_register_8(config, AX_REG_BGNDRSSITHR, 0x00);

    /* max chunk size = 240 bytes - largest possible */
    ax_hw_write_register_8(config, AX_REG_PKTCHUNKSIZE,
                           AX_PKT_MAXIMUM_CHUNK_SIZE_240_BYTES);

    /* write pkt_misc_flags */
    ax_hw_write_register_8(config, AX_REG_PKTMISCFLAGS, mod->par.pkt_misc_flags);

    /* metadata to store */
    ax_hw_write_register_8(config, AX_REG_PKTSTOREFLAGS,
                           config->pkt_store_flags);

    /* packet accept flags. always accept some things, more from config */
    if (mod->il2p_enabled == true)
    {
        ax_hw_write_register_8(config, AX_REG_PKTACCEPTFLAGS,
                               AX_PKT_ACCEPT_MULTIPLE_CHUNKS |  /* (LRGP) */  //tkc - now accepting multiple chunks
                               //AX_PKT_ACCEPT_SIZE_FAILURES |
                               AX_PKT_ACCEPT_ADDRESS_FAILURES | /* (ADDRF) */
                               AX_PKT_ACCEPT_CRC_FAILURES |
                                // AX_PKT_ACCEPT_ABORTED | /* (ABORTED) (for testing only)*/
                               //AX_PKT_ACCEPT_RESIDUE |          /* (RESIDUE) */
                                   config->pkt_accept_flags);
    }
    else
    {
        ax_hw_write_register_8(config, AX_REG_PKTACCEPTFLAGS,
                               AX_PKT_ACCEPT_MULTIPLE_CHUNKS |  /* (LRGP) */  //tkc - now accepting multiple chunks
                               //AX_PKT_ACCEPT_SIZE_FAILURES |
                               AX_PKT_ACCEPT_ADDRESS_FAILURES | /* (ADDRF) */
                               // AX_PKT_ACCEPT_CRC_FAILURES |
                               // AX_PKT_ACCEPT_ABORTED | /* (ABORTED) (for testing only)*/
                               //AX_PKT_ACCEPT_RESIDUE |          /* (RESIDUE) */
                               config->pkt_accept_flags);
    }
}

/**
 * 5.24 low power oscillator
 */
void ax_set_low_power_osc(ax_config *config, ax_wakeup_config *wakeup_config)
{
    uint32_t refdiv;

    if (wakeup_config)
    { /* lposc used for wakeups */
        /* set reference for calibration */
        refdiv = (uint32_t)((float)config->f_xtal / 640.0);
        if (refdiv > 0xffff)
        {
            /* could happen for f_xtals > 41 MHz */
            /* this is an error, but we set a reasonable value */
            refdiv = 0xffff;
        }
        ax_hw_write_register_8(config, AX_REG_LPOSCREF, refdiv & 0xffff);

        /* config */
        ax_hw_write_register_8(config, AX_REG_LPOSCCONFIG,
                               AX_LPOSC_ENABLE |
                                   AX_LPOSC_640_HZ |
                                   AX_LPOSC_CALIBF); /* calib on falling edge */
    }
}

/**
 * 5.25 digital to analog converter
 */
void ax_set_digital_to_analog_converter(ax_config *config)
{
    ax_hw_write_register_8(config, AX_REG_DACVALUE, 0xC); /* Shift down top 12 bits */
    ax_hw_write_register_8(config, AX_REG_DACCONFIG, AX_DAC_MODE_DELTA_SIGMA | config->dac_config);
}

/**
 * 5.26 'performance tuning'
 */
void ax_set_performance_tuning(ax_config *config, ax_modulation *mod)
{
    /**
     * TODO
     */
    ax_hw_write_register_8(config, AX_REG_REF, 0x03); /* 0xF0D */

    ax_hw_write_register_8(config, 0xF1C, 0x07); /* const */

    switch (mod->par.perftuning_option)
    {
    case 1:
        /* axradiolab */
        ax_hw_write_register_8(config, 0xF21, 0x68); /* !! */
        ax_hw_write_register_8(config, 0xF22, 0xFF); /* !! */
        ax_hw_write_register_8(config, 0xF23, 0x84); /* !! */
        ax_hw_write_register_8(config, 0xF26, 0x98); /* !! */
        break;
    default:
        /* datasheet */
        ax_hw_write_register_8(config, 0xF21, 0x5c); /* !! */
        ax_hw_write_register_8(config, 0xF22, 0x53); /* !! */
        ax_hw_write_register_8(config, 0xF23, 0x76); /* !! */
        ax_hw_write_register_8(config, 0xF26, 0x92); /* !! */
    }

    ax_hw_write_register_8(config, 0xF44, 0x25); /* !! */
                                                 // ax_hw_write_register_8(config, 0xF44, 0x24); /* !! */
}

/**
 * register settings
 */
void ax_set_registers(ax_config *config, ax_modulation *mod,
                      ax_wakeup_config *wakeup_config)
{
    // MODULATION, ENCODING, FRAMING, FEC
    ax_set_modulation_parameters(config, mod);

    // PINFUNC
    ax_set_pin_configuration(config);

    // WAKEUP
    ax_set_wakeup_timer(config, wakeup_config);

    // IFFREQ, DECIMATION, RXDATARATE, MAXRFOFFSET, FSKD
    ax_set_rx_parameters(config, mod);

    // AGC, TIMEGAIN, DRGAIN, PHASEGAIN, FREQUENCYGAIN, AMPLITUDEGAIN, FREQDEV
    if (mod->continuous)
    {                                                             /* continuous transmission */
        ax_hw_write_register_8(config, AX_REG_RXPARAMSETS, 0xFF); /* 3, 3, 3, 3 */
        ax_set_rx_parameter_set(config, AX_REG_RX_PARAMETER3, &mod->par.rx_param_sets[3]);
    }
    else
    {                                                             /* occasional packets */
        ax_hw_write_register_8(config, AX_REG_RXPARAMSETS, 0xF4); /* 0, 1, 3, 3 */
        ax_set_rx_parameter_set(config, AX_REG_RX_PARAMETER0, &mod->par.rx_param_sets[0]);
        ax_set_rx_parameter_set(config, AX_REG_RX_PARAMETER1, &mod->par.rx_param_sets[1]);
        ax_set_rx_parameter_set(config, AX_REG_RX_PARAMETER3, &mod->par.rx_param_sets[3]);
    }

    // MODCFG, FSKDEV, TXRATE, TXPWRCOEFF
    ax_set_tx_parameters(config, mod);

    // PLLVCOI, PLLRNGCLK
    ax_set_pll_parameters(config);

    // BBTUNE, BBOFFSCAP
    ax_set_baseband_parameters(config);

    // PKTADDRCFG, PKTLENCFG
    ax_set_packet_parameters(config, mod);

    // MATCH0PAT, MATCH1PAT
    ax_set_pattern_match_parameters(config, mod);

    // TMGRX, RSSIABSTHR, PKTCHUNKSIZE, PKTACCEPTFLAGS
    ax_set_packet_controller_parameters(config, mod, wakeup_config);

    // LPOSC
    ax_set_low_power_osc(config, wakeup_config);

    // DACCONFIG
    ax_set_digital_to_analog_converter(config);

    // 0xFxx
    ax_set_performance_tuning(config, mod);
}

/**
 * register settings for transmit
 */
void ax_set_registers_tx(ax_config *config, ax_modulation *mod)
{
    ax_set_synthesiser_parameters(config,
                                  &synth_transmit,
                                  &config->synthesiser.A,
                                  config->synthesiser.vco_type); // changed from synth_operation to synth_transmit to match radiolab

    /* AFSK */
    if ((mod->modulation & 0xf) == AX_MODULATION_AFSK)
    {
        ax_set_afsk_tx_parameters(config, mod);
    }

    ax_hw_write_register_8(config, 0xF00, 0x0F); /* const */
    ax_hw_write_register_8(config, 0xF18, 0x06); /* ?? */
}

/**
 * register settings for receive
 */
void ax_set_registers_rx(ax_config *config, ax_modulation *mod)
{
    ax_set_synthesiser_parameters(config,
                                  &synth_receive,
                                  &config->synthesiser.B,
                                  config->synthesiser.vco_type);

    /* AFSK */
    if ((mod->modulation & 0xf) == AX_MODULATION_AFSK)
    {
        ax_set_afsk_rx_parameters(config, mod);
    }

    ax_hw_write_register_8(config, 0xF00, 0x0F); /* const */
    ax_hw_write_register_8(config, 0xF18, 0x02); /* ?? */
}

/**
 * VCO FUNCTIONS ------------------------------------------
 */

/**
 * Performs a ranging operation
 *
 * updates values in synth structure
 */
enum ax_vco_ranging_result ax_do_vco_ranging(ax_config *config,
                                             uint16_t pllranging,
                                             ax_synthesiser *synth,
                                             enum ax_vco_type vco_type)
{
    uint8_t r;

    /* set vco range (VCOR) to 8 if unknown */
    synth->vco_range = (synth->vco_range_known == 0) ? 8 : synth->vco_range;

    /* set rf div (RFDIV) if unknown */
    if (synth->rfdiv == AX_RFDIV_UKNOWN)
    {
        synth->rfdiv = (synth->frequency < 525 * 1000 * 1000) ? AX_RFDIV_1 : AX_RFDIV_0;
    }

    /* Set default 100kHz loop BW for ranging */
    ax_set_synthesiser_parameters(config, &synth_ranging, synth, vco_type);

    /* Set RNGSTART bit (PLLRANGINGA,B) */
    ax_hw_write_register_8(config, pllranging,
                           synth->vco_range | AX_PLLRANGING_RNG_START);

    /* Wait for RNGSTART bit to clear */
    do
    {
        r = ax_hw_read_register_8(config, pllranging);
    } while (r & AX_PLLRANGING_RNG_START);

    /* Check RNGERR bit */
    if (r & AX_PLLRANGING_RNGERR)
    {
        /* ranging error */
        Log.error(F("Ranging error!\r\n"));
        return AX_VCO_RANGING_FAILED;
    }

    Log.trace(F("Ranging done r = %X\r\n"), r);

    /* Update vco_range */
    synth->vco_range = r & 0xF;
    synth->vco_range_known = 1;
    synth->frequency_when_last_ranged = synth->frequency;

    return AX_VCO_RANGING_SUCCESS;
}

/**
 * Ranges both VCOs
 *
 * re-ranging is required for > 5MHz in 868/915 or > 2.5MHz in 433
 */
enum ax_vco_ranging_result ax_vco_ranging(ax_config *config)
{
    enum ax_vco_ranging_result resultA, resultB;

    Log.trace(F("starting vco ranging...\r\n"));

    /* Enable TCXO if used */
    if (config->tcxo_enable)
    {
        config->tcxo_enable();
    }

    /* Set PWRMODE to STANDBY */
    ax_set_pwrmode(config, AX_PWRMODE_STANDBY);

    /* Set FREQA,B registers to correct value */
    ax_set_synthesiser_frequencies(config);

    /* Manual VCO current, 27 = 1350uA VCO1, 270uA VCO2 */
    ax_hw_write_register_8(config, AX_REG_PLLVCOI,
                           AX_PLLVCOI_ENABLE_MANUAL | 27);

    /* Wait for oscillator to be stable */
    ax_wait_for_oscillator(config);

    /* do ranging */
    resultA = ax_do_vco_ranging(config, AX_REG_PLLRANGINGA,
                                &config->synthesiser.A, config->synthesiser.vco_type);
    resultB = ax_do_vco_ranging(config, AX_REG_PLLRANGINGB,
                                &config->synthesiser.B, config->synthesiser.vco_type);

    /* Set PWRMODE to POWERDOWN */
    // see note below about wake on radio.  I don't believe you have to shut down the chip
    ax_set_pwrmode(config, AX_PWRMODE_POWERDOWN);

    /* Disable TCXO if used */
    // this doesn't really do anything since the tcxo_disable function isn't defined.  I think this was to support
    // lower power modes using wake on radio --tkc 8/12/24
    if (config->tcxo_disable)
    {
        config->tcxo_disable();
    }

    if (((resultA == AX_VCO_RANGING_SUCCESS) ||     /* success */
         (config->synthesiser.A.frequency == 0)) && /* or not used */
        ((resultB == AX_VCO_RANGING_SUCCESS) ||     /* success */
         (config->synthesiser.B.frequency == 0)))   /* or not used */
    {
        return AX_VCO_RANGING_SUCCESS; /* currently assume with need both VCOs */
    }

    return AX_VCO_RANGING_FAILED;
}

/**
 * PUBLIC FUNCTIONS ------------------------------------------
 */

/**
 * set tweakable parameters to their default values
 */
void ax_default_params(ax_config *config, ax_modulation *mod)
{
    ax_populate_params(config, mod, &mod->par);
}

/**
 * adjust frequency registers
 *
 * frequency A
 */
int ax_adjust_frequency_A(ax_config *config, uint32_t frequency)
{
    uint8_t radiostate;
    int32_t delta_f;
    uint32_t abs_delta_f;
    ax_synthesiser *synth = &config->synthesiser.A;

    if (config->pwrmode == AX_PWRMODE_DEEPSLEEP)
    {
        /* can't do anything in deepsleep */
        // this should cause a reset from the external watchdog.
        // TODO:  look into storing failure modes in a non-volatile variable (log)
        Log.error(F("in deep sleep for some reason\r\n"));
        while (1)
            ;
        return AX_INIT_PORT_FAILED;
    }

    // detect if we're in wire mode, if so we're going to be stuck in FULLTX, so we need to drop to STANDBY while we re-range
    if (ax_hw_read_register_8(config, AX_REG_PINFUNCDATA) == 0x84)
    {
        // if so, change power state to STANDBY
        Log.trace(F("changing to STANDBY\r\n"));
        ax_set_pwrmode(config, AX_PWRMODE_STANDBY);
    }

    /* wait for current operations to finish */
    do
    {
        radiostate = ax_hw_read_register_8(config, AX_REG_RADIOSTATE) & 0xF;
        Log.trace(F("waiting on radiostate: %X\r\n"), radiostate);
    } while (radiostate == AX_RADIOSTATE_TX);

    /* set new frequency */
    synth->frequency = frequency;

    /* frequency difference since last ranging */
    delta_f = synth->frequency_when_last_ranged - frequency;
    abs_delta_f = (delta_f < 0) ? -delta_f : delta_f; /* abs */

    /* if ∆f > f/256 (2.05MHz @ 525MHz) */
    if (abs_delta_f > (synth->frequency_when_last_ranged / 256))
    {
        /* Need to re-range VCO */
        Log.trace(F("need to re-range the VCO\r\n"));

        /* clear assumptions about frequency */
        synth->rfdiv = AX_RFDIV_UKNOWN;
        synth->vco_range_known = 0;

        // everything up to here only applied to VCO A
        // before ranging, we need to set the synth frequencies
        // this is done in ax_vco_ranging.
        Log.trace(F("frequency check: %i\r\n"), config->synthesiser.A.frequency);
        /* re-range both VCOs */
        if (ax_vco_ranging(config) != AX_VCO_RANGING_SUCCESS)
        {
            Log.error(F("ranging failed\r\n"));
            // TODO: create a log entry
            return AX_INIT_VCO_RANGING_FAILED;
        }
        // ax_vco_ranging leaves the chip in POWERDOWN, with VCO B selected
    }
    else
    {
        /* no need to re-range */
        Log.trace(F("no need it says, check the next command!\r\n"));
        ax_set_synthesiser_frequencies(config);
    }

    // why not always return to FULL_TX?

    // detect if we're in wire mode, and if so we are sweeping, so go back to transmitting
    if (ax_hw_read_register_8(config, AX_REG_PINFUNCDATA) == 0x84)
    {
        // if so, change power state to FULLTX
        Log.trace(F("returning to FULLTX\r\n"));
        ax_set_pwrmode(config, AX_PWRMODE_FULLTX);
    }

    return AX_INIT_OK;
}

/**
 * adjust frequency registers
 *
 * frequency B - always used for receiving
 */
int ax_adjust_frequency_B(ax_config *config, uint32_t frequency)
{
    uint8_t radiostate;
    int32_t delta_f;
    uint32_t abs_delta_f;
    ax_synthesiser *synth = &config->synthesiser.B;

    if (config->pwrmode == AX_PWRMODE_DEEPSLEEP)
    {
        /* can't do anything in deepsleep */
        // TODO:  look into storing failure modes in a non-volatile variable (log)
        Log.warning(F("in deep sleep for some reason\r\n"));
        while (1)
            ;
        return AX_INIT_PORT_FAILED;
    }

    // detect if we're in wire mode, if so we're going to be stuck in FULLTX, so we need to drop to STANDBY while we re-range
    if (ax_hw_read_register_8(config, AX_REG_PINFUNCDATA) == 0x84)
    {
        // if so, change power state to STANDBY
        Log.trace(F("changing to STANDBY\r\n"));
        ax_set_pwrmode(config, AX_PWRMODE_STANDBY);
    }

    /* wait for current operations to finish */
    do
    {
        radiostate = ax_hw_read_register_8(config, AX_REG_RADIOSTATE) & 0xF;
        Log.trace(F("waiting on radiostate\r\n"));
    } while (radiostate == AX_RADIOSTATE_TX);

    /* set new frequency */
    synth->frequency = frequency;

    /* frequency difference since last ranging */
    delta_f = synth->frequency_when_last_ranged - frequency;
    abs_delta_f = (delta_f < 0) ? -delta_f : delta_f; /* abs */

    /* if ∆f > f/256 (2.05MHz @ 525MHz) */
    if (abs_delta_f > (synth->frequency_when_last_ranged / 256))
    {
        /* Need to re-range VCO */

        /* clear assumptions about frequency */
        synth->rfdiv = AX_RFDIV_UKNOWN;
        synth->vco_range_known = 0;

        Log.trace(F("frequency check: %i\r\n"), config->synthesiser.B.frequency);

        /* re-range both VCOs */
        if (ax_vco_ranging(config) != AX_VCO_RANGING_SUCCESS)
        {
            Log.error(F("ranging failed\r\n"));
            return AX_INIT_VCO_RANGING_FAILED;
        }
        // ax_vco_ranging leaves the chip in POWERDOWN, with VCO B selected
    }
    else
    {
        /* no need to re-range */
        Log.trace(F("no need it says, check the next command!\r\n"));
        ax_set_synthesiser_frequencies(config);
    }

    // Set power mode to full RX
    ax_set_pwrmode(config, AX_PWRMODE_FULLRX);

    // detect if we're in wire mode, and if so we are sweeping, so go back to transmitting
    /*
    if (ax_hw_read_register_8(config, AX_REG_PINFUNCDATA) == 0x84)
    {
      //if so, change power state to FULLRX
      Log.trace(F("returning to FULLRX\r\n"));
      ax_set_pwrmode(config, AX_PWRMODE_FULLRX);
    }
    */
    return AX_INIT_OK;
}

/**
 * force quick update of frequency registers
 *
 * * delta with current frequency f must be < f/256
 * * must be in a suitable mode to do this
 * * frequency A
 */
int ax_force_quick_adjust_frequency_A(ax_config *config, uint32_t frequency)
{
    ax_synthesiser *synth = &config->synthesiser.A;

    /* set new frequency */
    synth->frequency = frequency;

    /* don't re-range, just change */
    ax_set_synthesiser_frequencies(config);

    return AX_INIT_OK;
}

/**
 * force quick update of frequency registers
 *
 * * delta with current frequency f must be < f/256
 * * must be in a suitable mode to do this
 * * frequency B
 */
int ax_force_quick_adjust_frequency_B(ax_config *config, uint32_t frequency)
{
    ax_synthesiser *synth = &config->synthesiser.B;

    /* set new frequency */
    synth->frequency = frequency;

    /* don't re-range, just change */
    ax_set_synthesiser_frequencies(config);

    return AX_INIT_OK;
}

/**
 * Configure and switch to FULLTX
 */
void ax_tx_on(ax_config *config, ax_modulation *mod)
{
    if (mod->par.is_params_set != 0x51)
    {
        Log.error(F("mod->par must be set first! call ax_default_params...\r\n"));
        // TODO:  look into storing failure modes in a non-volatile variable (log)
        while (1)
            ;
    }

    Log.trace(F("going for transmit...\r\n"));

    /* Registers */
    ax_set_registers(config, mod, NULL);
    ax_set_registers_tx(config, mod);

    /* Enable TCXO if used */
    if (config->tcxo_enable)
    {
        config->tcxo_enable();
    }

    /* Clear FIFO */
    ax_fifo_clear(config);

    /* Place chip in FULLTX mode */
    ax_set_pwrmode(config, AX_PWRMODE_FULLTX);

    /* Wait for oscillator to start running  */
    ax_wait_for_oscillator(config);

    /* Set PWRMODE to POWERDOWN */
    // ax_set_pwrmode(AX_PWRMODE_DEEPSLEEP);

    /* Disable TCXO if used */
    if (config->tcxo_disable)
    {
        config->tcxo_disable();
    }
}

/**
 * Loads packet into the FIFO for transmission
 */
void ax_tx_packet(ax_config *config, ax_modulation *mod,
                  uint8_t *packet, uint16_t length)
{
    if (config->pwrmode != AX_PWRMODE_FULLTX)
    {
        Log.error(F("PWRMODE must be FULLTX before writing to FIFO!\r\n"));
        return;
    }

    /* Ensure the SVMODEM bit (POWSTAT) is set high (See 3.1.1) */
    // failure causes a reset
    while (!(ax_hw_read_register_8(config, AX_REG_POWSTAT) & AX_POWSTAT_SVMODEM))
        ;

    /* Write preamble and packet to the FIFO */
    ax_fifo_tx_data(config, mod, packet, length);

    Log.trace(F("packet written to FIFO!\r\n"));
}

/**
 * Loads packet into the FIFO for transmission
 */
// this function is never used --tkc 8/12/24
void ax_tx_beacon(ax_config *config,
                  uint8_t *packet, uint16_t length)
{
    if (config->pwrmode != AX_PWRMODE_FULLTX)
    {
        Log.error(F("PWRMODE must be FULLTX before writing to FIFO!\r\n"));
        return;
    }

    /* Ensure the SVMODEM bit (POWSTAT) is set high (See 3.1.1) */
    // failure causes a reset
    while (!(ax_hw_read_register_8(config, AX_REG_POWSTAT) & AX_POWSTAT_SVMODEM))
        ;

    /* let's set the packet to read out MSB first */
    uint8_t address_config = ax_hw_read_register_8(config, AX_REG_PKTADDRCFG);
    Log.trace(F("address config: %d\r\n"), address_config);
    ax_hw_write_register_8(config, AX_REG_PKTADDRCFG, address_config | 0x80);

    /* Write packet to the FIFO */
    ax_fifo_tx_beacon(config, packet, length);

    Log.trace(F("address config: %d\r\n"), address_config | 0x80);
    Log.trace(F("beacon written to FIFO!\r\n"));

    // now wait for transmit
    while (ax_RADIOSTATE(config) != AX_RADIOSTATE_TX)
        ; // modified to only trigger when radio state is Tx

    /* now that it's been committed (transmitting) we can undo the MSB change */
    ax_hw_write_register_8(config, AX_REG_PKTADDRCFG, address_config);
    Log.trace(F("address config: %d\r\n"), address_config);
}

/**
 * Loads 1000 bits-times of zeros into the FIFO for tranmission
 */
// this is never used -- tkc 8/12/24
void ax_tx_1k_zeros(ax_config *config)
{
    if (config->pwrmode != AX_PWRMODE_FULLTX)
    {
        Log.error(F("PWRMODE must be FULLTX before writing to FIFO!\r\n"));
        return;
    }

    // Ensure the SVMODEM bit (POWSTAT) is set high (See 3.1.1)
    while (!(ax_hw_read_register_8(config, AX_REG_POWSTAT) & AX_POWSTAT_SVMODEM))
        ;

    // Write 1k zeros to fifo
    ax_fifo_tx_1k_zeros(config);
}

/**
 * Configure and switch to FULLRX
 */
void ax_rx_on(ax_config *config, ax_modulation *mod)
{
    if (mod->par.is_params_set != 0x51)
    {
        Log.error(F("mod->par must be set first! call ax_default_params...\r\n"));
        // causes a reset
        while (1)
            ;
    }

    /* Meta-data can be automatically added to FIFO, see PKTSTOREFLAGS */

    ax_set_registers(config, mod, NULL);

    /* Place chip in FULLRX mode */
    ax_set_pwrmode(config, AX_PWRMODE_FULLRX);

    ax_set_registers_rx(config, mod); /* set rx registers */
    ax_SET_SYNTH_B(config);

    /* Enable TCXO if used */
    if (config->tcxo_enable)
    {
        config->tcxo_enable();
    }

    /* Clear FIFO */
    ax_fifo_clear(config);

    /* Tune Baseband - Experimental */
    // ax_hw_write_register_8(config, AX_REG_BBTUNE, 0x10);
}

/**
 * Configure and switch to WORRX
 */
void ax_rx_wor(ax_config *config, ax_modulation *mod,
               ax_wakeup_config *wakeup_config)
{
    if (mod->par.is_params_set != 0x51)
    {
        Log.error(F("mod->par must be set first! call ax_default_params...\r\n"));
        // causes a reset on Silversat board
        while (1)
            ;
    }

    /* Meta-data can be automatically added to FIFO, see PKTSTOREFLAGS */

    ax_set_registers(config, mod, wakeup_config);

    /* Place chip in FULLRX mode */
    ax_set_pwrmode(config, AX_PWRMODE_WORRX);

    ax_set_registers_rx(config, mod); /* set rx registers */

    /* Enable TCXO if used */
    if (config->tcxo_enable)
    {
        config->tcxo_enable();
    }

    /* Clear FIFO */
    ax_fifo_clear(config);

    /* Tune Baseband - Experimental */
    // ax_hw_write_register_8(config, AX_REG_BBTUNE, 0x10);
}

/**
 * Reads packets from the FIFO
 */
int ax_rx_packet(ax_config *config, ax_packet *rx_pkt, ax_modulation *modulation)
{
    ax_rx_chunk rx_chunk;
    uint16_t pkt_wr_index = 0;
    uint16_t length;
    float offset;

    /* compile parts of the pkt structure, 0x80 is flag for the data itself */
    uint8_t pkt_parts_list = (config->pkt_store_flags & 0x1E) | 0x80;
    uint8_t pkt_parts = 0;
    // this is short enough that the watchdog shouldn't fire...or could it?
    // TODO: make sure we don't need to feed the watchdog, it doesn't seem like we need to based on results
    // as long as pkt_wr_index = 0, then it should drop back to main.
    while (1)
    {
        //  let's see what states show up as we go along
        // Log.trace(F("radio state: %X\r\n"), ax_hw_read_register_8(config, AX_REG_RADIOSTATE) & 0xF);
        // Log.trace(F("TRK P %d\r\n"), ax_hw_read_register_16(config, AX_REG_TRKPHASE));
        // Log.trace(F("TRK F %d\r\n"), ax_hw_read_register_24(config, AX_REG_TRKRFFREQ));

        /* Check if FIFO is not empty */
        if (ax_fifo_rx_data(config, &rx_chunk))
        {
            /* Got something from FIFO */
            switch (rx_chunk.chunk_t)
            {
                case AX_FIFO_CHUNK_DATA:
                {
                    length = rx_chunk.chunk.data.length; //there's the first mystery byte (always 0xC8..should be flags, but isn't)

                    Log.trace(F("flags %X\r\n"), rx_chunk.chunk.data.flags);
                    Log.trace(F("length %d\r\n"), length);
                    Log.trace(F("pkt write index %d\r\n"), pkt_wr_index);

                    /* print byte-by-byte */
                    /*        
                    for (int i = 0; i < length+1; i++)
                    {
                        Log.verbose(F("data %d: %X\r\n"), i,
                                    rx_chunk.chunk.data.data[i]);
                    }
                    */

                    // if pkt_start is not set and pkt_end flag is set and pkt_write_index = 0, then it's bad
                    // that is, it's signalling that it's the end, but it hasn't started.
                    if (!(rx_chunk.chunk.data.flags & AX_FIFO_RXDATA_PKTSTART) && (pkt_wr_index == 0)){
                        Log.trace(F("end flag set and write index  = 0\r\n"));
                        return 0;                            
                    }

                    // no harm in this check, but it should never happen..i've really locked down what we accept.
                    if ((rx_chunk.chunk.data.flags & AX_FIFO_RXDATA_ABORT) || (rx_chunk.chunk.data.flags & AX_FIFO_RXDATA_SIZEFAIL)) 
                    { // checks if the abort, sizefail, addrfail and residue flags are set
                        // this is a bad packet, discard
                        Log.trace(F("bad packet, no cookie!\r\n"));
                        // return 0;
                        return 0;
                    }

                    /* if the current chunk would overflow packet data buffer, discard */
                    if ((pkt_wr_index + length) > AX_PACKET_MAX_DATA_LENGTH)
                    {
                        Log.error(F("overflow\r\n"));
                        return 0;
                    }

                    /* copy in this chunk */
                    memcpy(rx_pkt->data + pkt_wr_index, rx_chunk.chunk.data.data + 1, length);
                    pkt_wr_index += length; 

                    //it's a first chunk and the command byte isn't 0xAA or 0x00
                    if (modulation->il2p_enabled != 1)
                    {
                        if (pkt_wr_index == 0)
                        {
                            int command_byte = 2;
                            if (((modulation->framing & 0xE) == AX_FRAMING_MODE_HDLC)) command_byte = 1;
                            if (rx_chunk.chunk.data.data[command_byte] != 0xAA || rx_chunk.chunk.data.data[command_byte] != 0x00) return 0;
                            // a little stronger condition.  This completely drops out of the loop if the command byte is wrong.
                        }
                    }               

                    /* are we done for this packet */
                    if (rx_chunk.chunk.data.flags & AX_FIFO_RXDATA_PKTEND)
                    {
                        rx_pkt->length = pkt_wr_index;

                        /* print byte-by-byte */
                        /*
                        for (int i = 0; i < rx_pkt->length; i++)
                        {
                        Log.trace(F("data %d: %C %c\r\n"), i,
                                    rx_pkt->data[i],
                                    rx_pkt->data[i]);
                        }

                        if (0)
                        {
                        Log.trace(F("FEC FEC FEC %X\r\n"), ax_hw_read_register_8(config, AX_REG_FECSTATUS));
                        }
                        */
                        pkt_parts |= 0x80;
                    }
                    
                    break;
                }

                case AX_FIFO_CHUNK_RSSI:
                    Log.notice(F("rssi %d dBm\r\n"), rx_chunk.chunk.rssi);

                    rx_pkt->rssi = rx_chunk.chunk.rssi;
                    pkt_parts |= AX_PKT_STORE_RSSI;
                    break;

                case AX_FIFO_CHUNK_RFFREQOFFS:
                    Log.notice(F("rf offset %d Hz\r\n"), (int)rx_chunk.chunk.rffreqoffs);
                    rx_pkt->rffreqoffs = rx_chunk.chunk.rffreqoffs;
                    pkt_parts |= AX_PKT_STORE_RF_OFFSET;
                    break;

                case AX_FIFO_CHUNK_FREQOFFS:
                    offset = rx_chunk.chunk.freqoffs * 2000;
                    Log.notice(F("freq offset %f\r\n"), offset / (1 << 16));

                    /* todo add data to back */
                    pkt_parts |= AX_PKT_STORE_FREQUENCY_OFFSET;
                    break;

                case AX_FIFO_CHUNK_DATARATE:
                    /* todo process datarate */
                    Log.notice(F("datarate TODO\r\n"));
                    pkt_parts |= AX_PKT_STORE_DATARATE_OFFSET;
                    break;
                default:

                    Log.error(F("some other chunk type %X\r\n"), rx_chunk.chunk_t);
                    break;
                }
            if (pkt_parts == pkt_parts_list)
            {
                /* we have all the parts for a packet */
                //PROCESS HERE
                /* print byte-by-byte */
                for (int i = 0; i < rx_pkt->length; i++) Log.verbose(F("data %d: %X\r\n"), i, rx_pkt->data[i]);

                ax_fifo_clear(config);  //clear the fifo...i want to make sure there's nothing left in it.

                if (modulation->il2p_enabled)
                {
                    //we should now have the length byte, command code, il2p framing, il2p header, header parity, payload, payload parity
                    Log.verbose(F("rx_pkt length %i\r\n"), rx_pkt->length);  //total length incl len byte, cmd, etc...
                    //grab the command code
                    Log.verbose(F("the command code is: %X\r\n"), rx_pkt->data[1]);  //it's after the length byte
                    unsigned char command_code = rx_pkt->data[1];
                    Log.verbose(F("the three sync bytes are: %X, %X, %X\r\n"), rx_pkt->data[2], rx_pkt->data[3], rx_pkt->data[4]);

                    //check CRC - it should be the last four bytes
                    uint32_t received_crc = *(rx_pkt->data+rx_pkt->length-4)<<24 | //example length: = 228, grab bytes 224, 225, 226, 227
                                          *(rx_pkt->data+rx_pkt->length-3)<<16 | 
                                          *(rx_pkt->data+rx_pkt->length-2)<<8 | 
                                          *(rx_pkt->data+rx_pkt->length-1);
                    Log.verbose(F("received crc: %X\r\n"),received_crc);
                    //if (!il2p_CRC.verify(rx_pkt->data + 5, rx_pkt->length-5-4, received_crc)) return 0;

                    IL2P_CRC il2p_crc_2;
                    const int length_framing = 5;  //length byte + command byte + 3 il2p framing bytes
                    const int length_crc = 4;
                    const int il2p_header_length = 13;
                    const int il2p_header_parity_length = 2;


                    Log.verbose(F("pkt start byte: %X\r\n"), *(rx_pkt->data+length_framing));  //5=len + cmd + 3 x frame
                    Log.verbose(F("pkt end byte: %X\r\n"), *(rx_pkt->data+ rx_pkt->length - length_framing)); //example length = 228, grab byte 223
                    Log.verbose(F("length-10: %d \r\n"), rx_pkt->length-10);
                    //if (!(il2p_crc_2.verify(rx_pkt->data + length_framing, rx_pkt->length-length_framing-length_crc-1, received_crc))) Log.verbose(F("BAD CRC!\r\n"));
                    //else Log.verbose(F("SUCCESS!!!\r\n")); 
                    uint16_t extracted_crc = il2p_crc_2.extract_crc(received_crc);

                    // Process IL2P header
                    unsigned char decoded_header[13];
                    unsigned char descrambled_header[13];
                    
                    int decode_success_header = il2p_decode_rs(rx_pkt->data + length_framing, il2p_header_length, il2p_header_parity_length, decoded_header);  //header starts in byte 5
                    Log.verbose(F("HEADER decode success = %i\r\n"), decode_success_header);
                    if (decode_success_header < 0)
                    {
                        Log.error(F("IL2P HEADER could not be recovered\r\n"));
                        return 0; //the header can't be recovered
                    }
                    il2p_descramble_block(decoded_header, descrambled_header, il2p_header_length);

                    //so now we have the header back (theoretically)
                    //could add a compare here and break if it doesn't match
                    //for now, just output it
                    Log.verbose(F("Received HEADER block\r\n"));
                    for (int i=0; i<13; i++) Log.verbose(F("%X, "),descrambled_header[i]);
                    Log.verbose(F("\r\n"));

                    //Process IL2P data
                    unsigned char decoded_data[255];  //not optimizing array size here
                    unsigned char descrambled_data[255];

                    Log.verbose(F("first byte: %X\r\n"), *(rx_pkt->data + length_framing+il2p_header_length+il2p_header_parity_length+4));
                    //final data size should be length-15 (for header) - 16 (for parity bytes) - 1 (for cmd); 
                    //starting location is offset by header, length and cmd
                    //int decode_success_data = il2p_decode_rs(rx_chunk.chunk.data.data + 17 + 4, rx_pkt->length- 32 - 4 - 4, 16, decoded_data); //now 4 more for the CRC
                    const int data_parity = 16;
                    const int fixed_length = length_framing + il2p_header_length + il2p_header_parity_length + data_parity + length_crc;  //should = 40
                    int data_size = rx_pkt->length - fixed_length;
                    int decode_success_data = il2p_decode_rs(rx_pkt->data + length_framing + il2p_header_length + il2p_header_parity_length, data_size, data_parity, decoded_data); //now 4 more for the CRC
                    
                    Log.verbose(F("DATA decode success = %i\r\n"), decode_success_data);
                    if (decode_success_data < 0)
                    {
                        Log.error(F("IL2P DATA could not be recovered\r\n"));
                        return 0; //the header can't be recovered
                    }

                    il2p_descramble_block(decoded_data, descrambled_data, data_size);

                    uint16_t ax25_crc = il2p_crc_2.calculate_AX25(descrambled_data, data_size);
                    Log.notice("AX25 CRC (RX) = %X\r\n", ax25_crc);
                    if (ax25_crc == extracted_crc) Log.notice("Success! CRC matches\r\n");
                    else("BAD CRC!\r\n");

                    Log.verbose(F("Received DATA block\r\n"));
                    //for (int i=0; i< rx_pkt->length-40; i++) Log.verbose(F("%i: %X\r\n"), i, descrambled_data[i]);
                    Log.verbose(F("\r\n"));
                    rx_pkt->data[0] = command_code;  //gotta put the command code back
                    
                    for (int i = 0; i< data_size; i++) rx_pkt->data[i+1] = descrambled_data[i]; 
                    rx_pkt->length -= (fixed_length - 1);  //one less for the cmd byte
                    Log.verbose(F("final packet length: %i\r\n"), rx_pkt->length);
                }
                return 1;
            }
        }
        else if (pkt_wr_index == 0)
        {
            /* nothing to read from fifo */
            return 0;
        }
    }

    /* Disable TCXO if used */
    if (config->tcxo_disable)
    {
        config->tcxo_disable();
    }
}

/**
 * Waits for any ongoing operations to complete, and then shuts down the radio
 */
void ax_off(ax_config *config)
{
    /* Wait for ongoing transmit to complete by polling RADIOSTATE */
    uint8_t radiostate;

    do
    {
        radiostate = ax_hw_read_register_8(config, AX_REG_RADIOSTATE) & 0xF;
    } while ((radiostate == AX_RADIOSTATE_TX_PLL_SETTLING) || (radiostate == AX_RADIOSTATE_TX) || (radiostate == AX_RADIOSTATE_TX_TAIL));

    ax_set_pwrmode(config, AX_PWRMODE_POWERDOWN);

    Log.trace(F("ax_off complete!\r\n"));
}

/**
 * Shuts down the radio immediately, even if an operation is in progress
 */
void ax_force_off(ax_config *config)
{
    ax_set_pwrmode(config, AX_PWRMODE_POWERDOWN);
}

/**
 * immediately updates pinfunc
 */
void ax_set_pinfunc_sysclk(ax_config *config, pinfunc_t func)
{
    _pinfunc_sysclk = func;
    ax_hw_write_register_8(config, AX_REG_PINFUNCSYSCLK, _pinfunc_sysclk);
}

void ax_set_pinfunc_dclk(ax_config *config, pinfunc_t func)
{
    _pinfunc_dclk = func;
    ax_hw_write_register_8(config, AX_REG_PINFUNCDCLK, _pinfunc_dclk);
}

void ax_set_pinfunc_data(ax_config *config, pinfunc_t func)
{
    _pinfunc_data = func;
    ax_hw_write_register_8(config, AX_REG_PINFUNCDATA, _pinfunc_data);
}

void ax_set_pinfunc_antsel(ax_config *config, pinfunc_t func)
{
    _pinfunc_antsel = func;
    ax_hw_write_register_8(config, AX_REG_PINFUNCANTSEL, _pinfunc_antsel);
}

void ax_set_pinfunc_pwramp(ax_config *config, pinfunc_t func)
{
    _pinfunc_pwramp = func;
    ax_hw_write_register_8(config, AX_REG_PINFUNCPWRAMP, _pinfunc_pwramp);
}

/**
 * immediately updates tx path
 */
void ax_set_tx_path(ax_config *config, enum ax_transmit_path path)
{
    config->transmit_path = path;

    uint8_t modcfga = ax_hw_read_register_8(config, AX_REG_MODCFGA);
    modcfga &= ~0x3;
    modcfga |= ax_modcfga_tx_parameters_tx_path(config->transmit_path);
    ax_hw_write_register_8(config, AX_REG_MODCFGA, modcfga);
}

/**
 * Initialise radio.
 *
 * * reset
 * * check SPI interface functions
 * * range VCOs
 */
int ax_init(ax_config *config)
{
#ifndef _AX_DUMMY
    /* must set spi_transfer */
    if (!config->spi_transfer)
    {
        return AX_INIT_SET_SPI;
    }

    /* Scratch */
    uint8_t scratch = ax_scratch(config);
    Log.trace(F("Scratch %X\r\n"), scratch);

    if (scratch != AX_SCRATCH)
    {
        Log.error(F("Bad scratch value\r\n"));

        return AX_INIT_BAD_SCRATCH;
    }

    /* Revision */
    uint8_t silicon_revision = ax_silicon_revision(config);
    Log.trace(F("Silicon Revision %X\r\n"), silicon_revision);

    if (silicon_revision != AX_SILICONREVISION)
    {
        Log.error(F("Bad Silicon Revision value.\r\n"));

        return AX_INIT_BAD_REVISION;
    }

    /* Reset the chip */

    /* Set RST bit (PWRMODE) */
    ax_hw_write_register_8(config, AX_REG_PWRMODE, AX_PWRMODE_RST);

    /* Set the PWRMODE register to POWERDOWN, also clears RST bit */
    ax_set_pwrmode(config, AX_PWRMODE_POWERDOWN);
#endif

    /* Set xtal parameters. The function sets values in config that we
     * need for other parameter calculations */
    ax_set_xtal_parameters(config);

#ifndef _AX_DUMMY
    /* Perform auto-ranging for both VCOs */
    if (ax_vco_ranging(config) != AX_VCO_RANGING_SUCCESS)
    {
        return AX_INIT_VCO_RANGING_FAILED; /* ranging fail */
    }
#endif

    return AX_INIT_OK;
}

/**
 * write beacon tx data - we know that beacons are small (<<256 bytes, so I removed the extended packet logic)
 */
void ax_fifo_tx_beacon(ax_config *config,
                       uint8_t *data, uint16_t length)
{
    uint8_t header[8];
    uint8_t fifocount;

    /* wait for enough space to contain both the preamble and chunk */
    do
    {
        fifocount = ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
    } while (fifocount > (256 - (length + 20))); // where does 20 come from?  fifocount is the number of bytes that can be read without an underflow...should this be fifofree???  no, I think you're waiting for bytes to be read
    // this may be important during configuration when more data is written to the FIFO?

    /* debugging */
    // uint8_t fifostat = ax_hw_read_register_8(config, AX_REG_FIFOCOUNT);
    // uint16_t fifofree = ax_hw_read_register_16(config, AX_REG_FIFOFREE);
    // Log.trace(F("fifo status (txbeacon): %X\r\n"), fifostat);
    // Log.trace(F("fifo count (txbeacon): %X\r\n"), fifocount);
    // Log.trace(F("fifo free (txbeacon): %X\r\n"), fifofree);

    /* write chunk */
    header[0] = AX_FIFO_CHUNK_DATA;
    header[1] = length + 1; /* incl flags */
    header[2] = AX_FIFO_TXDATA_UNENC | AX_FIFO_TXDATA_RAW | AX_FIFO_TXDATA_NOCRC | AX_FIFO_TXDATA_PKTSTART | AX_FIFO_TXDATA_PKTEND;
    ax_hw_write_fifo(config, header, 3);
    ax_hw_write_fifo(config, data, (uint8_t)length);

    // Log.trace(F("fifo status (txbeacon): %X\r\n"), fifostat);
    // Log.trace(F("fifo count (txbeacon): %X\r\n"), fifocount);
    // Log.trace(F("fifo free (txbeacon): %X\r\n"), fifofree);

    ax_fifo_commit(config); /* commit */
}

/* extended command for RSSI value */
uint8_t ax_RSSI(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_RSSI);
}

/* extended command for BGNDRSSI value */
uint8_t ax_BGNDRSSI(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_BGNDRSSI);
}

/* extended command for Radio State register value */
uint8_t ax_RADIOSTATE(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_RADIOSTATE) & 0x0F;
}

/* extended command for POWSTAT value */
uint8_t ax_POWSTAT(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_POWSTAT);
}

/* extended command for POWSTICKYSTAT value */
uint8_t ax_POWSTICKYSTAT(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_POWSTICKYSTAT);
}

/* extended command to query the fifo status register */
uint8_t ax_FIFOSTAT(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_FIFOSTAT);
}

/* extended command to display the current data */
uint8_t ax_FIFODATA(ax_config *config)
{
    return ax_hw_read_register_8(config, AX_REG_FIFODATA);
}

/* returns the number of bytes currently written to the FIFO */
uint16_t ax_FIFOCOUNT(ax_config *config)
{
    return ax_hw_read_register_16(config, AX_REG_FIFOCOUNT);
}

/* returns the number of bytes that can written to the FIFO without causing an overflow */
uint16_t ax_FIFOFREE(ax_config *config)
{
    return ax_hw_read_register_16(config, AX_REG_FIFOFREE);
}

uint16_t ax_TXPWRCOEFFB(ax_config *config)
{
    return ax_hw_read_register_16(config, AX_REG_TXPWRCOEFFB);
}

/* MODIFY_TX_POWER - modifies the output power between 0.1 and 1, esentially +5 to 15dBm theoretically*/
/* it's actually a sixteen bit value assuming there's no pre-distortion.  You're adjusting TXPWRCOEFFB */
uint16_t ax_MODIFY_TX_POWER(ax_config *config, float new_power)
{
    float p;
    uint16_t pwr;
    // new_power should be a float between 0.1 and 1
    if (config->transmit_power_limit > 0)
    {
        p = MIN(new_power, config->transmit_power_limit);
    }
    else
    {
        p = new_power;
    }
    pwr = (uint16_t)((p * (1 << 12)) + 0.5);
    pwr = (pwr > 0xFFF) ? 0xFFF : pwr; /* max 0xFFF */
    ax_hw_write_register_16(config, AX_REG_TXPWRCOEFFB, pwr);
    // current_mod->power = new_power;  // modify the structure

    Log.trace(F("power %f = %X\r\n"), new_power, pwr);
    return ax_hw_read_register_16(config, AX_REG_TXPWRCOEFFB);
}

// this function is to support mode changing on the fly
uint16_t ax_MODIFY_FEC(ax_config *config, ax_modulation *current_mod, bool FEC)
{
    if (FEC == false)
    {
        current_mod->fec = 0; // FSK
        current_mod->bitrate = 9600;
        Log.trace(F("FEC off; bitrate is 9600\r\n"));
    }
    else
    {
        current_mod->fec = 1;
        current_mod->bitrate = 19200;
        Log.trace(F("FEC on; bitrate now 19200\r\n"));
    }

    return current_mod->fec;
}

// this function is to support mode changing on the fly
uint16_t ax_MODIFY_SHAPING(ax_config *config, ax_modulation *current_mod, uint8_t shaping)
{
    if (shaping == 0)
    {
        current_mod->shaping = AX_MODCFGF_FREQSHAPE_UNSHAPED; // FSK
        current_mod->parameters.fsk.modulation_index = 2.0 / 3;
    }
    else if (shaping == 1)
    {
        current_mod->shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_5;
        current_mod->parameters.fsk.modulation_index = 0.5;
    }
    else if (shaping == 2)
    {
        current_mod->shaping = AX_MODCFGF_FREQSHAPE_GAUSSIAN_BT_0_3;
        current_mod->parameters.fsk.modulation_index = 0.5;
    }
    else
    {
        Log.error(F("ERROR: Shaping index out of bounds\r\n"));
    }

    Log.trace(F("new shaping configured\r\n"));
    return current_mod->shaping;
}

uint8_t ax_TOGGLE_SYNTH(ax_config *config) // this command toggles between frequency A and frequency B
{
    // read the current values`
    uint8_t loop_val = ax_hw_read_register_8(config, AX_REG_PLLLOOP);
    uint8_t loopboost_val = ax_hw_read_register_8(config, AX_REG_PLLLOOPBOOST);

    // toggle the frequency select bit
    loop_val = loop_val ^ AX_PLLLOOP_FREQSEL_B; // xor with 1 to toggle
    loopboost_val = loopboost_val ^ AX_PLLLOOP_FREQSEL_B;

    // write it out
    ax_hw_write_register_8(config, AX_REG_PLLLOOP, loop_val);
    ax_hw_write_register_8(config, AX_REG_PLLLOOPBOOST, loopboost_val);

    return (loop_val >> 7);
}

uint8_t ax_SET_SYNTH_A(ax_config *config) // this command toggles between frequency A and frequency B
{
    // read the current values`
    uint8_t loop_val = ax_hw_read_register_8(config, AX_REG_PLLLOOP);
    uint8_t loopboost_val = ax_hw_read_register_8(config, AX_REG_PLLLOOPBOOST);

    loop_val = loop_val & 0x7F; // set the frequency select bit low
    loopboost_val = loopboost_val & 0x7F;

    // write it out
    ax_hw_write_register_8(config, AX_REG_PLLLOOP, loop_val);
    ax_hw_write_register_8(config, AX_REG_PLLLOOPBOOST, loopboost_val);

    return (loop_val >> 7);
}

uint8_t ax_SET_SYNTH_B(ax_config *config) // this command toggles between frequency A and frequency B
{
    // read the current values`
    uint8_t loop_val = ax_hw_read_register_8(config, AX_REG_PLLLOOP);
    uint8_t loopboost_val = ax_hw_read_register_8(config, AX_REG_PLLLOOPBOOST);

    loop_val = loop_val | 0x80; // set the frequency select bit high
    loopboost_val = loopboost_val | 0x80;

    // write it out
    ax_hw_write_register_8(config, AX_REG_PLLLOOP, loop_val);
    ax_hw_write_register_8(config, AX_REG_PLLLOOPBOOST, loopboost_val);

    return (loop_val >> 7);
}

void ax_SET_IRQMRADIOCTRL(ax_config *config)
{
    //read the current value
    uint16_t regvalue = ax_hw_read_register_16(config, AX_REG_IRQMASK);
    //set the RADIOCTRL IRQ bit
    regvalue |= AX_IRQMRADIOCTRL;
    //write it out
    ax_hw_write_register_16(config, AX_REG_IRQMASK, regvalue);
    //There's now an interrupt whenever the radio finishes receiving or transmitting
    regvalue = ax_hw_read_register_16(config, AX_REG_RADIOEVENTMASK);
    regvalue |= AX_REVMDONE;
    //theoretically it should only interrupt on a event done (tx or rx)
    ax_hw_write_register_16(config, AX_REG_RADIOEVENTMASK, regvalue);
}
