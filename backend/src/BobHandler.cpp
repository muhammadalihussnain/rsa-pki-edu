#include "BobHandler.h"
#include "Certificate.h"
#include "Session.h"
#include "RSA.h"
#include "BigInteger.h"
#include <sstream>

// ── JSON helpers ──────────────────────────────────────────────────────────────

/// Extract a string value for a top-level key from a flat JSON object.
static std::string jsonExtract(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        pos++;
        auto end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    auto end = json.find_first_of(",}\n", pos);
    return json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

/// Extract the raw value (string or object) for a key — returns the raw JSON slice.
static std::string jsonExtractRaw(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";

    if (json[pos] == '{') {
        // Find matching closing brace
        int depth = 0;
        size_t start = pos;
        for (size_t i = pos; i < json.size(); i++) {
            if (json[i] == '{') depth++;
            else if (json[i] == '}') { depth--; if (depth == 0) return json.substr(start, i - start + 1); }
        }
    }
    if (json[pos] == '"') {
        pos++;
        auto end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    auto end = json.find_first_of(",}\n", pos);
    return json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

static std::string jsonEscape(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

/// Hex-string → BigInteger
static BigInteger hexToBigInt(const std::string& hex) {
    BigInteger result(0ULL);
    BigInteger sixteen(16ULL);
    for (char c : hex) {
        uint64_t nibble = 0;
        if (c >= '0' && c <= '9') nibble = static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') nibble = static_cast<uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') nibble = static_cast<uint64_t>(c - 'A' + 10);
        result = result * sixteen + BigInteger(nibble);
    }
    return result;
}

// ── Step builder ──────────────────────────────────────────────────────────────

struct VerifyStep {
    int         index;
    std::string title;
    std::string description;
    std::string value;
    bool        passed;   ///< true = check passed, false = failed
};

static std::string stepsToJson(const std::vector<VerifyStep>& steps) {
    std::ostringstream o;
    o << "[\n";
    for (size_t i = 0; i < steps.size(); i++) {
        const auto& s = steps[i];
        o << "    {\n";
        o << "      \"index\": "       << s.index                  << ",\n";
        o << "      \"title\": \""     << jsonEscape(s.title)      << "\",\n";
        o << "      \"description\": \""<< jsonEscape(s.description)<< "\",\n";
        o << "      \"value\": \""     << jsonEscape(s.value)      << "\",\n";
        o << "      \"passed\": "      << (s.passed ? "true" : "false") << "\n";
        o << "    }";
        if (i + 1 < steps.size()) o << ",";
        o << "\n";
    }
    o << "  ]";
    return o.str();
}

// ── Main handler ──────────────────────────────────────────────────────────────

std::string handleBobVerify(const std::string& requestBody) {
    // ── Parse inputs ──────────────────────────────────────────────────────────
    std::string sessionId = jsonExtract(requestBody, "session_id");
    std::string document  = jsonExtract(requestBody, "document");
    std::string hashHex   = jsonExtract(requestBody, "hash_hex");
    std::string signature = jsonExtract(requestBody, "signature");

    if (sessionId.empty() || hashHex.empty() || signature.empty()) {
        return R"({"error":"Missing session_id, hash_hex, or signature"})";
    }

    // Extract certificate fields from the nested "certificate" object
    std::string certJson   = jsonExtractRaw(requestBody, "certificate");
    std::string certSubject    = jsonExtract(certJson, "subject");
    std::string certIssuer     = jsonExtract(certJson, "issuer");
    std::string certSerial     = jsonExtract(certJson, "serial");
    std::string certHash       = jsonExtract(certJson, "cert_hash");
    std::string certCASig      = jsonExtract(certJson, "ca_signature");

    // Extract cert public key (nested object inside certificate)
    std::string pubKeyJson = jsonExtractRaw(certJson, "public_key");
    std::string certPubN   = jsonExtract(pubKeyJson, "n");
    std::string certPubE   = jsonExtract(pubKeyJson, "e");

    if (certSubject.empty() || certPubN.empty() || certCASig.empty()) {
        return R"({"error":"Invalid or incomplete certificate"})";
    }

    // Get CA's public key from session
    RSAKeyPair caKeys;
    if (!SessionStore::instance().getActorKeys(sessionId, "CA", caKeys)) {
        return R"({"error":"CA keys not found in session"})";
    }

    std::vector<VerifyStep> steps;
    bool overallPassed = true;

    // ── Step 1: Verify certificate CA signature ───────────────────────────────
    {
        Certificate cert;
        cert.subject      = certSubject;
        cert.issuer       = certIssuer;
        cert.publicKeyN   = certPubN;
        cert.publicKeyE   = certPubE;
        cert.serial       = certSerial;
        cert.certHash     = certHash;
        cert.caSignature  = certCASig;

        bool certValid = CertificateAuthority::verifyCertificate(
            cert, caKeys.n.toString(), caKeys.e.toString());

        if (!certValid) overallPassed = false;

        steps.push_back({
            1,
            "Verify Certificate (CA Signature Check)",
            "Bob uses the CA's public key to recover the cert hash: recovered = ca_signature^e_CA mod n_CA. "
            "If recovered == cert_hash the certificate is authentic and was issued by the trusted CA.",
            certValid
                ? "PASS — recovered hash matches cert_hash. Certificate is trusted."
                : "FAIL — recovered hash does not match. Certificate is not trusted.",
            certValid
        });

        if (!certValid) {
            // No point continuing — certificate isn't trusted
            std::ostringstream out;
            out << "{\n";
            out << "  \"session_id\": \"" << sessionId       << "\",\n";
            out << "  \"verified\": false,\n";
            out << "  \"failure_step\": 1,\n";
            out << "  \"failure_reason\": \"Certificate CA signature is invalid — certificate not trusted\",\n";
            out << "  \"steps\": " << stepsToJson(steps) << "\n";
            out << "}";
            return out.str();
        }
    }

    // ── Step 2: Extract Alice's public key from certificate ───────────────────
    steps.push_back({
        2,
        "Extract Alice's Public Key from Certificate",
        "Since the certificate is trusted, Bob can safely use the public key it contains. "
        "This key (n, e) belongs to Alice and was vouched for by the CA.",
        "n = " + certPubN.substr(0, 24) + "...  e = " + certPubE,
        true
    });

    // ── Step 3: Show the SHA-256 hash Bob received ────────────────────────────
    // Bob receives the hash that Alice computed and signed.
    // For the educational demo the hash travels with the signed packet.
    std::string computedHashHex = hashHex;

    steps.push_back({
        3,
        "SHA-256 Hash of Document",
        "This is the SHA-256 fingerprint of Alice's document. "
        "Bob will verify that Alice's signature decrypts to exactly this value.",
        computedHashHex,
        true
    });

    // ── Step 4: Decrypt the signature using Alice's public key ────────────────
    BigInteger sigInt(signature);
    BigInteger alicePubN(certPubN);
    BigInteger alicePubE(certPubE);

    BigInteger recoveredHashInt = RSACrypto::verify(sigInt, alicePubE, alicePubN);
    std::string recoveredHashDecimal = recoveredHashInt.toString();

    steps.push_back({
        4,
        "Decrypt Signature with Alice's Public Key",
        "recovered = signature^e_Alice mod n_Alice. "
        "This reverses Alice's signing operation and recovers the hash integer she originally signed.",
        "recovered = " + recoveredHashDecimal,
        true
    });

    // ── Step 5: Compare hashes ────────────────────────────────────────────────
    // Convert computedHashHex to integer with same reduction as signing used
    BigInteger computedHashInt = hexToBigInt(computedHashHex);
    BigInteger computedHashReduced = computedHashInt % alicePubN;

    bool hashesMatch = (recoveredHashInt == computedHashReduced);
    if (!hashesMatch) overallPassed = false;

    steps.push_back({
        5,
        "Compare Hashes",
        "Bob reduces his computed SHA-256 hash mod n_Alice (same reduction Alice used when signing), "
        "then checks: recovered == computed_hash mod n. A match proves the document is authentic and unmodified.",
        hashesMatch
            ? "PASS — hashes match. Document is authentic and from Alice."
            : "FAIL — hash mismatch. Document has been tampered with or signature is forged.",
        hashesMatch
    });

    // ── Build response ────────────────────────────────────────────────────────
    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \"" << sessionId                         << "\",\n";
    out << "  \"verified\": "     << (overallPassed ? "true" : "false") << ",\n";
    if (!overallPassed) {
        out << "  \"failure_step\": 5,\n";
        out << "  \"failure_reason\": \"Document hash mismatch — document was tampered or signature is invalid\",\n";
    }
    out << "  \"steps\": " << stepsToJson(steps) << "\n";
    out << "}";
    return out.str();
}
