// Caosmo port shim (Eloy Platas, 2026): minimal stand-in for a Deluge firmware header so the GPL-3.0 Deluge
// effects in Source/deluge build on the desktop. Licensed GPL-3.0-or-later like the code it supports.
#pragma once
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
inline void* delugeAlloc (size_t n, bool = false) { return std::calloc (1, n); }
inline void delugeDealloc (void* p) { std::free (p); }
inline void* allocLowSpeed (size_t n, void* = nullptr) { return std::calloc (1, n); }
inline void* allocMaxSpeed (size_t n, void* = nullptr) { return std::calloc (1, n); }
class GeneralMemoryAllocator {
public:
	static GeneralMemoryAllocator& get() { static GeneralMemoryAllocator g; return g; }
	template <typename T> void* allocStealable (void* = nullptr) { return std::calloc (1, sizeof (T)); }
	void checkStack (const char*) {}
};
