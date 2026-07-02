#pragma once
#include <string>
#include <cstdint>
#include <array>

/**
 * @brief Custom SHA-256 implementation from scratch.
 *
 * No stdlib crypto (openssl, <crypto>, etc.) is used.
 * Follows FIPS 180-4 specification.
 */
class SHA256 {
public:
    /// Compute SHA-256 of arbitrary bytes; returns 32-byte digest
    static std::array<uint8_t, 32> hash(const uint8_t* data, size_t len);

    /// Convenience: hash a std::string
    static std::array<uint8_t, 32> hash(const std::string& data);

    /// Returns lowercase hex string of 64 chars
    static std::string hexdigest(const std::string& data);
    static std::string hexdigest(const uint8_t* data, size_t len);
};
