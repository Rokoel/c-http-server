# Rokoel Web Server

A small POSIX C web server that serves static files from the configured views
directory. It supports basic `GET` and `POST` request parsing, threaded client
handling, MIME-type detection, logging, and a configurable 404 page.

## Requirements

- Linux or another POSIX-compatible system
- CMake 3.15 or newer
- A C11-compatible compiler
- POSIX threads

## Build

Configure and build with CMake:

```sh
cmake -S . -B build
cmake --build build
```

The executable is created at `build/server`.

## Run

Run the server from the `build` directory so the default relative paths work:

```sh
cd build
./server [port]
```

If no port is supplied, the server uses port `8080`. Runtime logs are written
to `../logs/` by default.

## Configuration

The views and logs directories can be set during CMake configuration:

```sh
cmake -S . -B build \
  -DROKOEL_VIEWS_FOLDER=/path/to/public/views \
  -DROKOEL_LOGS_FOLDER=/path/to/logs
```

The default views directory is `../public/views`.

## Source layout

- `core/core.c` — program entry point and server loop
- `core/server.c` — listening socket setup
- `core/socket.c` — connection handling and responses
- `core/split_buffer.c` — HTTP request parsing
- `core/get_filepath.c` — URL-to-file resolution
- `core/helpful.c` — logging, time formatting, port parsing, and file reading
- `public/views/` — static website files

## Current limitations

- Requests are limited by the server’s fixed request buffer.
- POST body handling is basic and does not yet fully implement HTTP framing.
- A new thread is created for each accepted connection.
- Automated tests are not currently included.
