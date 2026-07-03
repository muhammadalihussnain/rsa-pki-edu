#pragma once
#include <string>

/**
 * @brief Handle POST /api/tls/handshake
 *
 * Simulates a TLS 1.3-like handshake between Bob (client) and Alice (server).
 *
 * Request JSON:
 * {
 *   "session_id": "...",
 *   "inject_fake_alice": false   // optional, default false
 * }
 *
 * Response JSON:
 * {
 *   "session_id": "...",
 *   "success": true/false,
 *   "failure_reason": "...",     // only if success=false
 *   "packets": [ ... ]           // ordered handshake packets
 * }
 */
std::string handleTLSHandshake(const std::string& requestBody);
