// Caosmo port shim (Eloy Platas, 2026): a portable stand-in for the tiny part of the Argon NEON wrapper
// (ArgonHalf<float>, a 2-lane float vector) used by dsp/reverb/cosine_oscillator.hpp, so the Mutable
// reverb builds on desktop CPUs. Lane-wise semantics match the NEON version.
#pragma once
#include <array>
#include <cstddef>

template <typename T>
struct ArgonHalf
{
    std::array<T, 2> v {};
    constexpr ArgonHalf() = default;
    constexpr ArgonHalf (T s) : v { s, s } {}
    static constexpr ArgonHalf Load (const T* p) { ArgonHalf a; a.v = { p[0], p[1] }; return a; }
    constexpr T& operator[] (size_t i) { return v[i]; }
    constexpr const T& operator[] (size_t i) const { return v[i]; }
    static constexpr size_t size() { return 2; }
    template <typename F>
    constexpr ArgonHalf each_lane_with_index (F&& f) const { ArgonHalf a = *this; for (int i = 0; i < 2; ++i) f (a.v[(size_t) i], i); return a; }
    friend constexpr ArgonHalf operator+ (ArgonHalf a, ArgonHalf b) { return { a.v[0] + b.v[0], a.v[1] + b.v[1] }; }
    friend constexpr ArgonHalf operator- (ArgonHalf a, ArgonHalf b) { return { a.v[0] - b.v[0], a.v[1] - b.v[1] }; }
    friend constexpr ArgonHalf operator* (ArgonHalf a, ArgonHalf b) { return { a.v[0] * b.v[0], a.v[1] * b.v[1] }; }
    constexpr ArgonHalf (T a, T b) : v { a, b } {}
};
