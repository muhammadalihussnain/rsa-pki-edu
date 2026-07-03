#include "AttackerHandler.h"
#include "Certificate.h"
#include "Session.h"
#include "RSA.h"
#include "Hash.h"
#include "BigInteger.h"
#include <sstream>
#include <random>
#include <iomanip>
#include <ctime>

// ── JSON helpers ──────────────────────────────────────────────────────────────

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

static std::string isoDate(int offsetDays = 0) {
    std::time_t t = std::time(nullptr) + static_cast<std::time_t>(offsetDays) * 86400;
    std::tm* tm = std::gmtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return buf;
}

static std::string generateSerial() {
    std::mt19937 rng(std::random_device{}());
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << rng();
    return ss.str();
}

// Serialise a certificate to JSON (inline, for the response payload)
static std::string certToJson(const Certificate& c) {
    std::ostringstream o;
    o << "{\n";
    o << "    \"subject\": \""     << c.subject    << "\",\n";
    o << "    \"issuer\": \""      << c.issuer     << "\",\n";
    o << "    \"public_key\": { \"n\": \"" << c.publicKeyN << "\", \"e\": \"" << c.publicKeyE << "\" },\n";
    o << "    \"serial\": \""      << c.serial     << "\",\n";
    o << "    \"valid_from\": \""  << c.validFrom  << "\",\n";
    o << "    \"valid_to\": \""    << c.validTo    << "\",\n";
    o << "    \"cert_hash\": \""   << c.certHash   << "\",\n";
    o << "    \"ca_signature\": \""<< c.caSignature<< "\"\n";
    o << "  }";
    return o.str();
}

// ── Handler ───────────────────────────────────────────────────────────────────

std::string handleFakeAliceAttempt(const std::string& requestBody) {
    std::string sessionId = jsonExtract(requestBody, "session_id");
    std::string document  = jsonExtract(requestBody, "document");
    std::string scenario  = jsonExtract(requestBody, "scenario");

    if (sessionId.empty() || document.empty()) {
        return R"({"error":"Missing session_id or document"})";
    }
    if (scenario.empty()) scenario = "forged_cert";

    // Get Fake-Alice's key pair
    RSAKeyPair fakeKeys;
    if (!SessionStore::instance().getActorKeys(sessionId, "Fake-Alice", fakeKeys)) {
        return R"({"error":"Fake-Alice keys not found in session"})";
    }

    std::string fakeN = fakeKeys.n.toString();
    std::string fakeE = fakeKeys.e.toString();

    // ── Build the attacker's document and hash ────────────────────────────────
    std::string attackDoc  = document;
    std::string callout;
    std::string scenarioDesc;

    if (scenario == "tampered_doc") {
        // Scenario B: modify the document content
        attackDoc  = document + " [MODIFIED BY ATTACKER]";
        callout    = "Fake-Alice tampered with the document after Alice signed it. "
                     "The certificate passes (CA signed it for Alice), but the SHA-256 hash "
                     "of the modified document does not match what was signed. "
                     "Bob's Step 5 (hash comparison) will fail.";
        scenarioDesc = "Fake-Alice sends a tampered document with Alice's original signature. "
                       "The certificate is valid but the document content has been changed.";
    } else {
        // Scenario A (default): forged certificate — self-signed with Fake-Alice's own key
        callout    = "Fake-Alice created a certificate that claims to be for 'Alice' but "
                     "signed it with her own private key instead of the CA's key. "
                     "Bob will compute: recovered = ca_signature ^ e_CA mod n_CA. "
                     "The result will NOT match the cert_hash because the CA never signed this. "
                     "Bob's Step 1 (certificate check) will fail immediately.";
        scenarioDesc = "Fake-Alice forged a certificate claiming to be Alice, "
                       "signed with her own private key instead of the CA's.";
    }

    std::string hashHex = SHA256::hexdigest(attackDoc);
    BigInteger hashInt  = hexToBigInt(hashHex);
    BigInteger hashForSign = hashInt % fakeKeys.n;
    BigInteger fakeSignature = RSACrypto::sign(hashForSign, fakeKeys.d, fakeKeys.n);
    std::string fakeSigStr = fakeSignature.toString();

    // ── Build the fake certificate ────────────────────────────────────────────
    Certificate fakeCert;
    fakeCert.subject    = "Alice";      // claims to be Alice
    fakeCert.issuer     = "CA";         // claims to be issued by CA
    fakeCert.publicKeyN = fakeN;        // but contains Fake-Alice's public key
    fakeCert.publicKeyE = fakeE;
    fakeCert.serial     = generateSerial();
    fakeCert.validFrom  = isoDate(0);
    fakeCert.validTo    = isoDate(365);

    if (scenario == "tampered_doc") {
        // For tampered_doc: the cert itself is structurally like Alice's (same fields),
        // but it's self-signed by Fake-Alice so CA check will still fail.
        // This makes the educational contrast clear: even with a "real-looking" cert,
        // without the CA's actual private key you can't forge a valid CA signature.
        BigInteger certHashInt = CertificateAuthority::computeCertHash(
            fakeCert.subject, fakeN, fakeCert.serial, fakeKeys.n);
        BigInteger selfSig = RSACrypto::sign(certHashInt, fakeKeys.d, fakeKeys.n);
        fakeCert.certHash    = certHashInt.toString();
        fakeCert.caSignature = selfSig.toString();
    } else {
        // Scenario A: self-sign the cert — will fail CA verification
        BigInteger certHashInt = CertificateAuthority::computeCertHash(
            fakeCert.subject, fakeN, fakeCert.serial, fakeKeys.n);
        BigInteger selfSig = RSACrypto::sign(certHashInt, fakeKeys.d, fakeKeys.n);
        fakeCert.certHash    = certHashInt.toString();
        fakeCert.caSignature = selfSig.toString();
    }

    // ── Build response ────────────────────────────────────────────────────────
    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \""    << sessionId              << "\",\n";
    out << "  \"attacker\": \"Fake-Alice\",\n";
    out << "  \"scenario\": \""      << scenario               << "\",\n";
    out << "  \"description\": \""   << jsonEscape(scenarioDesc) << "\",\n";
    out << "  \"document\": \""      << jsonEscape(attackDoc)  << "\",\n";
    out << "  \"hash_hex\": \""      << hashHex                << "\",\n";
    out << "  \"fake_signature\": \""<< fakeSigStr             << "\",\n";
    out << "  \"educational_callout\": \"" << jsonEscape(callout) << "\",\n";
    out << "  \"fake_certificate\": " << certToJson(fakeCert)  << "\n";
    out << "}";
    return out.str();
}
