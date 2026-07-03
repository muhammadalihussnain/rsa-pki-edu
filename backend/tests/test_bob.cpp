#include <gtest/gtest.h>
#include "BobHandler.h"
#include "CAHandler.h"
#include "DocumentHandler.h"
#include "SessionHandler.h"
#include "Education.h"
#include "Hash.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── Helper: set up a full session and produce a signed document ───────────────

struct SignedDocBundle {
    std::string sessionId;
    std::string document;
    std::string signature;
    std::string certJson; // the certificate JSON block
};

static SignedDocBundle makeBundle(const std::string& sid, const std::string& doc) {
    handleSessionInit(sid, 64);

    // Issue cert for Alice
    std::string issuedJson = handleIssueCertificate(
        R"({"session_id":")" + sid + R"(","subject":"Alice"})");

    // Extract certificate block (everything after "certificate": )
    auto certPos = issuedJson.find("\"certificate\":");
    std::string certBlock = "";
    if (certPos != std::string::npos) {
        certPos += std::string("\"certificate\":").size();
        while (certPos < issuedJson.size() &&
               (issuedJson[certPos] == ' ' || issuedJson[certPos] == '\n')) certPos++;
        // find matching brace
        int depth = 0;
        size_t start = certPos;
        for (size_t i = certPos; i < issuedJson.size(); i++) {
            if (issuedJson[i] == '{') depth++;
            else if (issuedJson[i] == '}') { depth--; if (depth == 0) { certBlock = issuedJson.substr(start, i - start + 1); break; } }
        }
    }

    // Sign the document
    std::string hashHex = SHA256::hexdigest(doc);
    std::string signJson = handleSignDocument(
        R"({"session_id":")" + sid + R"(","hash_hex":")" + hashHex + R"("})");

    // Extract signature value
    std::string sig;
    auto sigPos = signJson.find("\"signature\": \"");
    if (sigPos != std::string::npos) {
        sigPos += std::string("\"signature\": \"").size();
        auto end = signJson.find('"', sigPos);
        sig = signJson.substr(sigPos, end - sigPos);
    }

    return {sid, doc, sig, certBlock};
}

// ── Bob verify: happy path ─────────────────────────────────────────────────────

TEST(BobVerify, SuccessfulVerification) {
    auto b = makeBundle("bob-happy-1", "Hello from Alice");
    std::string body = R"({"session_id":")" + b.sessionId + R"(",)"
                       R"("document":"Hello from Alice",)"
                       R"("signature":")" + b.signature + R"(",)"
                       R"("certificate":)" + b.certJson + R"(})";
    std::string json = handleBobVerify(body);

    EXPECT_TRUE(contains(json, "\"verified\": true"));
    EXPECT_TRUE(contains(json, "\"steps\""));
}

TEST(BobVerify, ReturnsAllFiveSteps) {
    auto b = makeBundle("bob-steps-1", "Test document content");
    std::string body = R"({"session_id":")" + b.sessionId + R"(",)"
                       R"("document":"Test document content",)"
                       R"("signature":")" + b.signature + R"(",)"
                       R"("certificate":)" + b.certJson + R"(})";
    std::string json = handleBobVerify(body);

    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 2"));
    EXPECT_TRUE(contains(json, "\"index\": 3"));
    EXPECT_TRUE(contains(json, "\"index\": 4"));
    EXPECT_TRUE(contains(json, "\"index\": 5"));
}

TEST(BobVerify, AllStepsPassOnValidData) {
    auto b = makeBundle("bob-allpass-1", "Valid document");
    std::string body = R"({"session_id":")" + b.sessionId + R"(",)"
                       R"("document":"Valid document",)"
                       R"("signature":")" + b.signature + R"(",)"
                       R"("certificate":)" + b.certJson + R"(})";
    std::string json = handleBobVerify(body);

    // All 5 steps should report "passed": true
    // Count occurrences of "passed": true
    size_t count = 0, pos = 0;
    while ((pos = json.find("\"passed\": true", pos)) != std::string::npos) { count++; pos++; }
    EXPECT_EQ(count, 5u);
}

// ── Bob verify: tampered document ────────────────────────────────────────────

TEST(BobVerify, TamperedDocumentFails) {
    auto b = makeBundle("bob-tamper-1", "Original document");
    // Send a different document but the same signature
    std::string body = R"({"session_id":")" + b.sessionId + R"(",)"
                       R"("document":"TAMPERED document",)"
                       R"("signature":")" + b.signature + R"(",)"
                       R"("certificate":)" + b.certJson + R"(})";
    std::string json = handleBobVerify(body);

    EXPECT_TRUE(contains(json, "\"verified\": false"));
    EXPECT_TRUE(contains(json, "\"failure_step\": 5"));
}

// ── Bob verify: missing fields ────────────────────────────────────────────────

TEST(BobVerify, MissingSessionIdReturnsError) {
    std::string json = handleBobVerify(R"({"document":"x","signature":"1","certificate":{}})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(BobVerify, MissingDocumentReturnsError) {
    std::string json = handleBobVerify(R"({"session_id":"x","signature":"1","certificate":{}})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(BobVerify, MissingSignatureReturnsError) {
    std::string json = handleBobVerify(R"({"session_id":"x","document":"x","certificate":{}})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

// ── Education theory endpoints ────────────────────────────────────────────────

TEST(BobEducation, CertVerificationTheoryHasSections) {
    std::string json = Education::certificateVerificationTheoryJson();
    EXPECT_TRUE(contains(json, "\"sections\""));
    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 4"));
    EXPECT_TRUE(contains(json, "\"title\""));
    EXPECT_TRUE(contains(json, "\"body\""));
}

TEST(BobEducation, SigVerificationTheoryHasSections) {
    std::string json = Education::signatureVerificationTheoryJson();
    EXPECT_TRUE(contains(json, "\"sections\""));
    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 4"));
    EXPECT_TRUE(contains(json, "SHA-256"));
}
