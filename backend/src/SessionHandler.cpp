#include "SessionHandler.h"
#include "RSA.h"
#include <sstream>

static const std::vector<std::string> ACTORS = {"Alice", "Bob", "CA", "Fake-Alice"};

/**
 * @brief Generate key pairs for all four actors and persist server-side.
 *
 * Only public keys (n, e) are returned in the JSON response.
 * Private keys (d) are stored in the SessionStore and never serialised.
 */
std::string handleSessionInit(const std::string& sessionId, int bits) {
    std::vector<ActorKeyPair> pairs;
    pairs.reserve(ACTORS.size());

    for (const auto& name : ACTORS) {
        RSAKeyPair kp = RSACrypto::generateKeypair(bits);
        pairs.push_back({name, kp, bits});
    }

    SessionStore::instance().store(sessionId, pairs);
    auto pubKeys = SessionStore::instance().getPublicKeys(sessionId);
    return publicKeysToJson(sessionId, pubKeys);
}

std::string publicKeysToJson(const std::string& sessionId,
                              const std::vector<ActorPublicKey>& keys) {
    std::ostringstream out;
    out << "{\n  \"session_id\": \"" << sessionId << "\",\n";
    out << "  \"actors\": [\n";

    for (size_t i = 0; i < keys.size(); ++i) {
        const auto& k = keys[i];
        out << "    {\n";
        out << "      \"name\": \"" << k.name << "\",\n";
        out << "      \"public_key\": {\n";
        out << "        \"n\": \"" << k.n << "\",\n";
        out << "        \"e\": \"" << k.e << "\"\n";
        out << "      },\n";
        out << "      \"bits\": " << k.bits << "\n";
        out << "    }";
        if (i + 1 < keys.size()) out << ",";
        out << "\n";
    }

    out << "  ]\n}";
    return out.str();
}
