#pragma once
#include "RSA.h"
#include <string>

/**
 * @brief Represents an X.509-style digital certificate.
 *
 * Fields mirror real certificate structure for educational purposes.
 */
struct Certificate {
    std::string subject;        ///< Name of the certificate owner
    std::string issuer;         ///< Name of the CA that issued this cert
    std::string publicKeyN;     ///< Subject's RSA modulus (decimal string)
    std::string publicKeyE;     ///< Subject's RSA public exponent (decimal string)
    std::string serial;         ///< Unique serial number (hex string)
    std::string validFrom;      ///< Validity start (ISO date string)
    std::string validTo;        ///< Validity end (ISO date string)
    std::string caSignature;    ///< CA's RSA signature over the cert hash (decimal string)
    std::string certHash;       ///< Hash value that was signed (decimal string)
};

/**
 * @brief Certificate Authority operations.
 *
 * Uses the CA's RSA private key (stored in SessionStore) to sign certificates.
 * No pre-built crypto library is used — signing is done via BigInteger/RSACrypto.
 */
class CertificateAuthority {
public:
    /**
     * @brief Issue a certificate for a subject using the CA's session keys.
     *
     * Computes hash = simple_hash(subject + publicKeyN + serial),
     * then signs it: signature = hash^d_ca mod n_ca.
     *
     * @param sessionId      Active session (CA keys must exist)
     * @param subject        Name of the entity being certified
     * @param subjectKeyN    Subject's RSA modulus as decimal string
     * @param subjectKeyE    Subject's RSA public exponent as decimal string
     * @param serial         Serial number string
     * @param ok             Set to false if CA keys not found
     * @return Populated Certificate (caSignature will be empty if ok=false)
     */
    static Certificate issueCertificate(const std::string& sessionId,
                                        const std::string& subject,
                                        const std::string& subjectKeyN,
                                        const std::string& subjectKeyE,
                                        const std::string& serial,
                                        bool& ok);

    /**
     * @brief Verify a certificate's CA signature.
     *
     * Recovers: recovered = caSignature^e_ca mod n_ca
     * Checks:   recovered == certHash
     *
     * @param cert       The certificate to verify
     * @param caKeyN     CA's public modulus (decimal string)
     * @param caKeyE     CA's public exponent (decimal string)
     * @return true if signature is valid
     */
    static bool verifyCertificate(const Certificate& cert,
                                  const std::string& caKeyN,
                                  const std::string& caKeyE);

    /**
     * @brief Compute the educational hash of a certificate's fields.
     *
     * hash = (sum of char values in subject+n+serial) mod n_ca
     * Simple enough to follow manually; not cryptographically secure.
     */
    static BigInteger computeCertHash(const std::string& subject,
                                      const std::string& pubKeyN,
                                      const std::string& serial,
                                      const BigInteger& modulus);
};
