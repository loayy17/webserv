#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "Utils.hpp"
#include "SessionResult.hpp"

typedef std::map<String, SessionResult> SessionMap;

class SessionManager
{
   public:
    SessionManager();
    SessionManager(const SessionManager& other);
    SessionManager& operator=(const SessionManager& other);
    ~SessionManager();

    String   createSession(const String& userId);
    SessionResult   * getSession(const String& sessionId);
    bool     removeSession(const String& sessionId);
    void     cleanupExpiredSessions(int timeoutSeconds);

    bool isValid(const String& sessionId) const;
    String regenerateId(const String& oldSessionId);
    static String buildSetCookieHeader(const String& sessionId);
    static String buildExpiredCookieHeader();
    size_t getSessionCount() const;

   private:
    SessionMap sessions;

    String generateSessionId() const;
    bool   isIdTaken(const String& id) const;
};

#endif
