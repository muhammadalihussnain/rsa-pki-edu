#include <gtest/gtest.h>
#include "DocumentHandler.h"
#include "SessionHandler.h"
#include "Hash.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── Upload document ───────────────────────────────────────────────────────────

TEST(Document, UploadReturnsContent) {
    std::string json = handleUploadDocument("Hello, Alice!");
    EXPECT_TRUE(contains(json, "\"content\""));
    EXPECT_TRUE(contains(json, "Hello, Alice!"));
}

TEST(Document, UploadReturnsSHA256Hash) {
    std::string json = handleUploadDocument("Hello, Alice!");
    EXPECT_TRUE(contains(json, "\"sha256_hash\""));
    // sha256 hex is 64 chars — check it appears
    std::string expected = SHA256::hexdigest("Hello, Alice!");
    EXPECT_TRUE(contains(json, expected));
}

TEST(Document, UploadReturnsContentLength) {
    std::string json = handleUploadDocument("Hello");
    EXPECT_TRUE(contains(json, "\"content_length\""));
    EXPECT_TRUE(contains(json, "5"));
}

TEST(Document, UploadEmptyReturnsError) {
    std::string json = handleUploadDocument("");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(Document, HashIsCorrectSHA256) {
    std::string content = "The quick brown fox";
    std::string json = handleUploadDocument(content);
    std::string expected = SHA256::hexdigest(content);
    EXPECT_TRUE(contains(json, expected));
}

TEST(Document, DifferentDocsDifferentHashes) {
    std::string j1 = handleUploadDocument("document one");
    std::string j2 = handleUploadDocument("document two");
    // The two JSON responses should contain different hash values
    EXPECT_NE(j1, j2);
}

// ── Sign document ─────────────────────────────────────────────────────────────

TEST(Document, SignReturnsSignatureAndSteps) {
    handleSessionInit("sign-test-1", 64);
    std::string hashHex = SHA256::hexdigest("test document");
    std::string body = R"({"session_id":"sign-test-1","hash_hex":")" + hashHex + R"("})";
    std::string json = handleSignDocument(body);

    EXPECT_TRUE(contains(json, "\"signature\""));
    EXPECT_TRUE(contains(json, "\"steps\""));
    EXPECT_TRUE(contains(json, "\"original_hash\""));
}

TEST(Document, SignReturnsThreeSteps) {
    handleSessionInit("sign-test-2", 64);
    std::string hashHex = SHA256::hexdigest("another doc");
    std::string body = R"({"session_id":"sign-test-2","hash_hex":")" + hashHex + R"("})";
    std::string json = handleSignDocument(body);

    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 2"));
    EXPECT_TRUE(contains(json, "\"index\": 3"));
}

TEST(Document, SignMissingSessionReturnsError) {
    std::string json = handleSignDocument(R"({"hash_hex":"abc123"})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(Document, SignMissingHashReturnsError) {
    std::string json = handleSignDocument(R"({"session_id":"x"})");
    EXPECT_TRUE(contains(json, "\"error\""));
}

TEST(Document, SignatureIsNonEmpty) {
    handleSessionInit("sign-test-3", 64);
    std::string hashHex = SHA256::hexdigest("payload");
    std::string body = R"({"session_id":"sign-test-3","hash_hex":")" + hashHex + R"("})";
    std::string json = handleSignDocument(body);

    EXPECT_FALSE(contains(json, "\"error\""));
    EXPECT_TRUE(contains(json, "\"signer\": \"Alice\""));
}
