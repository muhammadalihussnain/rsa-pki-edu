#pragma once
#include "RSA.h"
#include <string>
#include <vector>

/**
 * @brief TLS message types mirroring TLS 1.3 record types.
 */
enum class TLSMessageType {
    ClientHello,
    ServerHello,
    Certificate,
    KeyExchange,
    Finished,
    Alert
};

/**
 * @brief One simulated TLS handshake packet.
 */
struct TLSPacket {
    int            index;
    TLSMessageType type;
    std::string    typeName;   ///< Human-readable type name
    std::string    from;
    std::string    to;
    std::string    description;
    std::string    payload;    ///< JSON-formatted packet fields
    bool           ok;         ///< false if this packet indicates failure
};

/**
 * @brief Simplified TLS 1.3-like handshake state machine.
 *
 * Sequence:
 *  1. ClientHello   Bob   → Alice     nonce_bob, supported_ciphers
 *  2. ServerHello   Alice → Bob       nonce_alice, chosen_cipher, certificate
 *  3. KeyExchange   Bob   → Alice     session_key encrypted with Alice's pubkey
 *  4. Finished      Alice → Bob       HMAC of full transcript with session key
 *  5. Finished      Bob   → Alice     HMAC verification + acknowledgement
 *
 * If Fake-Alice injects (inject_fake_alice = true):
 *  - ServerHello carries Fake-Alice's certificate (self-signed)
 *  - KeyExchange is encrypted with Fake-Alice's key (Bob thinks it's Alice)
 *  - Alice's Finished HMAC fails because she never received the session key
 *  → Handshake aborted with Alert
 */
class TLSSession {
public:
    /**
     * @param sessionId        Active session (Alice + Bob + CA keys must exist)
     * @param injectFakeAlice  If true, ServerHello uses Fake-Alice's cert
     */
    explicit TLSSession(const std::string& sessionId, bool injectFakeAlice = false);

    /// Run the full handshake; returns all packets in order
    std::vector<TLSPacket> run();

private:
    std::string sessionId_;
    bool        injectFakeAlice_;

    // Handshake state
    std::string nonceClient_;   ///< Bob's random nonce (hex)
    std::string nonceServer_;   ///< Alice's random nonce (hex)
    std::string sessionKey_;    ///< Shared symmetric key (plaintext, server-side only)
    std::string encSessionKey_; ///< Session key encrypted with Alice's (or Fake's) pubkey
    std::string transcript_;    ///< Concatenation of all packet payloads for HMAC

    // Key material from session
    RSAKeyPair aliceKeys_;
    RSAKeyPair fakeAliceKeys_;
    RSAKeyPair caKeys_;

    // Helpers
    std::string randomHex(int bytes) const;
    std::string computeFinishedHmac() const;
    TLSPacket   buildAlert(int index, const std::string& reason);
};
