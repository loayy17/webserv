#include "SessionManager.hpp"

SessionManager::SessionManager() {}

SessionManager::SessionManager(const SessionManager& other) : sessions(other.sessions) {}

SessionManager& SessionManager::operator=(const SessionManager& other) {
    if (this != &other)
        sessions = other.sessions;
    return *this;
}

SessionManager::~SessionManager() {}

bool SessionManager::isIdTaken(const String& id) const {
    return sessions.find(id) != sessions.end();
}

String SessionManager::createSession(const String& userId) {
    String id;
    do {
        id = generateGUID();
    } while (isIdTaken(id));

    sessions[id] = SessionResult(id, userId);
    Logger::info("[SESSION]: Created session " + id.substr(0, 8) + "... for user " + userId);
    return id;
}

SessionResult* SessionManager::getSession(const String& sessionId) {
    SessionMap::iterator it = sessions.find(sessionId);
    if (it == sessions.end())
        return NULL;
    if (it->second.isExpired(SESSION_TIMEOUT)) {
        Logger::info("[SESSION]: Expired session " + sessionId.substr(0, 8) + "...");
        sessions.erase(it);
        return NULL;
    }
    it->second.touch();
    return &it->second;
}

bool SessionManager::removeSession(const String& sessionId) {
    SessionMap::iterator it = sessions.find(sessionId);
    if (it == sessions.end())
        return false;
    Logger::info("[SESSION]: Removed session " + sessionId.substr(0, 8) + "...");
    sessions.erase(it);
    return true;
}

void SessionManager::cleanupExpiredSessions(int timeoutSeconds) {
    SessionMap::iterator it = sessions.begin();
    while (it != sessions.end()) {
        if (it->second.isExpired(timeoutSeconds)) {
            Logger::info("[SESSION]: Cleanup expired session " + it->first.substr(0, 8) + "...");
            SessionMap::iterator toErase = it;
            ++it;
            sessions.erase(toErase);
        } else {
            ++it;
        }
    }
}

bool SessionManager::isValid(const String& sessionId) const {
    SessionMap::const_iterator it = sessions.find(sessionId);
    if (it == sessions.end())
        return false;
    return !it->second.isExpired(SESSION_TIMEOUT);
}

String SessionManager::regenerateId(const String& oldSessionId) {
    SessionMap::iterator it = sessions.find(oldSessionId);
    if (it == sessions.end())
        return "";

    String        newId = generateGUID();
    SessionResult copy  = it->second;
    copy.sessionId      = newId;
    copy.touch();
    sessions.erase(it);
    sessions[newId] = copy;

    Logger::info("[SESSION]: Regenerated " + oldSessionId.substr(0, 8) + "... -> " + newId.substr(0, 8) + "...");
    return newId;
}

String SessionManager::buildSetCookieHeader(const String& sessionId) {
    return String(SESSION_COOKIE_NAME) + "=" + sessionId + "; Path=/; HttpOnly";
}

String SessionManager::buildExpiredCookieHeader() {
    return String(SESSION_COOKIE_NAME) + "=; Path=/; HttpOnly; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0";
}

size_t SessionManager::getSessionCount() const {
    return sessions.size();
}
