#include "TLSHandler.h"
#include "TLSSession.h"
#include <sstream>

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

static std::string packetToJson(const TLSPacket& p) {
    std::ostringstream o;
    o << "{\n";
    o << "      \"index\": "       << p.index            << ",\n";
    o << "      \"type\": \""      << p.typeName         << "\",\n";
    o << "      \"from\": \""      << p.from             << "\",\n";
    o << "      \"to\": \""        << p.to               << "\",\n";
    o << "      \"description\": \""<< jsonEsc(p.description) << "\",\n";
    o << "      \"payload\": "     << p.payload          << ",\n";
    o << "      \"ok\": "          << (p.ok ? "true" : "false") << "\n";
    o << "    }";
    return o.str();
}

std::string handleTLSHandshake(const std::string& requestBody) {
    std::string sessionId  = jsonExtract(requestBody, "session_id");
    std::string injectStr  = jsonExtract(requestBody, "inject_fake_alice");
    bool injectFake = (injectStr == "true");

    if (sessionId.empty()) {
        return R"({"error":"Missing session_id"})";
    }

    TLSSession session(sessionId, injectFake);
    std::vector<TLSPacket> packets = session.run();

    // Determine overall success
    bool success = true;
    std::string failureReason;
    for (const auto& pkt : packets) {
        if (!pkt.ok) {
            success       = false;
            failureReason = pkt.description;
            break;
        }
    }

    std::ostringstream out;
    out << "{\n";
    out << "  \"session_id\": \""  << sessionId                          << "\",\n";
    out << "  \"mode\": \""        << (injectFake ? "fake_alice" : "normal") << "\",\n";
    out << "  \"success\": "       << (success ? "true" : "false")        << ",\n";
    if (!success) {
        out << "  \"failure_reason\": \"" << jsonEsc(failureReason) << "\",\n";
    }
    out << "  \"packets\": [\n";
    for (size_t i = 0; i < packets.size(); i++) {
        out << "    " << packetToJson(packets[i]);
        if (i + 1 < packets.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n";
    out << "}";
    return out.str();
}
