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

// Caosmo port note: subset of upstream util/lookuptables/lookuptables.h. Modified by Eloy Platas / Caosmo, 2026.
#pragma once
#include <cstdint>
extern const int16_t sineWaveSmall[257];
extern const uint16_t expTableSmall[257];
extern const int16_t tanHSmall[];
extern const int16_t tanH2d[][129];
