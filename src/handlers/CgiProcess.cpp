#include "CgiProcess.hpp"
#include "../utils/Utils.hpp"

CgiProcess::CgiProcess()
    : _pid(-1),
      _writeFd(-1),
      _readFd(-1),
      _clientBuffer(NULL),
      _bodyOffset(0),
      _bodyLength(0),
      _writeOffset(0),
      _useInternal(false),
      _requestSize(0),
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
      _clientBuffer(other._clientBuffer),
      _bodyOffset(other._bodyOffset),
      _bodyLength(other._bodyLength),
      _writeOffset(other._writeOffset),
      _internalBuffer(other._internalBuffer),
      _useInternal(other._useInternal),
      _requestSize(other._requestSize),
      _output(other._output),
      _startTime(other._startTime),
      _active(other._active),
      _readDone(other._readDone),
      _writeDone(other._writeDone),
      _exited(other._exited),
      _exitStatus(other._exitStatus) {}

CgiProcess& CgiProcess::operator=(const CgiProcess& other) {
    if (this != &other) {
        _pid            = other._pid;
        _writeFd        = other._writeFd;
        _readFd         = other._readFd;
        _clientBuffer   = other._clientBuffer;
        _bodyOffset     = other._bodyOffset;
        _bodyLength     = other._bodyLength;
        _writeOffset    = other._writeOffset;
        _internalBuffer = other._internalBuffer;
        _useInternal    = other._useInternal;
        _requestSize    = other._requestSize;
        _output         = other._output;
        _startTime      = other._startTime;
        _active         = other._active;
        _readDone       = other._readDone;
        _writeDone      = other._writeDone;
        _exited         = other._exited;
        _exitStatus     = other._exitStatus;
    }
    return *this;
}

CgiProcess::~CgiProcess() {}

void CgiProcess::init(pid_t pid, int writeFd, int readFd, const String* clientBuffer, size_t bodyOffset, size_t bodyLength, size_t requestSize) {
    reset();
    _pid          = pid;
    _writeFd      = writeFd;
    _readFd       = readFd;
    _clientBuffer = clientBuffer;
    _bodyOffset   = bodyOffset;
    _bodyLength   = bodyLength;
    _writeOffset  = 0;
    _requestSize  = requestSize;
    _useInternal  = false;
    _startTime    = getCurrentTime();
    _active       = true;
}

void CgiProcess::initInternal(pid_t pid, int writeFd, int readFd, const String& internalBody, size_t requestSize) {
    reset();
    _pid            = pid;
    _writeFd        = writeFd;
    _readFd         = readFd;
    _internalBuffer = internalBody;
    _bodyLength     = _internalBuffer.size();
    _writeOffset    = 0;
    _requestSize    = requestSize;
    _useInternal    = true;
    _startTime      = getCurrentTime();
    _active         = true;
}

void CgiProcess::reset() {
    _pid          = INVALID_FD;
    _writeFd      = INVALID_FD;
    _readFd       = INVALID_FD;
    _clientBuffer = NULL;
    _bodyOffset   = 0;
    _bodyLength   = 0;
    _writeOffset  = 0;
    _internalBuffer.clear();
    _useInternal = false;
    _requestSize = 0;
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
size_t CgiProcess::getRequestSize() const {
    return _requestSize;
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
    return _writeDone || _bodyLength == 0 || _writeOffset >= _bodyLength;
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

    const char* data;
    if (_useInternal) {
        data = _internalBuffer.c_str() + _writeOffset;
    } else {
        if (!_clientBuffer) {
            _writeDone = true;
            return true;
        }
        data = _clientBuffer->c_str() + _bodyOffset + _writeOffset;
    }

    size_t  remaining = _bodyLength - _writeOffset;
    ssize_t w         = write(fd, data, remaining);
    if (w > 0)
        _writeOffset += w;
    else if (w < 0) {
        if (fd != INVALID_FD) {
            close(fd);
            _writeFd = INVALID_FD;
        }
        _writeDone = true;
        return true;
    }

    if (isWriteDone()) {
        if (fd != INVALID_FD) {
            close(fd);
            _writeFd = INVALID_FD;
        }
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