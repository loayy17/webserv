#include "CgiProcess.hpp"
#include "../utils/Utils.hpp"

CgiProcess::CgiProcess()
    : _pid(-1),
      _writeFd(-1),
      _readFd(-1),
      _writeOffset(0),
      _startTime(0),
      _active(false),
      _readDone(false),
      _writeDone(false),
      _exited(false),
      _exitStatus(0) {}

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
    _startTime  = getCurrentTime();
    _active     = true;
    _readDone   = false;
    _writeDone  = false;
    _exited     = false;
    _exitStatus = 0;
}

void CgiProcess::reset() {
    _pid     = INVALID_FD;
    _writeFd = INVALID_FD;
    _readFd  = INVALID_FD;
    _writeBuffer.clear();
    _writeOffset = 0;
    _output.clear();
    _startTime  = 0;
    _active     = false;
    _readDone   = false;
    _writeDone  = false;
    _exited     = false;
    _exitStatus = 0;
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
    return _writeDone || _writeBuffer.empty() || _writeOffset >= _writeBuffer.size();
}

bool CgiProcess::writeBody(int fd) {
    if (isWriteDone()) {
        if (fd != INVALID_FD) {
            close(fd);
            _writeFd = INVALID_FD;
        }
        _writeDone = true;
        return true;
    }
    const char* data      = _writeBuffer.c_str() + _writeOffset;
    size_t      remaining = _writeBuffer.size() - _writeOffset;
    ssize_t     w         = write(fd, data, remaining);
    if (w > 0)
        _writeOffset += w;
    else if (w < 0) {
        close(fd);
        _writeFd   = INVALID_FD;
        _writeDone = true;
        return true;
    }
    if (isWriteDone()) {
        close(fd);
        _writeFd   = INVALID_FD;
        _writeDone = true;
        return true;
    }
    return false;
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

    if (_readFd != INVALID_FD) {
        close(_readFd);
        _readFd = INVALID_FD;
    }
    _readDone = true;
    return false;
}
bool CgiProcess::isReadDone() const {
    return _readDone;
}
bool CgiProcess::hasExited() const {
    return _exited;
}
int CgiProcess::getExitStatus() const {
    return _exitStatus;
}
void CgiProcess::setExited(int status) {
    _exited     = true;
    _exitStatus = status;
}
void CgiProcess::setReadDone() {
    _readDone = true;
}
void CgiProcess::setWriteDone() {
    _writeDone = true;
}

void CgiProcess::cleanup() {
    if (!_active)
        return;
    if (_pid > 0) {
        kill(_pid, SIGKILL);
        waitpid(_pid, NULL, WNOHANG);
    }
    if (_writeFd != INVALID_FD)
        close(_writeFd);
    if (_readFd != INVALID_FD)
        close(_readFd);
    reset();
}
