/*
 * Copyright © 2016-2023 Synthstrom Audible Limited
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

// Modified by Eloy Platas / Caosmo, 2026-11-06: the EQ, sample-rate-reduction and bitcrush code of
// ModControllableAudio (model/mod_controllable/mod_controllable_audio.cpp) moved into a standalone class.
// The DSP lines are unchanged; the only edits replace reads of the song's UnpatchedParamSet with plain
// function arguments carrying the same Deluge parameter values.
#pragma once
#include "dsp/stereo_sample.h"
#include "util/functions.h"
#include <span>

class DelugeChannelFX {
public:
	void processEQ(std::span<StereoSample> buffer, int32_t bass, int32_t treble, int32_t bassFreqParam,
	               int32_t trebleFreqParam) {
	// EQ -------------------------------------------------------------------------------------
	bool thisDoBass = (bass != 0);
	bool thisDoTreble = (treble != 0);

	// Bass. No-change represented by 0. Off completely represented by -536870912
	int32_t positive = (bass >> 1) + 1073741824;
	int32_t bassAmount = (multiply_32x32_rshift32_rounded(positive, positive) << 1) - 536870912;

	// Treble. No-change represented by 536870912
	positive = (treble >> 1) + 1073741824;
	int32_t trebleAmount = multiply_32x32_rshift32_rounded(positive, positive) << 1;

	if (thisDoBass || thisDoTreble) {

		if (thisDoBass) {
			bassFreq = getExp(120000000, (bassFreqParam >> 5) * 6);
		}

		if (thisDoTreble) {
			trebleFreq = getExp(700000000, (trebleFreqParam >> 5) * 6);
		}

		for (StereoSample& sample : buffer) {
			doEQ(thisDoBass, thisDoTreble, &sample.l, &sample.r, bassAmount, trebleAmount);
		}
	}
	}

	void processSRRAndBitcrushing(std::span<StereoSample> buffer, int32_t* postFXVolume, int32_t sampleRateReduction,
	                              int32_t bitcrushing) {
	uint32_t bitCrushMaskForSRR = 0xFFFFFFFF;

	bool srrEnabled = (sampleRateReduction != -2147483648);

	// Bitcrushing ------------------------------------------------------------------------------
	if (bitcrushing >= -2113929216) {
		uint32_t positivePreset =
		    ((uint32_t)bitcrushing + 2147483648u) >> 29;
		if (positivePreset > 4) {
			*postFXVolume >>= (positivePreset - 4);
		}

		// If not also doing SRR
		if (!srrEnabled) {
			uint32_t mask = 0xFFFFFFFF << (19 + (positivePreset));
			for (StereoSample& sample : buffer) {
				sample.l &= mask;
				sample.r &= mask;
			}
		}

		else {
			bitCrushMaskForSRR = 0xFFFFFFFF << (18 + (positivePreset));
		}
	}

	// Sample rate reduction -------------------------------------------------------------------------------------
	if (srrEnabled) {

		// Set up for first time
		if (!sampleRateReductionOnLastTime) {
			sampleRateReductionOnLastTime = true;
			lastSample.l = lastSample.r = 0;
			grabbedSample.l = grabbedSample.r = 0;
			lowSampleRatePos = 0;
		}

		// This function, slightly unusually, uses 22 bits to represent "1". That's 4194304. I tried using 24, but stuff
		// started clipping off where I needed it if sample rate too low

		uint32_t positivePreset =
		    (uint32_t)sampleRateReduction + 2147483648u;
		int32_t lowSampleRateIncrement = getExp(4194304, (positivePreset >> 3));
		int32_t highSampleRateIncrement = ((uint32_t)0xFFFFFFFF / (lowSampleRateIncrement >> 6)) << 6;
		// int32_t highSampleRateIncrement = getExp(4194304, -(int32_t)(positivePreset >> 3)); // This would work too

		for (StereoSample& sample : buffer) {
			// Convert down.
			// If time to "grab" another sample for down-conversion...
			if (lowSampleRatePos < 4194304) {
				int32_t strength2 = lowSampleRatePos;
				int32_t strength1 = 4194303 - strength2;

				lastGrabbedSample = grabbedSample; // What was current is now last
				grabbedSample.l = multiply_32x32_rshift32_rounded(lastSample.l, strength1 << 9)
				                  + multiply_32x32_rshift32_rounded(sample.l, strength2 << 9);
				grabbedSample.r = multiply_32x32_rshift32_rounded(lastSample.r, strength1 << 9)
				                  + multiply_32x32_rshift32_rounded(sample.r, strength2 << 9);
				grabbedSample.l &= bitCrushMaskForSRR;
				grabbedSample.r &= bitCrushMaskForSRR;

				// Set the "time" at which we want to "grab" our next sample for down-conversion.
				lowSampleRatePos += lowSampleRateIncrement;

				// "Re-sync" the up-conversion spinner.
				// I previously had it using strength2 instead of "lowSampleRatePos & 16777215", but that just works
				// better. Ah, writing the massive explanation would take ages.
				highSampleRatePos =
				    multiply_32x32_rshift32_rounded(lowSampleRatePos & 4194303, highSampleRateIncrement << 8) << 2;
			}
			lowSampleRatePos -= 4194304; // We're one step closer to grabbing our next sample for down-conversion
			lastSample = sample;

			// Convert up
			// Would only overshoot if we raised the sample rate during playback
			int32_t strength2 = std::min(highSampleRatePos, (uint32_t)4194303);
			int32_t strength1 = 4194303 - strength2;
			sample.l = (multiply_32x32_rshift32_rounded(lastGrabbedSample.l, strength1 << 9)
			            + multiply_32x32_rshift32_rounded(grabbedSample.l, strength2 << 9))
			           << 2;
			sample.r = (multiply_32x32_rshift32_rounded(lastGrabbedSample.r, strength1 << 9)
			            + multiply_32x32_rshift32_rounded(grabbedSample.r, strength2 << 9))
			           << 2;

			highSampleRatePos += highSampleRateIncrement;
		}
	}
	else {
		sampleRateReductionOnLastTime = false;
	}
}

private:
	[[gnu::always_inline]] inline void doEQ(bool doBass, bool doTreble, int32_t* inputL, int32_t* inputR,
	                                        int32_t bassAmount, int32_t trebleAmount) {
	int32_t trebleOnlyL;
	int32_t trebleOnlyR;

	if (doTreble) {
		int32_t distanceToGoL = *inputL - withoutTrebleL;
		int32_t distanceToGoR = *inputR - withoutTrebleR;
		withoutTrebleL += multiply_32x32_rshift32(distanceToGoL, trebleFreq) << 1;
		withoutTrebleR += multiply_32x32_rshift32(distanceToGoR, trebleFreq) << 1;
		trebleOnlyL = *inputL - withoutTrebleL;
		trebleOnlyR = *inputR - withoutTrebleR;
		*inputL = withoutTrebleL; // Input now has had the treble removed. Or is this bad?
		*inputR = withoutTrebleR; // Input now has had the treble removed. Or is this bad?
	}

	if (doBass) {
		int32_t distanceToGoL = *inputL - bassOnlyL;
		int32_t distanceToGoR = *inputR - bassOnlyR;
		bassOnlyL += multiply_32x32_rshift32(distanceToGoL, bassFreq); // 33554432
		bassOnlyR += multiply_32x32_rshift32(distanceToGoR, bassFreq); // 33554432
	}

	if (doTreble) {
		*inputL += (multiply_32x32_rshift32(trebleOnlyL, trebleAmount) << 3);
		*inputR += (multiply_32x32_rshift32(trebleOnlyR, trebleAmount) << 3);
	}
	if (doBass) {
		*inputL += (multiply_32x32_rshift32(bassOnlyL, bassAmount) << 3);
		*inputR += (multiply_32x32_rshift32(bassOnlyR, bassAmount) << 3);
	}
}

	int32_t bassFreq{0};
	int32_t trebleFreq{0};
	int32_t withoutTrebleL{0};
	int32_t bassOnlyL{0};
	int32_t withoutTrebleR{0};
	int32_t bassOnlyR{0};
	bool sampleRateReductionOnLastTime{false};
	uint32_t lowSampleRatePos{0};
	uint32_t highSampleRatePos{0};
	StereoSample lastSample{0, 0};
	StereoSample grabbedSample{0, 0};
	StereoSample lastGrabbedSample{0, 0};
};
