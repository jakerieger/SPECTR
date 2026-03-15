#include "PluginEditor.hpp"

namespace SPECTR {
    // Dark GitHub-inspired theme
    class SPECTRLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        SPECTRLookAndFeel() {
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff58a6ff));
            setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff30363d));
            setColour(juce::Slider::thumbColourId, juce::Colour(0xff58a6ff));
            setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21262d));
            setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff388bfd));
            setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc9d1d9));
        }

        void drawRotarySlider(juce::Graphics& g,
                              int x,
                              int y,
                              int width,
                              int height,
                              float sliderPos,
                              float rotaryStartAngle,
                              float rotaryEndAngle,
                              juce::Slider&) override {
            const float radius  = static_cast<float>(juce::jmin(width, height)) * 0.5f - 4.0f;
            const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
            const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
            const float angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            // Track background
            juce::Path track;
            track.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour(juce::Colour(0xff30363d));
            g.strokePath(track,
                         juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Filled arc
            juce::Path arc;
            arc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            g.setColour(juce::Colour(0xff58a6ff));
            g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Thumb dot
            const float thumbX = centreX + radius * std::cos(angle - juce::MathConstants<float>::halfPi);
            const float thumbY = centreY + radius * std::sin(angle - juce::MathConstants<float>::halfPi);
            g.setColour(juce::Colours::white);
            g.fillEllipse(thumbX - 3.0f, thumbY - 3.0f, 6.0f, 6.0f);

            // Centre dot
            g.setColour(juce::Colour(0xff161b22));
            g.fillEllipse(centreX - radius * 0.25f, centreY - radius * 0.25f, radius * 0.5f, radius * 0.5f);
        }

        void
        drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool isDown) override {
            auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
            g.setColour(isDown ? juce::Colour(0xff388bfd) : juce::Colour(0xff21262d));
            g.fillRoundedRectangle(bounds, 4.0f);
            g.setColour(juce::Colour(0xff30363d));
            g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        }
    };

    static SPECTRLookAndFeel gLookAndFeel;

    SPECTREditor::SPECTREditor(SPECTRProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
        setLookAndFeel(&gLookAndFeel);
        setSize(900, 580);
        setResizable(false, false);

        // --- Wavetable display & load button ---
        addAndMakeVisible(mWavetableDisplay);
        mWavetableDisplay.setWavetableAndFrame(&p.getWavetableData(), 0.0f);

        mLoadButton.setButtonText("LOAD WAVETABLE");
        mLoadButton.onClick = [this] { openFilePicker(); };
        addAndMakeVisible(mLoadButton);

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
        g.fillAll(juce::Colour(0xff0d1117));

        // Section headers
        auto drawHeader = [&](juce::Rectangle<int> r, const juce::String& text) {
            g.setColour(juce::Colour(0xff30363d));
            g.drawHorizontalLine(r.getY() - 1, static_cast<float>(r.getX()), static_cast<float>(r.getRight()));
            g.setColour(juce::Colour(0xff8b949e));
            g.setFont(juce::Font(10.0f).withStyle(juce::Font::bold));
            g.drawText(text, r.getX(), r.getY() - 18, 120, 14, juce::Justification::centredLeft);
        };

        // Title
        g.setColour(juce::Colour(0xffc9d1d9));
        g.setFont(juce::Font(20.0f).withStyle(juce::Font::bold));
        g.drawText("SPECTR", 20, 16, 200, 28, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xff8b949e));
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

        // Wavetable display: fills most of the OSC area
        const int knobW = 70;
        mWavetableDisplay.setBounds(pad, oscY, w - pad * 2 - knobW - 10, oscH - 30);

        // Load button below display
        mLoadButton.setBounds(pad, oscY + oscH - 26, 140, 24);

        // Frame pos knob to the right of display
        const int kx = w - pad - knobW;
        mFramePosKnob.setBounds(kx, oscY + oscH / 2 - 45, knobW, 70);
        // --- ENVELOPE section ---
        const int envY = 315;
        const int envH = 230;

        // ADSR display
        mAdsrDisplay.setBounds(pad, envY, w - pad * 2 - knobW - 10, envH - 30);

        // ADSR + Gain knobs: arranged vertically to the right
        const int ky = envY;
        const int kh = 68;
        mAttackKnob.setBounds(kx, ky, knobW, kh);
        mDecayKnob.setBounds(kx, ky + kh, knobW, kh);
        mSustainKnob.setBounds(kx, ky + kh * 2, knobW, kh);
        mReleaseKnob.setBounds(kx, ky + kh * 3, knobW, kh - 4);
    }

    void SPECTREditor::sliderValueChanged(juce::Slider* slider) {
        if (slider == &mFramePosKnob.slider) {
            mWavetableDisplay.setWavetableAndFrame(&audioProcessor.getWavetableData(),
                                                   static_cast<float>(slider->getValue()));
            return;
        }

        // Any ADSR knob changed — update the display
        juce::ADSR::Parameters p;
        p.attack  = static_cast<float>(mAttackKnob.slider.getValue());
        p.decay   = static_cast<float>(mDecayKnob.slider.getValue());
        p.sustain = static_cast<float>(mSustainKnob.slider.getValue());
        p.release = static_cast<float>(mReleaseKnob.slider.getValue());
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
              const bool ok           = audioProcessor.loadWavetableFromFile(chosen);

              if (ok) {
                  // Refresh display after successful load
                  mWavetableDisplay.setWavetableAndFrame(&audioProcessor.getWavetableData(),
                                                         static_cast<float>(mFramePosKnob.slider.getValue()));
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