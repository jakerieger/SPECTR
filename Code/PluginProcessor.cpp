#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

namespace SPECTR {
    namespace ParamID {
        static const juce::String FramePos   = "framePos";
        static const juce::String Attack     = "attack";
        static const juce::String Decay      = "decay";
        static const juce::String Sustain    = "sustain";
        static const juce::String Release    = "release";
        static const juce::String MasterGain = "masterGain";
    }  // namespace ParamID

    SPECTRProcessor::SPECTRProcessor()
        : AudioProcessor(BusesProperties()
                           .withInput("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "Parameters", createParameterLayout()) {
        // Register format manager (WAV, AIFF, etc.)
        mFormatManager.registerBasicFormats();

        // Add 8 voices and one sound
        for (int i = 0; i < kNumVoices; ++i) {
            mSynth.addVoice(new Synth::Voice());
        }

        mSynth.addSound(new Synth::WavetableSound());

        // Listen to all params
        for (auto* param : apvts.processor.getParameters()) {
            if (const auto* p = _DCast<juce::RangedAudioParameter*>(param))
                apvts.addParameterListener(p->getParameterID(), this);
        }

        // Init default ADSR
        mAdsrParams = {0.01f, 0.1f, 0.7f, 0.3f};
    }

    SPECTRProcessor::~SPECTRProcessor() {
        for (auto* param : apvts.processor.getParameters())
            if (const auto* p = _DCast<juce::RangedAudioParameter*>(param))
                apvts.removeParameterListener(p->getParameterID(), this);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout SPECTRProcessor::createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>(ParamID::FramePos,
                                                                     "Frame Position",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.0f));

        params.push_back(
          std::make_unique<juce::AudioParameterFloat>(ParamID::Attack,
                                                      "Attack",
                                                      juce::NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.3f),
                                                      0.01f));

        params.push_back(
          std::make_unique<juce::AudioParameterFloat>(ParamID::Decay,
                                                      "Decay",
                                                      juce::NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.3f),
                                                      0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(ParamID::Sustain,
                                                                     "Sustain",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.7f));

        params.push_back(
          std::make_unique<juce::AudioParameterFloat>(ParamID::Release,
                                                      "Release",
                                                      juce::NormalisableRange<float>(0.001f, 8.0f, 0.001f, 0.3f),
                                                      0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(ParamID::MasterGain,
                                                                     "Master Gain",
                                                                     juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                                                     0.75f));

        return {params.begin(), params.end()};
    }

    void SPECTRProcessor::prepareToPlay(const f64 sampleRate, const int samplesPerBlock) {
        mSynth.setCurrentPlaybackSampleRate(sampleRate);

        for (int i = 0; i < mSynth.getNumVoices(); ++i) {
            if (auto* v = _DCast<Synth::Voice*>(mSynth.getVoice(i))) { v->prepareToPlay(sampleRate, samplesPerBlock); }
        }

        pushWavetableToVoices();
        updateADSR();
    }

    void SPECTRProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        juce::ScopedNoDenormals noDenormals;
        buffer.clear();

        // Push current frame position to all voices
        const float fp = mFramePosition.load();
        for (int i = 0; i < mSynth.getNumVoices(); ++i)
            if (auto* v = _DCast<Synth::Voice*>(mSynth.getVoice(i))) v->setFramePosition(fp);

        mSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

        // Apply master gain
        const auto gain = apvts.getRawParameterValue(ParamID::MasterGain)->load();
        buffer.applyGain(gain);
    }

    void SPECTRProcessor::parameterChanged(const juce::String& paramID, float newValue) {
        if (paramID == ParamID::FramePos) {
            mFramePosition.store(newValue);
            return;
        }

        if (paramID == ParamID::Attack) {
            mAdsrParams.attack = newValue;
            updateADSR();
            return;
        }
        if (paramID == ParamID::Decay) {
            mAdsrParams.decay = newValue;
            updateADSR();
            return;
        }
        if (paramID == ParamID::Sustain) {
            mAdsrParams.sustain = newValue;
            updateADSR();
            return;
        }
        if (paramID == ParamID::Release) {
            mAdsrParams.release = newValue;
            updateADSR();
            return;
        }
    }

    void SPECTRProcessor::getStateInformation(juce::MemoryBlock& destData) {
        const auto state = apvts.copyState();
        const std::unique_ptr xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void SPECTRProcessor::setStateInformation(const void* data, const int sizeInBytes) {
        if (const std::unique_ptr xmlState(getXmlFromBinary(data, sizeInBytes));
            xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    bool SPECTRProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
            layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        return true;
    }

    juce::AudioProcessorEditor* SPECTRProcessor::createEditor() {
        return new SPECTREditor(*this);
    }

    const juce::String SPECTRProcessor::getName() const {
        return JucePlugin_Name;  // Macro defined by CMakeLists.txt (PRODUCT_NAME)
    }

    bool SPECTRProcessor::loadWavetableFromFile(const juce::File& file) {
        std::unique_ptr<juce::AudioFormatReader> reader(mFormatManager.createReaderFor(file));
        if (reader == nullptr) return false;

        // Read up to 4 seconds of audio at the native sample rate
        const int maxSamples = _Cast<int>(reader->sampleRate * 4.0);
        const int numSamples = _Cast<int>(std::min(_Cast<juce::int64>(maxSamples), reader->lengthInSamples));

        juce::AudioBuffer<f32> audio(_Cast<int>(reader->numChannels), numSamples);
        reader->read(&audio, 0, numSamples, 0, true, true);

        Synth::WavetableData newTable;
        const bool ok = mBuilder.build(audio, newTable, file.getFileNameWithoutExtension());

        if (ok) {
            // Swap in on the message thread; audio thread will pick it up next block.
            // This is safe because WavetableData is trivially copyable and the
            // assignment is word-atomic on all supported platforms.
            mWavetableData = std::move(newTable);
            pushWavetableToVoices();
        }

        return ok;
    }

    void SPECTRProcessor::updateADSR() const {
        for (int i = 0; i < mSynth.getNumVoices(); ++i)
            if (auto* v = _DCast<Synth::Voice*>(mSynth.getVoice(i))) v->setADSR(mAdsrParams);
    }

    void SPECTRProcessor::pushWavetableToVoices() const {
        for (int i = 0; i < mSynth.getNumVoices(); ++i)
            if (auto* v = dynamic_cast<Synth::Voice*>(mSynth.getVoice(i))) v->setWavetable(&mWavetableData);
    }
}  // namespace SPECTR

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SPECTR::SPECTRProcessor();
}