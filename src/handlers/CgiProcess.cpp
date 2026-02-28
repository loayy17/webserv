#include "CgiProcess.hpp"
#include "../utils/Utils.hpp"

CgiProcess::CgiProcess() : _pid(-1), _writeFd(-1), _readFd(-1), _writeOffset(0), _writeDone(true), _startTime(0), _active(false) {}

CgiProcess::CgiProcess(const CgiProcess& other)
    : _pid(other._pid),
      _writeFd(other._writeFd),
      _readFd(other._readFd),
      _writeBuffer(other._writeBuffer),
      _writeOffset(other._writeOffset),
      _writeDone(other._writeDone),
      _output(other._output),
      _startTime(other._startTime),
      _active(other._active) {}

CgiProcess& CgiProcess::operator=(const CgiProcess& other) {
    if (this != &other) {
        _pid         = other._pid;
        _writeFd     = other._writeFd;
        _readFd      = other._readFd;
        _writeBuffer = other._writeBuffer;
        _writeOffset = other._writeOffset;
        _writeDone   = other._writeDone;
        _output      = other._output;
        _startTime   = other._startTime;
        _active      = other._active;
    }
    return *this;
}

CgiProcess::~CgiProcess() {}

void CgiProcess::init(pid_t pid, int writeFd, int readFd) {
    _pid     = pid;
    _writeFd = writeFd;
    _readFd  = readFd;
    _writeBuffer.clear();
    _writeOffset = 0;
    _writeDone   = false;
    _output.clear();
    _startTime = getCurrentTime();
    _active    = true;
}

void CgiProcess::appendBuffer(const String& data) {
    _writeBuffer.append(data);
}

void CgiProcess::reset() {
    _pid     = INVALID_FD;
    _writeFd = INVALID_FD;
    _readFd  = INVALID_FD;
    _writeBuffer.clear();
    _writeOffset = 0;
    _writeDone   = false;
    _output.clear();
    _startTime = 0;
    _active    = false;
}

bool CgiProcess::isActive() const {
    return _active;
}
pid_t CgiProcess::getPid() const {
    return _pid;
}
int CgiProcess::getWriteFd() const {
    return _writeFd;
}
int CgiProcess::getReadFd() const {
    return _readFd;
}
time_t CgiProcess::getStartTime() const {
    return _startTime;
}
const String& CgiProcess::getOutput() const {
    return _output;
}

void CgiProcess::setWriteFd(int fd) {
    _writeFd = fd;
}
void CgiProcess::setReadFd(int fd) {
    _readFd = fd;
}

bool CgiProcess::isWriteDone() const {
    return _writeDone && (_writeBuffer.empty() || _writeOffset >= _writeBuffer.size());
}

void CgiProcess::setWriteDone(bool done) {
    _writeDone = done;
}

bool CgiProcess::writeBody(int fd) {
    if (isWriteDone())
        return true;
    bool madeProgress = false;
    while (_writeOffset < _writeBuffer.size()) {
        const char* data      = _writeBuffer.c_str() + _writeOffset;
        size_t      remaining = _writeBuffer.size() - _writeOffset;
        ssize_t     w         = write(fd, data, remaining);
        if (w > 0) {
            _writeOffset += w;
            madeProgress = true;
        } else {
            break;
        }
    }
    if (madeProgress)
        _startTime = getCurrentTime();

    if (_writeOffset >= _writeBuffer.size() && !_writeBuffer.empty()) {
        _writeBuffer.clear();
        _writeOffset = 0;
    }

    return isWriteDone();
}

void CgiProcess::appendOutput(const char* data, size_t len) {
    _output.append(data, len);
}

bool CgiProcess::handleRead() {
    char    buf[BUFFER_SIZE];
    bool    gotData = false;
    ssize_t n;
    while ((n = read(_readFd, buf, sizeof(buf))) > 0) {
        _output.append(buf, n);
        gotData = true;
    }
    if (gotData)
        _startTime = getCurrentTime();

    // Subject rule: never check errno.
    // If read returns 0, it means EOF. Return false to indicate we are done.
    if (n == 0)
        return false;
    return true; // Keep monitoring if we got data or if read returned -1 (non-blocking)
}

bool CgiProcess::finish() {
    if (_pid <= 0) 
        return false;
    if (_writeFd != INVALID_FD) {
        close(_writeFd);
        _writeFd = INVALID_FD;
    }
    if (_readFd != INVALID_FD) {
        close(_readFd);
        _readFd = INVALID_FD;
    }
    int   status = 0;
    pid_t ret    = waitpid(_pid, &status, WNOHANG);
    if (ret == 0) {
        kill(_pid, SIGTERM);
        ret = waitpid(_pid, &status, WNOHANG);
        if (ret == 0) {
            kill(_pid, SIGKILL);
            waitpid(_pid, &status, WNOHANG);
        }
    }
    _active = false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void CgiProcess::cleanup() {
    if (!_active)
        return;
    if (_pid > 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, WNOHANG);
    }
    if (_writeFd != INVALID_FD) {
        close(_writeFd);
        _writeFd = INVALID_FD;
    }
    if (_readFd != INVALID_FD) {
        close(_readFd);
        _readFd = INVALID_FD;
    }
    reset();
}

void CgiProcess::resetStartTime() {
    _startTime = getCurrentTime();
}
