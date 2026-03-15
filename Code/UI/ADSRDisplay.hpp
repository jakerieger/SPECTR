//
// Created by jr on 3/13/2026.
//

#pragma once

#include "PluginCommon.hpp"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace SPECTR::UI {
    // Draws an ADSR envelope curve based on the current parameter values.
    // Updates whenever any ADSR knob changes.
    class ADSRDisplay : public juce::Component {
    public:
        ADSRDisplay() {
            setOpaque(false);
        }

        void setParameters(const juce::ADSR::Parameters& p) {
            mParams = p;
            repaint();
        }

        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat().reduced(2.0f);

            g.setColour(juce::Colour(0xff0d1117));
            g.fillRoundedRectangle(bounds, 6.0f);

            g.setColour(juce::Colour(0xff30363d));
            g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

            const f32 padX = 10.0f, padY = 10.0f;
            const f32 w  = bounds.getWidth() - padX * 2.0f;
            const f32 h  = bounds.getHeight() - padY * 2.0f;
            const f32 bx = bounds.getX() + padX;
            const f32 by = bounds.getY() + padY;

            // --- Compute proportional segment widths ---
            // Use log scale on time segments so short values are visible
            auto logTime = [](f32 t) { return std::log1p(t * 20.0f); };

            const f32 tA   = logTime(mParams.attack);
            const f32 tD   = logTime(mParams.decay);
            const f32 tR   = logTime(mParams.release);
            const f32 tSus = logTime(0.2f);  // fixed visual width for sustain plateau
            const f32 tTot = tA + tD + tSus + tR;

            const f32 xA = bx + (tA / tTot) * w;
            const f32 xD = xA + (tD / tTot) * w;
            const f32 xS = xD + (tSus / tTot) * w;
            const f32 xE = bx + w;

            const f32 yBot = by + h;
            const f32 yTop = by;
            const f32 ySus = by + h * (1.0f - mParams.sustain);

            // --- Build path ---
            juce::Path env;
            env.startNewSubPath(bx, yBot);
            env.lineTo(xA, yTop);  // Attack
            env.lineTo(xD, ySus);  // Decay
            env.lineTo(xS, ySus);  // Sustain plateau
            env.lineTo(xE, yBot);  // Release

            // Fill
            juce::Path fill = env;
            fill.lineTo(xE, yBot);
            fill.lineTo(bx, yBot);
            fill.closeSubPath();

            juce::ColourGradient
              fillGrad(juce::Colour(0x4034d058), bx, yTop, juce::Colour(0x0034d058), bx, yBot, false);
            g.setGradientFill(fillGrad);
            g.fillPath(fill);

            // Stroke
            g.setColour(juce::Colour(0xff3fb950));
            g.strokePath(env, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Segment labels
            g.setFont(10.0f);
            g.setColour(juce::Colour(0xff8b949e));
            auto label = [&](f32 x, const juce::String& text) {
                g.drawText(text, _Cast<int>(x) - 12, _Cast<int>(yBot) + 1, 24, 12, juce::Justification::centred);
            };
            label(bx + (xA - bx) * 0.5f, "A");
            label(xA + (xD - xA) * 0.5f, "D");
            label(xD + (xS - xD) * 0.5f, "S");
            label(xS + (xE - xS) * 0.5f, "R");
        }

        void resized() override {}

    private:
        juce::ADSR::Parameters mParams {0.01f, 0.1f, 0.7f, 0.3f};
    };
}  // namespace SPECTR::UI
