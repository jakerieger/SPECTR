#include "PluginEditor.hpp"
#include "BinaryData.h"

namespace SPECTR {
    class SPECTRLookAndFeel : public juce::LookAndFeel_V4 {
    public:
        // Title bar background
        void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                        juce::Graphics& g,
                                        int w,
                                        int h,
                                        int titleSpaceX,
                                        int titleSpaceW,
                                        const juce::Image* icon,
                                        bool drawTitleTextOnLeft) override {
            g.fillAll(juce::Colour(0xff1a1a2e));  // your
            g.setColour(juce::Colours::white);
        }

        void drawButtonBackground(juce::Graphics& g,
                                  juce::Button& button,
                                  const juce::Colour& backgroundColour,
                                  const bool isMouseOverButton,
                                  const bool isButtonDown) override {
            if (_DCast<juce::DocumentWindow*>(button.getParentComponent())) {
                if (isButtonDown) {
                    g.fillAll(juce::Colours::transparentBlack);
                } else if (isMouseOverButton) {
                    g.fillAll(juce::Colours::transparentBlack);
                }

                return;
            }

            // Fall through to default for everything else
            LookAndFeel_V4::drawButtonBackground(g, button, backgroundColour, isMouseOverButton, isButtonDown);
        }
    };

    static SPECTRLookAndFeel gLookAndFeel {};

    static juce::String mimeForExtension(const juce::String& ext) {
        if (ext == "html") return "text/html";
        if (ext == "css") return "text/css";
        if (ext == "js") return "application/javascript";
        if (ext == "png") return "image/png";
        if (ext == "svg") return "image/svg+xml";
        if (ext == "woff2") return "font/woff2";
        return "application/octet-stream";
    }

    SPECTREditor::SPECTREditor(SPECTRProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
        juce::LookAndFeel::setDefaultLookAndFeel(&gLookAndFeel);

        setSize(1200, 800);
        setResizable(false, false);

        addAndMakeVisible(webView);
        webView.goToURL(webView.getResourceProviderRoot());

        for (auto* param : audioProcessor.getParameters()) {
            param->addListener(this);
        }
    }

    SPECTREditor::~SPECTREditor() {
        for (auto* param : audioProcessor.getParameters()) {
            param->removeListener(this);
        }
    }

    void SPECTREditor::resized() {
        webView.setBounds(getLocalBounds());
    }

    std::optional<juce::WebBrowserComponent::Resource> SPECTREditor::getResource(const juce::String& url) {
        const auto resourceName = (url == "/" ? "index.html" : url.trimCharactersAtStart("/"));

        int dataSize = 0;
        auto* data =
          BinaryData::getNamedResource(resourceName.replaceCharacter('.', '_').replaceCharacter('-', '_').toRawUTF8(),
                                       dataSize);

        if (data == nullptr) return std::nullopt;

        return juce::WebBrowserComponent::Resource {
          std::vector<std::byte>(_RCast<const std::byte*>(data), _RCast<const std::byte*>(data) + dataSize),
          mimeForExtension(resourceName.fromLastOccurrenceOf(".", false, false))};
    }

    void SPECTREditor::parameterValueChanged(
      const int parameterIndex,
      const float newValue) {  // parameterValueChanged can fire on the audio thread — must marshal to message thread
        const auto* param  = audioProcessor.getParameters()[parameterIndex];
        const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*>(param);
        if (ranged == nullptr) return;

        const auto paramID    = ranged->getParameterID();
        const auto normalized = ranged->getNormalisableRange().convertTo0to1(newValue);

        juce::MessageManager::callAsync([this, paramID, normalized] { updateUIParam(paramID, normalized); });
    }

    void SPECTREditor::parameterGestureChanged(int parameterIndex, bool gestureIsStarting) {}

    void SPECTREditor::updateUIParam(const juce::String& paramID, const f32 normalizedValue) {
        // C++ → JS: fires the event that JS listens to via window.__JUCE__.backend.addEventListener
        webView.emitEventIfBrowserIsVisible("paramChanged", [&] {
            auto obj = std::make_unique<juce::DynamicObject>();
            obj->setProperty("paramId", paramID);
            obj->setProperty("value", normalizedValue);
            return juce::var(obj.release());
        }());
    }

    void SPECTREditor::ipc_OpenFilePicker(const juce::String& oscId) {
        mFileChooser =
          std::make_unique<juce::FileChooser>("Load Wavetable Audio File",
                                              juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                              "*.wav;*.aif;*.aiff;*.flac",
                                              true);

        mFileChooser->launchAsync(
          juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
          [this, oscId](const juce::FileChooser& fc) {
              const auto results = fc.getResults();
              if (results.isEmpty()) return;
              const juce::File chosen = results[0];
              const bool ok           = audioProcessor.loadWavetableFromFile(chosen, mBuildMode);

              if (ok) {
                  // Refresh display after successful load
                  ipc_SendWavetableToUI(chosen.getFileNameWithoutExtension().toUpperCase(), oscId);
              } else {
                  juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                         "Load Failed",
                                                         "Could not load '" + chosen.getFileName() +
                                                           "'.\n"
                                                           "Please use a WAV, AIFF, or FLAC file.");
              }
          });
    }

    void SPECTREditor::ipc_SendWavetableToUI(const juce::String& wtName, const juce::String& oscId) {
        auto frames = juce::Array<juce::var>();

        for (int f = 0; f < Wavetable::kNumFrames; f++) {
            auto samples = juce::Array<juce::var>();
            for (int i = 0; i < Wavetable::kFrameSize; i++) {
                const f32 phase = _Cast<f32>(i) / Wavetable::kFrameSize;
                samples.add(audioProcessor.getWavetableData().frames[0][f].getSample(phase));
            }
            frames.add(juce::var(samples));
        }

        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty("oscId", oscId);
        obj->setProperty("frames", juce::var(frames));
        obj->setProperty("wtName", wtName);

        webView.emitEventIfBrowserIsVisible("wavetableData", juce::var(obj.release()));
    }

    void SPECTREditor::ipc_ActivateLicense(const juce::String& licenseJson) {
        const auto info = LicenseVerifier::verifyString(licenseJson);

        if (info.isValid) {
            LicenseVerifier::saveLicense(licenseJson);
            audioProcessor.licenseInfo = info;
        }

        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty("valid", info.isValid);
        obj->setProperty("name", info.name);
        obj->setProperty("email", info.email);
        obj->setProperty("product", info.product);

        webView.emitEventIfBrowserIsVisible("activeLicense", juce::var(obj.release()));
    }

    void SPECTREditor::ipc_GetLicenseInfo() {
        const auto& info = audioProcessor.licenseInfo;

        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty("valid", info.isValid);
        obj->setProperty("name", info.name);
        obj->setProperty("email", info.email);

        webView.emitEventIfBrowserIsVisible("getLicenseInfo", juce::var(obj.release()));
    }

    void SPECTREditor::ipc_ClearLicense() const {
        LicenseVerifier::clearSavedLicense();
        audioProcessor.licenseInfo = {};
    }
}  // namespace SPECTR