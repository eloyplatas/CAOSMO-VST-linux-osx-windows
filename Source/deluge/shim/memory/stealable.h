// Caosmo port shim (Eloy Platas, 2026): minimal stand-in for a Deluge firmware header so the GPL-3.0 Deluge
// effects in Source/deluge build on the desktop. Licensed GPL-3.0-or-later like the code it supports.
#pragma once
enum class StealableQueue { CURRENT_SONG_SAMPLE_DATA_REPITCHED_CACHE };
class Stealable {
public:
	virtual ~Stealable() = default;
	virtual bool mayBeStolen (void*) { return false; }
	virtual void steal (char const*) {}
	virtual StealableQueue getAppropriateQueue() { return StealableQueue::CURRENT_SONG_SAMPLE_DATA_REPITCHED_CACHE; }
};
