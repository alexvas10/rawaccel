#pragma once

// MSVC compatibility shims — include this before any common/ header on Linux
#define _copysign copysign
#define __forceinline inline __attribute__((always_inline))
