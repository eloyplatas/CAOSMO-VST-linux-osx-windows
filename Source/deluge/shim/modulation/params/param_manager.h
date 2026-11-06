// Caosmo port shim (Eloy Platas, 2026): minimal stand-in for a Deluge firmware header so the GPL-3.0 Deluge
// effects in Source/deluge build on the desktop. Licensed GPL-3.0-or-later like the code it supports.
#pragma once
#include "modulation/params/param_set.h"
class ParamManager {
public:
	UnpatchedParamSet unpatched;
	UnpatchedParamSet* getUnpatchedParamSet() { return &unpatched; }
};
class ParamManagerForTimeline : public ParamManager {};
