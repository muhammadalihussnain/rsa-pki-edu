#pragma once
#include <string>

/**
 * @brief Handle POST /api/bob/verify
 *
 * Expects JSON body:
 * {
 *   "session_id": "...",
 *   "document":   "...",          // original document text
 *   "signature":  "...",          // decimal string from sign-document
 *   "certificate": {              // IssuedCertificate object
 *     "subject":       "Alice",
 *     "issuer":        "CA",
 *     "public_key":    { "n": "...", "e": "..." },
 *     "serial":        "...",
 *     "cert_hash":     "...",
 *     "ca_signature":  "..."
 *   }
 * }
 *
 * Returns a 5-step verification log and an overall result.
 * All steps use only public keys — no private keys involved.
 */
std::string handleBobVerify(const std::string& requestBody);
