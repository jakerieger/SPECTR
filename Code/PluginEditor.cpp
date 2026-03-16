#include "PluginEditor.hpp"
#include "UI/LookAndFeel.hpp"

namespace SPECTR {
    static UI::SPECTRLookAndFeel gLookAndFeel;

    SPECTREditor::SPECTREditor(SPECTRProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
        setLookAndFeel(&gLookAndFeel);
        setSize(900, 620);
        setResizable(false, false);

        // --- Wavetable display & load button ---
        addAndMakeVisible(mWavetableDisplay);
        mWavetableDisplay.setWavetableAndFrame(&p.getWavetableData(), 0.0f);

        mLoadButton.setButtonText("LOAD WAVETABLE");
        mLoadButton.onClick = [this] { openFilePicker(); };
        addAndMakeVisible(mLoadButton);

        mModeButton.setButtonText("MORPH");
        mModeButton.setClickingTogglesState(true);
        mModeButton.onClick = [this] {
            const bool isSlice = mModeButton.getToggleState();
            mBuildMode         = isSlice ? BuildMode::Slice : BuildMode::FFTMorph;
            mModeButton.setButtonText(isSlice ? "SLICE" : "MORPH");
        };
        addAndMakeVisible(mModeButton);

        // --- Knobs ---
        addAndMakeVisible(mFramePosKnob);
        addAndMakeVisible(mAttackKnob);
        addAndMakeVisible(mDecayKnob);
        addAndMakeVisible(mSustainKnob);
        addAndMakeVisible(mReleaseKnob);
        addAndMakeVisible(mGainKnob);

        // --- ADSR display ---
        addAndMakeVisible(mAdsrDisplay);

        // --- APVTS attachments ---
        auto& apvts  = p.apvts;
        mAttFramePos = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts,
                                                                                              "framePos",
                                                                                              mFramePosKnob.slider);
        mAttAttack =
          std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "attack", mAttackKnob.slider);
        mAttDecay =
          std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "decay", mDecayKnob.slider);
        mAttSustain =
          std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sustain", mSustainKnob.slider);
        mAttRelease =
          std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "release", mReleaseKnob.slider);
        mAttGain =
          std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "masterGain", mGainKnob.slider);

        // --- Listen to knobs for display updates ---
        mFramePosKnob.slider.addListener(this);
        mAttackKnob.slider.addListener(this);
        mDecayKnob.slider.addListener(this);
        mSustainKnob.slider.addListener(this);
        mReleaseKnob.slider.addListener(this);

        // Trigger initial ADSR display
        sliderValueChanged(&mAttackKnob.slider);
    }

    SPECTREditor::~SPECTREditor() {
        mFramePosKnob.slider.removeListener(this);
        mAttackKnob.slider.removeListener(this);
        mDecayKnob.slider.removeListener(this);
        mSustainKnob.slider.removeListener(this);
        mReleaseKnob.slider.removeListener(this);
        setLookAndFeel(nullptr);
    }

    void SPECTREditor::paint(juce::Graphics& g) {
        // Background
        g.fillAll(UI::Colors::Background);

        // Section headers
        auto drawHeader = [&](juce::Rectangle<int> r, const juce::String& text) {
            g.setColour(UI::Colors::Panel);
            g.drawHorizontalLine(r.getY() - 1, _Cast<f32>(r.getX()), _Cast<f32>(r.getRight()));
            g.setColour(UI::Colors::TextPrimary);
            g.setFont(juce::Font(10.0f).withStyle(juce::Font::bold));
            g.drawText(text, r.getX(), r.getY() - 18, 120, 14, juce::Justification::centredLeft);
        };

        // Title
        g.setColour(UI::Colors::TextPrimary);
        g.setFont(juce::Font(20.0f).withStyle(juce::Font::bold));
        g.drawText("SPECTR", 20, 16, 200, 28, juce::Justification::centredLeft);
        g.setColour(UI::Colors::TextMuted);
        g.setFont(11.0f);
        g.drawText("Wavetable Synthesizer", 88, 22, 200, 18, juce::Justification::centredLeft);

        // Dividers
        const int pad = 20;
        drawHeader({pad, 60, getWidth() - pad * 2, 10}, "OSCILLATOR");
        drawHeader({pad, 310, getWidth() - pad * 2, 10}, "ENVELOPE");
    }

    void SPECTREditor::resized() {
        const int pad = 20;
        const int w   = getWidth();

        // --- OSC section ---
        const int oscY = 65;
        const int oscH = 230;

        const int knobW = 70;
        const int kx    = w - pad - knobW;

        // Wavetable display fills the OSC area (leave room for buttons at bottom)
        mWavetableDisplay.setBounds(pad, oscY, w - pad * 2 - knobW - 10, oscH - 30);

        // Load + Mode buttons side by side below the display
        mLoadButton.setBounds(pad, oscY + oscH - 26, 140, 24);
        mModeButton.setBounds(pad + 146, oscY + oscH - 26, 72, 24);

        // Frame pos knob to the right
        mFramePosKnob.setBounds(kx, oscY + oscH / 2 - 45, knobW, 70);

        // --- ENVELOPE section ---
        const int envY = 315;
        const int envH = 230;

        // ADSR display fills left side
        mAdsrDisplay.setBounds(pad, envY, w - pad * 2 - knobW - 10, envH - 30);

        // Four ADSR knobs stacked on the right
        const int kh = 58;
        mAttackKnob.setBounds(kx, envY, knobW, kh);
        mDecayKnob.setBounds(kx, envY + kh, knobW, kh);
        mSustainKnob.setBounds(kx, envY + kh * 2, knobW, kh);
        mReleaseKnob.setBounds(kx, envY + kh * 3, knobW, kh);

        // Master gain knob sits below the release knob
        mGainKnob.setBounds(kx, envY + kh * 4, knobW, kh - 4);
    }

    void SPECTREditor::sliderValueChanged(juce::Slider* slider) {
        if (slider == &mFramePosKnob.slider) {
            mWavetableDisplay.setWavetableAndFrame(&audioProcessor.getWavetableData(), _Cast<f32>(slider->getValue()));
            return;
        }

        // Any ADSR knob changed — update the display
        juce::ADSR::Parameters p;
        p.attack  = _Cast<f32>(mAttackKnob.slider.getValue());
        p.decay   = _Cast<f32>(mDecayKnob.slider.getValue());
        p.sustain = _Cast<f32>(mSustainKnob.slider.getValue());
        p.release = _Cast<f32>(mReleaseKnob.slider.getValue());
        mAdsrDisplay.setParameters(p);
    }

    void SPECTREditor::openFilePicker() {
        mFileChooser =
          std::make_unique<juce::FileChooser>("Load Wavetable Audio File",
                                              juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                              "*.wav;*.aif;*.aiff;*.flac",
                                              true);

        mFileChooser->launchAsync(
          juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
          [this](const juce::FileChooser& fc) {
              const auto results = fc.getResults();
              if (results.isEmpty()) return;
              const juce::File chosen = results[0];
              const bool ok           = audioProcessor.loadWavetableFromFile(chosen, mBuildMode);

              if (ok) {
                  // Refresh display after successful load
                  mWavetableDisplay.setWavetableAndFrame(&audioProcessor.getWavetableData(),
                                                         _Cast<f32>(mFramePosKnob.slider.getValue()));
              } else {
                  juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                         "Load Failed",
                                                         "Could not load '" + chosen.getFileName() +
                                                           "'.\n"
                                                           "Please use a WAV, AIFF, or FLAC file.");
              }
          });
    }
}  // namespace SPECTR