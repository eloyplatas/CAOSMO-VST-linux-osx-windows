// Caosmo port shim (Eloy Platas, 2026): minimal stand-in for a Deluge firmware header so the GPL-3.0 Deluge
// effects in Source/deluge build on the desktop. Licensed GPL-3.0-or-later like the code it supports.
// Values and the knob conversions below are unchanged from upstream modulation/params/param_set.cpp and
// param_collection.cpp; only the parameters the effects read are kept.
#pragma once
#include <cstdint>
namespace deluge::modulation::params {
enum UnpatchedShared : uint32_t { UNPATCHED_STUTTER_RATE, UNPATCHED_MOD_FX_OFFSET, UNPATCHED_MOD_FX_FEEDBACK, UNPATCHED_NUM };
enum Global : uint32_t { GLOBAL_DELAY_RATE, GLOBAL_NUM };
} // namespace deluge::modulation::params
class ModelStackWithAutoParam;
struct ShimParam {
	int32_t value = 0;
	void setCurrentValueBasicForSetup (int32_t v) { value = v; }
};
class UnpatchedParamSet {
public:
	ShimParam params[deluge::modulation::params::UNPATCHED_NUM];
	int32_t getValue (int32_t p) const { return params[p].value; }
	int32_t paramValueToKnobPos (int32_t paramValue, ModelStackWithAutoParam*) {
		if (paramValue >= (int32_t) (0x80000000 - (1 << 24))) return 64;
		return (paramValue + (1 << 24)) >> 25;
	}
	int32_t knobPosToParamValue (int32_t knobPos, ModelStackWithAutoParam*) {
		int32_t paramValue = 2147483647;
		if (knobPos < 64) paramValue = knobPos << 25;
		return paramValue;
	}
};
