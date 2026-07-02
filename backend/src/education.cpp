#include "Education.h"

namespace Education {

/**
 * @brief Returns a structured JSON string describing RSA theory.
 *
 * Includes:
 *  - steps: ordered list of key-generation explanation steps
 *  - example: small worked example (p=3, q=11) with each field explained
 *  - public_key / private_key summary
 */
std::string rsaTheoryJson() {
    // Note: all unicode replaced with ASCII to ensure portable compilation
    return R"JSON({
  "steps": [
    {
      "index": 1,
      "title": "Choose two prime numbers",
      "explanation": "Pick two distinct prime numbers p and q. They should be large in practice, but we use small ones here for clarity."
    },
    {
      "index": 2,
      "title": "Compute the modulus n",
      "explanation": "Calculate n = p x q. This modulus is part of both the public and private key."
    },
    {
      "index": 3,
      "title": "Compute Euler totient phi(n)",
      "explanation": "Calculate phi(n) = (p - 1) x (q - 1). This tells us how many integers less than n are coprime with n."
    },
    {
      "index": 4,
      "title": "Choose the public exponent e",
      "explanation": "Pick e such that 1 < e < phi(n) and gcd(e, phi(n)) = 1 (e and phi(n) are coprime). A common choice is 65537; in our example we use 7."
    },
    {
      "index": 5,
      "title": "Compute the private exponent d",
      "explanation": "Find d such that e x d = 1 (mod phi(n)). This is the modular multiplicative inverse of e."
    },
    {
      "index": 6,
      "title": "Keys are ready",
      "explanation": "Public key: (e, n) -- shared openly. Private key: (d, n) -- kept secret. Encrypt with public key, decrypt with private key."
    }
  ],
  "example": {
    "p": 3,
    "q": 11,
    "n": 33,
    "phi": 20,
    "e": 7,
    "d": 3,
    "steps": [
      { "label": "p = 3, q = 11",            "note": "Two small primes chosen for illustration" },
      { "label": "n = p x q = 33",            "note": "The modulus used in both keys" },
      { "label": "phi(n) = (3-1)(11-1) = 20", "note": "Euler totient" },
      { "label": "e = 7",                      "note": "Chosen because gcd(7, 20) = 1" },
      { "label": "d = 3",                      "note": "Because 7 x 3 = 21 = 1 (mod 20)" }
    ],
    "public_key":  { "e": 7, "n": 33 },
    "private_key": { "d": 3, "n": 33 },
    "verify": "Encrypt m=4: 4^7 mod 33 = 16. Decrypt: 16^3 mod 33 = 4."
  }
})JSON";
}

} // namespace Education

namespace Education {

std::string certificateTheoryJson() {
    return R"JSON({
  "sections": [
    {
      "index": 1,
      "title": "What is a Digital Certificate?",
      "body": "A digital certificate is a signed document that binds a public key to an identity. It is issued by a trusted third party called a Certificate Authority (CA). Anyone who trusts the CA can verify the certificate to be sure the public key truly belongs to the named subject."
    },
    {
      "index": 2,
      "title": "Certificate Fields",
      "body": "An X.509-style certificate contains: subject (who it belongs to), issuer (which CA signed it), public_key (the subject's RSA n and e), serial (unique identifier), validity (valid_from and valid_to dates), and ca_signature (the CA's RSA signature over a hash of the other fields)."
    },
    {
      "index": 3,
      "title": "How the CA Signs a Certificate",
      "body": "The CA computes a hash of the certificate fields (subject, public key, serial). It then signs the hash using its RSA private key: signature = hash^d_CA mod n_CA. This signature can only be produced by the CA because only the CA knows d_CA."
    },
    {
      "index": 4,
      "title": "How Anyone Verifies a Certificate",
      "body": "Anyone who has the CA's public key (e_CA, n_CA) can verify the certificate. They compute: recovered = signature^e_CA mod n_CA. If recovered equals the expected hash of the certificate fields, the certificate is authentic."
    }
  ],
  "mock_certificate": {
    "subject": "Alice",
    "issuer": "CA",
    "public_key": { "e": "65537", "n": "3233 (= 61 x 53)" },
    "serial": "0001",
    "valid_from": "2025-01-01",
    "valid_to": "2026-01-01",
    "ca_signature": "hash(Alice + n + 0001)^d_CA mod n_CA",
    "field_explanations": {
      "subject": "The entity this certificate is issued to",
      "issuer": "The CA that vouches for this certificate",
      "public_key": "The subject's RSA public key (safe to share)",
      "serial": "Unique number to distinguish certificates",
      "valid_from": "Certificate is not valid before this date",
      "valid_to": "Certificate expires after this date",
      "ca_signature": "Proof that the CA approved this certificate"
    }
  }
})JSON";
}

} // namespace Education
