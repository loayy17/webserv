#include "SessionResult.hpp"

SessionResult::SessionResult() : createdAt(0), lastActivity(0) {}

SessionResult::SessionResult(const String& sid, const String& uid) : sessionId(sid), userId(uid)
{
    createdAt    = getCurrentTime();
    lastActivity = createdAt;
}

SessionResult::SessionResult(const SessionResult& other)
    : sessionId(other.sessionId),
      userId(other.userId),
      createdAt(other.createdAt),
      lastActivity(other.lastActivity),
      data(other.data) {}

SessionResult& SessionResult::operator=(const SessionResult& other) {
    if (this != &other) {
        sessionId    = other.sessionId;
        userId       = other.userId;
        createdAt    = other.createdAt;
        lastActivity = other.lastActivity;
        data         = other.data;
    }
    return *this;
}

SessionResult::~SessionResult() {}

bool SessionResult::isExpired(int timeoutSeconds) const {
    return getDifferentTime(lastActivity, getCurrentTime()) > timeoutSeconds;
}

void SessionResult::touch() {
    updateTime(lastActivity);
}

String SessionResult::getData(const String& key) const {
    return findValueStrInMap(data, key);
}

void SessionResult::setData(const String& key, const String& value) {
    data[key] = value;
}

void SessionResult::removeData(const String& key) {
    data.erase(key);
}