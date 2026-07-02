#include "Certificate.h"
#include "Session.h"
#include <sstream>
#include <iomanip>
#include <ctime>

// ── Internal helpers ──────────────────────────────────────────────────────────

/// Simple ISO-8601 date string for the current time ± offset days
static std::string isoDate(int offsetDays = 0) {
    std::time_t t = std::time(nullptr) + static_cast<std::time_t>(offsetDays) * 86400;
    std::tm* tm = std::gmtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

// ── CertificateAuthority ──────────────────────────────────────────────────────

BigInteger CertificateAuthority::computeCertHash(const std::string& subject,
                                                  const std::string& pubKeyN,
                                                  const std::string& serial,
                                                  const BigInteger& modulus) {
    // Educational hash: sum all character values in the combined string,
    // then take mod n so the result fits for RSA signing.
    std::string combined = subject + pubKeyN + serial;
    uint64_t sum = 0;
    for (unsigned char c : combined) {
        sum += static_cast<uint64_t>(c);
    }
    // Add a multiplier to spread the value and avoid trivially small hashes
    sum = sum * 31u + 1u;
    BigInteger h(sum);
    // Reduce mod n so h < n (required for RSA)
    return h % modulus;
}

Certificate CertificateAuthority::issueCertificate(const std::string& sessionId,
                                                    const std::string& subject,
                                                    const std::string& subjectKeyN,
                                                    const std::string& subjectKeyE,
                                                    const std::string& serial,
                                                    bool& ok) {
    RSAKeyPair caKeys;
    if (!SessionStore::instance().getActorKeys(sessionId, "CA", caKeys)) {
        ok = false;
        return {};
    }
    ok = true;

    BigInteger hash = computeCertHash(subject, subjectKeyN, serial, caKeys.n);
    BigInteger sig  = RSACrypto::sign(hash, caKeys.d, caKeys.n);

    Certificate cert;
    cert.subject     = subject;
    cert.issuer      = "CA";
    cert.publicKeyN  = subjectKeyN;
    cert.publicKeyE  = subjectKeyE;
    cert.serial      = serial;
    cert.validFrom   = isoDate(0);
    cert.validTo     = isoDate(365);
    cert.certHash    = hash.toString();
    cert.caSignature = sig.toString();
    return cert;
}

bool CertificateAuthority::verifyCertificate(const Certificate& cert,
                                              const std::string& caKeyN,
                                              const std::string& caKeyE) {
    BigInteger n(caKeyN);
    BigInteger e(caKeyE);
    BigInteger sig(cert.caSignature);

    BigInteger recovered = RSACrypto::verify(sig, e, n);
    return recovered.toString() == cert.certHash;
}
