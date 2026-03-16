//
// Created by jr on 3/15/2026.
//

#pragma once

#include "PluginCommon.hpp"

namespace SPECTR::Wavetable {
    static constexpr i32 kFrameSize    = 2048;  // samples per frame
    static constexpr i32 kNumFrames    = 256;   // total frames in the table
    static constexpr i32 kNumMipLevels = 10;    // mip-map levels for bandlimiting

    // Each mip level removes the top half of harmonics, allowing the oscillator
    // to pick a level appropriate for the current playback pitch and avoid aliasing.
    static constexpr std::array kMipMaxHarmonics = {1024, 512, 256, 128, 64, 32, 16, 8, 4, 2};
}  // namespace SPECTR::Wavetable