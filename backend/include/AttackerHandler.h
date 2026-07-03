#pragma once
#include <string>

/**
 * @brief Handle POST /api/fake-alice/attempt
 *
 * Fake-Alice has her own RSA key pair (generated in the session) but NO
 * CA-issued certificate. She can mount two attack scenarios:
 *
 * Scenario A — "forged_cert": She self-signs a certificate (signs it with
 *   her own private key instead of the CA's). The cert looks real but the
 *   CA signature check will fail.
 *
 * Scenario B — "tampered_doc": She uses a valid-looking cert structure but
 *   signs a modified version of the document. Cert check passes (if cert
 *   structure is intact) but signature/hash comparison fails.
 *
 * Request JSON:
 * {
 *   "session_id": "...",
 *   "document":   "...",          // original document text (may be modified by attacker)
 *   "scenario":   "forged_cert"  // or "tampered_doc"
 * }
 *
 * Response includes:
 *   - attacker_name, scenario, description
 *   - fake_certificate (forged or structurally correct but untrusted)
 *   - fake_signature
 *   - hash_hex
 *   - educational_callout explaining the attack
 */
std::string handleFakeAliceAttempt(const std::string& requestBody);
