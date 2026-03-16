//
// Created by jr on 3/13/2026.
//

#pragma once

#include "Wavetable.hpp"
#include <juce_audio_basics/juce_audio_basics.h>

namespace SPECTR {
    enum class BuildMode {
        // FFT-analyze every kFrameSize-sample window, then interpolate/stretch the
        // results to exactly kNumFrames output frames with morphing between them.
        FFTMorph,
        // Slice the audio into consecutive kFrameSize-sample blocks with no
        // interpolation between frames.  actualNumFrames = numSamples / kFrameSize.
        // The position knob maps [0,1] -> [0, actualNumFrames).
        Slice,
    };

    // A single wavetable frame at one mip level.
    // Stored with one extra guard sample (= sample[0]) to allow branchless
    // linear interpolation at the wrap boundary.
    struct WavetableFrame {
        // kFrameSize + 1: the extra guard sample equals sample[0] and allows
        // branchless linear interpolation at the phase wrap boundary.
        std::vector<float> samples;

        WavetableFrame() : samples(Wavetable::kFrameSize + 1, 0.0f) {}

        _Nodisc f32 getSample(const f32 phase) const noexcept {
            // phase is [0, 1)
            const float scaled = phase * _Cast<float>(Wavetable::kFrameSize);
            const int idx0     = _Cast<int>(scaled);
            const float frac   = scaled - _Cast<float>(idx0);
            return samples[idx0] + frac * (samples[idx0 + 1] - samples[idx0]);
        }
    };

    // Full wavetable: 256 frames × kNumMipLevels mip levels.
    // WavetableBuilder fills this; WavetableOscillator reads from it.
    struct WavetableData {
        // [mipLevel][frameIndex]
        std::vector<std::vector<WavetableFrame>> frames;

        bool isLoaded {false};
        juce::String sourceName;

        int actualNumFrames {Wavetable::kNumFrames};
        BuildMode buildMode {BuildMode::FFTMorph};

        WavetableData() {
            frames.resize(Wavetable::kNumMipLevels);
            for (auto& mip : frames)
                mip.resize(Wavetable::kNumFrames);
        }

        // Choose mip level so that the highest retained harmonic stays below Nyquist.
        // We assume standard 440 Hz A4 tuning: freq = 440 * 2^((note-69)/12)
        // At sampleRate 44100, Nyquist = 22050 Hz.
        // maxHarmonic = floor(Nyquist / freq).  Pick the finest mip that fits.
        static int getMipLevel(const int midiNote) noexcept {
            // Higher note → fewer harmonics fit below Nyquist → coarser mip
            // Derived for 44100 Hz; safe to keep slightly conservative at other rates.
            const int maxH = _Cast<int>(22050.0 / (440.0 * std::pow(2.0, (midiNote - 69) / 12.0)));
            for (int i = 0; i < Wavetable::kNumMipLevels; ++i)
                if (Wavetable::kMipMaxHarmonics[i] <= maxH) { return i; }
            return Wavetable::kNumMipLevels - 1;
        }

        // Returns the interpolated sample for a given phase, frame position, and
        // MIDI note (used to select the appropriate mip level).
        //
        // normalizedPos - [0, 1)  position across actualNumFrames
        // phase         - [0, 1)  oscillator phase
        // midiNote      - 0..127  for mip level selection
        _Nodisc f32 getSample(const f32 normalizedPos, const f32 phase, const int midiNote) const noexcept {
            const int mip = getMipLevel(midiNote);
            const int N   = actualNumFrames;

            if (buildMode == BuildMode::Slice) {
                // In slice mode there is no morphing — snap to the nearest frame.
                const int fi = std::clamp(_Cast<int>(normalizedPos * _Cast<f32>(N)), 0, N - 1);
                return frames[mip][fi].getSample(phase);
            }

            // FFTMorph: linearly interpolate between adjacent frames.
            const f32 framePos = normalizedPos * _Cast<f32>(N - 1);
            const int f0       = std::clamp(_Cast<int>(framePos), 0, N - 1);
            const int f1       = std::min(f0 + 1, N - 1);
            const f32 frac     = framePos - _Cast<f32>(f0);

            const f32 s0 = frames[mip][f0].getSample(phase);
            const f32 s1 = frames[mip][f1].getSample(phase);
            return s0 + frac * (s1 - s0);
        }
    };

}  // namespace SPECTR