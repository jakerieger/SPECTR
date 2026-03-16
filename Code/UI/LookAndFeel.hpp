//
// Created by jr on 3/15/2026.
//

#pragma once

#include "PluginCommon.hpp"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SPECTR::UI {
    namespace Colors {
        static const juce::Colour Background {0xFF0D0D0D};
        static const juce::Colour Panel {0xFF1A1A1A};
        static const juce::Colour PanelBorder {0xFF2A2A2A};
        static const juce::Colour Orange {0xFFFF6B35};
        static const juce::Colour Teal {0xFF4ECDC4};
        static const juce::Colour TextPrimary {0xFFE8E8E8};
        static const juce::Colour TextMuted {0xFF888888};
        static const juce::Colour KnobTrack {0xFF333333};
    }  // namespace Colors

    class SPECTRLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        SPECTRLookAndFeel() {
            setColour(juce::Slider::thumbColourId, Colors::Teal);
            setColour(juce::Slider::rotarySliderFillColourId, Colors::Teal);
            setColour(juce::Slider::rotarySliderOutlineColourId, Colors::KnobTrack);
            setColour(juce::Label::textColourId, Colors::TextPrimary);
            setColour(juce::ListBox::backgroundColourId, Colors::Panel);
            setColour(juce::ListBox::outlineColourId, Colors::PanelBorder);
            setColour(juce::TextButton::buttonColourId, Colors::Panel);
            setColour(juce::TextButton::buttonOnColourId, Colors::Teal);
            setColour(juce::TextButton::textColourOffId, Colors::TextPrimary);
            setColour(juce::TextButton::textColourOnId, Colors::Background);
        }

        void drawRotarySlider(juce::Graphics& g,
                              const int x,
                              const int y,
                              const int width,
                              const int height,
                              const f32 sliderPos,
                              const f32 rotaryStartAngle,
                              const f32 rotaryEndAngle,
                              juce::Slider& slider) override {
            const f32 radius  = _Cast<f32>(juce::jmin(width / 2, height / 2)) - 6.0f;
            const f32 centreX = _Cast<f32>(x) + _Cast<f32>(width) * 0.5f;
            const f32 centreY = _Cast<f32>(y) + _Cast<f32>(height) * 0.5f;
            const f32 rx      = centreX - radius;
            const f32 ry      = centreY - radius;
            const f32 rw      = radius * 2.0f;
            const f32 angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            // Track arc
            {
                juce::Path track;
                track.addArc(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, true);
                g.setColour(Colors::KnobTrack);
                g.strokePath(track,
                             juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Value arc
            {
                const juce::Colour fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
                juce::Path value;
                value.addArc(rx, ry, rw, rw, rotaryStartAngle, angle, true);
                g.setColour(fillColour);
                g.strokePath(value,
                             juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            // Knob body
            {
                const f32 bodyRadius = radius * 0.72f;
                g.setColour(Colors::Panel);
                g.fillEllipse(centreX - bodyRadius, centreY - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

                g.setColour(Colors::PanelBorder);
                g.drawEllipse(centreX - bodyRadius, centreY - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.5f);
            }

            // Pointer line
            {
                const juce::Colour fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
                juce::Path pointer;
                const f32 pointerLength    = radius * 0.45f;
                constexpr f32 pointerThick = 2.5f;
                pointer.addRectangle(-pointerThick * 0.5f, -(radius * 0.68f), pointerThick, pointerLength);
                pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
                g.setColour(fillColour);
                g.fillPath(pointer);
            }
        }

        void drawButtonBackground(juce::Graphics& g,
                                  juce::Button& button,
                                  const juce::Colour& /*backgroundColour*/,
                                  const bool shouldDrawButtonAsHighlighted,
                                  const bool shouldDrawButtonAsDown) override {
            const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
            const bool isOn   = button.getToggleState();

            juce::Colour bg = isOn ? Colors::Teal : (shouldDrawButtonAsDown ? Colors::PanelBorder : Colors::Panel);
            if (shouldDrawButtonAsHighlighted && !isOn) bg = bg.brighter(0.1f);

            g.setColour(bg);
            g.fillRoundedRectangle(bounds, 0.0f);

            g.setColour(isOn ? Colors::Teal.darker(0.3f) : Colors::PanelBorder);
            g.drawRoundedRectangle(bounds, 0.0f, 1.0f);
        }
    };
}  // namespace SPECTR::UI