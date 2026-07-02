#include "Hash.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>

// ── SHA-256 constants (first 32 bits of fractional parts of cube roots of
//   first 64 primes) ────────────────────────────────────────────────────────

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Initial hash values (first 32 bits of fractional parts of square roots of
// first 8 primes)
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// ── Bit operations ────────────────────────────────────────────────────────────

static inline uint32_t rotr(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t ch(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ (~e & g);
}

static inline uint32_t maj(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

static inline uint32_t sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t gam0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t gam1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

// ── Core transform ────────────────────────────────────────────────────────────

static void processBlock(const uint8_t block[64], uint32_t state[8]) {
    uint32_t w[64];

    // Prepare message schedule
    for (int i = 0; i < 16; i++) {
        w[i] = (static_cast<uint32_t>(block[i * 4    ]) << 24)
             | (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
             | (static_cast<uint32_t>(block[i * 4 + 2]) <<  8)
             |  static_cast<uint32_t>(block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = gam1(w[i - 2]) + w[i - 7] + gam0(w[i - 15]) + w[i - 16];
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = sig0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

// ── Public API ────────────────────────────────────────────────────────────────

std::array<uint8_t, 32> SHA256::hash(const uint8_t* data, size_t len) {
    uint32_t state[8];
    std::memcpy(state, H0, sizeof(H0));

    // Pre-processing: padding
    // Total padded length must be ≡ 448 (mod 512) bits, then append 64-bit length
    uint64_t bitLen = static_cast<uint64_t>(len) * 8;

    // Build padded message in a buffer
    size_t paddedLen = len + 1;
    while ((paddedLen % 64) != 56) paddedLen++;
    paddedLen += 8; // for the 64-bit length

    std::vector<uint8_t> buf(paddedLen, 0);
    std::memcpy(buf.data(), data, len);
    buf[len] = 0x80; // append bit '1'

    // Append original length in bits as big-endian 64-bit
    for (int i = 7; i >= 0; i--) {
        buf[paddedLen - 8 + (7 - i)] = static_cast<uint8_t>(bitLen >> (i * 8));
    }

    // Process each 64-byte block
    for (size_t i = 0; i < paddedLen; i += 64) {
        processBlock(buf.data() + i, state);
    }

    // Produce final digest (big-endian)
    std::array<uint8_t, 32> digest;
    for (int i = 0; i < 8; i++) {
        digest[i * 4    ] = static_cast<uint8_t>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<uint8_t>(state[i] >>  8);
        digest[i * 4 + 3] = static_cast<uint8_t>(state[i]      );
    }
    return digest;
}

std::array<uint8_t, 32> SHA256::hash(const std::string& data) {
    return hash(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

std::string SHA256::hexdigest(const uint8_t* data, size_t len) {
    auto d = hash(data, len);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t b : d) ss << std::setw(2) << static_cast<int>(b);
    return ss.str();
}

std::string SHA256::hexdigest(const std::string& data) {
    return hexdigest(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}
