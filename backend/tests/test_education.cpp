#include <gtest/gtest.h>
#include "Education.h"
#include <string>

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// ── rsaTheoryJson structure ───────────────────────────────────────────────────

TEST(Education, JsonContainsSteps) {
    std::string json = Education::rsaTheoryJson();
    EXPECT_TRUE(contains(json, "\"steps\""));
    EXPECT_TRUE(contains(json, "\"title\""));
    EXPECT_TRUE(contains(json, "\"explanation\""));
}

TEST(Education, JsonContainsSixSteps) {
    std::string json = Education::rsaTheoryJson();
    // Each step has an "index" field 1-6
    EXPECT_TRUE(contains(json, "\"index\": 1"));
    EXPECT_TRUE(contains(json, "\"index\": 2"));
    EXPECT_TRUE(contains(json, "\"index\": 3"));
    EXPECT_TRUE(contains(json, "\"index\": 4"));
    EXPECT_TRUE(contains(json, "\"index\": 5"));
    EXPECT_TRUE(contains(json, "\"index\": 6"));
}

TEST(Education, JsonContainsExample) {
    std::string json = Education::rsaTheoryJson();
    EXPECT_TRUE(contains(json, "\"example\""));
    EXPECT_TRUE(contains(json, "\"p\": 3"));
    EXPECT_TRUE(contains(json, "\"q\": 11"));
    EXPECT_TRUE(contains(json, "\"n\": 33"));
    EXPECT_TRUE(contains(json, "\"phi\": 20"));
    EXPECT_TRUE(contains(json, "\"e\": 7"));
    EXPECT_TRUE(contains(json, "\"d\": 3"));
}

TEST(Education, JsonContainsPublicAndPrivateKey) {
    std::string json = Education::rsaTheoryJson();
    EXPECT_TRUE(contains(json, "\"public_key\""));
    EXPECT_TRUE(contains(json, "\"private_key\""));
}

TEST(Education, JsonContainsVerifyString) {
    std::string json = Education::rsaTheoryJson();
    // Verify the worked example proof string is present
    EXPECT_TRUE(contains(json, "\"verify\""));
    EXPECT_TRUE(contains(json, "4^7 mod 33"));
}
