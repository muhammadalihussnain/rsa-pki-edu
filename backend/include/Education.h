#pragma once
#include <string>

/**
 * @brief Provides structured RSA theory content for the education API.
 */
namespace Education {

/// @brief Returns the full RSA theory JSON payload for GET /api/education/rsa-theory
std::string rsaTheoryJson();

/// @brief Returns the certificate theory JSON payload for GET /api/education/certificate-theory
std::string certificateTheoryJson();

/// @brief Returns the certificate verification theory JSON for GET /api/education/certificate-verification
std::string certificateVerificationTheoryJson();

/// @brief Returns the signature verification theory JSON for GET /api/education/signature-verification
std::string signatureVerificationTheoryJson();

} // namespace Education
