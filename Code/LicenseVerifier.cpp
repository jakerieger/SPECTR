//
// Created by jr on 3/21/2026.
//

#include "LicenseVerifier.hpp"
#include "BinaryData.h"

#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <mbedtls/error.h>
#include <mbedtls/ecdsa.h>

namespace SPECTR {
    // ===========================================================================
    // Helpers
    // ===========================================================================

    static bool decodeBase64(const juce::String& encoded, juce::MemoryBlock& dst) {
        dst.reset();
        juce::MemoryOutputStream stream(dst, false);
        const bool ok = juce::Base64::convertFromBase64(stream, encoded);
        stream.flush();  // trim dst to exactly the bytes written
        return ok;
    }

    static std::vector<u8> getPublicKeyPem() {
        const auto data    = BinaryData::public_key_pem;
        constexpr int size = BinaryData::public_key_pemSize;
        std::vector<u8> pem(data, data + size);

        // Guarantee null termination - BinaryData doesn't promise one
        if (pem.empty() || pem.back() != '\0') pem.push_back('\0');

        return pem;
    }

    static juce::String mbedError(const int ret) {
        char buf[256] = {};
        mbedtls_strerror(ret, buf, sizeof(buf));
        return {buf};
    }

    // ===========================================================================
    // LicenseVerifier
    // ===========================================================================

    LicenseInfo LicenseVerifier::verifyFile(const juce::File& licFile) {
        if (!licFile.existsAsFile()) {
            DBG("LicenseVerifier: file not found — " + licFile.getFullPathName());
            return {};
        }

        return verifyImpl(licFile.loadFileAsString());
    }

    LicenseInfo LicenseVerifier::verifyString(const juce::String& jsonContent) {
        return verifyImpl(jsonContent);
    }

    juce::File LicenseVerifier::getLicenseStoragePath() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("Rieger DSP")
          .getChildFile("SPECTR")
          .getChildFile("license.lic");
    }

    bool LicenseVerifier::saveLicense(const juce::String& jsonContent) {
        const auto licFile = getLicenseStoragePath();

        if (!licFile.getParentDirectory().createDirectory()) {
            DBG("LicenseVerifier: could not create license directory");
            return false;
        }
        if (!licFile.replaceWithText(jsonContent)) {
            DBG("LicenseVerifier: could not write license file");
            return false;
        }

        DBG("LicenseVerifier: license saved to " + licFile.getFullPathName());
        return true;
    }

    LicenseInfo LicenseVerifier::loadSavedLicense() {
        return verifyFile(getLicenseStoragePath());
    }

    void LicenseVerifier::clearSavedLicense() {
        std::ignore = getLicenseStoragePath().deleteFile();
        DBG("LicenseVerifier: license cleared");
    }

    //  verifyImpl  (shared logic for verifyFile / verifyString)
    LicenseInfo LicenseVerifier::verifyImpl(const juce::String& jsonContent) {
        LicenseInfo result;  // isValid = false by default

        // 1. Parse outer JSON
        const auto outer = juce::JSON::parse(jsonContent);
        if (!outer.isObject()) {
            DBG("LicenseVerifier: invalid JSON format");
            return result;
        }

        const auto payloadB64 = outer["payload"].toString();
        const auto sigB64     = outer["signature"].toString();

        if (payloadB64.isEmpty() || sigB64.isEmpty()) {
            DBG("LicenseVerifier: missing payload or signature field");
            return result;
        }

        // 2. Decode base64
        juce::MemoryBlock payloadBytes, sigBytes;
        payloadBytes.reset();
        sigBytes.reset();

        if (!decodeBase64(payloadB64, payloadBytes) || payloadBytes.getSize() == 0) {
            DBG("LicenseVerifier: base64 decode failed for payload");
            return result;
        }

        if (!decodeBase64(sigB64, sigBytes) || sigBytes.getSize() == 0) {
            DBG("LicenseVerifier: base64 decode failed for signature");
            return result;
        }

        // 3. Crypto verification
        if (!ecdsaVerify(payloadBytes, sigBytes)) return result;

        // 4. Parse inner JSON
        const juce::String payloadStr(_Cast<const char*>(payloadBytes.getData()), payloadBytes.getSize());
        const auto payload = juce::JSON::parse(payloadStr);

        if (!payload.isObject()) {
            DBG("LicenseVerifier: invalid JSON format");
            return result;
        }

        // 5. Field validation
        const auto name    = payload["name"].toString().trim();
        const auto email   = payload["email"].toString().trim();
        const auto product = payload["product"].toString().trim();

        if (name.isEmpty() || email.isEmpty() || product.isEmpty()) {
            DBG("LicenseVerifier: one or more required payload fields are empty");
            return result;
        }

        if (product != kExpectedProduct) {
            DBG("LicenseVerifier: product mismatch — got '" + product + "', expected '" + kExpectedProduct + "'");
            return result;
        }

        result.isValid = true;
        result.name    = name;
        result.email   = email;
        result.product = product;
        return result;
    }

    bool LicenseVerifier::ecdsaVerify(const juce::MemoryBlock& payloadBytes, const juce::MemoryBlock& sigBytes) {
        // 1. Hash the payload with SHA-256
        u8 hash[32] = {};
        mbedtls_sha256(_Cast<const u8*>(payloadBytes.getData()), payloadBytes.getSize(), hash, false);

        // 2. Load the public key from BinaryData
        const auto pem = getPublicKeyPem();

        mbedtls_pk_context pk;
        mbedtls_pk_init(&pk);

        int ret = mbedtls_pk_parse_public_key(&pk, pem.data(), pem.size());

        if (ret != 0) {
            DBG("LicenseVerifier: failed to parse public key - " + mbedError(ret));
            mbedtls_pk_free(&pk);
            return false;
        }

        // 3. Verify the DER-encoded ECDSA signature
        ret = mbedtls_pk_verify(&pk,
                                MBEDTLS_MD_SHA256,
                                hash,
                                sizeof(hash),
                                _Cast<const u8*>(sigBytes.getData()),
                                sigBytes.getSize());

        mbedtls_pk_free(&pk);
        if (ret != 0) DBG("LicenseVerifier: signature invalid - " + mbedError(ret));

        return ret == 0;
    }
}  // namespace SPECTR