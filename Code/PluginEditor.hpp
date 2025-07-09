#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.hpp"

class MyPluginEditor final : public juce::AudioProcessorEditor {
public:
    explicit MyPluginEditor(MyPluginProcessor&);
    ~MyPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void updatePresetName();

private:
    void setupComponents();
    void createAttachments();

    MyPluginProcessor& audioProcessor;

    juce::String currentPresetName_;

    // UI components
    juce::Label gainLabel_;
    juce::Slider gainSlider_;

    // Param attachments
    // Link our gain slider value to the parameter in our PluginProcessor
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
    gainAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyPluginEditor)
};