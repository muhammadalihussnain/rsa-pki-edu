#include <gtest/gtest.h>
#include "Certificate.h"
#include "CAHandler.h"
#include "SessionHandler.h"
#include "Education.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── Certificate issuance ──────────────────────────────────────────────────────

TEST(Certificate, IssuedCertHasCorrectSubjectAndIssuer) {
    handleSessionInit("cert-test-1", 64);
    RSAKeyPair aliceKeys;
    SessionStore::instance().getActorKeys("cert-test-1", "Alice", aliceKeys);

    bool ok = false;
    Certificate cert = CertificateAuthority::issueCertificate(
        "cert-test-1",
        "Alice",
        aliceKeys.n.toString(),
        aliceKeys.e.toString(),
        "0001",
        ok
    );

    EXPECT_TRUE(ok);
    EXPECT_EQ(cert.subject, "Alice");
    EXPECT_EQ(cert.issuer,  "CA");
}

TEST(Certificate, IssuedCertHasAllFields) {
    handleSessionInit("cert-test-2", 64);
    RSAKeyPair aliceKeys;
    SessionStore::instance().getActorKeys("cert-test-2", "Alice", aliceKeys);

    bool ok = false;
    Certificate cert = CertificateAuthority::issueCertificate(
        "cert-test-2",
        "Alice",
        aliceKeys.n.toString(),
        aliceKeys.e.toString(),
        "0002",
        ok
    );

    EXPECT_TRUE(ok);
    EXPECT_FALSE(cert.serial.empty());
    EXPECT_FALSE(cert.validFrom.empty());
    EXPECT_FALSE(cert.validTo.empty());
    EXPECT_FALSE(cert.caSignature.empty());
    EXPECT_FALSE(cert.certHash.empty());
    EXPECT_FALSE(cert.publicKeyN.empty());
    EXPECT_FALSE(cert.publicKeyE.empty());
}

TEST(Certificate, SignatureIsValid) {
    handleSessionInit("cert-test-3", 64);
    RSAKeyPair aliceKeys;
    SessionStore::instance().getActorKeys("cert-test-3", "Alice", aliceKeys);
    RSAKeyPair caKeys;
    SessionStore::instance().getActorKeys("cert-test-3", "CA", caKeys);

    bool ok = false;
    Certificate cert = CertificateAuthority::issueCertificate(
        "cert-test-3",
        "Alice",
        aliceKeys.n.toString(),
        aliceKeys.e.toString(),
        "0003",
        ok
    );
    ASSERT_TRUE(ok);

    bool valid = CertificateAuthority::verifyCertificate(
        cert, caKeys.n.toString(), caKeys.e.toString());
    EXPECT_TRUE(valid);
}

TEST(Certificate, TamperedSignatureFailsVerification) {
    handleSessionInit("cert-test-4", 64);
    RSAKeyPair aliceKeys;
    SessionStore::instance().getActorKeys("cert-test-4", "Alice", aliceKeys);
    RSAKeyPair caKeys;
    SessionStore::instance().getActorKeys("cert-test-4", "CA", caKeys);

    bool ok = false;
    Certificate cert = CertificateAuthority::issueCertificate(
        "cert-test-4",
        "Alice",
        aliceKeys.n.toString(),
        aliceKeys.e.toString(),
        "0004",
        ok
    );
    ASSERT_TRUE(ok);

    // Tamper with the signature
    cert.caSignature = "1";
    bool valid = CertificateAuthority::verifyCertificate(
        cert, caKeys.n.toString(), caKeys.e.toString());
    EXPECT_FALSE(valid);
}

TEST(Certificate, UnknownSessionReturnsFalse) {
    bool ok = true;
    CertificateAuthority::issueCertificate(
        "nonexistent-session", "Alice", "123", "65537", "0001", ok);
    EXPECT_FALSE(ok);
}

// ── CAHandler JSON output ─────────────────────────────────────────────────────

TEST(CAHandler, IssueCertificateJsonHasTwoPackets) {
    handleSessionInit("handler-test-1", 64);
    std::string body = R"({"session_id":"handler-test-1","subject":"Alice"})";
    std::string json = handleIssueCertificate(body);

    EXPECT_TRUE(contains(json, "\"packets\""));
    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 2"));
    EXPECT_TRUE(contains(json, "CertificateRequest"));
    EXPECT_TRUE(contains(json, "\"Certificate\""));
}

TEST(CAHandler, IssueCertificateJsonHasCertificateField) {
    handleSessionInit("handler-test-2", 64);
    std::string body = R"({"session_id":"handler-test-2","subject":"Alice"})";
    std::string json = handleIssueCertificate(body);

    EXPECT_TRUE(contains(json, "\"certificate\""));
    EXPECT_TRUE(contains(json, "\"subject\": \"Alice\""));
    EXPECT_TRUE(contains(json, "\"issuer\": \"CA\""));
    EXPECT_TRUE(contains(json, "\"ca_signature\""));
}

TEST(CAHandler, MissingSessionIdReturnsError) {
    std::string json = handleIssueCertificate(R"({"subject":"Alice"})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(CAHandler, MissingSubjectReturnsError) {
    std::string json = handleIssueCertificate(R"({"session_id":"some-id"})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

// ── Certificate theory content ────────────────────────────────────────────────

TEST(CertificateTheory, JsonContainsSections) {
    std::string json = Education::certificateTheoryJson();
    EXPECT_TRUE(contains(json, "\"sections\""));
    EXPECT_TRUE(contains(json, "\"title\""));
    EXPECT_TRUE(contains(json, "\"body\""));
}

TEST(CertificateTheory, JsonContainsFourSections) {
    std::string json = Education::certificateTheoryJson();
    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 2"));
    EXPECT_TRUE(contains(json, "\"index\": 3"));
    EXPECT_TRUE(contains(json, "\"index\": 4"));
}

TEST(CertificateTheory, JsonContainsMockCertificate) {
    std::string json = Education::certificateTheoryJson();
    EXPECT_TRUE(contains(json, "\"mock_certificate\""));
    EXPECT_TRUE(contains(json, "\"subject\""));
    EXPECT_TRUE(contains(json, "\"issuer\""));
    EXPECT_TRUE(contains(json, "\"ca_signature\""));
    EXPECT_TRUE(contains(json, "\"field_explanations\""));
}
