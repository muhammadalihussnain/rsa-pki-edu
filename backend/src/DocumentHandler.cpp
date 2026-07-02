#include "DocumentHandler.h"
#include "Hash.h"
#include "Session.h"
#include "RSA.h"
#include "BigInteger.h"
#include <sstream>
#include <algorithm>

// ── Minimal JSON field extractor (reused from CAHandler pattern) ──────────────

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

// ── JSON string escaping ──────────────────────────────────────────────────────

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
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

// ── Handlers ──────────────────────────────────────────────────────────────────

/**
 * @brief Process uploaded document text: compute SHA-256 and return JSON.
 */
std::string handleUploadDocument(const std::string& content) {
    if (content.empty()) {
        return R"({"error":"Empty document"})";
    }

    // Limit to 10KB for safety
    std::string truncated = content.size() > 10240
        ? content.substr(0, 10240)
        : content;

    std::string hashHex = SHA256::hexdigest(truncated);

    std::ostringstream out;
    out << "{\n";
    out << "  \"content\": \""        << jsonEscape(truncated) << "\",\n";
    out << "  \"content_length\": "   << truncated.size()      << ",\n";
    out << "  \"sha256_hash\": \""    << hashHex               << "\"\n";
    out << "}";
    return out.str();
}

/**
 * @brief Alice signs the SHA-256 hash of the document.
 *
 * The hash hex string is converted to a BigInteger, then Alice's RSA private
 * key is used to produce: signature = hash_int^d_alice mod n_alice.
 *
 * Returns a step-by-step log so the UI can animate each stage.
 */
std::string handleSignDocument(const std::string& requestBody) {
    std::string sessionId = jsonExtract(requestBody, "session_id");
    std::string hashHex   = jsonExtract(requestBody, "hash_hex");

    if (sessionId.empty() || hashHex.empty()) {
        return R"({"error":"Missing session_id or hash_hex"})";
    }

    RSAKeyPair aliceKeys;
    if (!SessionStore::instance().getActorKeys(sessionId, "Alice", aliceKeys)) {
        return R"({"error":"Alice's keys not found in session"})";
    }

    // Convert hex hash string to BigInteger via decimal intermediate
    // Process hex string nibble by nibble: result = result*16 + digit
    BigInteger hashInt(0ULL);
    BigInteger sixteen(16ULL);
    for (char c : hashHex) {
        uint64_t nibble = 0;
        if (c >= '0' && c <= '9') nibble = static_cast<uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') nibble = static_cast<uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') nibble = static_cast<uint64_t>(c - 'A' + 10);
        hashInt = hashInt * sixteen + BigInteger(nibble);
    }

    // Reduce mod n so it fits for signing (hash may be larger than n for small keys)
    BigInteger hashForSign = hashInt % aliceKeys.n;

    // Sign: sig = hash^d mod n
    BigInteger signature = RSACrypto::sign(hashForSign, aliceKeys.d, aliceKeys.n);

    // Convert signature to hex for display
    // We return both decimal (for math display) and we keep hex_hash for reference
    std::string sigDecimal = signature.toString();
    std::string hashDecimal = hashForSign.toString();

    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \""    << sessionId    << "\",\n";
    out << "  \"original_hash\": \"" << hashHex      << "\",\n";
    out << "  \"hash_as_integer\": \""<< hashDecimal << "\",\n";
    out << "  \"signature\": \""     << sigDecimal   << "\",\n";
    out << "  \"signer\": \"Alice\",\n";
    out << "  \"steps\": [\n";
    out << "    {\n";
    out << "      \"index\": 1,\n";
    out << "      \"title\": \"Compute SHA-256 hash of document\",\n";
    out << "      \"description\": \"SHA-256 produces a 256-bit (64 hex char) fingerprint of the document. Any change to the document changes the hash completely.\",\n";
    out << "      \"value\": \"" << hashHex << "\"\n";
    out << "    },\n";
    out << "    {\n";
    out << "      \"index\": 2,\n";
    out << "      \"title\": \"Convert hash to integer\",\n";
    out << "      \"description\": \"The hex hash is interpreted as a big integer so RSA arithmetic can operate on it. Reduced mod n to fit Alice's key size.\",\n";
    out << "      \"value\": \"" << hashDecimal << "\"\n";
    out << "    },\n";
    out << "    {\n";
    out << "      \"index\": 3,\n";
    out << "      \"title\": \"Sign with Alice's private key\",\n";
    out << "      \"description\": \"signature = hash_int ^ d_Alice mod n_Alice. Only Alice can produce this because only she has d_Alice.\",\n";
    out << "      \"value\": \"" << sigDecimal << "\"\n";
    out << "    }\n";
    out << "  ]\n";
    out << "}";
    return out.str();
}
