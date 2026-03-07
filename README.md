*This project has been created as part of the 42 curriculum by loayy17.*

# Webserv

## Description

A non-blocking HTTP/1.1 server written in C++98 using `poll()`.

The server handles **GET**, **POST**, **DELETE**, and **HEAD** methods. It supports static file serving, file uploads (multipart and raw), CGI execution (`fork` + `pipe` + `execve`), directory listing, HTTP redirects, custom error pages, chunked transfer encoding, virtual hosts, and persistent (keep-alive) connections. Cookie-based session management is implemented as a bonus.

The configuration file format follows NGINX syntax with `server {}` and `location {}` blocks.

## Instructions

### Build

```bash
make          # compile
make clean    # remove object files
make fclean   # remove object files + binary
make re       # full rebuild
```

### Run

```bash
./webserv [configuration file]
```

If no configuration file is given, `default.conf` is used.

### Configuration

```nginx
http {
    client_max_body_size 5M;

    server {
        listen       0.0.0.0:8080;
        server_name  localhost;
        root         ./www;
        index        welcome.html;

        error_page 404 ./www/errors/404.html;

        location / {
            methods   GET POST DELETE;
            autoindex on;
        }

        location /upload {
            methods    GET POST;
            upload_dir ./uploads;
            client_max_body_size 10M;
        }

        location /cgi-bin {
            root     ./www/cgi-bin;
            methods  GET POST;
            cgi_pass .py /usr/bin/python3;
        }

        location /old-page {
            return 301 /;
        }
    }
}
```

| Directive | Scope | Description |
|---|---|---|
| `listen` | server | Address and port to bind |
| `server_name` | server | Virtual host name matched against `Host:` header |
| `root` | server / location | Filesystem root |
| `index` | server / location | Default file(s) for directory requests |
| `client_max_body_size` | http / server / location | Maximum request body size |
| `error_page` | server / location | Custom error page path |
| `methods` | location | Allowed HTTP methods |
| `autoindex` | location | Enable/disable directory listing |
| `return` | location | HTTP redirect (`return 301 /target;`) |
| `upload_dir` | location | Upload storage directory |
| `cgi_pass` | location | Map extension to CGI interpreter |

### Test

```bash
# GET
curl -v http://localhost:8080/

# POST upload
curl -X POST -F "file=@myfile.txt" http://localhost:8080/upload

# DELETE
curl -X DELETE http://localhost:8080/delete/myfile.txt

# HEAD
curl -I http://localhost:8080/

# Included testers
python3 tester.py
bash tests/config_tester.sh
bash tests/request_tester.sh
bash tests/router_tester.sh
```

## Resources

- [RFC 7230 — HTTP/1.1 Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1 Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 — The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

**AI Usage:** GitHub Copilot was used for code review, debugging (memory leak analysis, non-blocking I/O validation), clarifying RFC edge cases, and documentation. All generated code was reviewed and validated by the authors.

## Project Structure

```
webserv/
├── Makefile                  # Build system
├── webserv.conf              # Example configuration
├── default.conf              # CI/tester configuration
├── README.md                 # This file
├── src/
│   ├── main.cpp              # Entry point, signal setup
│   ├── config/               # NGINX-style config lexer, parser, data
│   │   ├── ConfigLexer.cpp/hpp
│   │   ├── ConfigParser.cpp/hpp
│   │   ├── ConfigToken.cpp/hpp
│   │   ├── ServerConfig.cpp/hpp
│   │   ├── LocationConfig.cpp/hpp
│   │   ├── ListenAddressConfig.cpp/hpp
│   │   ├── MimeTypes.cpp/hpp
│   │   └── mime.types
│   ├── server/               # Core networking
│   │   ├── Server.cpp/hpp        # Socket bind/listen/accept
│   │   ├── ServerManager.cpp/hpp # poll() loop, request dispatch
│   │   ├── Client.cpp/hpp        # Per-connection state
│   │   └── PollManager.cpp/hpp   # poll() fd management
│   ├── http/                 # HTTP protocol layer
│   │   ├── HttpRequest.cpp/hpp
│   │   ├── HttpResponse.cpp/hpp
│   │   ├── ResponseBuilder.cpp/hpp
│   │   ├── Router.cpp/hpp
│   │   └── RouteResult.cpp/hpp
│   ├── handlers/             # Request handlers
│   │   ├── CgiHandler.cpp/hpp
│   │   ├── CgiProcess.cpp/hpp
│   │   ├── StaticFileHandler.cpp/hpp
│   │   ├── DirectoryListingHandler.cpp/hpp
│   │   ├── UploaderHandler.cpp/hpp
│   │   ├── DeleteHandler.cpp/hpp
│   │   ├── ErrorPageHandler.cpp/hpp
│   │   ├── FileHandler.cpp/hpp
│   │   └── IHandler.hpp
│   └── utils/                # Shared utilities
│       ├── Constants.hpp
│       ├── Enums.hpp
│       ├── Types.hpp
│       ├── Utils.cpp/hpp
│       ├── Logger.cpp/hpp
│       ├── SessionManager.cpp/hpp
│       └── SessionResult.cpp/hpp
├── www/                      # Default web root
│   ├── welcome.html          # Interactive feature showcase
│   ├── cgi-bin/              # CGI scripts
│   ├── errors/               # Custom error pages
│   ├── login/                # Login/signup UI
│   └── uploads_store/        # Default upload storage
├── tests/                    # Test suites
└── YoupiBanane/              # Evaluation test data
```

---

<p align="center">
  <sub>Made with ❤️ at 42 — because understanding HTTP from the inside changes how you see every website.</sub>
</p>
