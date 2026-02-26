#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctime>
#include "../utils/Types.hpp"

class CgiProcess {
   private:
    pid_t  _pid;
    int    _writeFd;
    int    _readFd;
    String _writeBuffer;
    size_t _writeOffset;
    String _output;
    time_t _startTime;
    bool   _active;
    bool   _readDone;
    bool   _writeDone;
    bool   _exited;
    int    _exitStatus;

   public:
    CgiProcess();
    CgiProcess(const CgiProcess& other);
    CgiProcess& operator=(const CgiProcess& other);
    ~CgiProcess();

    void   init(pid_t pid, int writeFd, int readFd, const String& body);
    void   reset();
    bool   isActive() const;
    pid_t  getPid() const;
    int    getWriteFd() const;
    int    getReadFd() const;
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
};

#endif
