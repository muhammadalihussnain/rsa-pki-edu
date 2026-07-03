#include <gtest/gtest.h>
#include "AttackerHandler.h"
#include "BobHandler.h"
#include "SessionHandler.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── Fake-Alice attempt ────────────────────────────────────────────────────────

TEST(Attacker, ForgedCertReturnsAllFields) {
    handleSessionInit("atk-1", 64);
    std::string json = handleFakeAliceAttempt(
        R"({"session_id":"atk-1","document":"Hello","scenario":"forged_cert"})");

    EXPECT_TRUE(contains(json, "\"attacker\": \"Fake-Alice\""));
    EXPECT_TRUE(contains(json, "\"scenario\": \"forged_cert\""));
    EXPECT_TRUE(contains(json, "\"fake_certificate\""));
    EXPECT_TRUE(contains(json, "\"fake_signature\""));
    EXPECT_TRUE(contains(json, "\"hash_hex\""));
    EXPECT_TRUE(contains(json, "\"educational_callout\""));
}

TEST(Attacker, TamperedDocReturnsModifiedDocument) {
    handleSessionInit("atk-2", 64);
    std::string json = handleFakeAliceAttempt(
        R"({"session_id":"atk-2","document":"Original","scenario":"tampered_doc"})");

    EXPECT_TRUE(contains(json, "\"scenario\": \"tampered_doc\""));
    EXPECT_TRUE(contains(json, "MODIFIED BY ATTACKER"));
}

TEST(Attacker, FakeCertClaimsToBeAlice) {
    handleSessionInit("atk-3", 64);
    std::string json = handleFakeAliceAttempt(
        R"({"session_id":"atk-3","document":"Test","scenario":"forged_cert"})");

    // Fake cert claims subject is Alice but is self-signed
    EXPECT_TRUE(contains(json, "\"subject\": \"Alice\""));
    EXPECT_TRUE(contains(json, "\"issuer\": \"CA\""));
}

TEST(Attacker, DefaultScenarioIsForgedCert) {
    handleSessionInit("atk-4", 64);
    std::string json = handleFakeAliceAttempt(
        R"({"session_id":"atk-4","document":"Test"})");

    EXPECT_TRUE(contains(json, "\"scenario\": \"forged_cert\""));
}

TEST(Attacker, MissingSessionReturnsError) {
    std::string json = handleFakeAliceAttempt(R"({"document":"Test"})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

// ── Bob rejects forged cert (Step 1 fails) ───────────────────────────────────

TEST(Attacker, BobRejectsForgedCert) {
    handleSessionInit("atk-bob-1", 64);

    // Get fake-alice's attempt
    std::string attackJson = handleFakeAliceAttempt(
        R"({"session_id":"atk-bob-1","document":"Hello Bob","scenario":"forged_cert"})");

    // Extract fields from attackJson for bob/verify
    auto getStr = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\": \"";
        auto pos = attackJson.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        auto end = attackJson.find('"', pos);
        return attackJson.substr(pos, end - pos);
    };

    std::string hashHex   = getStr("hash_hex");
    std::string fakeSig   = getStr("fake_signature");

    // Extract fake_certificate block
    std::string certBlock;
    auto certPos = attackJson.find("\"fake_certificate\":");
    if (certPos != std::string::npos) {
        certPos += std::string("\"fake_certificate\":").size();
        while (certPos < attackJson.size() &&
               (attackJson[certPos] == ' ' || attackJson[certPos] == '\n')) certPos++;
        int depth = 0; size_t start = certPos;
        for (size_t i = certPos; i < attackJson.size(); i++) {
            if (attackJson[i] == '{') depth++;
            else if (attackJson[i] == '}') { depth--; if (depth == 0) { certBlock = attackJson.substr(start, i - start + 1); break; } }
        }
    }

    std::string bobBody = R"({"session_id":"atk-bob-1",)"
                          R"("document":"Hello Bob",)"
                          R"("hash_hex":")" + hashHex + R"(",)"
                          R"("signature":")" + fakeSig + R"(",)"
                          R"("certificate":)" + certBlock + R"(})";

    std::string bobResult = handleBobVerify(bobBody);

    EXPECT_TRUE(contains(bobResult, "\"verified\": false"));
    EXPECT_TRUE(contains(bobResult, "\"failure_step\": 1"));
}

// ── Bob rejects tampered document (Step 5 fails) ─────────────────────────────

TEST(Attacker, BobRejectsTamperedDocument) {
    handleSessionInit("atk-bob-2", 64);

    std::string attackJson = handleFakeAliceAttempt(
        R"({"session_id":"atk-bob-2","document":"Original doc","scenario":"tampered_doc"})");

    auto getStr = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\": \"";
        auto pos = attackJson.find(search);
        if (pos == std::string::npos) return "";
        pos += search.size();
        auto end = attackJson.find('"', pos);
        return attackJson.substr(pos, end - pos);
    };

    std::string hashHex = getStr("hash_hex");
    std::string fakeSig = getStr("fake_signature");

    std::string certBlock;
    auto certPos = attackJson.find("\"fake_certificate\":");
    if (certPos != std::string::npos) {
        certPos += std::string("\"fake_certificate\":").size();
        while (certPos < attackJson.size() &&
               (attackJson[certPos] == ' ' || attackJson[certPos] == '\n')) certPos++;
        int depth = 0; size_t start = certPos;
        for (size_t i = certPos; i < attackJson.size(); i++) {
            if (attackJson[i] == '{') depth++;
            else if (attackJson[i] == '}') { depth--; if (depth == 0) { certBlock = attackJson.substr(start, i - start + 1); break; } }
        }
    }

    std::string bobBody = R"({"session_id":"atk-bob-2",)"
                          R"("document":"Original doc",)"
                          R"("hash_hex":")" + hashHex + R"(",)"
                          R"("signature":")" + fakeSig + R"(",)"
                          R"("certificate":)" + certBlock + R"(})";

    std::string bobResult = handleBobVerify(bobBody);
    EXPECT_TRUE(contains(bobResult, "\"verified\": false"));
}
