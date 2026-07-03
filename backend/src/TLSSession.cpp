#include "TLSSession.h"
#include "Session.h"
#include "Certificate.h"
#include "CAHandler.h"
#include "HMAC.h"
#include "Hash.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string tlsTypeName(TLSMessageType t) {
    switch (t) {
        case TLSMessageType::ClientHello:  return "ClientHello";
        case TLSMessageType::ServerHello:  return "ServerHello";
        case TLSMessageType::Certificate:  return "Certificate";
        case TLSMessageType::KeyExchange:  return "ClientKeyExchange";
        case TLSMessageType::Finished:     return "Finished";
        case TLSMessageType::Alert:        return "Alert";
    }
    return "Unknown";
}

static std::string jsonEsc(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

// ── TLSSession ────────────────────────────────────────────────────────────────

TLSSession::TLSSession(const std::string& sessionId, bool injectFakeAlice)
    : sessionId_(sessionId), injectFakeAlice_(injectFakeAlice) {}

std::string TLSSession::randomHex(int bytes) const {
    std::mt19937_64 rng(std::random_device{}());
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < bytes / 8; i++)
        ss << std::setw(16) << rng();
    return ss.str().substr(0, bytes * 2);
}

std::string TLSSession::computeFinishedHmac() const {
    return HMAC_SHA256::hexdigest(sessionKey_, transcript_);
}

TLSPacket TLSSession::buildAlert(int index, const std::string& reason) {
    std::string payload = "{\"level\":\"fatal\",\"description\":\"" + jsonEsc(reason) + "\"}";
    return {
        index,
        TLSMessageType::Alert,
        "Alert",
        "Server",
        "Bob",
        "Handshake aborted: " + reason,
        payload,
        false
    };
}

std::vector<TLSPacket> TLSSession::run() {
    std::vector<TLSPacket> packets;

    // Load key material
    if (!SessionStore::instance().getActorKeys(sessionId_, "Alice",      aliceKeys_)     ||
        !SessionStore::instance().getActorKeys(sessionId_, "Fake-Alice", fakeAliceKeys_) ||
        !SessionStore::instance().getActorKeys(sessionId_, "CA",         caKeys_)) {
        packets.push_back(buildAlert(1, "Session keys not found"));
        return packets;
    }

    // ── Packet 1: ClientHello (Bob → Alice) ───────────────────────────────────
    nonceClient_ = randomHex(16);
    {
        std::ostringstream payload;
        payload << "{"
                << "\"version\":\"TLS 1.3\","
                << "\"random\":\"" << nonceClient_ << "\","
                << "\"cipher_suites\":[\"TLS_RSA_WITH_AES_128\",\"TLS_RSA_WITH_AES_256\"],"
                << "\"extensions\":{\"server_name\":\"alice.example.com\"}"
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        packets.push_back({
            1,
            TLSMessageType::ClientHello,
            tlsTypeName(TLSMessageType::ClientHello),
            "Bob", "Alice",
            "Bob initiates the handshake. He sends his random nonce and the cipher suites he supports.",
            p, true
        });
    }

    // ── Packet 2: ServerHello + Certificate (Alice → Bob) ────────────────────
    nonceServer_ = randomHex(16);

    // Determine which party responds (Alice or Fake-Alice if injecting)
    RSAKeyPair& serverKeys  = injectFakeAlice_ ? fakeAliceKeys_ : aliceKeys_;
    std::string serverName  = injectFakeAlice_ ? "Fake-Alice" : "Alice";
    std::string serverN     = serverKeys.n.toString();
    std::string serverE     = serverKeys.e.toString();

    // Build the certificate (real CA-signed for Alice, self-signed for Fake-Alice)
    bool certOk = false;
    std::string certSubject, certIssuer, certSig, certHash, certSerial;
    if (!injectFakeAlice_) {
        // Issue a real certificate for Alice
        Certificate cert = CertificateAuthority::issueCertificate(
            sessionId_, "Alice", serverN, serverE, "tls-01", certOk);
        certSubject = cert.subject;
        certIssuer  = cert.issuer;
        certSig     = cert.caSignature;
        certHash    = cert.certHash;
        certSerial  = cert.serial;
    } else {
        // Fake-Alice self-signs her cert
        certSubject = "Alice";   // claims to be Alice
        certIssuer  = "CA";
        certSerial  = "fake01";
        BigInteger fakeHash = CertificateAuthority::computeCertHash(
            "Alice", serverN, certSerial, serverKeys.n);
        BigInteger fakeSig  = RSACrypto::sign(fakeHash, serverKeys.d, serverKeys.n);
        certSig  = fakeSig.toString();
        certHash = fakeHash.toString();
        certOk   = true;
    }

    {
        std::ostringstream payload;
        payload << "{"
                << "\"version\":\"TLS 1.3\","
                << "\"random\":\"" << nonceServer_ << "\","
                << "\"chosen_cipher\":\"TLS_RSA_WITH_AES_256\","
                << "\"certificate\":{"
                <<   "\"subject\":\"" << certSubject << "\","
                <<   "\"issuer\":\""  << certIssuer  << "\","
                <<   "\"public_key\":{\"e\":\"" << serverE << "\",\"n\":\"" << serverN.substr(0, 20) << "...\"},"
                <<   "\"serial\":\""  << certSerial  << "\","
                <<   "\"ca_signature\":\"" << certSig.substr(0, 20) << "...\","
                <<   "\"cert_hash\":\"" << certHash << "\""
                << "}"
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        std::string desc = injectFakeAlice_
            ? "Fake-Alice intercepts and responds with her own self-signed certificate, claiming to be Alice."
            : "Alice responds with her TLS version, a server random nonce, chosen cipher, and her CA-signed certificate.";

        packets.push_back({
            2,
            TLSMessageType::ServerHello,
            tlsTypeName(TLSMessageType::ServerHello),
            serverName, "Bob",
            desc,
            p, true
        });
    }

    // ── Packet 3: Bob verifies certificate ────────────────────────────────────
    // Bob checks the CA signature on the certificate
    {
        Certificate certToVerify;
        certToVerify.publicKeyN  = serverN;
        certToVerify.publicKeyE  = serverE;
        certToVerify.subject     = certSubject;
        certToVerify.issuer      = certIssuer;
        certToVerify.serial      = certSerial;
        certToVerify.certHash    = certHash;
        certToVerify.caSignature = certSig;

        bool certValid = CertificateAuthority::verifyCertificate(
            certToVerify, caKeys_.n.toString(), caKeys_.e.toString());

        if (!certValid) {
            // Certificate check failed — emit Alert and stop
            packets.push_back(buildAlert(3,
                "Certificate verification failed — CA signature invalid. "
                "Fake-Alice's self-signed certificate was rejected."));
            return packets;
        }

        std::ostringstream payload;
        payload << "{"
                << "\"check\":\"certificate_verify\","
                << "\"result\":\"PASS\","
                << "\"detail\":\"recovered == cert_hash using CA public key\""
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        packets.push_back({
            3,
            TLSMessageType::Certificate,
            "CertificateVerify",
            "Bob", "Alice",
            "Bob verifies the server certificate using the CA's public key. "
            "Formula: recovered = ca_sig ^ e_CA mod n_CA. Must equal cert_hash.",
            p, true
        });
    }

    // ── Packet 4: KeyExchange (Bob → Alice) ───────────────────────────────────
    // Bob generates a random session key and encrypts it with server's public key
    sessionKey_ = randomHex(16);   // 128-bit session key (plaintext, educational)

    BigInteger sessionKeyInt(0ULL);
    BigInteger sixteen(16ULL);
    for (char c : sessionKey_) {
        uint64_t nibble = 0;
        if (c >= '0' && c <= '9') nibble = static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') nibble = static_cast<uint64_t>(c - 'a' + 10);
        sessionKeyInt = sessionKeyInt * sixteen + BigInteger(nibble);
    }

    BigInteger pubN(serverN);
    BigInteger pubE(serverE);
    BigInteger sessionKeyReduced = sessionKeyInt % pubN;
    BigInteger encKey = RSACrypto::encrypt(sessionKeyReduced, pubE, pubN);
    encSessionKey_   = encKey.toString();

    {
        std::ostringstream payload;
        payload << "{"
                << "\"encrypted_premaster_secret\":\"" << encSessionKey_.substr(0, 24) << "...\","
                << "\"key_exchange_method\":\"RSA\","
                << "\"detail\":\"enc = session_key_int ^ e_server mod n_server\""
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        packets.push_back({
            4,
            TLSMessageType::KeyExchange,
            tlsTypeName(TLSMessageType::KeyExchange),
            "Bob", serverName,
            "Bob encrypts a randomly-generated session key with the server's public key. "
            "Only the server (holder of the private key) can decrypt it.",
            p, true
        });
    }

    // ── Packet 5: Server Finished (Alice → Bob) ───────────────────────────────
    // Alice decrypts the session key, then computes HMAC of the transcript
    // In the injection scenario Alice never received the session key,
    // so she cannot produce the correct HMAC → we detect the mismatch
    std::string serverHmac;
    bool finishOk = true;

    if (!injectFakeAlice_) {
        // Alice decrypts enc_session_key with her private key
        BigInteger decKey = RSACrypto::decrypt(encKey, aliceKeys_.d, aliceKeys_.n);
        // Verify decrypted key matches what Bob sent
        finishOk   = (decKey == sessionKeyReduced);
        serverHmac = HMAC_SHA256::hexdigest(sessionKey_, transcript_);
    } else {
        // Fake-Alice decrypted a different key (with her own private key), so
        // the HMAC will be wrong — simulate the mismatch
        BigInteger fakeDecKey = RSACrypto::decrypt(encKey, fakeAliceKeys_.d, fakeAliceKeys_.n);
        // Fake-Alice computes HMAC with her (wrong) decrypted key
        serverHmac = HMAC_SHA256::hexdigest(fakeDecKey.toString(), transcript_);
        finishOk   = false;
    }

    {
        std::ostringstream payload;
        payload << "{"
                << "\"hmac\":\"" << serverHmac.substr(0, 24) << "...\","
                << "\"formula\":\"HMAC-SHA256(session_key, transcript)\","
                << "\"status\":\"" << (finishOk ? "OK" : "MISMATCH") << "\""
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        if (!finishOk) {
            packets.push_back(buildAlert(5,
                "Finished HMAC mismatch — session key decryption failed. "
                "Fake-Alice cannot compute the correct HMAC because she used the wrong private key."));
            return packets;
        }

        packets.push_back({
            5,
            TLSMessageType::Finished,
            tlsTypeName(TLSMessageType::Finished) + " (Server)",
            serverName, "Bob",
            "Alice decrypts the session key with her private key and computes "
            "HMAC-SHA256 of the entire handshake transcript. This proves both sides share the same key.",
            p, true
        });
    }

    // ── Packet 6: Client Finished (Bob → Alice) ───────────────────────────────
    // Bob computes his own HMAC and compares with the server's
    std::string clientHmac = HMAC_SHA256::hexdigest(sessionKey_, transcript_);

    {
        std::ostringstream payload;
        payload << "{"
                << "\"hmac\":\"" << clientHmac.substr(0, 24) << "...\","
                << "\"formula\":\"HMAC-SHA256(session_key, transcript)\","
                << "\"match\":\"" << (clientHmac == serverHmac ? "true" : "false") << "\","
                << "\"status\":\"Handshake complete\""
                << "}";
        std::string p = payload.str();
        transcript_ += p;

        packets.push_back({
            6,
            TLSMessageType::Finished,
            tlsTypeName(TLSMessageType::Finished) + " (Client)",
            "Bob", serverName,
            "Bob computes his own HMAC of the transcript. "
            "If it matches the server's HMAC, the handshake is complete and the channel is secure.",
            p, true
        });
    }

    return packets;
}
