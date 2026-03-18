#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include "PluginProcessor.hpp"

namespace SPECTR {
    class SPECTREditor final : public juce::AudioProcessorEditor,
                               private juce::AudioProcessorParameter::Listener {
    public:
        explicit SPECTREditor(SPECTRProcessor&);
        ~SPECTREditor() override;

        void resized() override;

    private:
        // Resource provider: serves BinaryData assets to the WebView
        static std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

        // Parameter listener: fires when DAW automation moves a parameter
        void parameterValueChanged(int parameterIndex, float newValue) override;
        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

        // Send a normalized value to the JS layer
        void updateUIParam(const juce::String& paramID, f32 normalizedValue);

        void openFilePicker(const juce::String& oscId = "osc1");
        void sendWavetableToUI(const juce::String& oscId = "osc1");

        SPECTRProcessor& audioProcessor;
        std::unique_ptr<juce::FileChooser> mFileChooser;
        BuildMode mBuildMode {BuildMode::FFTMorph};  // current mode for next load

        juce::WebBrowserComponent webView {
          juce::WebBrowserComponent::Options {}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2 {}.withUserDataFolder(
              juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory)))
            .withResourceProvider([](const auto& url) { return getResource(url); })
            .withNativeIntegrationEnabled()
            // JS → C++: js calls window.__JUCE__.backend.emitEvent("setParam", {...})
            .withEventListener("setParam",
                               [this](const juce::var& data) {
                                   if (const auto* obj = data.getDynamicObject()) {
                                       const auto paramId = obj->getProperty("paramId").toString();
                                       const auto value   = _Cast<f32>(_Cast<f64>(obj->getProperty("value")));
                                       if (auto* param = audioProcessor.apvts.getParameter(paramId))
                                           param->setValueNotifyingHost(value);
                                   }
                               })
            .withEventListener("openFilePicker", [this](const juce::var& data) {
                auto oscId = data["oscId"].toString();
                juce::MessageManager::callAsync([this, oscId] { openFilePicker(oscId); });
            })};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SPECTREditor)
    };
}  // namespace SPECTR