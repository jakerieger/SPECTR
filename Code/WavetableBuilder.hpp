//
// Created by jr on 3/13/2026.
//

#pragma once

#include "WavetableData.hpp"
#include <juce_dsp/juce_dsp.h>

namespace SPECTR {
    // Converts a raw audio buffer into a fully populated WavetableData struct.
    //
    // Two modes are available (see BuildMode in WavetableData.hpp):
    //
    // FFTMorph (default):
    //   1. Slice audio into kFrameSize windows
    //   2. FFT each window to extract complex harmonics
    //   3. Interpolate/stretch to exactly kNumFrames output frames
    //   4. DC removal + phase alignment to frame 0
    //   5. Synthesize each mip level (zero harmonics above cutoff)
    //   6. Fill WavetableFrame sample arrays (with guard sample)
    //
    // Slice:
    //   1. Slice audio into consecutive kFrameSize blocks (no overlap)
    //   2. Build mip levels directly from the time-domain samples
    //      (low-pass filter via harmonic truncation using the FFT pipeline)
    //   3. actualNumFrames = numSamples / kFrameSize  (no stretching to 256)
    class WavetableBuilder {
    public:
        WavetableBuilder();

        // FFTMorph build: analyzes audio and stretches to kNumFrames frames with
        // smooth morphing between them.
        // Returns true on success, false if audio is too short or silent.
        bool build(const juce::AudioBuffer<f32>& audio, WavetableData& outTable, const juce::String& sourceName);

        // Slice build: cuts audio into consecutive kFrameSize blocks.
        // actualNumFrames = numSamples / kFrameSize; no interpolation between frames.
        // Returns true on success, false if audio is too short or silent.
        bool buildSliced(const juce::AudioBuffer<f32>& audio, WavetableData& outTable, const juce::String& sourceName);

    private:
        //--- FFT analysis of one window -------------------------------------------
        // Window is always exactly kFrameSize samples (one fixed-stride slice).
        void analyzeCycle(const f32* cycle, std::vector<std::complex<f32>>& outBins) const;

        //--- Step 3: map source frames to kNumFrames output frames -----------------
        // Uses linear interpolation between adjacent source-frame bin sets.
        static void interpolateFrames(const std::vector<std::vector<std::complex<f32>>>& srcFrames,
                                      std::vector<std::vector<std::complex<f32>>>& outFrames);

        //--- Step 4: DC removal + phase alignment ----------------------------------
        static void removeDC(std::vector<std::complex<f32>>& bins);
        static void alignPhase(std::vector<std::complex<f32>>& bins, const std::vector<std::complex<f32>>& ref);

        //--- Step 5 & 6: synthesize samples from bins for one mip level ------------
        void
        synthesizeFrame(const std::vector<std::complex<f32>>& bins, int maxHarmonic, WavetableFrame& outFrame) const;

        //--- Helpers ---------------------------------------------------------------
        // Normalize a buffer to peak ±1.
        static void normalizePeak(std::vector<f32>& buf);

        static constexpr int kFFTSize = Wavetable::kFrameSize;  // must be power of 2

        std::unique_ptr<juce::dsp::FFT> fft;

        void ensureFFT() {
            if (fft == nullptr) fft = std::make_unique<juce::dsp::FFT>(11);  // 2^11 = kFFTSize
        }
    };
}  // namespace SPECTR