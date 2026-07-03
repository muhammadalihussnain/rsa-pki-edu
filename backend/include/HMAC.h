#pragma once
#include <string>
#include <cstdint>
#include <array>

/**
 * @brief Custom HMAC-SHA256 implementation from scratch.
 *
 * HMAC(K, m) = SHA256((K XOR opad) || SHA256((K XOR ipad) || m))
 * Follows RFC 2104. No stdlib crypto used.
 */
class HMAC_SHA256 {
public:
    /// Compute HMAC-SHA256; returns 32-byte digest
    static std::array<uint8_t, 32> compute(
        const std::string& key,
        const std::string& message);

    /// Returns lowercase hex string of HMAC-SHA256
    static std::string hexdigest(
        const std::string& key,
        const std::string& message);
};
