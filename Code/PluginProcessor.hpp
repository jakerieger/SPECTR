#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "WavetableData.hpp"
#include "WavetableBuilder.hpp"
#include "Voice.hpp"
#include "AnalogOscillator.hpp"
#include "LicenseVerifier.hpp"

namespace SPECTR {
    static constexpr int kNumVoices = 8;

    class SPECTRProcessor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener {
    public:
        SPECTRProcessor();
        ~SPECTRProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {};
        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
        bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

        juce::AudioProcessorEditor* createEditor() override;

        bool hasEditor() const override {
            return true;
        };

        const juce::String getName() const override;

        bool acceptsMidi() const override {
            return true;
        };

        bool producesMidi() const override {
            return false;
        };

        bool isMidiEffect() const override {
            return false;
        };

        double getTailLengthSeconds() const override {
            return 0.0;
        };

        int getNumPrograms() override {
            return 1;
        };

        int getCurrentProgram() override {
            return 0;
        };
        void setCurrentProgram(int index) override {};

        const juce::String getProgramName(int index) override {
            return {};
        };

        void changeProgramName(int index, const juce::String& newName) override {};

        void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;

        void parameterChanged(const juce::String& paramID, float newValue) override;

        bool loadWavetableFromFile(const juce::File& file, BuildMode mode = BuildMode::FFTMorph);

        juce::AudioProcessorValueTreeState apvts;

        const WavetableData& getWavetableData() const {
            return mWavetableData;
        }

        LicenseInfo licenseInfo;

    private:
        static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

        void updateADSR() const;
        void pushWavetableToVoices() const;

        juce::Synthesiser mSynth;
        WavetableData mWavetableData;
        WavetableBuilder mBuilder;
        juce::AudioFormatManager mFormatManager;

        std::atomic<f32> mFramePosition {0.f};
        juce::ADSR::Parameters mAdsrParams;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SPECTRProcessor)
    };
}  // namespace SPECTR