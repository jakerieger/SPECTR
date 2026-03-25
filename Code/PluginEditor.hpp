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

        // Called from web view
        void ipc_OpenFilePicker(const juce::String& oscId = "osc1");
        void ipc_SendWavetableToUI(const juce::String& wtName, const juce::String& oscId = "osc1");
        void ipc_ActivateLicense(const juce::String& licenseJson);
        void ipc_GetLicenseInfo();
        void ipc_ClearLicense() const;

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
            .withEventListener("openFilePicker",
                               [this](const juce::var& data) {
                                   auto oscId = data["oscId"].toString();
                                   juce::MessageManager::callAsync([this, oscId] { ipc_OpenFilePicker(oscId); });
                               })
            .withEventListener("activateLicense",
                               [this](const juce::var& data) {
                                   auto licenseJson = data.toString();
                                   juce::MessageManager::callAsync(
                                     [this, licenseJson] { ipc_ActivateLicense(licenseJson); });
                               })
            .withEventListener(
              "getLicenseInfo",
              [this](const juce::var& data) { juce::MessageManager::callAsync([this] { ipc_GetLicenseInfo(); }); })
            .withEventListener(
              "clearLicense",
              [this](const juce::var& data) { juce::MessageManager::callAsync([this] { ipc_ClearLicense(); }); }),
        };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SPECTREditor)
    };
}  // namespace SPECTR