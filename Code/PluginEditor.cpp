#include "PluginProcessor.hpp"
#include "PluginEditor.hpp"

MyPluginEditor::MyPluginEditor(MyPluginProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
    updatePresetName();
    setupComponents();
    createAttachments();

    // Set the plugin window size
    setSize(320, 220);
}

MyPluginEditor::~MyPluginEditor() = default;

void MyPluginEditor::paint(juce::Graphics& g) {
    // Draw your initial UI here. This typically just fills the background with a color or sets it to an image.
    // Most of the UI layout is done in `resized()`.

    g.fillAll(juce::Colour(0xFF121212));
}

void MyPluginEditor::resized() {
    // Do your UI layout here (positions, sizes, etc.)
    auto bounds = getLocalBounds();

    gainLabel_.setBounds(0, 20, bounds.getWidth(), 20);
    gainSlider_.setBounds((bounds.getWidth() / 2) - 60, (bounds.getHeight() / 2) - 60, 120, 120);
}

void MyPluginEditor::updatePresetName() {
    const juce::String currentPresetName = audioProcessor.getCurrentPresetName();
    currentPresetName_                   = currentPresetName;
}

void MyPluginEditor::setupComponents() {
    // Initialize and add your UI components here (knobs, buttons, labels, etc.)

    gainLabel_.setText("Gain", juce::dontSendNotification);
    gainLabel_.setFont(juce::Font(16.0f));
    gainLabel_.setJustificationType(juce::Justification::centred);
    gainLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(gainLabel_);

    gainSlider_.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    gainSlider_.setTextValueSuffix(" dB");
    addAndMakeVisible(gainSlider_);
}

void MyPluginEditor::createAttachments() {
    // Create your parameter attachments here
    gainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters_, "gain", gainSlider_);
}