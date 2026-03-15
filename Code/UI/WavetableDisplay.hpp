//
// Created by jr on 3/13/2026.
//

#pragma once

#include "Synth/WavetableData.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SPECTR::UI {
    // Draws the waveform of the currently selected wavetable frame.
    // Repainted whenever the frame position knob changes.
    class WavetableDisplay : public juce::Component {
    public:
        WavetableDisplay() {
            setOpaque(false);
        }

        // Called from the editor whenever the wavetable or position changes.
        void setWavetableAndFrame(const Synth::WavetableData* wt, f32 normalizedPosition) {
            mWavetable = wt;
            mPosition  = normalizedPosition;
            repaint();
        }

        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);

            // Background
            g.setColour(juce::Colour(0xFF0D1117));
            g.fillRoundedRectangle(bounds, 6.0f);

            // Border
            g.setColour(juce::Colour(0xFF30363D));
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

            if (mWavetable == nullptr || !mWavetable->isLoaded) {
                g.setColour(juce::Colour(0xFF484F58));
                g.setFont(13.0f);
                g.drawText("No wavetable loaded", bounds, juce::Justification::centred);
                return;
            }

            // Draw the waveform of the currently selected frame
            const int mip      = Synth::WavetableData::getMipLevel(60);  // display at mid-range
            const f32 framePos = mPosition * _Cast<f32>(Synth::Wavetable::kNumFrames - 1);
            const int f0       = _Cast<int>(framePos) % Synth::Wavetable::kNumFrames;
            const int f1       = (f0 + 1) % Synth::Wavetable::kNumFrames;
            const f32 frac     = framePos - std::floor(framePos);

            const f32 cx    = bounds.getCentreX();
            const f32 cy    = bounds.getCentreY();
            const f32 halfH = bounds.getHeight() * 0.42f;

            juce::Path wave;
            const int drawPoints = _Cast<int>(bounds.getWidth());

            for (int px = 0; px < drawPoints; ++px) {
                const f32 phase = _Cast<f32>(px) / _Cast<f32>(drawPoints);
                const f32 s0    = mWavetable->frames[mip][f0].getSample(phase);
                const f32 s1    = mWavetable->frames[mip][f1].getSample(phase);
                const f32 s     = s0 + frac * (s1 - s0);

                const f32 x = bounds.getX() + _Cast<f32>(px);
                const f32 y = cy - s * halfH;

                if (px == 0) wave.startNewSubPath(x, y);
                else wave.lineTo(x, y);
            }

            // Gradient fill under the wave
            juce::Path fill = wave;
            fill.lineTo(bounds.getRight(), cy);
            fill.lineTo(bounds.getX(), cy);
            fill.closeSubPath();

            const juce::ColourGradient grad(juce::Colour(0x4058A6FF),
                                            bounds.getX(),
                                            bounds.getY(),
                                            juce::Colour(0x0058A6FF),
                                            bounds.getX(),
                                            bounds.getBottom(),
                                            false);
            g.setGradientFill(grad);
            g.fillPath(fill);

            // Waveform line
            g.setColour(juce::Colour(0xFF58A6ff));
            g.strokePath(wave, juce::PathStrokeType(1.5f));

            // Frame position indicator label
            g.setColour(juce::Colour(0xFF8B949E));
            g.setFont(11.0f);
            const int frameIdx = juce::roundToInt(mPosition * (Synth::Wavetable::kNumFrames - 1));
            g.drawText("Frame " + juce::String(frameIdx),
                       bounds.reduced(6, 4).withHeight(14),
                       juce::Justification::topRight);
        }

    private:
        const Synth::WavetableData* mWavetable {nullptr};
        f32 mPosition {0.0f};
    };
}  // namespace SPECTR::UI
