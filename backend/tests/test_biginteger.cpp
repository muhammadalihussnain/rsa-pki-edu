#include <gtest/gtest.h>
#include "BigInteger.h"

// ── Construction & toString ───────────────────────────────────────────────────

TEST(BigInteger, ConstructFromUint64) {
    EXPECT_EQ(BigInteger(0ULL).toString(), "0");
    EXPECT_EQ(BigInteger(1ULL).toString(), "1");
    EXPECT_EQ(BigInteger(123456789ULL).toString(), "123456789");
    EXPECT_EQ(BigInteger(1000000000ULL).toString(), "1000000000");
}

TEST(BigInteger, ConstructFromString) {
    EXPECT_EQ(BigInteger("0").toString(), "0");
    EXPECT_EQ(BigInteger("42").toString(), "42");
    EXPECT_EQ(BigInteger("999999999999999999").toString(), "999999999999999999");
}

// ── Comparison ────────────────────────────────────────────────────────────────

TEST(BigInteger, Comparison) {
    BigInteger a(10ULL), b(20ULL), c(10ULL);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(b >= a);
}

// ── Addition ──────────────────────────────────────────────────────────────────

TEST(BigInteger, Addition) {
    EXPECT_EQ((BigInteger(3ULL) + BigInteger(11ULL)).toString(), "14");
    EXPECT_EQ((BigInteger(999999999ULL) + BigInteger(1ULL)).toString(), "1000000000");
    // Large: 10^18 + 1
    BigInteger big("1000000000000000000");
    EXPECT_EQ((big + BigInteger(1ULL)).toString(), "1000000000000000001");
}

// ── Subtraction ───────────────────────────────────────────────────────────────

TEST(BigInteger, Subtraction) {
    EXPECT_EQ((BigInteger(20ULL) - BigInteger(7ULL)).toString(), "13");
    EXPECT_EQ((BigInteger(1000000000ULL) - BigInteger(1ULL)).toString(), "999999999");
}

// ── Multiplication ────────────────────────────────────────────────────────────

TEST(BigInteger, Multiplication) {
    EXPECT_EQ((BigInteger(3ULL) * BigInteger(11ULL)).toString(), "33");
    EXPECT_EQ((BigInteger(0ULL) * BigInteger(12345ULL)).toString(), "0");
    EXPECT_EQ((BigInteger(1000000ULL) * BigInteger(1000000ULL)).toString(), "1000000000000");
}

// ── Division & Modulo ─────────────────────────────────────────────────────────

TEST(BigInteger, DivisionAndModulo) {
    EXPECT_EQ((BigInteger(33ULL) / BigInteger(11ULL)).toString(), "3");
    EXPECT_EQ((BigInteger(33ULL) % BigInteger(11ULL)).toString(), "0");
    EXPECT_EQ((BigInteger(34ULL) % BigInteger(11ULL)).toString(), "1");
    EXPECT_EQ((BigInteger(7ULL) / BigInteger(3ULL)).toString(), "2");
    EXPECT_EQ((BigInteger(7ULL) % BigInteger(3ULL)).toString(), "1");
}

// ── modPow ────────────────────────────────────────────────────────────────────

TEST(BigInteger, ModPow) {
    // 2^10 mod 1000 = 24
    EXPECT_EQ(BigInteger::modPow(BigInteger(2ULL),
                                  BigInteger(10ULL),
                                  BigInteger(1000ULL)).toString(), "24");
    // RSA small example: 3^7 mod 33 = 9 (since 3^7=2187, 2187%33=9)
    EXPECT_EQ(BigInteger::modPow(BigInteger(3ULL),
                                  BigInteger(7ULL),
                                  BigInteger(33ULL)).toString(), "9");
    // any^0 = 1
    EXPECT_EQ(BigInteger::modPow(BigInteger(5ULL),
                                  BigInteger(0ULL),
                                  BigInteger(13ULL)).toString(), "1");
}

// ── isPrime ───────────────────────────────────────────────────────────────────

TEST(BigInteger, IsPrime) {
    EXPECT_FALSE(BigInteger(1ULL).isPrime());
    EXPECT_TRUE(BigInteger(2ULL).isPrime());
    EXPECT_TRUE(BigInteger(3ULL).isPrime());
    EXPECT_FALSE(BigInteger(4ULL).isPrime());
    EXPECT_TRUE(BigInteger(5ULL).isPrime());
    EXPECT_TRUE(BigInteger(11ULL).isPrime());
    EXPECT_FALSE(BigInteger(33ULL).isPrime());
    EXPECT_TRUE(BigInteger(97ULL).isPrime());
    EXPECT_FALSE(BigInteger(100ULL).isPrime());
}

// ── modInverse ────────────────────────────────────────────────────────────────

TEST(BigInteger, ModInverse) {
    // 7^-1 mod 20 = 3 (since 7*3 = 21 ≡ 1 mod 20)
    EXPECT_EQ(BigInteger::modInverse(BigInteger(7ULL),
                                      BigInteger(20ULL)).toString(), "3");
    // 3^-1 mod 11 = 4 (since 3*4=12 ≡ 1 mod 11)
    EXPECT_EQ(BigInteger::modInverse(BigInteger(3ULL),
                                      BigInteger(11ULL)).toString(), "4");
}

// ── Bit operations ────────────────────────────────────────────────────────────

TEST(BigInteger, BitOps) {
    BigInteger n(8ULL); // 1000 in binary
    EXPECT_EQ((n >> 1).toString(), "4");
    EXPECT_EQ((n << 1).toString(), "16");
    EXPECT_TRUE(n.isEven());
    EXPECT_FALSE(BigInteger(7ULL).isEven());
}

TEST(BigInteger, BitLength) {
    EXPECT_EQ(BigInteger(0ULL).bitLength(), 0);
    EXPECT_EQ(BigInteger(1ULL).bitLength(), 1);
    EXPECT_EQ(BigInteger(4ULL).bitLength(), 3);   // 100 in binary
    EXPECT_EQ(BigInteger(255ULL).bitLength(), 8); // 11111111
}

TEST(BigInteger, GetBit) {
    BigInteger n(6ULL); // 110 in binary
    EXPECT_FALSE(n.getBit(0)); // LSB = 0
    EXPECT_TRUE(n.getBit(1));  // bit 1 = 1
    EXPECT_TRUE(n.getBit(2));  // bit 2 = 1
    EXPECT_FALSE(n.getBit(3)); // bit 3 = 0
}

TEST(BigInteger, ExtGcd) {
    BigInteger x, y;
    BigInteger gcd = BigInteger::extGcd(BigInteger(30ULL), BigInteger(20ULL), x, y);
    EXPECT_EQ(gcd.toString(), "10");
}

TEST(BigInteger, ModInverseThrowsWhenNotCoprime) {
    // gcd(4, 6) = 2 ≠ 1, so no inverse exists
    EXPECT_THROW(
        BigInteger::modInverse(BigInteger(4ULL), BigInteger(6ULL)),
        std::domain_error
    );
}

TEST(BigInteger, IsZeroAndIsOne) {
    EXPECT_TRUE(BigInteger(0ULL).isZero());
    EXPECT_FALSE(BigInteger(1ULL).isZero());
    EXPECT_TRUE(BigInteger(1ULL).isOne());
    EXPECT_FALSE(BigInteger(2ULL).isOne());
}
