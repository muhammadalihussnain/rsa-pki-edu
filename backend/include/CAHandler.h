#pragma once
#include "Certificate.h"
#include <string>

/**
 * @brief Handle POST /api/ca/issue-certificate
 *
 * Expects JSON body: { "session_id": "...", "subject": "Alice" }
 * Uses the session's CA keys to sign a certificate for Alice's public key.
 *
 * @return JSON string with issued certificate and packet exchange log,
 *         or error JSON if session/keys not found.
 */
std::string handleIssueCertificate(const std::string& requestBody);

/**
 * @brief Serialise a Certificate to JSON.
 */
std::string certificateToJson(const Certificate& cert);
