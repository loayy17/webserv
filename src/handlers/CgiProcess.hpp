#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "../utils/Types.hpp"

class CgiProcess {
   public:
    CgiProcess();
    CgiProcess(const CgiProcess& other);
    CgiProcess& operator=(const CgiProcess& other);
    ~CgiProcess();

    void init(pid_t pid, int writeFd, int readFd, const String* clientBuffer, size_t bodyOffset, size_t bodyLength,
              size_t requestSize);

    // New init for internal buffer (used for chunked)
    void initInternal(pid_t pid, int writeFd, int readFd, const String& internalBody, size_t requestSize);

    void  reset();
    bool  isActive() const;
    pid_t getPid() const;
    int   getWriteFd() const;
    int   getReadFd() const;
    size_t getRequestSize() const;
    time_t getStartTime() const;
    String getOutput() const;

    void setWriteFd(int fd);
    void setReadFd(int fd);
    bool isWriteDone() const;

    bool writeBody(int fd);
    void appendOutput(const char* data, size_t len);
    bool handleRead();
    bool isReadDone() const;

    bool hasExited() const;
    int  getExitStatus() const;
    void setExited(int status);

    void setReadDone();
    void setWriteDone();
    void cleanup();

   private:
    pid_t         _pid;
    int           _writeFd;
    int           _readFd;

    // Streaming from client buffer
    const String* _clientBuffer;
    size_t        _bodyOffset;
    size_t        _bodyLength;
    size_t        _writeOffset;

    // Internal buffer for cases where we can't point to client buffer (e.g. chunked)
    String        _internalBuffer;
    bool          _useInternal;

    size_t        _requestSize; // size of the HTTP request to remove from client buffer when done
    String        _output;
    time_t        _startTime;
    bool          _active;
    bool          _readDone;
    bool          _writeDone;
    bool          _exited;
    int           _exitStatus;
};

#endif
