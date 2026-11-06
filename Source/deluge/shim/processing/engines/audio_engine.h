// Caosmo port shim (Eloy Platas, 2026): minimal stand-in for a Deluge firmware header so the GPL-3.0 Deluge
// effects in Source/deluge build on the desktop. Licensed GPL-3.0-or-later like the code it supports.
#pragma once
#include <cstdint>
namespace AudioEngine {
inline bool renderInStereo = true;
inline uint32_t audioSampleTimer = 0;
inline uint32_t timeThereWasLastSomeReverb = 0;
// The Deluge feeds grain reverb straight into the song reverb; Caosmo routes it to its own Deluge reverb.
inline int32_t* grainReverbBackdoor = nullptr;
inline int32_t grainReverbBackdoorSize = 0;
inline void feedReverbBackdoorForGrain (int32_t index, int32_t value) {
	if (grainReverbBackdoor && index >= 0 && index < grainReverbBackdoorSize) grainReverbBackdoor[index] += value;
}
} // namespace AudioEngine
