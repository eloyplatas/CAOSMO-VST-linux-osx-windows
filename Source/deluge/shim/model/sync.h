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

// Caosmo port note: subset of upstream model/sync.h (values unchanged). Modified by Eloy Platas / Caosmo, 2026.
#pragma once
#include <cstdint>
// type in the selection menu. There are 9 different levels for each type (see
// also SyncLevel)
enum SyncType : uint8_t {
	SYNC_TYPE_EVEN = 0,
	SYNC_TYPE_TRIPLET = 10,
	SYNC_TYPE_DOTTED = 19,
};

// Triplet swing not supported yet.
#define MIN_SWING_INERVAL 1
#define MAX_SWING_INTERVAL (SYNC_TYPE_TRIPLET - 1)
#define NUM_SWING_INTERVALS SYNC_TYPE_TRIPLET

// NOTE: These names are correct only for default resolution!
enum SyncLevel : uint8_t {
	SYNC_LEVEL_NONE = 0,
	SYNC_LEVEL_WHOLE = 1,
	SYNC_LEVEL_2ND = 2,
	SYNC_LEVEL_4TH = 3,
	SYNC_LEVEL_8TH = 4,
	SYNC_LEVEL_16TH = 5,
	SYNC_LEVEL_32ND = 6,
	SYNC_LEVEL_64TH = 7,
	SYNC_LEVEL_128TH = 8,
	SYNC_LEVEL_256TH = 9,
};

#define MAX_SYNC_LEVEL 9
#define NUM_SYNC_VALUES 28
