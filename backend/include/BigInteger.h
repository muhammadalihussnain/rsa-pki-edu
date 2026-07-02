#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Arbitrary-precision non-negative integer for RSA math.
 *
 * Internally stores digits in base 10^9 (little-endian).
 * Supports the arithmetic operations required by RSA.
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

    // Comparison
    bool operator==(const BigInteger& rhs) const;
    bool operator!=(const BigInteger& rhs) const;
    bool operator<(const BigInteger& rhs) const;
    bool operator<=(const BigInteger& rhs) const;
    bool operator>(const BigInteger& rhs) const;
    bool operator>=(const BigInteger& rhs) const;

    // Bit operations
    BigInteger operator>>(int shift) const;
    BigInteger operator<<(int shift) const;
    bool isEven() const;
    bool isZero() const;
    bool isOne() const;
    int bitLength() const;
    bool getBit(int pos) const;

    /// @brief Modular exponentiation: (base^exp) mod m
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

    // Extended Euclidean: returns gcd; sets x, y s.t. a*x + b*y = gcd
    static BigInteger extGcd(const BigInteger& a,
                             const BigInteger& b,
                             BigInteger& x,
                             BigInteger& y);

    /// @brief Modular inverse of a mod m (m must be coprime to a)
    static BigInteger modInverse(const BigInteger& a, const BigInteger& m);

private:
    static constexpr uint64_t BASE = 1000000000ULL; // 10^9
    std::vector<uint64_t> digits_; // little-endian base-10^9 digits

    void trim();
    static BigInteger fromDigits(std::vector<uint64_t> d);
};
