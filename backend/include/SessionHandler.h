#pragma once
#include "Session.h"
#include <string>
#include <vector>

/**
 * @brief Generates key pairs for all actors and stores them in the session.
 *
 * @param sessionId   Unique session identifier
 * @param bits        Key size in bits (512 for dev speed, 2048 for production)
 * @return JSON string with all public keys; private keys remain server-side
 */
std::string handleSessionInit(const std::string& sessionId, int bits = 512);

/**
 * @brief Serialise a list of public keys to JSON.
 */
std::string publicKeysToJson(const std::string& sessionId,
                              const std::vector<ActorPublicKey>& keys);
