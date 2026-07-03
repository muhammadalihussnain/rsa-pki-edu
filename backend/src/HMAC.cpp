#include "HMAC.h"
#include "Hash.h"
#include <sstream>
#include <iomanip>
#include <cstring>

// HMAC block size for SHA-256 is 64 bytes
static const size_t BLOCK_SIZE = 64;

std::array<uint8_t, 32> HMAC_SHA256::compute(
    const std::string& key,
    const std::string& message)
{
    // Step 1: normalise key to block size
    std::array<uint8_t, 64> k = {};

    if (key.size() > BLOCK_SIZE) {
        // Key too long — hash it first
        auto hk = SHA256::hash(key);
        std::memcpy(k.data(), hk.data(), 32);
    } else {
        std::memcpy(k.data(), key.data(), key.size());
        // rest is already zero-padded
    }

    // Step 2: build inner and outer padded keys
    std::array<uint8_t, 64> ipad_key, opad_key;
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        ipad_key[i] = k[i] ^ 0x36u;
        opad_key[i] = k[i] ^ 0x5cu;
    }

    // Step 3: inner hash = SHA256(ipad_key || message)
    std::string inner;
    inner.resize(BLOCK_SIZE + message.size());
    std::memcpy(&inner[0],           ipad_key.data(), BLOCK_SIZE);
    std::memcpy(&inner[BLOCK_SIZE],  message.data(),  message.size());
    auto inner_hash = SHA256::hash(inner);

    // Step 4: outer hash = SHA256(opad_key || inner_hash)
    std::string outer;
    outer.resize(BLOCK_SIZE + 32);
    std::memcpy(&outer[0],          opad_key.data(),  BLOCK_SIZE);
    std::memcpy(&outer[BLOCK_SIZE], inner_hash.data(), 32);

    return SHA256::hash(outer);
}

std::string HMAC_SHA256::hexdigest(
    const std::string& key,
    const std::string& message)
{
    auto d = compute(key, message);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : d) ss << std::setw(2) << static_cast<int>(b);
    return ss.str();
}
