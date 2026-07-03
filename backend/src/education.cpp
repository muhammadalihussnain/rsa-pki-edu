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
    return R"JSON({
  "what_is_rsa": {
    "title": "What is RSA?",
    "body": "RSA (Rivest-Shamir-Adleman) is a public-key cryptosystem invented in 1977. It relies on the mathematical difficulty of factoring the product of two large prime numbers. RSA uses a key pair: a public key anyone can see, and a private key only the owner holds. Data encrypted with the public key can only be decrypted with the private key, and vice versa.",
    "uses": ["Encrypting messages", "Digital signatures", "Key exchange in TLS/HTTPS", "Certificate authorities (PKI)"]
  },
  "flavours": [
    {
      "name": "RSA-OAEP",
      "full_name": "Optimal Asymmetric Encryption Padding",
      "use": "Encryption",
      "security": "High",
      "note": "Recommended for encrypting data. Adds random padding to prevent chosen-ciphertext attacks."
    },
    {
      "name": "RSA-PSS",
      "full_name": "Probabilistic Signature Scheme",
      "use": "Digital Signatures",
      "security": "High",
      "note": "Recommended for signing. Uses randomised padding; provably secure."
    },
    {
      "name": "RSA-PKCS1 v1.5",
      "full_name": "Public Key Cryptography Standards #1",
      "use": "Encryption & Signatures",
      "security": "Medium",
      "note": "Legacy standard. Still widely used in TLS but vulnerable to padding oracle attacks if misused."
    },
    {
      "name": "Raw RSA",
      "full_name": "Textbook RSA",
      "use": "Educational only",
      "security": "Low",
      "note": "No padding. Deterministic and malleable. Never use in production. Used here to teach the math."
    }
  ],
  "steps": [
    {
      "index": 1,
      "title": "Choose two distinct prime numbers p and q",
      "explanation": "Pick two distinct prime numbers p and q. Condition: p != q and both must be prime.",
      "condition": "p != q,  p and q are prime"
    },
    {
      "index": 2,
      "title": "Compute the modulus n = p x q",
      "explanation": "Multiply p and q to get n. This modulus appears in both the public and private key. Its size (in bits) is the RSA key size.",
      "condition": "n = p x q"
    },
    {
      "index": 3,
      "title": "Compute Euler totient: phi(n) = (p-1)(q-1)",
      "explanation": "phi(n) counts how many integers from 1 to n are coprime with n. For RSA this simplifies to (p-1)(q-1) because p and q are prime.",
      "condition": "phi(n) = (p - 1) x (q - 1)"
    },
    {
      "index": 4,
      "title": "Choose public exponent e",
      "explanation": "Pick e such that it is coprime with phi(n). Condition: 1 < e < phi(n) and gcd(e, phi(n)) = 1. Common choice is 65537.",
      "condition": "1 < e < phi(n)  and  gcd(e, phi(n)) = 1"
    },
    {
      "index": 5,
      "title": "Compute private exponent d",
      "explanation": "Find d as the modular inverse of e mod phi(n). Condition: (e x d) mod phi(n) = 1. The public and private keys are mathematical inverses — one encrypts what only the other can decrypt.",
      "condition": "(e x d) mod phi(n) = 1"
    },
    {
      "index": 6,
      "title": "Publish public key, keep private key secret",
      "explanation": "Public key = (e, n) — share freely. Private key = (d, n) — never share. Discard p, q, phi(n).",
      "condition": "Public: (e, n)   Private: (d, n)"
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
      { "label": "p = 3, q = 11",            "condition": "p != q, both prime", "note": "Two small primes chosen for illustration" },
      { "label": "n = 3 x 11 = 33",           "condition": "n = p x q",         "note": "The modulus used in both keys" },
      { "label": "phi = (3-1)(11-1) = 20",    "condition": "phi = (p-1)(q-1)",   "note": "Euler totient of n" },
      { "label": "e = 7",                      "condition": "gcd(7, 20) = 1, 1 < 7 < 20", "note": "7 is coprime with 20" },
      { "label": "d = 3",                      "condition": "(7 x 3) mod 20 = 1", "note": "21 mod 20 = 1 — confirmed" }
    ],
    "public_key":  { "e": 7, "n": 33 },
    "private_key": { "d": 3, "n": 33 },
    "verify": "Encrypt m=4: c = 4^7 mod 33 = 16384 mod 33 = 16. Decrypt: m = 16^3 mod 33 = 4096 mod 33 = 4."
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

namespace Education {

std::string certificateVerificationTheoryJson() {
    return R"JSON({
  "title": "How Bob Verifies a Certificate",
  "sections": [
    {
      "index": 1,
      "title": "Why Bob Cannot Trust a Public Key Directly",
      "body": "Anyone can claim to be Alice and publish a public key. Without a certificate, Bob has no way to know the key really belongs to Alice. A Certificate Authority (CA) solves this by signing Alice's public key, creating a verifiable bond between her identity and her key."
    },
    {
      "index": 2,
      "title": "The Verification Formula",
      "body": "Bob uses the CA's public key (e_CA, n_CA) which he already trusts. He computes: recovered = ca_signature ^ e_CA mod n_CA. If recovered equals the cert_hash stored in the certificate, the certificate is authentic — it was definitely signed by the CA and has not been altered."
    },
    {
      "index": 3,
      "title": "What a Passing Check Means",
      "body": "A valid CA signature proves two things: (1) The CA vouched for Alice's identity, and (2) The certificate has not been tampered with since the CA signed it. Bob can now trust that the public key inside the certificate truly belongs to Alice."
    },
    {
      "index": 4,
      "title": "What a Failing Check Means",
      "body": "If recovered != cert_hash, the certificate is invalid. Either it was not signed by the trusted CA (self-signed or forged), or it was modified after signing. Bob must reject it and stop — using an untrusted certificate is a serious security risk."
    }
  ]
})JSON";
}

std::string signatureVerificationTheoryJson() {
    return R"JSON({
  "title": "How Bob Verifies Alice's Signature",
  "sections": [
    {
      "index": 1,
      "title": "What a Digital Signature Proves",
      "body": "A digital signature proves two things: (1) Authenticity — the document came from Alice because only she has her private key. (2) Integrity — the document has not changed since Alice signed it, because any change produces a different hash."
    },
    {
      "index": 2,
      "title": "The Verification Steps",
      "body": "Step 1: Bob computes SHA-256 of the document he received. Step 2: Bob decrypts Alice's signature using her public key: recovered = signature ^ e_Alice mod n_Alice. Step 3: Bob compares the recovered integer with his computed hash (reduced mod n). If they match, the signature is valid."
    },
    {
      "index": 3,
      "title": "Why This is Secure",
      "body": "Only Alice can produce a signature that decrypts (with her public key) to the correct hash. An attacker without Alice's private key cannot forge a signature. Any change to the document changes the SHA-256 hash, making the comparison fail and exposing tampering."
    },
    {
      "index": 4,
      "title": "The Combined Security Guarantee",
      "body": "Certificate verification + signature verification together give Bob a strong guarantee: the document came from the real Alice (cert verified), and the document is exactly as she sent it (signature verified). This is the foundation of PKI security."
    }
  ]
})JSON";
}

} // namespace Education
