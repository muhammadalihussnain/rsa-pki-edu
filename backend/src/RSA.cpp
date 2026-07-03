#include "RSA.h"
#include <stdexcept>

/**
 * @brief Generate a random prime of exactly `bits` bits.
 */
BigInteger RSACrypto::generatePrime(int bits) {
    return BigInteger::randomPrime(bits);
}

/**
 * @brief Generate an RSA key pair.
 *
 * 1. Pick two distinct primes p, q each of bits/2 bits.
 * 2. n = p * q
 * 3. phi = (p-1) * (q-1)
 * 4. Choose e = 65537 (standard public exponent)
 * 5. d = e^-1 mod phi
 */
RSAKeyPair RSACrypto::generateKeypair(int bits) {
    int half = bits / 2;
    BigInteger p, q;
    do {
        p = generatePrime(half);
        q = generatePrime(half);
    } while (p == q);

    BigInteger n = p * q;
    BigInteger one(1ULL);
    BigInteger phi = (p - one) * (q - one);

    BigInteger e(65537ULL);
    BigInteger d = BigInteger::modInverse(e, phi);

    return {n, e, d};
}

/**
 * @brief Encrypt: c = m^e mod n
 */
BigInteger RSACrypto::encrypt(const BigInteger& m,
                        const BigInteger& e,
                        const BigInteger& n) {
    return BigInteger::modPow(m, e, n);
}

/**
 * @brief Decrypt: m = c^d mod n
 */
BigInteger RSACrypto::decrypt(const BigInteger& c,
                        const BigInteger& d,
                        const BigInteger& n) {
    return BigInteger::modPow(c, d, n);
}

/**
 * @brief Sign: sig = hash^d mod n
 */
BigInteger RSACrypto::sign(const BigInteger& hash,
                     const BigInteger& d,
                     const BigInteger& n) {
    return BigInteger::modPow(hash, d, n);
}

/**
 * @brief Verify: recovered = sig^e mod n
 */
BigInteger RSACrypto::verify(const BigInteger& signature,
                       const BigInteger& e,
                       const BigInteger& n) {
    return BigInteger::modPow(signature, e, n);
}
