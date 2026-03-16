#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.hpp"
#include "UI/WavetableDisplay.hpp"
#include "UI/ADSRDisplay.hpp"

namespace SPECTR {
    // A simple labelled knob (Slider + Label + value readout)
    class LabelledKnob : public juce::Component {
    public:
        juce::Slider slider;

        explicit LabelledKnob(const juce::String& labelText, const juce::String& suffix = "")
            : mLabel(labelText), mSuffix(suffix) {
            slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setPopupDisplayEnabled(true, true, nullptr);
            addAndMakeVisible(slider);
        }

        void paint(juce::Graphics& g) override {
            // Draw label below the knob
            g.setColour(juce::Colour(0xff8b949e));
            g.setFont(11.0f);
            const auto lb   = getLocalBounds();
            const int knobH = lb.getHeight() - 16;
            g.drawText(mLabel, 0, knobH, lb.getWidth(), 14, juce::Justification::centred);
        }
        void resized() override {
            const auto b    = getLocalBounds();
            const int knobH = b.getHeight() - 16;
            slider.setBounds(b.withHeight(knobH));
        }

    private:
        juce::String mLabel, mSuffix;
    };

    class SPECTREditor final : public juce::AudioProcessorEditor,
                               private juce::Slider::Listener {
    public:
        explicit SPECTREditor(SPECTRProcessor&);
        ~SPECTREditor() override;

        void paint(juce::Graphics&) override;
        void resized() override;

    private:
        void sliderValueChanged(juce::Slider* slider) override;
        void openFilePicker();

        SPECTRProcessor& audioProcessor;

        // Wavetable section
        UI::WavetableDisplay mWavetableDisplay;
        juce::TextButton mLoadButton {"LOAD"};
        juce::TextButton mModeButton {"MORPH"};  // toggles between FFTMorph and Slice
        LabelledKnob mFramePosKnob {"POSITION"};

        // ADSR section
        UI::ADSRDisplay mAdsrDisplay;
        LabelledKnob mAttackKnob {"ATTACK"};
        LabelledKnob mDecayKnob {"DECAY"};
        LabelledKnob mSustainKnob {"SUSTAIN"};
        LabelledKnob mReleaseKnob {"RELEASE"};

        // Master
        LabelledKnob mGainKnob {"GAIN"};
        // APVTS attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttFramePos;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttAttack;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttDecay;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttSustain;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttRelease;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttGain;

        std::unique_ptr<juce::FileChooser> mFileChooser;
        BuildMode mBuildMode {BuildMode::FFTMorph};  // current mode for next load

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SPECTREditor)
    };
}  // namespace SPECTR