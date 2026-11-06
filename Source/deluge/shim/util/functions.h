/*
 * Copyright © 2014-2023 Synthstrom Audible Limited
 *
 * This file is part of The Synthstrom Audible Deluge Firmware.
 *
 * The Synthstrom Audible Deluge Firmware is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

// Caosmo port note: this file is NOT part of upstream as-is. It collects, unchanged, the small subset of
// util/functions.h, util/functions.cpp and util/lookuptables/lookuptables.cpp that the Deluge effects need,
// so the effects can be built outside the firmware. Modified by Eloy Platas / Caosmo, 2026.
#pragma once
#include "definitions_cxx.hpp"
#include "util/fixedpoint.h"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <string_view>
#include "memory/general_memory_allocator.h"

extern const int16_t sineWaveSmall[257];
extern const uint16_t expTableSmall[257];
extern uint32_t z, w, jcong;

// bits must be *less* than 32! I.e. 31 or less
[[gnu::always_inline]] inline int32_t signed_saturate_operand_unknown(int32_t val, int32_t bits) {

	// Despite having this switch at the per-audio-sample level, it doesn't introduce any slowdown compared to just
	// always saturating by the same amount!
	switch (bits) {
	case 31:
		return signed_saturate<31>(val);
	case 30:
		return signed_saturate<30>(val);
	case 29:
		return signed_saturate<29>(val);
	case 28:
		return signed_saturate<28>(val);
	case 27:
		return signed_saturate<27>(val);
	case 26:
		return signed_saturate<26>(val);
	case 25:
		return signed_saturate<25>(val);
	case 24:
		return signed_saturate<24>(val);
	case 23:
		return signed_saturate<23>(val);
	case 22:
		return signed_saturate<22>(val);
	case 21:
		return signed_saturate<21>(val);
	case 20:
		return signed_saturate<20>(val);
	case 19:
		return signed_saturate<19>(val);
	case 18:
		return signed_saturate<18>(val);
	case 17:
		return signed_saturate<17>(val);
	case 16:
		return signed_saturate<16>(val);
	case 15:
		return signed_saturate<15>(val);
	case 14:
		return signed_saturate<14>(val);
	case 13:
		return signed_saturate<13>(val);
	default:
		return signed_saturate<12>(val);
	}
}

// lshift must be greater than 0! Not 0
[[gnu::always_inline]] inline int32_t lshiftAndSaturateUnknown(int32_t val, uint8_t lshift) {
	return signed_saturate_operand_unknown(val, 32 - lshift) << lshift;
}

[[gnu::always_inline]] inline int32_t increaseMagnitude(int32_t number, int32_t magnitude) {
	if (magnitude >= 0) {
		return number << magnitude;
	}
	return number >> (-magnitude);
}

[[gnu::always_inline]] inline int32_t increaseMagnitudeAndSaturate(int32_t number, int32_t magnitude) {
	if (magnitude > 0) {
		return lshiftAndSaturateUnknown(number, magnitude);
	}
	return number >> (-magnitude);
}

[[gnu::always_inline]] inline int32_t getMagnitudeOld(uint32_t input) {
	return 32 - clz(input);
}

[[gnu::always_inline]] inline int32_t getMagnitude(uint32_t input) {
	return 31 - clz(input);
}

int32_t interpolateTable(uint32_t input, int32_t numBitsInInput, const uint16_t* table, int32_t numBitsInTableSize = 8);
int32_t getExp(int32_t presetValue, int32_t adjustment);
int32_t quickLog(uint32_t input);
bool shouldDoPanning(int32_t panAmount, int32_t* amplitudeL, int32_t* amplitudeR);
int32_t getFinalParameterValueExp(int32_t paramNeutralValue, int32_t patchedValue);
int32_t cableToExpParamShortcut(int32_t sourceValue);
int32_t cableToLinearParamShortcut(int32_t sourceValue);
int32_t getFinalParameterValueVolume(int32_t paramNeutralValue, int32_t patchedValue);
int32_t getFinalParameterValueLinear(int32_t paramNeutralValue, int32_t patchedValue);
// upstream fills this from getParamNeutralValue(); the shim only needs GLOBAL_DELAY_RATE (= kMaxSampleValue upstream)
extern int32_t paramNeutralValues[];
int32_t random(int32_t upperLimit);

#include "util/waves.h"
#include "modulation/params/param_set.h"
namespace params = deluge::modulation::params;

template <uint8_t lshift>
[[gnu::always_inline]] inline int32_t lshiftAndSaturate(int32_t val) {
	return signed_saturate<32 - lshift>(val) << lshift;
}

template <unsigned saturationAmount>
[[gnu::always_inline]] inline int32_t getTanH(int32_t input) {
	uint32_t workingValue;

	if (saturationAmount)
		workingValue = (uint32_t)lshiftAndSaturate<saturationAmount>(input) + 2147483648u;
	else
		workingValue = (uint32_t)input + 2147483648u;

	return interpolateTableSigned(workingValue, 32, tanHSmall, 8) >> (saturationAmount + 2);
}

[[gnu::always_inline]] inline int32_t getTanHUnknown(int32_t input, uint32_t saturationAmount) {
	uint32_t workingValue;

	if (saturationAmount)
		workingValue = (uint32_t)lshiftAndSaturateUnknown(input, saturationAmount) + 2147483648u;
	else
		workingValue = (uint32_t)input + 2147483648u;

	return interpolateTableSigned(workingValue, 32, tanHSmall, 8) >> (saturationAmount + 2);
}

[[gnu::always_inline]] inline uint8_t getRandom255() {
	return CONG >> 24;
}

[[gnu::always_inline]] inline int32_t getNoise() {
	return CONG;
}

/// generates a triangle distribution from -1q31 to 1q31 centered on 0 (Irwin-Hall)
inline q31_t sampleTriangleDistribution() {
	auto u1 = getNoise();
	auto u2 = getNoise();
	auto s = add_saturate(u1, u2);
	return s;
}











// input must not have any extra bits set than numBitsInInput specifies
// Output of this function (unlike the regular 1d one) is only +- 1073741824
[[gnu::always_inline]] inline int32_t interpolateTableSigned2d(uint32_t inputX, uint32_t inputY,
                                                               int32_t numBitsInInputX, int32_t numBitsInInputY,
                                                               const int16_t* table, int32_t numBitsInTableSizeX,
                                                               int32_t numBitsInTableSizeY) {

	int32_t whichValue = inputY >> (numBitsInInputY - numBitsInTableSizeY);

	int32_t tableSizeOneRow = (1 << numBitsInTableSizeX) + 1;

	int32_t value1 =
	    interpolateTableSigned(inputX, numBitsInInputX, &table[whichValue * tableSizeOneRow], numBitsInTableSizeX);
	int32_t value2 = interpolateTableSigned(inputX, numBitsInInputX, &table[(whichValue + 1) * tableSizeOneRow],
	                                        numBitsInTableSizeX);

	int32_t lshiftAmount = 31 + numBitsInTableSizeY - numBitsInInputY;

	uint32_t strength2;

	if (lshiftAmount >= 0)
		strength2 = (inputY << lshiftAmount) & 2147483647;
	else
		strength2 = (inputY >> (0 - lshiftAmount)) & 2147483647;

	uint32_t strength1 = 2147483647 - strength2;
	return multiply_32x32_rshift32(value1, strength1) + multiply_32x32_rshift32(value2, strength2);
}

[[gnu::always_inline]] inline int32_t getTanHAntialiased(int32_t input, uint32_t* lastWorkingValue,
                                                         uint32_t saturationAmount) {

	uint32_t workingValue = (uint32_t)lshiftAndSaturateUnknown(input, saturationAmount) + 2147483648u;

	int32_t toReturn = interpolateTableSigned2d(workingValue, *lastWorkingValue, 32, 32, &tanH2d[0][0], 7, 6)
	                   >> (saturationAmount + 1);
	*lastWorkingValue = workingValue;
	return toReturn;
}

