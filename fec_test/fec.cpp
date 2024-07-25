/*
 *  AX5043 OS-independent driver
 *
 *  Copyright (C) 2019 Libre Space Foundation (https://libre.space)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fec.h"
//#include "ax5043.h"
#include <Arduino.h>
#include <string.h>


#define MODFF(x) mod255(x)
#define MM     (8)
#define NN     (255)
#define PRT_LENGTH (32)
#define FCR_RS    (112)
#define PRIM   (11)
#define IPRIM  (116)
#define FF       (NN)
#define ALPHA_TO_LEN (256)
#define INDEX_OF_LEN (256)
#define GENPOLY_LEN (33)

static const uint8_t alpha_to[ALPHA_TO_LEN] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x87, 0x89, 0x95, 0xAD, 0xDD, 0x3D, 0x7A, 0xF4,
	0x6F, 0xDE, 0x3B, 0x76, 0xEC, 0x5F, 0xBE, 0xFB, 0x71, 0xE2, 0x43, 0x86, 0x8B, 0x91, 0xA5, 0xCD,
	0x1D, 0x3A, 0x74, 0xE8, 0x57, 0xAE, 0xDB, 0x31, 0x62, 0xC4, 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0x67,
	0xCE, 0x1B, 0x36, 0x6C, 0xD8, 0x37, 0x6E, 0xDC, 0x3F, 0x7E, 0xFC, 0x7F, 0xFE, 0x7B, 0xF6, 0x6B,
	0xD6, 0x2B, 0x56, 0xAC, 0xDF, 0x39, 0x72, 0xE4, 0x4F, 0x9E, 0xBB, 0xF1, 0x65, 0xCA, 0x13, 0x26,
	0x4C, 0x98, 0xB7, 0xE9, 0x55, 0xAA, 0xD3, 0x21, 0x42, 0x84, 0x8F, 0x99, 0xB5, 0xED, 0x5D, 0xBA,
	0xF3, 0x61, 0xC2, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0,
	0x47, 0x8E, 0x9B, 0xB1, 0xE5, 0x4D, 0x9A, 0xB3, 0xE1, 0x45, 0x8A, 0x93, 0xA1, 0xC5, 0x0D, 0x1A,
	0x34, 0x68, 0xD0, 0x27, 0x4E, 0x9C, 0xBF, 0xF9, 0x75, 0xEA, 0x53, 0xA6, 0xCB, 0x11, 0x22, 0x44,
	0x88, 0x97, 0xA9, 0xD5, 0x2D, 0x5A, 0xB4, 0xEF, 0x59, 0xB2, 0xE3, 0x41, 0x82, 0x83, 0x81, 0x85,
	0x8D, 0x9D, 0xBD, 0xFD, 0x7D, 0xFA, 0x73, 0xE6, 0x4B, 0x96, 0xAB, 0xD1, 0x25, 0x4A, 0x94, 0xAF,
	0xD9, 0x35, 0x6A, 0xD4, 0x2F, 0x5E, 0xBC, 0xFF, 0x79, 0xF2, 0x63, 0xC6, 0x0B, 0x16, 0x2C, 0x58,
	0xB0, 0xE7, 0x49, 0x92, 0xA3, 0xC1, 0x05, 0x0A, 0x14, 0x28, 0x50, 0xA0, 0xC7, 0x09, 0x12, 0x24,
	0x48, 0x90, 0xA7, 0xC9, 0x15, 0x2A, 0x54, 0xA8, 0xD7, 0x29, 0x52, 0xA4, 0xCF, 0x19, 0x32, 0x64,
	0xC8, 0x17, 0x2E, 0x5C, 0xB8, 0xF7, 0x69, 0xD2, 0x23, 0x46, 0x8C, 0x9F, 0xB9, 0xF5, 0x6D, 0xDA,
	0x33, 0x66, 0xCC, 0x1F, 0x3E, 0x7C, 0xF8, 0x77, 0xEE, 0x5B, 0xB6, 0xEB, 0x51, 0xA2, 0xC3, 0x00,
};

static const uint8_t index_of[INDEX_OF_LEN] = {
	0xFF, 0x00, 0x01, 0x63, 0x02, 0xC6, 0x64, 0x6A, 0x03, 0xCD, 0xC7, 0xBC, 0x65, 0x7E, 0x6B, 0x2A,
	0x04, 0x8D, 0xCE, 0x4E, 0xC8, 0xD4, 0xBD, 0xE1, 0x66, 0xDD, 0x7F, 0x31, 0x6C, 0x20, 0x2B, 0xF3,
	0x05, 0x57, 0x8E, 0xE8, 0xCF, 0xAC, 0x4F, 0x83, 0xC9, 0xD9, 0xD5, 0x41, 0xBE, 0x94, 0xE2, 0xB4,
	0x67, 0x27, 0xDE, 0xF0, 0x80, 0xB1, 0x32, 0x35, 0x6D, 0x45, 0x21, 0x12, 0x2C, 0x0D, 0xF4, 0x38,
	0x06, 0x9B, 0x58, 0x1A, 0x8F, 0x79, 0xE9, 0x70, 0xD0, 0xC2, 0xAD, 0xA8, 0x50, 0x75, 0x84, 0x48,
	0xCA, 0xFC, 0xDA, 0x8A, 0xD6, 0x54, 0x42, 0x24, 0xBF, 0x98, 0x95, 0xF9, 0xE3, 0x5E, 0xB5, 0x15,
	0x68, 0x61, 0x28, 0xBA, 0xDF, 0x4C, 0xF1, 0x2F, 0x81, 0xE6, 0xB2, 0x3F, 0x33, 0xEE, 0x36, 0x10,
	0x6E, 0x18, 0x46, 0xA6, 0x22, 0x88, 0x13, 0xF7, 0x2D, 0xB8, 0x0E, 0x3D, 0xF5, 0xA4, 0x39, 0x3B,
	0x07, 0x9E, 0x9C, 0x9D, 0x59, 0x9F, 0x1B, 0x08, 0x90, 0x09, 0x7A, 0x1C, 0xEA, 0xA0, 0x71, 0x5A,
	0xD1, 0x1D, 0xC3, 0x7B, 0xAE, 0x0A, 0xA9, 0x91, 0x51, 0x5B, 0x76, 0x72, 0x85, 0xA1, 0x49, 0xEB,
	0xCB, 0x7C, 0xFD, 0xC4, 0xDB, 0x1E, 0x8B, 0xD2, 0xD7, 0x92, 0x55, 0xAA, 0x43, 0x0B, 0x25, 0xAF,
	0xC0, 0x73, 0x99, 0x77, 0x96, 0x5C, 0xFA, 0x52, 0xE4, 0xEC, 0x5F, 0x4A, 0xB6, 0xA2, 0x16, 0x86,
	0x69, 0xC5, 0x62, 0xFE, 0x29, 0x7D, 0xBB, 0xCC, 0xE0, 0xD3, 0x4D, 0x8C, 0xF2, 0x1F, 0x30, 0xDC,
	0x82, 0xAB, 0xE7, 0x56, 0xB3, 0x93, 0x40, 0xD8, 0x34, 0xB0, 0xEF, 0x26, 0x37, 0x0C, 0x11, 0x44,
	0x6F, 0x78, 0x19, 0x9A, 0x47, 0x74, 0xA7, 0xC1, 0x23, 0x53, 0x89, 0xFB, 0x14, 0x5D, 0xF8, 0x97,
	0x2E, 0x4B, 0xB9, 0x60, 0x0F, 0xED, 0x3E, 0xE5, 0xF6, 0x87, 0xA5, 0x17, 0x3A, 0xA3, 0x3C, 0xB7,
};

static const uint8_t genpoly[GENPOLY_LEN] = {
	0x00, 0xF9, 0x3B, 0x42, 0x04, 0x2B, 0x7E, 0xFB, 0x61, 0x1E, 0x03, 0xD5, 0x32, 0x42, 0xAA, 0x05,
	0x18, 0x05, 0xAA, 0x42, 0x32, 0xD5, 0x03, 0x1E, 0x61, 0xFB, 0x7E, 0x2B, 0x04, 0x42, 0x3B, 0xF9,
	0x00,
};

static int
mod255(int16_t x)
{
	return (x % 255);
}

static bool
pad_valid(uint8_t pad)
{
	if (pad < 0 || pad > 222) {
		return false;
	} else {
		return true;
	}
}

/**
 * Creates 32 byte parity of the Reed Solomon (255, 223)
 * @param parity a 32 byte buffer to hold the calculated  parity
 * @param data the input data
 * @param len the size of the input data. Normally this should be 223 but
 * in case of the available data are less, proper zero padding will be
 * considered
 */
void
rs_encode(uint8_t *parity, const uint8_t *data, size_t len)
{
	memset(parity, 0, sizeof(uint8_t) * PRT_LENGTH);
	uint8_t pad = 223 - len;
	uint8_t feedback;
	for (int i = 0; i < NN - PRT_LENGTH - pad; i++) {
		feedback = index_of[data[i] ^ parity[0]];
		if (feedback != FF) {
			for (int j = 1; j < PRT_LENGTH; j++) {
				parity[j] ^= alpha_to[mod255(feedback + genpoly[PRT_LENGTH - j])];
			}
		}
		memmove(&parity[0], &parity[1], sizeof(uint8_t) * (PRT_LENGTH - 1));
		if (feedback != FF) {
			parity[PRT_LENGTH - 1] = alpha_to[mod255(feedback + genpoly[0])];
		} else {
			parity[PRT_LENGTH - 1] = 0;
		}
	}
}
/**
 * @param data RS encoded input data. The last 32 bytes should be the parity
 * bytes. Decoding is performed in place.
 * @param len the size of the input data (including the 32 parity bytes).
 * Normally this should be 255. If not, padding should be considered.
 * @return the number of corrected bits or -1 in case of unrecoverable decoding
 * error
 */
int
rs_decode(uint8_t *data, size_t len)
{
	uint8_t pad = 0;
	uint8_t q;
	uint8_t tmp;
	uint8_t num1;
	uint8_t num2;
	uint8_t den;
	uint8_t temp_r;
	uint8_t l_arr[PRT_LENGTH + 1];
	uint8_t s[PRT_LENGTH];
	uint8_t no_eras = 0;
	uint8_t b[PRT_LENGTH + 1];
	uint8_t t[PRT_LENGTH + 1];
	uint8_t evaluator[PRT_LENGTH + 1];
	uint8_t root[PRT_LENGTH];
	uint8_t reg[PRT_LENGTH + 1];
	uint8_t loc[PRT_LENGTH];
	int16_t l_val;
	int16_t conf_2;
	int16_t eval_val;
	int16_t i;
	int16_t j;
	int16_t conf_1;
	int16_t k;
	int16_t syndrome_flag;
	int16_t corrected_bits;

	syndrome_flag = 0;
	pad = 255 - len;

	if (!pad_valid(pad)) {
		return (-1);
	}

	//Syndrome Generation
	for (i = 0; i < PRT_LENGTH; i++) {
		s[i] = data[0];
	}
	for (j = 1; j < NN - pad; j++) {
		for (i = 0; i < PRT_LENGTH; i++) {
			if (s[i] == 0) {
				s[i] = data[j];
			} else {
				s[i] = data[j] ^ alpha_to[MODFF(index_of[s[i]] + (FCR_RS + i) * PRIM)];
			}
		}
	}

	//Syndrome Transformation
	for (i = 0; i < PRT_LENGTH; i++) {
		syndrome_flag |= s[i];
		s[i] = index_of[s[i]];
	}

	if (syndrome_flag == 0) {
		return 0;
	}

	memset(&l_arr[1], 0, PRT_LENGTH * sizeof(l_arr[0]));

	l_arr[0] = 1;
	for (i = 0; i < PRT_LENGTH + 1; i++) {
		b[i] = index_of[l_arr[i]];
	}

	//Error Locator
	conf_1 = 0;
	conf_2 = 0;
	while (++conf_1 <= PRT_LENGTH) {
		temp_r = 0;
		for (i = 0; i < conf_1; i++) {
			if ((l_arr[i] != 0) && (s[conf_1 - i - 1] != FF)) {
				temp_r ^= alpha_to[MODFF(index_of[l_arr[i]] + s[conf_1 - i - 1])];
			}
		}
		temp_r = index_of[temp_r];
		if (temp_r == FF) {
			memmove(&b[1], b, PRT_LENGTH * sizeof(b[0]));
			b[0] = FF;
		} else {
			t[0] = l_arr[0];
			for (i = 0; i < PRT_LENGTH; i++) {
				if (b[i] != FF) {
					t[i + 1] = l_arr[i + 1] ^ alpha_to[MODFF(temp_r + b[i])];
				} else {
					t[i + 1] = l_arr[i + 1];
				}
			}
			if (2 * conf_2 <= conf_1 + no_eras - 1) {
				conf_2 = conf_1 + no_eras - conf_2;

				for (i = 0; i <= PRT_LENGTH; i++) {
					b[i] = (l_arr[i] == 0) ? FF : MODFF(index_of[l_arr[i]] - temp_r + NN);
				}
			} else {
				memmove(&b[1], b, PRT_LENGTH * sizeof(b[0]));
				b[0] = FF;
			}
			memcpy(l_arr, t, (PRT_LENGTH + 1) * sizeof(t[0]));
		}
	}

	//Array Transformation
	l_val = 0;
	for (i = 0; i < PRT_LENGTH + 1; i++) {
		l_arr[i] = index_of[l_arr[i]];
		if (l_arr[i] != FF) {
			l_val = i;
		}
	}

	//Error Root Finder
	memcpy(&reg[1], &l_arr[1], PRT_LENGTH * sizeof(reg[0]));
	corrected_bits = 0;
	for (i = 1, k = IPRIM - 1; i <= NN; i++, k = MODFF(k + IPRIM)) {
		q = 1;
		for (j = l_val; j > 0; j--)
			if (reg[j] != FF) {
				reg[j] = MODFF(reg[j] + j);
				q ^= alpha_to[reg[j]];
			}

		if (q != 0) {
			continue;
		}
		root[corrected_bits] = i;
		loc[corrected_bits] = k;
		if (++corrected_bits == l_val) {
			break;
		}
	}

	if (l_val != corrected_bits) {
		return 0;
	}

	//Error Evaluator
	eval_val = l_val - 1;
	for (i = 0; i <= eval_val; i++) {
		tmp = 0;
		for (j = i; j >= 0; j--) {
			if ((s[i - j] != FF) && (l_arr[j] != FF)) {
				tmp ^= alpha_to[MODFF(s[i - j] + l_arr[j])];
			}
		}
		evaluator[i] = index_of[tmp];
	}

	//Error Value Calculator
	for (j = corrected_bits - 1; j >= 0; j--) {
		num1 = 0;
		for (i = eval_val; i >= 0; i--) {
			if (evaluator[i] != FF) {
				num1 ^= alpha_to[MODFF(evaluator[i] + i * root[j])];
			}
		}
		num2 = alpha_to[MODFF(root[j] * (FCR_RS - 1) + NN)];
		den = 0;

		for (i = min(l_val, PRT_LENGTH - 1) & ~1; i >= 0; i -= 2) {
			if (l_arr[i + 1] != FF) {
				den ^= alpha_to[MODFF(l_arr[i + 1] + i * root[j])];
			}
		}

		if (num1 != 0 && loc[j] >= pad) {
			data[loc[j] - pad] ^= alpha_to[MODFF(index_of[num1] + index_of[num2] + NN -
			                                     index_of[den])];
		}
	}

	return corrected_bits;
}

/* CCSDS polynomial functions */
static const uint8_t CCSDS_CONV_G1[7] = {1, 1, 1, 1, 0, 0, 1};
static const uint8_t CCSDS_CONV_G2[7] = {1, 0, 1, 1, 0, 1, 1};

/**
 * Static helper function to write punctured output to buffer. It is used for
 * all puncured codes except 2/3 that unrolling the code is feasible.
 *
 * @param out the out buffer that holds the result
 * @param data the input data
 * @param len size of input data in bytes
 * @param puncturing_pattern_c1 puncturing pattern for c1 polynomial function
 * @param puncturing_pattern_c2 puncturing pattern for c2 polynomial function
 * @param puncturing_pattern_length the length of the polynomials
 */
static void
calculate_punctured_output(uint8_t *out, const uint8_t *data, const size_t len,
                           const uint8_t *puncturing_pattern_c1,
                           const uint8_t *puncturing_pattern_c2,
                           const uint8_t puncturing_pattern_length);

/**
 * Static helper function to read specific bit from 1 byte length word.
 * Which bit is read is defined by the user.
 *
 * @param data the 1 byte length word to be read
 * @param input_bit the bit position to be read (starting from 0 for MS)
 *
 * @returns the bit value in an 1 byte word
 */
static inline uint8_t
read_bit(const uint8_t data, const uint8_t input_bit);

/**
 * Static helper function to calculate the output bit for given polynomial.
 *
 * @param shift_reg the shift register holding the bits (1 byte per bit)
 * @param polynomial the polynomial used for the calculation (1 byte per bit)
 *
 * @returns the out bit that was calculated
 */
static inline uint8_t
calculate_generator_output(const uint8_t *shift_reg, const uint8_t *polynomial);

/**
 * Virtual shift registers function used for conv_encoder_27.
 * @param reg array of registers
 * @param len size of array
 */
static void
bit_shift(uint8_t *reg, uint8_t len);

/**
 * CCSDS compliant K=7 R=1/2 convolutional encoder. Each call to this function
 * is completely independent from each other and there is no state information.
 * Initially the K-1 stages are set to 0. It is responsibility of the caller to
 * add tailbits or proper padding. With these assumptions, the size of the
 * encoded data will be always \f$2 \times len\f$
 *
 * @param out the out buffer to hold the result. It is responsibility of the
 * caller to provide enough space for the result
 *
 * @param data the input data
 * @param len the number of input bytes in the \a data buffer
 */
void
conv_encoder_1_2_7(uint8_t *out, const uint8_t *data, size_t len)
{
	uint8_t shift_reg_7[CCSDS_CONV_CONSTRAINT_LENGTH_BITS] = {0, 0, 0, 0, 0, 0, 0};
	uint8_t bit = 0;
	uint8_t out_g1 = 0;
	uint8_t out_g2 = 0;

	/* Clean output buffer from possible garbage value */
	*out = 0;

	for (int i = 0; i < len; i++) {
		/* Read 1st bit */
		bit = read_bit(data[i], 0);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 1st and 2nd bits */
		*out |= (((out_g1 & 1) << 7));
		*out |= ((!(out_g2 & 1) << 6));

		/* Read 2nd bit */
		bit = read_bit(data[i], 1);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 3rd and 4th bits */
		*out |= (((out_g1 & 1) << 5));
		*out |= ((!(out_g2 & 1) << 4));

		/* Read 3rd bit */
		bit = read_bit(data[i], 2);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 5th and 6th bits */
		*out |= (((out_g1 & 1) << 3));
		*out |= ((!(out_g2 & 1) << 2));

		/* Read 4th bit */
		bit = read_bit(data[i], 3);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 7th and 8th bits */
		*out |= (((out_g1 & 1) << 1));
		*out |= ((!(out_g2 & 1)));

		/* 8bits written to output move to next word */
		out++;
		*out = 0;

		/* Read 5th bit */
		bit = read_bit(data[i], 4);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 1st and 2nd bits */
		*out |= (((out_g1 & 1) << 7));
		*out |= ((!(out_g2 & 1) << 6));

		/* Read 6th bit */
		bit = read_bit(data[i], 5);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 3rd and 4th bits */
		*out |= (((out_g1 & 1) << 5));
		*out |= ((!(out_g2 & 1) << 4));

		/* Read 7th bit */
		bit = read_bit(data[i], 6);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 5th and 6th bits */
		*out |= (((out_g1 & 1) << 3));
		*out |= ((!(out_g2 & 1) << 2));

		/* Read 8th bit */
		bit = read_bit(data[i], 7);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 7th and 8th bits */
		*out |= (((out_g1 & 1) << 1));
		*out |= ((!(out_g2 & 1)));

		/* 8 bits written to output move to next word */
		out++;
		*out = 0;
	}
}

/**
 * CCSDS compliant K=7 R=2/3 convolutional encoder. Each call to this function
 * is completely independent from each other and there is no state information.
 * Initially the K-1 stages are set to 0. It is responsibility of the caller to
 * add tailbits or proper padding. With these assumptions, the size of the
 * encoded data will be always \f$2/3 \times len\f$
 *
 * @param out the out buffer to hold the result. It is responsibility of the
 * caller to provide enough space for the result
 *
 * @param data the input data
 * @param len the number of input bytes in the \a data buffer
 */
void
conv_encoder_2_3_7(uint8_t *out, const uint8_t *data, size_t len)
{
	/* Pancuring Pattern for 2/3 */
	/* C1: [1, 0] */
	/* C2: [1, 1] */
	uint8_t shift_reg_7[CCSDS_CONV_CONSTRAINT_LENGTH_BITS] = {0, 0, 0, 0, 0, 0, 0};
	uint8_t bit = 0;
	uint8_t out_g1 = 0;
	uint8_t out_g2 = 0;

	/* Clean output buffer from possible garbage value */
	*out = 0;

	for (int i = 0; i < len; i++) {
		/* Read 1st bit */
		bit = read_bit(data[i], 0);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 1st and 2nd bits */
		*out |= (((out_g1 & 1) << 7));
		*out |= (((out_g2 & 1) << 6));

		/* Read 2nd bit */
		bit = read_bit(data[i], 1);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 3rd bit */
		*out |= (((out_g2 & 1) << 5));

		/* Read 3rd bit */
		bit = read_bit(data[i], 2);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 4th and 5th bits */
		*out |= (((out_g1 & 1) << 4));
		*out |= (((out_g2 & 1) << 3));

		/* Read 4th bit */
		bit = read_bit(data[i], 3);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 6th bit */
		*out |= (((out_g2 & 1) << 2));

		/* Read 5th bit */
		bit = read_bit(data[i], 4);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 7th and 8th bits */
		*out |= ((out_g1 & 1) << 1);
		*out |= ((out_g2 & 1));

		/* 8bits written to output move to next word */
		out++;
		*out = 0;

		/* Read 6th bit */
		bit = read_bit(data[i], 5);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 1st bit */
		*out |= ((out_g2 & 1) << 7);

		/* Read 7th bit */
		bit = read_bit(data[i], 6);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 2nd and 3rd bits */
		*out |= (((out_g1 & 1) << 6));
		*out |= (((out_g2 & 1) << 5));

		/* Read 8th bit */
		bit = read_bit(data[i], 7);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 4th bit */
		*out |= (((out_g2 & 1) << 4));

		/* Read 8 bits from input move to next word */
		i++;

		/* Read 1st bit */
		bit = read_bit(data[i], 0);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 5th and 6th bits */
		*out |= (((out_g1 & 1) << 3));
		*out |= (((out_g2 & 1) << 2));

		/* Read 2nd bit */
		bit = read_bit(data[i], 1);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 7th bit */
		*out |= (((out_g2 & 1) << 1));

		/* Read 3rd bit */
		bit = read_bit(data[i], 2);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 8th bit */
		*out |= ((out_g1 & 1));

		/* 8 bits written to output move to next word */
		out++;
		*out = 0;

		/* Write convolution output 1st bit */
		*out |= (((out_g2 & 1) << 7));

		/* Read 4th bit */
		bit = read_bit(data[i], 3);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 2nd bit */
		*out |= (((out_g2 & 1) << 6));

		/* Read 5th bit */
		bit = read_bit(data[i], 4);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 3rd and 4th bit */
		*out |= (((out_g1 & 1) << 5));
		*out |= (((out_g2 & 1) << 4));

		/* Read 6th bit */
		bit = read_bit(data[i], 5);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 5th bit */
		*out |= (((out_g2 & 1) << 3));

		/* Read 7th bit */
		bit = read_bit(data[i], 6);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 6th and 7th bits */
		*out |= (((out_g1 & 1) << 2));
		*out |= (((out_g2 & 1) << 1));

		/* Read 8th bit */
		bit = read_bit(data[i], 7);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G2 */
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output 8th bit */
		*out |= (((out_g2 & 1)));

		/* 8 bits written to output move to next word */
		out++;
		*out = 0;
	}
}

/**
 * CCSDS compliant K=7 R=3/4 convolutional encoder. Each call to this function
 * is completely independent from each other and there is no state information.
 * Initially the K-1 stages are set to 0. It is responsibility of the caller to
 * add tailbits or proper padding. With these assumptions, the size of the
 * encoded data will be always \f$3/4 \times len\f$
 *
 * @param out the out buffer to hold the result. It is responsibility of the
 * caller to provide enough space for the result
 *
 * @param data the input data
 * @param len the number of input bytes in the \a data buffer
 */
void
conv_encoder_3_4_7(uint8_t *out, const uint8_t *data, size_t len)
{
	/* Pancuring Pattern for 3/4 */
	const uint8_t C1[3] = {1, 0, 1};
	const uint8_t C2[3] = {1, 1, 0};

	calculate_punctured_output(out, data, len, C1, C2, 3);
}

/**
 * CCSDS compliant K=7 R=5/6 convolutional encoder. Each call to this function
 * is completely independent from each other and there is no state information.
 * Initially the K-1 stages are set to 0. It is responsibility of the caller to
 * add tailbits or proper padding. With these assumptions, the size of the
 * encoded data will be always \f$5/6 \times len\f$
 *
 * @param out the out buffer to hold the result. It is responsibility of the
 * caller to provide enough space for the result
 *
 * @param data the input data
 * @param len the number of input bytes in the \a data buffer
 */
void
conv_encoder_5_6_7(uint8_t *out, const uint8_t *data, size_t len)
{
	/* Pancturing Pattern for 5/6 */
	const uint8_t C1[5] = {1, 0, 1, 0, 1};
	const uint8_t C2[5] = {1, 1, 0, 1, 0};

	calculate_punctured_output(out, data, len, C1, C2, 5);
}

/**
 * CCSDS compliant K=7 R=7/8 convolutional encoder. Each call to this function
 * is completely independent from each other and there is no state information.
 * Initially the K-1 stages are set to 0. It is responsibility of the caller to
 * add tailbits or proper padding. With these assumptions, the size of the
 * encoded data will be always \f$7/8 \times len\f$
 *
 * @param out the out buffer to hold the result. It is responsibility of the
 * caller to provide enough space for the result
 *
 * @param data the input data
 * @param len the number of input bytes in the \a data buffer
 */
void
conv_encoder_7_8_7(uint8_t *out, const uint8_t *data, size_t len)
{
	/* Pancturing Pattern for 7/8 */
	const uint8_t C1[7] = {1, 0, 0, 0, 1, 0, 1};
	const uint8_t C2[7] = {1, 1, 1, 1, 0, 1, 0};

	calculate_punctured_output(out, data, len, C1, C2, 7);
}

static void
calculate_punctured_output(uint8_t *out, const uint8_t *data, const size_t len,
                           const uint8_t *puncturing_pattern_c1,
                           const uint8_t *puncturing_pattern_c2,
                           const uint8_t puncturing_pattern_length)
{
	uint8_t shift_reg_7[CCSDS_CONV_CONSTRAINT_LENGTH_BITS] = {0, 0, 0, 0, 0, 0, 0};
	uint8_t bit = 0;
	size_t processed = 0;
	uint8_t input_bit = 0;
	uint8_t out_bit = 0;
	uint8_t out_g1 = 0;
	uint8_t out_g2 = 0;
	uint8_t puncturing_pattern_index = 0;

	/* Clean output buffer from possible garbage value */
	*out = 0;

	while (processed < len * 8) {
		/* Read each bit */
		bit = read_bit(*data, input_bit++);
		/* bit shift virtual register */
		bit_shift(shift_reg_7, bit);
		/* Calculate output for G1 and G2 */
		out_g1 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G1);
		out_g2 = calculate_generator_output(shift_reg_7, CCSDS_CONV_G2);

		/* Write convolution output to out */
		*out |= (((out_g1 & puncturing_pattern_c1[puncturing_pattern_index])
		          << (7 - out_bit)));
		out_bit += puncturing_pattern_c1[puncturing_pattern_index];
		*out |= (((out_g2 & puncturing_pattern_c2[puncturing_pattern_index])
		          << (7 - out_bit)));
		out_bit += puncturing_pattern_c2[puncturing_pattern_index];

		/* Move pancture index to next value and reset if needed */
		puncturing_pattern_index = (puncturing_pattern_index + 1) %
		                           puncturing_pattern_length;

		/* Increase processed bits */
		processed++;
		/* Check if processed 8 bits */
		if (out_bit == 8) {
			out++;
			*out = 0;
			out_bit = 0;
		}
		/* Check if out byte full */
		if (input_bit == 8) {
			data++;
			input_bit = 0;
		}
	}
}

static inline uint8_t
read_bit(const uint8_t data, const uint8_t input_bit)
{
	return (data >> (7 - input_bit)) & 1;
}

static inline uint8_t
calculate_generator_output(const uint8_t *shift_reg, const uint8_t *polynomial)
{
	uint8_t out = 0;

	for (int i = 0; i < CCSDS_CONV_CONSTRAINT_LENGTH_BITS; i++) {
		out ^= shift_reg[i] * polynomial[i];
	}
	return out;
}

static void
bit_shift(uint8_t *reg, uint8_t d_in)
{
	for (int i = 6; i > 0; i--) {
		reg[i] = reg[i - 1];
	}
	reg[0] = d_in;
}
