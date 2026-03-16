//
// Created by jr on 3/13/2026.
//

#include "WavetableBuilder.hpp"

namespace SPECTR {
    WavetableBuilder::WavetableBuilder() = default;

    bool WavetableBuilder::build(const juce::AudioBuffer<f32>& audio,
                                 WavetableData& outTable,
                                 const juce::String& sourceName) {
        ensureFFT();

        // Mix to mono if needed
        const int numChannels = audio.getNumChannels();
        const int numSamples  = audio.getNumSamples();

        // Audio too short
        if (numSamples < 64) return false;

        std::vector<f32> mono(_Cast<size_t>(numSamples), 0.0f);
        for (int ch = 0; ch < numChannels; ++ch) {
            const f32* src = audio.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                mono[_Cast<size_t>(i)] += src[i];
            }
        }

        if (numChannels > 1)
            for (auto& s : mono)
                s /= _Cast<f32>(numChannels);

        // Check for silence
        const f32 peak =
          *std::ranges::max_element(mono, [](const f32 a, const f32 b) { return std::abs(a) < std::abs(b); });
        if (std::abs(peak) < _SmallFloat) return false;

        // Slice into contiguous kFrameSize windows. The FFT treats each window
        // as one complete period, so no cycle-boundary alignment is needed.
        // A partial window at the end is dropped.
        const int numWindows = numSamples / kFFTSize;
        if (numWindows == 0) return false;

        // Clamp to kNumFrames output frames
        const int numSrcFrames = std::min(numWindows, Wavetable::kNumFrames);

        // FFT analysis of each window
        std::vector<std::vector<std::complex<f32>>> srcFrameBins(_Cast<size_t>(numSrcFrames));
        for (int i = 0; i < numSrcFrames; ++i)
            analyzeCycle(mono.data() + i * kFFTSize, srcFrameBins[_Cast<size_t>(i)]);

        // Interpolate / stretch to kNumFrames
        std::vector<std::vector<std::complex<f32>>> outFrameBins;
        interpolateFrames(srcFrameBins, outFrameBins);

        // DC removal + phase alignment to frame 0
        const auto& ref = outFrameBins[0];
        for (auto& bins : outFrameBins) {
            removeDC(bins);
            alignPhase(bins, ref);
        }

        // Synthesize all mip levels for all frames
        for (int mip = 0; mip < Wavetable::kNumMipLevels; ++mip) {
            const int maxH = Wavetable::kMipMaxHarmonics[mip];
            for (int f = 0; f < Wavetable::kNumFrames; ++f)
                synthesizeFrame(outFrameBins[_Cast<size_t>(f)], maxH, outTable.frames[mip][f]);
        }

        outTable.isLoaded        = true;
        outTable.sourceName      = sourceName;
        outTable.actualNumFrames = Wavetable::kNumFrames;
        outTable.buildMode       = BuildMode::FFTMorph;

        return true;
    }

    bool WavetableBuilder::buildSliced(const juce::AudioBuffer<f32>& audio,
                                       WavetableData& outTable,
                                       const juce::String& sourceName) {
        ensureFFT();

        // Mix to mono
        const int numChannels = audio.getNumChannels();
        const int numSamples  = audio.getNumSamples();

        if (numSamples < kFFTSize) return false;

        std::vector<f32> mono(_Cast<size_t>(numSamples), 0.0f);
        for (int ch = 0; ch < numChannels; ++ch) {
            const f32* src = audio.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                mono[_Cast<size_t>(i)] += src[i];
        }
        if (numChannels > 1)
            for (auto& s : mono)
                s /= _Cast<f32>(numChannels);

        // Check for silence
        const f32 peak =
          *std::ranges::max_element(mono, [](const f32 a, const f32 b) { return std::abs(a) < std::abs(b); });
        if (std::abs(peak) < _SmallFloat) return false;

        // How many complete kFFTSize blocks fit in the full buffer?
        const int totalSlices = numSamples / kFFTSize;
        if (totalSlices == 0) return false;

        // Trim silence: compute peak per slice, then drop leading and trailing
        // slices whose peak is below 1% of the overall file peak.
        // This prevents a 4-frame file from producing 252 silent ghost frames.
        const f32 silenceThreshold = std::abs(peak) * 0.01f;

        int firstSlice = 0;
        int lastSlice  = totalSlices - 1;  // inclusive

        for (int i = 0; i < totalSlices; ++i) {
            const f32* block = mono.data() + _Cast<size_t>(i) * kFFTSize;
            f32 blockPeak    = 0.0f;
            for (int s = 0; s < kFFTSize; ++s)
                blockPeak = std::max(blockPeak, std::abs(block[s]));
            if (blockPeak >= silenceThreshold) {
                firstSlice = i;
                break;
            }
        }

        for (int i = totalSlices - 1; i >= firstSlice; --i) {
            const f32* block = mono.data() + _Cast<size_t>(i) * kFFTSize;
            f32 blockPeak    = 0.0f;
            for (int s = 0; s < kFFTSize; ++s)
                blockPeak = std::max(blockPeak, std::abs(block[s]));
            if (blockPeak >= silenceThreshold) {
                lastSlice = i;
                break;
            }
        }

        const int numSlices = std::min(lastSlice - firstSlice + 1, Wavetable::kNumFrames);
        if (numSlices == 0) return false;

        // FFT-analyse each trimmed slice (same pipeline as FFTMorph, but no stretching).
        std::vector<std::vector<std::complex<f32>>> sliceBins(_Cast<size_t>(numSlices));
        for (int i = 0; i < numSlices; ++i)
            analyzeCycle(mono.data() + _Cast<size_t>(firstSlice + i) * kFFTSize, sliceBins[_Cast<size_t>(i)]);

        // DC removal + phase alignment to slice 0
        const auto& ref = sliceBins[0];
        for (auto& bins : sliceBins) {
            removeDC(bins);
            alignPhase(bins, ref);
        }

        // Synthesize mip levels for each slice.
        // Frames beyond numSlices are left as the default (silent) WavetableFrame.
        for (int mip = 0; mip < Wavetable::kNumMipLevels; ++mip) {
            const int maxH = Wavetable::kMipMaxHarmonics[mip];
            for (int f = 0; f < numSlices; ++f)
                synthesizeFrame(sliceBins[_Cast<size_t>(f)], maxH, outTable.frames[mip][f]);
        }

        outTable.isLoaded        = true;
        outTable.sourceName      = sourceName;
        outTable.actualNumFrames = numSlices;
        outTable.buildMode       = BuildMode::Slice;

        return true;
    }

    void WavetableBuilder::analyzeCycle(const f32* cycle, std::vector<std::complex<f32>>& outBins) const {
        // performRealOnlyForwardTransform contract (from JUCE docs):
        //   - Buffer size must be 2 * N
        //   - Input: raw real samples in the FIRST HALF [0..N-1], second half zeroed
        //   - Output (onlyCalculateNonNegativeFrequencies=true):
        //       at least (N/2)+1 complex numbers as interleaved [re,im] pairs
        //       starting at index 0, i.e. bin k → [k*2], [k*2+1]
        std::vector<f32> fftBuffer(kFFTSize * 2, 0.0f);

        // Windows are always exactly kFrameSize samples — copy directly into
        // the first half of the FFT buffer. No resampling needed.
        std::copy(cycle, cycle + kFFTSize, fftBuffer.data());

        fft->performRealOnlyForwardTransform(fftBuffer.data(), true);

        // Read (N/2)+1 complex bins from the interleaved output
        constexpr int numBins = kFFTSize / 2 + 1;
        outBins.resize(numBins);
        for (int k = 0; k < numBins; ++k)
            outBins[_Cast<size_t>(k)] = {fftBuffer[_Cast<size_t>(k * 2)], fftBuffer[_Cast<size_t>(k * 2 + 1)]};
    }

    void WavetableBuilder::interpolateFrames(const std::vector<std::vector<std::complex<f32>>>& srcFrames,
                                             std::vector<std::vector<std::complex<f32>>>& outFrames) {
        const int src     = _Cast<int>(srcFrames.size());
        constexpr int dst = Wavetable::kNumFrames;
        const int nBins   = _Cast<int>(srcFrames[0].size());
        outFrames.resize(dst);

        for (int di = 0; di < dst; ++di) {
            // Map output frame index into source frame space
            const f32 pos  = _Cast<f32>(di) * _Cast<f32>(src - 1) / _Cast<f32>(dst - 1);
            const int si0  = std::min(_Cast<int>(pos), src - 1);
            const int si1  = std::min(si0 + 1, src - 1);
            const f32 frac = pos - _Cast<f32>(si0);

            outFrames[_Cast<size_t>(di)].resize(_Cast<size_t>(nBins));
            for (int k = 0; k < nBins; ++k) {
                outFrames[_Cast<size_t>(di)][_Cast<size_t>(k)] =
                  srcFrames[_Cast<size_t>(si0)][_Cast<size_t>(k)] * (1.0f - frac) +
                  srcFrames[_Cast<size_t>(si1)][_Cast<size_t>(k)] * frac;
            }
        }
    }

    void WavetableBuilder::removeDC(std::vector<std::complex<f32>>& bins) {
        if (!bins.empty()) bins[0] = {0.f, 0.f};
    }

    void WavetableBuilder::alignPhase(std::vector<std::complex<f32>>& bins, const std::vector<std::complex<f32>>& ref) {
        if (bins.size() < 2 || ref.size() < 2) return;

        // Phase difference at the fundamental
        const auto refPhase   = std::arg(ref[1]);
        const auto framePhase = std::arg(bins[1]);
        const auto delta      = refPhase - framePhase;

        // Rotate every bin by -delta * k (linear phase shift = time shift)
        const int numBins = _Cast<int>(bins.size());
        for (int k = 1; k < numBins; ++k) {
            const auto rot = delta * _Cast<f32>(k);
            const std::complex<f32> rotator(std::cos(rot), std::sin(rot));
            bins[_Cast<size_t>(k)] *= rotator;
        }
    }

    void WavetableBuilder::synthesizeFrame(const std::vector<std::complex<f32>>& bins,
                                           const int maxHarmonic,
                                           WavetableFrame& outFrame) const {
        // performRealOnlyInverseTransform contract (from JUCE docs):
        //   - Buffer size must be 2 * N
        //   - Input: interleaved [re,im] complex pairs (same format as forward output)
        //   - Output: reconstituted real samples in the FIRST HALF [0..N-1]
        std::vector<f32> ifftBuffer(kFFTSize * 2, 0.0f);

        const int numBins = _Cast<int>(bins.size());
        const int limit   = std::min(maxHarmonic + 1, numBins);

        // Write positive-frequency bins as interleaved [re, im]
        for (int k = 0; k < limit; ++k) {
            ifftBuffer[_Cast<size_t>(k * 2)]     = bins[_Cast<size_t>(k)].real();
            ifftBuffer[_Cast<size_t>(k * 2 + 1)] = bins[_Cast<size_t>(k)].imag();
        }

        // Conjugate mirror (negative-frequency) bins — required for a real-valued output
        for (int k = 1; k < limit - 1; ++k) {
            const int mirror                          = kFFTSize - k;
            ifftBuffer[_Cast<size_t>(mirror * 2)]     = bins[_Cast<size_t>(k)].real();
            ifftBuffer[_Cast<size_t>(mirror * 2 + 1)] = -bins[_Cast<size_t>(k)].imag();
        }

        fft->performRealOnlyInverseTransform(ifftBuffer.data());

        // Output is in the first half [0..kFFTSize-1] — read sequentially, not at [i*2]
        float peak = 0.0f;
        for (int i = 0; i < kFFTSize; ++i)
            peak = std::max(peak, std::abs(ifftBuffer[_Cast<size_t>(i)]));
        if (peak > _SmallFloat)
            for (int i = 0; i < kFFTSize; ++i)
                outFrame.samples[_Cast<size_t>(i)] = ifftBuffer[_Cast<size_t>(i)] / peak;
        else
            for (int i = 0; i < kFFTSize; ++i)
                outFrame.samples[_Cast<size_t>(i)] = 0.0f;

        // Guard sample for branchless wrap interpolation
        outFrame.samples[_Cast<size_t>(kFFTSize)] = outFrame.samples[0];
    }

    void WavetableBuilder::normalizePeak(std::vector<f32>& buf) {
        float peak = 0.f;
        for (const f32 s : buf)
            peak = std::max(peak, std::abs(s));
        if (peak > _SmallFloat)
            for (f32& s : buf)
                s /= peak;
    }
}  // namespace SPECTR