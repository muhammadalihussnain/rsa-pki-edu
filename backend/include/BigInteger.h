#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

/**
 * @brief Arbitrary-precision non-negative integer for RSA math.
 *
 * Internally stores limbs in base 2^32 (little-endian uint32_t).
 * This makes bit-level ops (>>, <<, getBit, isEven) O(n/32) instead
 * of O(n) divisions — critical for Miller-Rabin on large numbers.
 */
class BigInteger {
public:
    BigInteger();
    explicit BigInteger(uint64_t value);
    explicit BigInteger(const std::string& decimal);

    // Arithmetic
    BigInteger operator+(const BigInteger& rhs) const;
    BigInteger operator-(const BigInteger& rhs) const;  ///< Requires *this >= rhs
    BigInteger operator*(const BigInteger& rhs) const;
    BigInteger operator%(const BigInteger& mod) const;
    BigInteger operator/(const BigInteger& rhs) const;

    /// @brief Combined division and modulo — avoids double computation
    static std::pair<BigInteger, BigInteger> divmod(const BigInteger& a,
                                                     const BigInteger& b);

    // Comparison
    bool operator==(const BigInteger& rhs) const;
    bool operator!=(const BigInteger& rhs) const;
    bool operator<(const BigInteger& rhs) const;
    bool operator<=(const BigInteger& rhs) const;
    bool operator>(const BigInteger& rhs) const;
    bool operator>=(const BigInteger& rhs) const;

    // Bit operations — O(1) or O(n/32) thanks to base-2^32 representation
    BigInteger operator>>(int shift) const;
    BigInteger operator<<(int shift) const;
    bool isEven() const;
    bool isZero() const;
    bool isOne() const;
    int  bitLength() const;
    bool getBit(int pos) const;

    /// @brief Modular exponentiation: (base^exp) mod m  — square-and-multiply
    static BigInteger modPow(const BigInteger& base,
                             const BigInteger& exp,
                             const BigInteger& mod);

    /// @brief Miller-Rabin primality test with `rounds` witnesses
    bool isPrime(int rounds = 20) const;

    /// @brief Generate a random `bits`-bit BigInteger (top bit set)
    static BigInteger random(int bits);

    /// @brief Generate a random prime of exactly `bits` bits
    static BigInteger randomPrime(int bits);

    std::string toString() const;

    // Extended Euclidean
    static BigInteger extGcd(const BigInteger& a, const BigInteger& b,
                             BigInteger& x, BigInteger& y);

    /// @brief Modular inverse of a mod m
    static BigInteger modInverse(const BigInteger& a, const BigInteger& m);

private:
    std::vector<uint32_t> digits_; ///< little-endian base-2^32 limbs

    void trim();
    static BigInteger fromDigits(std::vector<uint32_t> d);
};
