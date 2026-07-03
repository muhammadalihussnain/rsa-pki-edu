#pragma once
#include "RSA.h"
#include <string>
#include <unordered_map>
#include <mutex>

/**
 * @brief Public view of one actor's RSA key (safe to send to client).
 */
struct ActorPublicKey {
    std::string name;
    std::string n;   ///< Modulus as decimal string
    std::string e;   ///< Public exponent as decimal string
    int bits;        ///< Nominal key size in bits
};

/**
 * @brief Full key pair stored server-side (private key never leaves).
 */
struct ActorKeyPair {
    std::string name;
    RSAKeyPair  keys;
    int         bits;
};

/**
 * @brief In-memory session store.
 *
 * Holds one set of actor key pairs per session ID.
 * Thread-safe via mutex.
 */
class SessionStore {
public:
    static SessionStore& instance() {
        static SessionStore inst;
        return inst;
    }

    /// Store a set of actor key pairs under a session ID
    void store(const std::string& sessionId,
               std::vector<ActorKeyPair> pairs);

    /// Retrieve all public keys for a session (returns empty if not found)
    std::vector<ActorPublicKey> getPublicKeys(const std::string& sessionId) const;

    /// Get a specific actor's full key pair (for server-side crypto operations)
    bool getActorKeys(const std::string& sessionId,
                      const std::string& actorName,
                      RSAKeyPair& out) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<ActorKeyPair>> sessions_;
};
