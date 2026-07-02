#pragma once
#include <string>

/**
 * @brief Handle POST /api/alice/upload-document
 *
 * Expects multipart/form-data with a "file" field containing text content.
 * Returns: { content, sha256_hash, content_length }
 */
std::string handleUploadDocument(const std::string& content);

/**
 * @brief Handle POST /api/alice/sign-document
 *
 * Expects JSON: { "session_id": "...", "hash_hex": "..." }
 * Alice signs the hash using her RSA private key.
 * Returns: { original_hash, signature_hex, certificate_ref, steps[] }
 */
std::string handleSignDocument(const std::string& requestBody);
