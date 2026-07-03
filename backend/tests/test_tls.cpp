#include <gtest/gtest.h>
#include "TLSHandler.h"
#include "HMAC.h"
#include "Hash.h"
#include "SessionHandler.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── HMAC-SHA256 ───────────────────────────────────────────────────────────────

TEST(HMAC, OutputIs64HexChars) {
    EXPECT_EQ(HMAC_SHA256::hexdigest("key", "message").size(), 64u);
}

TEST(HMAC, Deterministic) {
    EXPECT_EQ(HMAC_SHA256::hexdigest("k", "m"), HMAC_SHA256::hexdigest("k", "m"));
}

TEST(HMAC, DifferentKeysDifferentOutput) {
    EXPECT_NE(HMAC_SHA256::hexdigest("key1", "msg"), HMAC_SHA256::hexdigest("key2", "msg"));
}

TEST(HMAC, DifferentMessagesDifferentOutput) {
    EXPECT_NE(HMAC_SHA256::hexdigest("key", "msg1"), HMAC_SHA256::hexdigest("key", "msg2"));
}

TEST(HMAC, LongKey) {
    // Key longer than 64 bytes should be hashed first
    std::string longKey(100, 'k');
    EXPECT_EQ(HMAC_SHA256::hexdigest(longKey, "message").size(), 64u);
}

// ── TLS Handshake — normal (Alice) ───────────────────────────────────────────

TEST(TLS, NormalHandshakeSucceeds) {
    handleSessionInit("tls-normal-1", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-normal-1","inject_fake_alice":false})");
    EXPECT_TRUE(contains(json, "\"success\": true"));
}

TEST(TLS, NormalHandshakeHasSixPackets) {
    handleSessionInit("tls-normal-2", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-normal-2"})");
    // Check packet indices 1-6
    for (int i = 1; i <= 6; i++) {
        EXPECT_TRUE(contains(json, "\"index\": " + std::to_string(i)));
    }
}

TEST(TLS, NormalHandshakeHasExpectedMessageTypes) {
    handleSessionInit("tls-normal-3", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-normal-3"})");
    EXPECT_TRUE(contains(json, "\"ClientHello\""));
    EXPECT_TRUE(contains(json, "\"ServerHello\""));
    EXPECT_TRUE(contains(json, "\"CertificateVerify\""));
    EXPECT_TRUE(contains(json, "\"ClientKeyExchange\""));
    EXPECT_TRUE(contains(json, "Finished"));
}

TEST(TLS, NormalHandshakeAllPacketsOk) {
    handleSessionInit("tls-normal-4", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-normal-4"})");
    // All packets should have "ok": true
    EXPECT_FALSE(contains(json, "\"ok\": false"));
}

// ── TLS Handshake — Fake-Alice injection ────────────────────────────────────

TEST(TLS, FakeAliceInjectionFails) {
    handleSessionInit("tls-fake-1", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-fake-1","inject_fake_alice":true})");
    EXPECT_TRUE(contains(json, "\"success\": false"));
}

TEST(TLS, FakeAliceInjectionShowsAlert) {
    handleSessionInit("tls-fake-2", 64);
    std::string json = handleTLSHandshake(
        R"({"session_id":"tls-fake-2","inject_fake_alice":true})");
    EXPECT_TRUE(contains(json, "\"Alert\""));
    EXPECT_TRUE(contains(json, "\"failure_reason\""));
}

TEST(TLS, MissingSessionIdReturnsError) {
    std::string json = handleTLSHandshake(R"({})");
    EXPECT_TRUE(contains(json, "\"error\""));
}
