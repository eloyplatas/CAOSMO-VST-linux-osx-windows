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

// Caosmo port note: NOT the upstream file. A minimal subset of upstream definitions_cxx.hpp, definitions.h,
// model/sync.h and util/functions.h (enum and constant values unchanged) needed to build the Deluge effects
// outside the firmware. Modified by Eloy Platas / Caosmo, 2026.
#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace util {
template <typename E>
constexpr auto to_underlying(E e) noexcept { return static_cast<std::underlying_type_t<E>>(e); }
template <typename T>
[[nodiscard]] constexpr T map(T x, T in_min, T in_max, T out_min, T out_max) {
	return out_min + ((x - in_min) * (out_max - out_min)) / (in_max - in_min);
}
} // namespace util

#define SSI_TX_BUFFER_NUM_SAMPLES 128

constexpr int32_t kModFXBufferSize = 512;
constexpr int32_t kModFXBufferIndexMask = (kModFXBufferSize - 1);
constexpr int32_t kModFXMaxDelay = ((kModFXBufferSize - 1) << 16);

constexpr int32_t kModFXGrainBufferSize = 65536;
constexpr int32_t kModFXGrainBufferIndexMask = (kModFXGrainBufferSize - 1);

constexpr int32_t kFlangerMinTime = (3 << 16);
constexpr int32_t kFlangerAmplitude = (kModFXMaxDelay - kFlangerMinTime);
constexpr int32_t kFlangerOffset = ((kModFXMaxDelay + kFlangerMinTime) >> 1);

enum class LFOType : uint8_t {
	SINE,
	TRIANGLE,
	SQUARE,
	SAW,
	SAMPLE_AND_HOLD,
	RANDOM_WALK,
	WARBLER,
};

constexpr int32_t kNumLFOTypes = util::to_underlying(LFOType::WARBLER) + 1;

enum class ModFXType : uint8_t {
	NONE,
	FLANGER,
	CHORUS,
	PHASER,
	CHORUS_STEREO,
	WARBLE,
	DIMENSION,
	GRAIN, // Look below if you want to add another one
};
constexpr int32_t kNumModFXTypes = util::to_underlying(ModFXType::GRAIN) + 1;

constexpr int32_t kNumAllpassFiltersPhaser = 6;

enum class Error {
	NONE,
	INSUFFICIENT_RAM,
	UNSPECIFIED,
	SD_CARD,
	NO_FURTHER_PRESETS,
	FILE_CORRUPTED,
	FILE_UNREADABLE, // Or not found, I think?
	FILE_UNSUPPORTED,
	FILE_FIRMWARE_VERSION_TOO_NEW,
	RESULT_TAG_UNUSED,
	FOLDER_DOESNT_EXIST,
	WRITE_PROTECTED,
	BUG,
	WRITE_FAIL,
	FILE_TOO_BIG,
	PRESET_IN_USE,
	NO_FURTHER_FILES_THIS_DIRECTION,
	FILE_ALREADY_EXISTS,
	FILE_NOT_FOUND,
	ABORTED_BY_USER,
	MAX_FILE_SIZE_REACHED,
	SD_CARD_FULL,
	FILE_NOT_LOADABLE_AS_WAVETABLE,
	FILE_NOT_LOADABLE_AS_WAVETABLE_BECAUSE_STEREO,
	NO_FURTHER_DIRECTORY_LEVELS_TO_GO_UP,
	NO_ERROR_BUT_GET_OUT,
	INSUFFICIENT_RAM_FOR_FOLDER_CONTENTS_SIZE,
	SD_CARD_NOT_PRESENT,
	SD_CARD_NO_FILESYSTEM,
	INVALID_PATTERN_VERSION,
	OUT_OF_BUFFER_SPACE,
	INVALID_SYSEX_FORMAT,
	POS_PAST_STRING,
	FILE_NOT_SAVED,
};

enum class ModFXParam {
	DEPTH,
	FEEDBACK,
	OFFSET,
};
constexpr auto kNumModFXParams = util::to_underlying(ModFXParam::OFFSET) + 1;


constexpr int32_t kSampleRate = 44100;
constexpr int32_t kBitDepth = 24;
constexpr int32_t kMaxSampleValue = 1 << kBitDepth; // 2 ** kBitDepth
