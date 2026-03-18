#include "PluginEditor.hpp"
#include "BinaryData.h"

namespace SPECTR {
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

    void SPECTREditor::openFilePicker(const juce::String& oscId) {
        mFileChooser =
          std::make_unique<juce::FileChooser>("Load Wavetable Audio File",
                                              juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                              "*.wav;*.aif;*.aiff;*.flac",
                                              true);

        mFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this, oscId](const juce::FileChooser& fc) {
                                      const auto results = fc.getResults();
                                      if (results.isEmpty()) return;
                                      const juce::File chosen = results[0];
                                      const bool ok = audioProcessor.loadWavetableFromFile(chosen, mBuildMode);

                                      if (ok) {
                                          // Refresh display after successful load
                                          sendWavetableToUI(oscId);
                                      } else {
                                          juce::AlertWindow::showMessageBoxAsync(
                                            juce::AlertWindow::WarningIcon,
                                            "Load Failed",
                                            "Could not load '" + chosen.getFileName() +
                                              "'.\n"
                                              "Please use a WAV, AIFF, or FLAC file.");
                                      }
                                  });
    }

    void SPECTREditor::sendWavetableToUI(const juce::String& oscId) {
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
        webView.emitEventIfBrowserIsVisible("wavetableData", juce::var(obj.release()));
    }
}  // namespace SPECTR