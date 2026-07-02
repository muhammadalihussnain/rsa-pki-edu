#include "CAHandler.h"
#include "Session.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>

// ── Minimal JSON field extractor ──────────────────────────────────────────────

/// Extract the string value for a given key from a flat JSON object.
/// Returns empty string if not found.
static std::string jsonExtract(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";

    // skip whitespace
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        // string value
        pos++;
        auto end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    // non-string (number/bool) — read until delimiter
    auto end = json.find_first_of(",}\n", pos);
    return json.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

// ── Serial generation ─────────────────────────────────────────────────────────

static std::string generateSerial() {
    std::mt19937 rng(std::random_device{}());
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << rng();
    return ss.str();
}

// ── JSON serialisation ────────────────────────────────────────────────────────

std::string certificateToJson(const Certificate& cert) {
    std::ostringstream o;
    o << "{\n";
    o << "  \"subject\": \""    << cert.subject    << "\",\n";
    o << "  \"issuer\": \""     << cert.issuer     << "\",\n";
    o << "  \"public_key\": {\n";
    o << "    \"n\": \""        << cert.publicKeyN << "\",\n";
    o << "    \"e\": \""        << cert.publicKeyE << "\"\n";
    o << "  },\n";
    o << "  \"serial\": \""     << cert.serial     << "\",\n";
    o << "  \"valid_from\": \"" << cert.validFrom  << "\",\n";
    o << "  \"valid_to\": \""   << cert.validTo    << "\",\n";
    o << "  \"cert_hash\": \""  << cert.certHash   << "\",\n";
    o << "  \"ca_signature\": \""<< cert.caSignature << "\"\n";
    o << "}";
    return o.str();
}

// ── Handler ───────────────────────────────────────────────────────────────────

std::string handleIssueCertificate(const std::string& requestBody) {
    std::string sessionId = jsonExtract(requestBody, "session_id");
    std::string subject   = jsonExtract(requestBody, "subject");

    if (sessionId.empty() || subject.empty()) {
        return R"({"error":"Missing session_id or subject"})";
    }

    // Retrieve subject's public key from session
    RSAKeyPair subjectKeys;
    if (!SessionStore::instance().getActorKeys(sessionId, subject, subjectKeys)) {
        return R"({"error":"Subject not found in session"})";
    }

    std::string serial = generateSerial();
    std::string subN   = subjectKeys.n.toString();
    std::string subE   = subjectKeys.e.toString();

    bool ok = false;
    Certificate cert = CertificateAuthority::issueCertificate(
        sessionId, subject, subN, subE, serial, ok);

    if (!ok) {
        return R"({"error":"CA keys not found in session"})";
    }

    // Build packet exchange log for UI animation
    // Packet 1: Alice → CA (certificate request)
    // Packet 2: CA → Alice (signed certificate)
    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \"" << sessionId << "\",\n";
    out << "  \"packets\": [\n";
    out << "    {\n";
    out << "      \"index\": 1,\n";
    out << "      \"from\": \"" << subject << "\",\n";
    out << "      \"to\": \"CA\",\n";
    out << "      \"type\": \"CertificateRequest\",\n";
    out << "      \"description\": \"" << subject << " sends public key (n, e) and identity to CA requesting a certificate.\",\n";
    out << "      \"payload\": {\n";
    out << "        \"subject\": \"" << subject << "\",\n";
    out << "        \"public_key_n\": \"" << subN.substr(0, 20) << "...\"\n";
    out << "      }\n";
    out << "    },\n";
    out << "    {\n";
    out << "      \"index\": 2,\n";
    out << "      \"from\": \"CA\",\n";
    out << "      \"to\": \"" << subject << "\",\n";
    out << "      \"type\": \"Certificate\",\n";
    out << "      \"description\": \"CA signs the certificate with its private key and returns it to " << subject << ".\",\n";
    out << "      \"payload\": " << certificateToJson(cert) << "\n";
    out << "    }\n";
    out << "  ],\n";
    out << "  \"certificate\": " << certificateToJson(cert) << "\n";
    out << "}";
    return out.str();
}
