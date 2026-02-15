#include "CgiProcess.hpp"
#include "../utils/Utils.hpp"

CgiProcess::CgiProcess() : _pid(-1), _writeFd(-1), _readFd(-1), _writeOffset(0), _startTime(0), _active(false) {}

CgiProcess::CgiProcess(const CgiProcess& other)
    : _pid(other._pid),
      _writeFd(other._writeFd),
      _readFd(other._readFd),
      _writeBuffer(other._writeBuffer),
      _writeOffset(other._writeOffset),
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
        _output      = other._output;
        _startTime   = other._startTime;
        _active      = other._active;
    }
    return *this;
}

CgiProcess::~CgiProcess() {}

void CgiProcess::init(pid_t pid, int writeFd, int readFd, const String& body) {
    _pid         = pid;
    _writeFd     = writeFd;
    _readFd      = readFd;
    _writeBuffer = body;
    _writeOffset = 0;
    _output.clear();
    _startTime = getCurrentTime();
    _active    = true;
}

void CgiProcess::reset() {
    _pid     = INVALID_FD;
    _writeFd = INVALID_FD;
    _readFd  = INVALID_FD;
    _writeBuffer.clear();
    _writeOffset = 0;
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
String CgiProcess::getOutput() const {
    return _output;
}

void CgiProcess::setWriteFd(int fd) {
    _writeFd = fd;
}
void CgiProcess::setReadFd(int fd) {
    _readFd = fd;
}

bool CgiProcess::isWriteDone() const {
    return _writeBuffer.empty() || _writeOffset >= _writeBuffer.size();
}

bool CgiProcess::writeBody(int fd) {
    if (isWriteDone())
        return true;
    const char* data      = _writeBuffer.c_str() + _writeOffset;
    size_t      remaining = _writeBuffer.size() - _writeOffset;
    ssize_t     w         = write(fd, data, remaining);
    if (w > 0)
        _writeOffset += w;
    else if (w < 0)
        return false;
    return isWriteDone();
}

void CgiProcess::appendOutput(const char* data, size_t len) {
    _output.append(data, len);
}

bool CgiProcess::handleRead() {
    char    buf[BUFFER_SIZE];
    ssize_t n = read(_readFd, buf, sizeof(buf));
    if (n > 0) {
        _output.append(buf, n);
        return true;
    }
    return false;
}

bool CgiProcess::finish() {
    if (_writeFd != INVALID_FD) {
        close(_writeFd);
        _writeFd = INVALID_FD;
    }
    if (_readFd != INVALID_FD) {
        close(_readFd);
        _readFd = INVALID_FD;
    }
    int status = 0;
    // WNOHANG is used to check if the child process has finished
    // when i give them to waitpid it will wait for the child process to finish
    // without WNOHANG it will block the main thread until the child process finishes
    // withit the main process not being blocked we can handle other requests
    pid_t ret = waitpid(_pid, &status, WNOHANG);
    if (ret == 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, &status, 0);
    }
    _active = false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void CgiProcess::cleanup() {
    if (!_active)
        return;
    if (_pid > 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, 0);
    }
    if (_writeFd != INVALID_FD)
        close(_writeFd);
    if (_readFd != INVALID_FD)
        close(_readFd);
    reset();
}
