#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

MyPluginProcessor::MyPluginProcessor()
    : AudioProcessor(BusesProperties()
                       .withInput("Input", juce::AudioChannelSet::stereo(), true)
                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "Parameters", createParameterLayout()) {}

MyPluginProcessor::~MyPluginProcessor() = default;

const juce::String MyPluginProcessor::getName() const {
    return JucePlugin_Name; // Macro defined by CMakeLists.txt (PRODUCT_NAME)
}

bool MyPluginProcessor::acceptsMidi() const {
    return false;
}

bool MyPluginProcessor::producesMidi() const {
    return false;
}

bool MyPluginProcessor::isMidiEffect() const {
    return false;
}

double MyPluginProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int MyPluginProcessor::getNumPrograms() {
    return 1;
}

int MyPluginProcessor::getCurrentProgram() {
    return 0;
}

void MyPluginProcessor::setCurrentProgram(int index) {}

const juce::String MyPluginProcessor::getProgramName(int index) {
    return {};
}

void MyPluginProcessor::changeProgramName(int index, const juce::String& newName) {}

void MyPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    sampleRate_ = sampleRate; // Grabs sample rate from host (DAW) and sets it as processor sample rate

    // Init smoothed values (10ms ramp time in this case)
    gainSmoothed_.reset(sampleRate, 0.01);
    gainSmoothed_.setCurrentAndTargetValue(parameters_.getRawParameterValue("gain")->load());
}

void MyPluginProcessor::releaseResources() {
    // Clean up resources here
}

bool MyPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Support stereo input and output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;

    return true;
}

void MyPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    // Denormal numbers, also known as subnormal numbers, are very small floating-point numbers that can cause
    // significant performance slowdowns on some CPUs. When a CPU encounters a denormal number during calculation,
    // it may need to handle it in software or through a slower hardware path, leading to increased processing time.
    //
    // This line temporarily disables them, improving CPU performance.
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const float gainCurrent = parameters_.getRawParameterValue("gain")->load();
    // update smoothed values
    if (gainCurrent != gainPrevious_) {
        gainSmoothed_.setTargetValue(gainCurrent);
        gainPrevious_ = gainCurrent;
    }

    // Do your audio processing here!
    for (auto sample = 0; sample < buffer.getNumSamples(); ++sample) {
        const float smoothedGain = gainSmoothed_.getNextValue();
        const float gainLinear = juce::Decibels::decibelsToGain(smoothedGain);

        for (auto channel = 0; channel < totalNumOutputChannels; ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            channelData[sample] *= gainLinear; // Apply gain
        }
    }
}

bool MyPluginProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* MyPluginProcessor::createEditor() {
    return new MyPluginEditor(*this);
}

void MyPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    // Save plugin state here
    const auto state = parameters_.copyState();
    const std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MyPluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    // Restore plugin state here
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        if (xmlState->hasTagName(parameters_.state.getType())) {
            parameters_.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

void MyPluginProcessor::setCurrentPresetName(const juce::String& name) {
    currentPresetName_ = name;
    if (auto* editor = getActiveEditor()) {
        if (auto* pluginEditor = dynamic_cast<MyPluginEditor*>(editor)) { pluginEditor->updatePresetName(); }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MyPluginProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Create parameters here
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gain",
                                                                 "Gain",
                                                                 juce::NormalisableRange<float>(-30.0f, 30.0f, 0.1f),
                                                                 0.0f,
                                                                 "dB"));

    return {params.begin(), params.end()};
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MyPluginProcessor();
}