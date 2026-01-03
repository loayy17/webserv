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
├─ CMakeLists.txt
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

Manual single-file build (if your `main.cpp` is at project root):

```bash
g++ -std=c++98 main.cpp -o server
./server
```

If using the scaffolded `src/` tree with CMake (recommended for multi-file builds):

```bash
mkdir -p build && cd build
cmake ..
make
./webserv    # or the target name you set in CMakeLists
```

Note: ensure `src/CMakeLists.txt` exists and lists sources; the top-level `CMakeLists.txt` in this scaffold references `src/`.

## Troubleshooting: port already in use

If the server fails to `bind()` because the port is in use, either stop the other process or pick a different port.

Find the process using a port (example for port 8080):

```bash
ss -ltnp | grep :8080
sudo lsof -i :8080
```

Stop the process (use the PID from the commands above):

```bash
kill <PID>
# or if necessary:
sudo kill -9 <PID>
```

Or change the server listening port in configuration or source (`htons(...)`) and recompile.

Note: `SO_REUSEADDR` allows reusing addresses in `TIME_WAIT`, but it does not allow two different processes to listen on the same TCP port.

## Contributing

- Follow the repository layout and add unit tests under `tests/`.
- Open a PR with clear description and targeted changes.

## License

Add your preferred license file to the repo (e.g., `LICENSE`).

---

If you'd like, I can:
- generate a `src/CMakeLists.txt` to build the scaffolded sources, or
- make `main.cpp` accept a `--port` argument and env var for runtime configuration.
Tell me which and I'll implement it.


