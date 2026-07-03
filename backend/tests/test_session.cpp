#include <gtest/gtest.h>
#include "SessionHandler.h"
#include "Session.h"
#include <string>

static bool contains(const std::string& h, const std::string& n) {
    return h.find(n) != std::string::npos;
}

// ── handleSessionInit ─────────────────────────────────────────────────────────

TEST(Session, InitReturnsAllFourActors) {
    std::string json = handleSessionInit("test-sess-1", 64);
    EXPECT_TRUE(contains(json, "\"Alice\""));
    EXPECT_TRUE(contains(json, "\"Bob\""));
    EXPECT_TRUE(contains(json, "\"CA\""));
    EXPECT_TRUE(contains(json, "\"Fake-Alice\""));
}

TEST(Session, InitReturnsSessionId) {
    std::string json = handleSessionInit("test-sess-2", 64);
    EXPECT_TRUE(contains(json, "\"session_id\""));
    EXPECT_TRUE(contains(json, "test-sess-2"));
}

TEST(Session, InitReturnsPublicKeyFields) {
    std::string json = handleSessionInit("test-sess-3", 64);
    EXPECT_TRUE(contains(json, "\"public_key\""));
    EXPECT_TRUE(contains(json, "\"n\""));
    EXPECT_TRUE(contains(json, "\"e\""));
    EXPECT_TRUE(contains(json, "\"bits\""));
}

TEST(Session, InitNeverExposesPrivateKey) {
    std::string json = handleSessionInit("test-sess-4", 64);
    // The letter "d" as a standalone JSON key must not appear
    EXPECT_FALSE(contains(json, "\"d\""));
    EXPECT_FALSE(contains(json, "private"));
}

TEST(Session, InitKeyBitsCorrect) {
    std::string json = handleSessionInit("test-sess-5", 64);
    EXPECT_TRUE(contains(json, "\"bits\": 64"));
}

// ── SessionStore ──────────────────────────────────────────────────────────────

TEST(Session, StoreAndRetrievePublicKeys) {
    handleSessionInit("store-test", 64);
    auto keys = SessionStore::instance().getPublicKeys("store-test");
    EXPECT_EQ(keys.size(), 4u);
}

TEST(Session, ActorNamesCorrect) {
    handleSessionInit("names-test", 64);
    auto keys = SessionStore::instance().getPublicKeys("names-test");
    std::vector<std::string> names;
    for (const auto& k : keys) names.push_back(k.name);
    EXPECT_NE(std::find(names.begin(), names.end(), "Alice"),    names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "Bob"),      names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "CA"),       names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "Fake-Alice"), names.end());
}

TEST(Session, PrivateKeyRetrievableServerSide) {
    handleSessionInit("priv-test", 64);
    RSAKeyPair kp;
    bool found = SessionStore::instance().getActorKeys("priv-test", "Alice", kp);
    EXPECT_TRUE(found);
    EXPECT_FALSE(kp.d.isZero());
    EXPECT_FALSE(kp.n.isZero());
}

TEST(Session, UnknownSessionReturnsEmpty) {
    auto keys = SessionStore::instance().getPublicKeys("nonexistent-session");
    EXPECT_TRUE(keys.empty());
}

TEST(Session, UnknownActorReturnsFalse) {
    handleSessionInit("actor-test", 64);
    RSAKeyPair kp;
    bool found = SessionStore::instance().getActorKeys("actor-test", "Mallory", kp);
    EXPECT_FALSE(found);
}

// ── Security audit: encrypt/decrypt round-trip with stored keys ───────────────

TEST(Session, StoredKeysAreUsable) {
    handleSessionInit("usable-test", 64);
    RSAKeyPair kp;
    ASSERT_TRUE(SessionStore::instance().getActorKeys("usable-test", "Bob", kp));

    BigInteger msg(7ULL);
    BigInteger cipher    = RSACrypto::encrypt(msg, kp.e, kp.n);
    BigInteger recovered = RSACrypto::decrypt(cipher, kp.d, kp.n);
    EXPECT_EQ(recovered.toString(), "7");
}
