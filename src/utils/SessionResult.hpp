#ifndef SESSION_RESULT_HPP
#define SESSION_RESULT_HPP

#include "Utils.hpp"

class SessionResult
{
    public:
        SessionResult();
        SessionResult(const String& sid, const String& uid);
        SessionResult(const SessionResult& other);
        SessionResult& operator=(const SessionResult& other);
        ~SessionResult();

        bool isExpired(int timeoutSeconds) const;
        void touch();
        String getData(const String& key) const;
        void   setData(const String& key, const String& value);
        void   removeData(const String& key);

        String    sessionId;
        String    userId;
        time_t    createdAt;
        time_t    lastActivity;
        MapString data;

};

#endif