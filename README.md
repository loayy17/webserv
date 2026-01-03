# Webserv

Lightweight HTTP server project scaffold. This README documents the repository layout, runtime workflow, build/run instructions, and quick troubleshooting.

## Project layout

Top-level layout (key files):

```
Webserv/
├─ src/
│   ├─ main.cpp
│   ├─ server/
│   │   ├─ Server.hpp
│   │   ├─ Server.cpp
│   │   ├─ PollManager.hpp
│   │   ├─ PollManager.cpp
│   │   ├─ Client.hpp
│   │   └─ Client.cpp
│   ├─ http/
│   │   ├─ HttpRequest.hpp
│   │   ├─ HttpRequest.cpp
│   │   ├─ HttpResponse.hpp
│   │   ├─ HttpResponse.cpp
│   │   ├─ Router.hpp
│   │   └─ Router.cpp
│   ├─ controllers/
│   │   ├─ BaseController.hpp
│   │   ├─ BaseController.cpp
│   │   ├─ StaticController.hpp
│   │   ├─ StaticController.cpp
│   │   ├─ ErrorController.hpp
│   │   ├─ ErrorController.cpp
│   │   ├─ ApiController.hpp
│   │   ├─ ApiController.cpp
│   │   └─ UploadController.hpp
│   │       UploadController.cpp
│   └─ utils/
│       ├─ Logger.hpp
│       ├─ Logger.cpp
│       ├─ FileManager.hpp
│       └─ FileManager.cpp
├─ include/
├─ tests/
└─ README.md
```

## Runtime workflow

1. Server start
   - `socket()` → create server socket
   - set `SO_REUSEADDR` (and optionally `SO_REUSEPORT`)
   - `bind()` to port(s)
   - `fcntl()` → set non-blocking
   - `listen()`

2. Poll loop (`PollManager`)
   - `poll()` / `epoll_wait()` to monitor fds

3. Accept new clients
   - `accept()` incoming connections
   - create `Client` object, set non-blocking, add to poll

4. Read requests
   - read into client buffer
   - parse `HttpRequest` and enqueue for processing

5. Route & handle
   - `Router` dispatches to controllers (`StaticController`, `ApiController`, `UploadController`, `ErrorController`)
   - controller builds `HttpResponse`

6. Write responses
   - write response to socket (handle partial writes)
   - respect `Connection: keep-alive` or close

7. Logging & utilities
   - `Logger` for requests/errors
   - `FileManager` for safe file I/O

## Build & run (quick)

```bash
c++ -Wall -Wextra -Werror -std=c++98 main.cpp -o server
./server
```
