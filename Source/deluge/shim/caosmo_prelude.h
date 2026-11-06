// Caosmo port shim (Eloy Platas, 2026), GPL v3 or later: force-included before every Deluge source file.
// The firmware relies on standard headers being pulled in transitively by its toolchain (GCC/newlib).
// Other standard libraries (MSVC STL, libc++) do not do that, so the headers are included here.
#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <numbers>
#include <span>
#include <type_traits>
#include <utility>
