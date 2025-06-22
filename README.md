# Socket Library

A lightweight C++ socket communication library for sending and receiving messages or files over TCP connections. Designed to support both client and server modes.

---

## Features

* Support for both server and client roles
* Send and receive:

  * Text messages
  * Binary file attachments
* Custom 128-byte header format with metadata
* Easy integration into other C++ projects
* Error logging with optional verbosity

---

## File Structure

```
├── socket.cpp          # Socket class implementation
├── socket.hpp          # Socket class declaration
├── socket_test.cpp     # Sample test app for server/client
├── Makefile            # Build file
├── README.md           # This file
```

---

## Header Format

Each transmission begins with a fixed-size (128 bytes) ASCII header:

```
fileType:<message|attachment>,fileName:<name>,fileSize:<bytes>;
```

* `fileType`: "message" or "attachment"
* `fileName`: used for attachments only
* `fileSize`: size of the content after the header

---

## Build Instructions

```bash
make            # Builds the test application
make clean      # Removes build artifacts
```

---

## Usage

### Start a Server

```bash
./socket_test server 0.0.0.0 12345 ./received
```

### Run a Client

```bash
./socket_test client 127.0.0.1 12345 "Hello from client" ./sample.txt
```

---

## Integration Example

```cpp
Socket client("127.0.0.1", 12345, true, false);
std::string msg = "Hello";
client.sendMessage(msg);
client.sendFile("image.jpg");
```

---

## Limitations

* Only handles one client at a time (single-threaded server)
* No encryption (use TLS wrappers for secure transport)

---

## Future Improvements

* Multithreaded server support
* Timeout and retry logic
* Secure (TLS) communication
* File checksum validation

---

## License

This project is open-source and free to use under the GPL-3.0 License.
