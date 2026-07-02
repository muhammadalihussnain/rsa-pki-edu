#include <httplib.h>
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include "Education.h"
#include "SessionHandler.h"
#include "CAHandler.h"
#include "DocumentHandler.h"

/// @brief Generate a simple random session ID (hex string)
static std::string generateSessionId() {
    std::mt19937_64 rng(std::random_device{}());
    std::ostringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << rng();
    return ss.str();
}

int main() {
    httplib::Server svr;

    // ── Health ────────────────────────────────────────────────────────────────
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","service":"rsa-pki-edu"})",
                        "application/json");
    });

    // ── Phase 2: RSA Theory ───────────────────────────────────────────────────
    svr.Get("/api/education/rsa-theory", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(Education::rsaTheoryJson(), "application/json");
    });

    // ── Phase 3: Session Init ─────────────────────────────────────────────────
    // Generates RSA key pairs for Alice, Bob, CA, Fake-Alice.
    // Private keys stay server-side; only public keys are returned.
    svr.Post("/api/session/init", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        std::string sessionId = generateSessionId();
        // Use 256-bit keys for interactive speed in this educational context.
        // In a production PKI, 2048+ bit keys would be used.
        std::string body = handleSessionInit(sessionId, 256);
        res.set_content(body, "application/json");
    });

    // ── Phase 4: Certificate Theory ───────────────────────────────────────────
    svr.Get("/api/education/certificate-theory", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(Education::certificateTheoryJson(), "application/json");
    });

    // ── Phase 4: Issue Certificate ────────────────────────────────────────────
    svr.Post("/api/ca/issue-certificate", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(handleIssueCertificate(req.body), "application/json");
    });

    // ── Phase 5: Upload Document ──────────────────────────────────────────────
    // Accepts plain text body; returns content + SHA-256 hash
    svr.Post("/api/alice/upload-document", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(handleUploadDocument(req.body), "application/json");
    });

    // ── Phase 5: Sign Document ────────────────────────────────────────────────
    // Alice signs the document hash with her RSA private key
    svr.Post("/api/alice/sign-document", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(handleSignDocument(req.body), "application/json");
    });

    std::cout << "Server listening on http://localhost:8081\n";
    svr.listen("0.0.0.0", 8081);
    return 0;
}
