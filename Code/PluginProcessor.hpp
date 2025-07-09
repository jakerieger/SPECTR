#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class MyPluginProcessor : public juce::AudioProcessor {
public:
    MyPluginProcessor();
    ~MyPluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::String getCurrentPresetName() const {
        return currentPresetName_;
    }

    void setCurrentPresetName(const juce::String& name);

    juce::AudioProcessorValueTreeState parameters_;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    double sampleRate_ {44100};
    juce::String currentPresetName_ {"Init"};

    // Smoothed params to avoid crackling when adjusting param values
    // The param tree is what the UI connects to, this is just for the processor
    juce::SmoothedValue<float> gainSmoothed_ {0.0f};
    float gainPrevious_ {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyPluginProcessor)
};