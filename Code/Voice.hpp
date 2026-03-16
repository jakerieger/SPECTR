//
// Created by jr on 3/13/2026.
//

#pragma once

#include "WavetableOscillator.hpp"
#include <juce_audio_processors/juce_audio_processors.h>

namespace SPECTR {
    // One of 8 polyphonic voices.  Owns a WavetableOscillator and a juce::ADSR.
    // The Synthesiser base class handles note stealing and voice allocation.
    //
    // The wavetable pointer and frame position are set from the processor
    // before each processBlock call.
    class Voice : public juce::SynthesiserVoice {
    public:
        // Called by the processor to inject shared state before rendering.
        void setWavetable(const WavetableData* wt) {
            mWavetable = wt;
        }

        void setFramePosition(const f32 pos01) {
            mOscillator.setFramePosition(pos01);
        }

        void setADSR(const juce::ADSR::Parameters& params) {
            mAdsr.setParameters(params);
        }

        // juce::SynthesiserVoice overrides
        bool canPlaySound(juce::SynthesiserSound* sound) override {
            return sound != nullptr;
        }

        void startNote(const int midiNoteNumber, const float velocity, juce::SynthesiserSound* sound, int) override {
            // noteOn must be called before mAdsr.noteOn() so the oscillator phase
            // and pitch increment are set before the first render call.
            mOscillator.noteOn(midiNoteNumber, velocity, mOscillator.getPhase());
            mAdsr.noteOn();
            mIsActive = true;
        }

        void stopNote(float, const bool allowTailOff) override {
            if (allowTailOff) {
                mAdsr.noteOff();
            } else {
                mAdsr.reset();
                clearCurrentNote();
                mIsActive = false;
            }
        }

        void
        renderNextBlock(juce::AudioBuffer<float>& outputBuffer, const int startSample, const int numSamples) override {
            if (!mIsActive) return;

            // Resize temp buffer if needed (avoids per-block allocation)
            if (_Cast<int>(mTempBuffer.size()) < numSamples) mTempBuffer.resize(_Cast<size_t>(numSamples));

            std::fill_n(mTempBuffer.begin(), numSamples, 0.0f);
            mOscillator.process(mWavetable, mTempBuffer.data(), numSamples);

            // Scale per voice to prevent clipping with polyphony.
            // 1/sqrt(N) is the standard equal-power sum normalization.
            // With kNumVoices = 8 this gives ~0.354, keeping the summed output near 0 dBFS.
            static constexpr f32 kPolyGain = 1.0f / _Sqrt8;
            // Apply ADSR sample-by-sample and accumulate into all output channels
            for (int i = 0; i < numSamples; ++i) {
                const f32 env = mAdsr.getNextSample();
                const f32 s   = mTempBuffer[_Cast<size_t>(i)] * env * kPolyGain;
                for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                    outputBuffer.addSample(ch, startSample + i, s);
            }

            // If the ADSR release tail has fully decayed, free the voice
            if (!mAdsr.isActive()) {
                clearCurrentNote();
                mIsActive = false;
            }
        }

        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}

        void prepareToPlay(const f64 sampleRate, int) {
            mAdsr.setSampleRate(sampleRate);
            mOscillator.prepare(sampleRate);
        }

    private:
        WavetableOscillator mOscillator;
        juce::ADSR mAdsr;
        const WavetableData* mWavetable {nullptr};
        std::vector<f32> mTempBuffer;
        bool mIsActive {false};
    };

    // A trivial SynthesiserSound required by juce::Synthesiser.
    // All voices accept it (see canPlaySound above).
    struct WavetableSound : public juce::SynthesiserSound {
        bool appliesToNote(int midiNoteNumber) override {
            return true;
        }
        bool appliesToChannel(int midiNoteNumber) override {
            return true;
        }
    };
}  // namespace SPECTR