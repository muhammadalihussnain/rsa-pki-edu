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
    std::string formula;   ///< Math formula shown prominently
    std::string value;     ///< Computed/result value
    bool        passed;
};

static std::string stepsToJson(const std::vector<VerifyStep>& steps) {
    std::ostringstream o;
    o << "[\n";
    for (size_t i = 0; i < steps.size(); i++) {
        const auto& s = steps[i];
        o << "    {\n";
        o << "      \"index\": "        << s.index                   << ",\n";
        o << "      \"title\": \""      << jsonEscape(s.title)       << "\",\n";
        o << "      \"description\": \""<< jsonEscape(s.description) << "\",\n";
        o << "      \"formula\": \""    << jsonEscape(s.formula)     << "\",\n";
        o << "      \"value\": \""      << jsonEscape(s.value)       << "\",\n";
        o << "      \"passed\": "       << (s.passed ? "true" : "false") << "\n";
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

        std::string caE = caKeys.e.toString();
        std::string caN = caKeys.n.toString().substr(0, 20) + "...";

        steps.push_back({
            1,
            "Verify Certificate — CA Signature",
            "Bob has the CA's public key (e_CA, n_CA) pre-installed as a trust anchor. "
            "He recovers the certificate hash to confirm the CA actually signed it. "
            "If the certificate was tampered with or forged, this check fails immediately.",
            "recovered = ca_signature ^ e_CA  mod  n_CA",
            certValid
                ? "PASS  recovered == cert_hash  (Certificate is authentic and trusted)"
                : "FAIL  recovered != cert_hash  (Certificate is invalid or forged)",
            certValid
        });

        if (!certValid) {
            std::ostringstream out;
            out << "{\n";
            out << "  \"session_id\": \"" << sessionId << "\",\n";
            out << "  \"verified\": false,\n";
            out << "  \"failure_step\": 1,\n";
            out << "  \"failure_reason\": \"Certificate CA signature is invalid — certificate not trusted\",\n";
            out << "  \"alice_signature\": \"" << signature << "\",\n";
            out << "  \"steps\": " << stepsToJson(steps) << "\n";
            out << "}";
            return out.str();
        }
    }

    // ── Step 2: Extract Alice's public key from certificate ───────────────────
    {
        std::string eDisplay = certPubE;
        std::string nDisplay = certPubN.substr(0, 24) + "...";
        steps.push_back({
            2,
            "Extract Alice's Public Key from Certificate",
            "The certificate is trusted, so the public key inside it genuinely belongs to Alice. "
            "Bob extracts (e_Alice, n_Alice) — these are the ONLY keys Bob uses for signature verification. "
            "Bob's own private key plays no role in this process.",
            "Public key from cert:  e = " + eDisplay + ",  n = " + nDisplay,
            "e_Alice = " + eDisplay + "\nn_Alice = " + certPubN,
            true
        });
    }

    // ── Step 3: Document hash ─────────────────────────────────────────────────
    steps.push_back({
        3,
        "SHA-256 Hash of the Document",
        "This is the SHA-256 fingerprint of Alice's document — the exact value Alice computed and signed. "
        "Any modification to the document (even one byte) produces a completely different hash.",
        "hash = SHA-256(document)",
        hashHex,
        true
    });

    // ── Step 4: Decrypt signature with Alice's PUBLIC key ─────────────────────
    // NOTE: Bob uses Alice's PUBLIC key — NOT Bob's private key.
    // This recovers the hash integer Alice used when signing.
    BigInteger sigInt(signature);
    BigInteger alicePubN(certPubN);
    BigInteger alicePubE(certPubE);

    BigInteger recoveredHashInt = RSACrypto::verify(sigInt, alicePubE, alicePubN);
    std::string recoveredDecimal = recoveredHashInt.toString();

    steps.push_back({
        4,
        "Decrypt Signature with Alice's Public Key",
        "Bob applies Alice's PUBLIC key to her signature. This is the inverse of signing: "
        "Alice encrypted with her private key, so only her public key can reverse it. "
        "The result is the hash integer Alice originally signed.",
        "recovered = signature ^ e_Alice  mod  n_Alice",
        "recovered = " + recoveredDecimal,
        true
    });

    // ── Step 5: Compare recovered hash with document hash ────────────────────
    BigInteger computedHashInt = hexToBigInt(hashHex);
    BigInteger computedHashReduced = computedHashInt % alicePubN;

    bool hashesMatch = (recoveredHashInt == computedHashReduced);
    if (!hashesMatch) overallPassed = false;

    steps.push_back({
        5,
        "Compare Hashes",
        "Bob converts the SHA-256 hex to an integer and reduces it mod n_Alice (the same "
        "operation Alice applied before signing). If recovered == hash mod n, the signature "
        "is valid — the document is authentic and unmodified.",
        "recovered  ==  SHA-256(doc) mod n_Alice  ?",
        hashesMatch
            ? "PASS  " + recoveredDecimal + "  ==  " + computedHashReduced.toString()
            : "FAIL  " + recoveredDecimal + "  !=  " + computedHashReduced.toString(),
        hashesMatch
    });

    // ── Build response ────────────────────────────────────────────────────────
    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \"" << sessionId << "\",\n";
    out << "  \"verified\": " << (overallPassed ? "true" : "false") << ",\n";
    out << "  \"alice_signature\": \"" << signature << "\",\n";
    out << "  \"hash_hex\": \"" << hashHex << "\",\n";
    if (!overallPassed) {
        out << "  \"failure_step\": 5,\n";
        out << "  \"failure_reason\": \"Hash mismatch — document was tampered or signature is forged\",\n";
    }
    out << "  \"steps\": " << stepsToJson(steps) << "\n";
    out << "}";
    return out.str();
}
