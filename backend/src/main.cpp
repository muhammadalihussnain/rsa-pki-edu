#include <httplib.h>
#include <iostream>

/**
 * @brief Entry point. Starts HTTP server with /api/health endpoint.
 */
int main() {
    httplib::Server svr;

    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok","service":"rsa-pki-edu"})",
                        "application/json");
    });

    std::cout << "Server listening on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
