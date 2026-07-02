#include <gtest/gtest.h>
#include "RSA.h"

// ── Small known-value RSA example ─────────────────────────────────────────────
// p=3, q=11 → n=33, phi=20, e=7, d=3
// (as used in Phase 2 educational content)

TEST(RSA, EncryptDecryptSmall) {
    BigInteger n(33ULL), e(7ULL), d(3ULL);
    BigInteger plaintext(4ULL);

    BigInteger cipher = RSA::encrypt(plaintext, e, n);
    BigInteger recovered = RSA::decrypt(cipher, d, n);

    EXPECT_EQ(recovered.toString(), plaintext.toString());
}

TEST(RSA, SignVerifySmall) {
    BigInteger n(33ULL), e(7ULL), d(3ULL);
    BigInteger hash_value(5ULL);

    BigInteger sig = RSA::sign(hash_value, d, n);
    BigInteger recovered = RSA::verify(sig, e, n);

    EXPECT_EQ(recovered.toString(), hash_value.toString());
}

TEST(RSA, EncryptDecryptRoundTrip) {
    // Use a slightly larger hand-crafted example: p=61, q=53
    // n=3233, phi=3120, e=17, d=2753
    BigInteger n(3233ULL), e(17ULL), d(2753ULL);
    BigInteger plaintext(65ULL);

    BigInteger cipher = RSA::encrypt(plaintext, e, n);
    BigInteger recovered = RSA::decrypt(cipher, d, n);

    EXPECT_EQ(recovered.toString(), "65");
}

TEST(RSA, SignVerifyRoundTrip) {
    BigInteger n(3233ULL), e(17ULL), d(2753ULL);
    BigInteger hash_value(42ULL);

    BigInteger sig = RSA::sign(hash_value, d, n);
    BigInteger verified = RSA::verify(sig, e, n);

    EXPECT_EQ(verified.toString(), "42");
}

TEST(RSA, GeneratePrimeIsPrime) {
    // Generate small primes (16-bit) and verify they are prime
    for (int i = 0; i < 3; ++i) {
        BigInteger p = RSA::generatePrime(16);
        EXPECT_TRUE(p.isPrime());
    }
}

TEST(RSA, GenerateKeypairAndRoundTrip) {
    // Use a small key (64-bit) to keep test fast
    RSAKeyPair kp = RSA::generateKeypair(64);

    // n should be larger than a small plaintext
    BigInteger plaintext(42ULL);
    EXPECT_TRUE(plaintext < kp.n);

    BigInteger cipher    = RSA::encrypt(plaintext, kp.e, kp.n);
    BigInteger recovered = RSA::decrypt(cipher, kp.d, kp.n);
    EXPECT_EQ(recovered.toString(), "42");
}

TEST(RSA, SignVerifyWithGeneratedKey) {
    RSAKeyPair kp = RSA::generateKeypair(64);

    BigInteger hash_value(99ULL);
    BigInteger sig      = RSA::sign(hash_value, kp.d, kp.n);
    BigInteger verified = RSA::verify(sig, kp.e, kp.n);
    EXPECT_EQ(verified.toString(), "99");
}
