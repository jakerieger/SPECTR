//
// Created by jr on 3/13/2026.
//

#pragma once

#include <array>
#include <vector>
#include <memory>

#define _Cast static_cast
#define _CCast const_cast
#define _DCast dynamic_cast
#define _RCast reinterpret_cast

#define _Nodisc [[nodiscard]]
#define _Unused(x) (void)(x)
#define _SmallFloat 1e-6f

namespace SPECTR {
    using u8   = uint8_t;
    using u16  = uint16_t;
    using u32  = uint32_t;
    using u64  = uint64_t;
    using uptr = uintptr_t;

    using i8   = int8_t;
    using i16  = int16_t;
    using i32  = int32_t;
    using i64  = int64_t;
    using iptr = intptr_t;

#ifndef _MSC_VER
    using u128 = __uint128_t;
    using i128 = __int128_t;
#endif

    using f32 = float;
    using f64 = double;
}  // namespace SPECTR