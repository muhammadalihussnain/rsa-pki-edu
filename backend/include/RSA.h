#pragma once
#include "BigInteger.h"
#include <tuple>

/**
 * @brief RSA key pair container.
 */
struct RSAKeyPair {
    BigInteger n; ///< Modulus
    BigInteger e; ///< Public exponent
    BigInteger d; ///< Private exponent
};

/**
 * @brief RSA operations: key generation, encrypt, decrypt, sign, verify.
 *
 * No pre-built crypto libraries are used — all math is done via BigInteger.
 * Named RSACrypto to avoid collision with OpenSSL's RSA typedef.
 */
class RSACrypto {
public:
    /// @brief Generate a random `bits`-bit prime suitable for RSA
    static BigInteger generatePrime(int bits);

    /// @brief Generate an RSA key pair with modulus of ~`bits` bits
    static RSAKeyPair generateKeypair(int bits);

    /// @brief Encrypt plaintext m: c = m^e mod n
    static BigInteger encrypt(const BigInteger& m,
                              const BigInteger& e,
                              const BigInteger& n);

    /// @brief Decrypt ciphertext c: m = c^d mod n
    static BigInteger decrypt(const BigInteger& c,
                              const BigInteger& d,
                              const BigInteger& n);

    /// @brief Sign a hash: sig = hash^d mod n
    static BigInteger sign(const BigInteger& hash,
                           const BigInteger& d,
                           const BigInteger& n);

    /// @brief Verify a signature: recovered = sig^e mod n
    static BigInteger verify(const BigInteger& signature,
                             const BigInteger& e,
                             const BigInteger& n);
};
