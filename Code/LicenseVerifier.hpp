//
// Created by jr on 3/18/2026.
//

#pragma once

#include "PluginCommon.hpp"
#include <juce_core/juce_core.h>

namespace SPECTR {
    // ============================================================
    //  LicenseVerifier
    //
    //  Verifies ECDSA-signed .lic files produced by sign_license.py.
    //  The license file is a JSON object:
    //    { "payload": "<base64>", "signature": "<base64>" }
    //  where payload is itself a base64-encoded JSON object:
    //    { "name": "...", "email": "...", "product": "...", "version": 1 }
    //
    //  All verification is offline — the public key is embedded in
    //  BinaryData (public_key.pem via juce_add_binary_data).
    // ============================================================

    struct LicenseInfo {
        bool isValid {false};
        juce::String name;
        juce::String email;
        juce::String product;

        _Nodisc juce::String statusString() const {
            if (isValid) return "Licensed to" + name;
            return "Unlicensed";
        }
    };

    class LicenseVerifier {
    public:
        static constexpr auto kExpectedProduct = "SPECTR";

        static LicenseInfo verifyFile(const juce::File& licFile);
        static LicenseInfo verifyString(const juce::String& jsonContent);
        static juce::File getLicenseStoragePath();
        static bool saveLicense(const juce::String& jsonContent);
        static LicenseInfo loadSavedLicense();
        static void clearSavedLicense();

    private:
        // Decode base64, parse JSON, run ECDSA verify
        static LicenseInfo verifyImpl(const juce::String& jsonContent);

        // ECDSA-P256 + SHA-256 signature check via mbedTLS
        // payloadBytes  — the raw UTF-8 bytes that were signed
        // sigBytes      — the DER-encoded signature from the .lic file
        static bool ecdsaVerify(const juce::MemoryBlock& payloadBytes, const juce::MemoryBlock& sigBytes);
    };
}  // namespace SPECTR