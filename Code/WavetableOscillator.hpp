//
// Created by jr on 3/13/2026.
//

#pragma once

#include "WavetableData.hpp"

namespace SPECTR {
    // WavetableOscillator
    //
    // A single oscillator that reads from a WavetableData at a given pitch and
    // frame position.  Designed to be embedded inside a SynthVoice; one instance
    // per voice.
    //
    // Thread safety: all members live on the audio thread only.
    class WavetableOscillator {
    public:
        WavetableOscillator() = default;

        // Call from prepareToPlay / voice::startNote
        void prepare(const f64 sampleRate) {
            mSampleRate = sampleRate;
        }

        // Start playing a MIDI note.  framePosition is in [0, kNumFrames).
        void noteOn(const int midiNote, const f32 velocity, const f32 framePosition) {
            mMidiNote      = midiNote;
            mVelocity      = velocity;
            mPhase         = 0.0f;
            mFramePosition = framePosition;
            updatePhaseIncrement();
        }

        // Update wavetable position (from the scrub knob, 0..1 normalized).
        // Stored as-is; WavetableData::getSample interprets it relative to actualNumFrames.
        void setFramePosition(const f32 normalizedPos) {
            mFramePosition = normalizedPos;
        }

        // Render `numSamples` into `output`, adding to existing content.
        // wavetable may be nullptr (silence).
        void process(const WavetableData* wavetable, f32* output, int numSamples) {
            if (wavetable == nullptr || !wavetable->isLoaded) return;

            const f32 phaseInc = mPhaseIncrement;
            f32 phase          = mPhase;
            const f32 framePos = mFramePosition;
            const int note     = mMidiNote;

            for (int i = 0; i < numSamples; ++i) {
                output[i] += wavetable->getSample(framePos, phase, note) * mVelocity;
                phase += phaseInc;
                if (phase >= 1.0f) phase -= 1.0f;
            }

            mPhase = phase;
        }

        _Nodisc int getMidiNote() const noexcept {
            return mMidiNote;
        }

        _Nodisc f32 getPhase() const noexcept {
            return mPhase;
        }

    private:
        void updatePhaseIncrement() {
            // Frequency from MIDI note (equal temperament, A4 = 440 Hz)
            const f64 freq  = 440.0 * std::pow(2.0, (mMidiNote - 69) / 12.0);
            mPhaseIncrement = _Cast<f32>(freq / mSampleRate);
        }

        f64 mSampleRate {44100.0};
        f32 mPhase {0.0f};
        f32 mPhaseIncrement {0.0f};
        f32 mFramePosition {0.0f};
        f32 mVelocity {1.0f};
        int mMidiNote {60};
    };
}  // namespace SPECTR
