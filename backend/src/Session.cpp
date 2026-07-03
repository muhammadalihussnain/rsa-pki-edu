#include "Session.h"

void SessionStore::store(const std::string& sessionId,
                         std::vector<ActorKeyPair> pairs) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[sessionId] = std::move(pairs);
}

std::vector<ActorPublicKey> SessionStore::getPublicKeys(
        const std::string& sessionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return {};

    std::vector<ActorPublicKey> result;
    for (const auto& kp : it->second) {
        result.push_back({
            kp.name,
            kp.keys.n.toString(),
            kp.keys.e.toString(),
            kp.bits
        });
    }
    return result;
}

bool SessionStore::getActorKeys(const std::string& sessionId,
                                const std::string& actorName,
                                RSAKeyPair& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return false;
    for (const auto& kp : it->second) {
        if (kp.name == actorName) {
            out = kp.keys;
            return true;
        }
    }
    return false;
}
