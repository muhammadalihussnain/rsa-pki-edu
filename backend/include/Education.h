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

} // namespace Education
