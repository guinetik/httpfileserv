# httpfileserv

![Screenshot](image.png)

A lightweight C library for serving files over HTTP. Zero dependencies, minimal footprint.

## Demo

![Demo](demo.gif)
## Features

- Serve files and directory listings over HTTP
- Sleek and minimalistic design
- Cross-platform support (Linux, macOS, Windows) with clean separation of platform-specific code
- Directory listing with file sizes and modification dates
- Proper MIME type detection for common file types
- URL decoding for proper handling of special characters in URLs
- Basic security features (path traversal prevention)
- Extensive debugging and error handling
- Simple API for integration into other applications
- Custom MIME type configuration
- Request callbacks for logging and monitoring
- Socket timeout management to prevent stalled connections
- TCP_NODELAY support for improved responsiveness
- Keep-alive connections

## Project Structure

```
httpfileserv/
├── bin/                  # Compiled binaries
├── include/              # Header files
│   ├── httpfileserv.h    # Main header
│   ├── httpfileserv_lib.h # Library API
│   ├── http_response.h   # HTTP response handling
│   ├── platform.h        # Platform abstraction layer
│   └── utils.h           # Utility functions
├── src/                  # Source files
│   ├── httpfileserv.c    # Main server implementation
│   ├── httpfileserv_lib.c # Library API implementation
│   ├── http_response.c   # HTTP response handling
│   ├── template.c        # Template processing
│   ├── directory_template.html # HTML template for directory listings
│   ├── utils.c           # Utility functions
│   └── platform/         # Platform-specific code
│       ├── platform.c    # Platform selection
│       ├── windows/      # Windows implementation
│       │   └── platform_windows.c
│       └── unix/         # Unix implementation
│           └── platform_unix.c
├── obj/                  # Object files (created during build)
├── build.bat             # Windows build script
├── Makefile              # Unix/Linux build file
├── run.bat               # Windows run script
└── README.md             # This file
```

## Building

### Prerequisites

- C compiler (GCC, Clang, or MSVC)
- Make (for Unix/Linux) or Visual Studio Build Tools (for Windows)

### Compilation

```bash
# Clone the repository
git clone https://github.com/guinetik/httpfileserv.git
cd httpfileserv

# Build on Unix/Linux/macOS
make

# Install (Unix-like systems)
sudo make install
```

### Windows Specific Instructions

On Windows, there are two options to build and run the project:

#### Option 1: Windows-specific version (recommended for Windows users)

This version is optimized for Windows and uses native Windows APIs:

```cmd
# Build the Windows-specific version
build.bat

# Run the server (serves the current directory by default)
run.bat

# Run the server with a specific directory
run.bat C:\Files
```

#### Option 2: Cross-platform version

This version aims to be compatible with Windows, Linux, and macOS:

```cmd
# Using MinGW
mingw32-make

# Using Visual Studio Developer Command Prompt
cd msvc
nmake /f Makefile.msvc
```

## Usage

### Standalone Server

```bash
# Unix/Linux/macOS
./bin/httpfileserv /path/to/directory

# Windows
bin\httpfileserv.exe C:\path\to\directory
```

Then open your browser to http://localhost:8080/

### Library Integration

To use as a library in your own C project:

```c
#include "httpfileserv_lib.h"

int main() {
    // Initialize and start the server with a specific directory
    int result = start_server("/path/to/serve", 8080);
    if (result != 0) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    
    // Set a custom MIME type
    set_mime_type("md", "text/markdown");
    
    // Set a request callback for logging
    set_request_callback(my_request_handler);
    
    // Set server options
    set_server_option("timeout", "120");
    
    // Application code here
    
    // When done, stop the server
    stop_server();
    
    return 0;
}

// Example callback function
void my_request_handler(const char* method, const char* path, int status_code) {
    printf("[%s] %s - %d\n", method, path, status_code);
}
```

## Motivation and learnings

This is my first C project ever. I always felt kind of intimidated by it, but I needed to serve files over HTTP and I thought C was appropriate for the task. This section is for the future-me to document what I learned from this project. I'm sure I'll forget it all.

Learning C after being comfortable with JavaScript, Java, and Rust was a wild ride. In JavaScript, I never worried about memory - the garbage collector handles everything. In Java, objects get cleaned up automatically. Even in Rust, the borrow checker ensures memory safety and I know stuff is going to go wrong at compile time. But C? C forced me to manually malloc() memory and remember to free() it everywhere. No safety nets, that was a little intimidating.

The project has solidified my understanding of some key C programming patterns:

- **Structs vs Objects**: Instead of classes with methods, C uses structs for data and functions that operate on them. My `dir_listing_data` struct stored the state while separate functions processed it - a totally different mindset from OOP.

- **Memory Management**: I had to manually allocate buffers for strings with `malloc()`, resize them with `realloc()` when needed, and explicitly `free()` them when done. No garbage collector to save me if I forgot!

- **Function Pointers**: For callbacks like directory listing, I passed function pointers around - way more low-level than JavaScript's casual closures or Java's lambda expressions. I'm still not sure I understand how it works completely but that didn't stop me from using it.

- **Cross-Platform Abstraction**: Windows and Unix handle sockets completely differently. I had to create a platform abstraction layer with `#ifdef _WIN32` conditionals everywhere, hiding platform differences behind common functions. Not sure if this is the best way to do this but it works for now.

- **Buffer Sizing in C**: Buffer management is a fundamental aspect of C programming, and this project made that clear. Unlike higher-level languages, C requires explicit allocation and resizing of memory for strings and buffers, and it's up to the programmer to prevent overflows and leaks. There is no built-in dynamic string type or automatic resizing, so I had to implement manual buffer growth and always account for the null terminator. The standard approach I used was:

```c
// Initial allocation with some reasonable size
char* buffer = malloc(INITIAL_SIZE);
if (!buffer) {
    // Always check allocation success!
    return NULL;
}
size_t buffer_size = INITIAL_SIZE;
size_t used_size = 0;

// When adding content
if (used_size + new_content_length + 1 > buffer_size) {
    // Need more space - double the buffer size plus what we need
    buffer_size = buffer_size * 2 + new_content_length;
    char* new_buffer = realloc(buffer, buffer_size);
    if (!new_buffer) {
        // Reallocation failed - handle error and free original!
        free(buffer);
        return NULL;
    }
    buffer = new_buffer;
}

// Now we can safely add content
strcat(buffer + used_size, new_content);
used_size += new_content_length;
```

This pattern is all over the project, especially in the directory listing function where I'm building HTML content dynamically. It's verbose compared to `myString += newContent` in JavaScript, but it's necessary to prevent buffer overflows - one of the most common security vulnerabilities in C code.

The doubling strategy (reallocating to twice the current size plus what we need) is a classic optimization to prevent too many reallocations while still being memory-efficient. Each reallocation is potentially expensive, so we want to minimize them.

I also learned to be paranoid about null-termination. Every buffer that holds a string must have space for the null terminator and be explicitly terminated - something higher-level languages handle automatically. One missing null terminator can lead to hours of debugging weird string behavior.

- **Building on Windows:** Unlike npm install or cargo build, C doesn't have a standard build system, which is liberating and terrifying at the same time. I ended up using the Visual Studio compiler (cl.exe) from the command line since I didn't want to deal with the full Visual Studio IDE. The Windows build process requires finding the right include directories and libraries, while on Linux it's usually just gcc with the right flags. My build.bat script handles all this Windows-specific setup.

- **Building on Unix:** For Linux and macOS, I used a Makefile to automate the build. It's much simpler and more flexible than the Windows batch script—just run `make` to build or `make clean` to remove build files. Unix build tools are standardized, so the Makefile works out of the box, unlike on Windows where setup is more complicated. Having both build.bat and Makefile shows how different building C projects can be on each platform.

### Just WTF is an http server?

So I built this HTTP file server, but what exactly is an HTTP server anyway? Let me explain it to myself before I forget.

At its core, an HTTP server is just a program that listens for network connections, understands the HTTP protocol, and sends back responses. It's like a waiter at a restaurant - it waits for customers (clients/browsers), takes their orders (HTTP requests), and brings back food (files/data).

```
                              +-------------+
                              |             |
                              |  Internet   |
                              |             |
                              +------+------+
                                     |
                                     | HTTP Requests
                                     | (GET /file.txt)
                                     v
+----------------+           +-------+-------+           +----------------+
|                |  Request  |               |   read    |                |
|    Browser     +---------->+  HTTP Server  +---------->+   Filesystem   |
|    Client      |           |  (This app)   |           |                |
|                |<----------+               |<----------+                |
+----------------+  Response |               |   data    +----------------+
                   (200 OK)  +---------------+
                   + Content
```

The basic flow in my implementation works like this:

1. **Socket Creation**: First, I create a "socket" which is just a communication endpoint. It's like installing a telephone in your house so people can call you.
   ```c
   server_fd = socket(AF_INET, SOCK_STREAM, 0)
   ```

2. **Binding**: Then I "bind" this socket to a specific port (default 8080). This is like saying "my phone number is 8080" - now anyone who wants to talk to my server knows where to call.
   ```c
   bind(server_fd, (struct sockaddr *)&address, sizeof(address))
   ```

3. **Listening**: Next, I put the socket in "listen" mode, which means it's ready to accept incoming connections. This is like picking up the phone and saying "hello?"
   ```c
   listen(server_fd, 10)  // Can queue up to 10 connections
   ```

4. **Accepting Connections**: The server then waits in an infinite loop for clients to connect. When someone connects, the `accept()` function creates a new socket just for that client.
   ```c
   client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)
   ```

5. **Reading Requests**: Once connected, the server reads the HTTP request from the client. HTTP requests look like:
   ```
   GET /path/to/file.html HTTP/1.1
   Host: example.com
   ...other headers...
   ```

6. **Processing Requests**: The server parses this request to figure out what the client wants. In my case, it's either a file or a directory listing.

7. **Sending Responses**: Finally, the server sends back an HTTP response with headers and the requested content:
   ```
   HTTP/1.1 200 OK
   Content-Type: text/html
   Content-Length: 12345
   
   <html>...content here...</html>
   ```

The trickiest parts were:

- **Cross-platform compatibility**: Windows and Unix handle sockets completely differently. I had to create a platform abstraction layer with `#ifdef _WIN32` conditionals everywhere.

- **Memory management**: Building dynamic HTML for directory listings required careful buffer management. I had to manually `malloc()` and `realloc()` as the content grew.

- **Error handling**: Network programming has a million ways to fail. Every socket operation needs error checking.

- **MIME types**: The server needs to tell browsers what kind of file it's sending. I implemented a simple MIME type detection system based on file extensions.

- **Path traversal security**: I had to make sure users couldn't request files like "../../../etc/passwd" to access stuff outside the served directory.

The most satisfying part was seeing it all work together - type a URL, browser sends request, server parses it, reads a file from disk, and sends it back. All those layers of abstraction from high-level HTTP down to raw TCP/IP sockets, and it just works to render a beautiful html page with my directory listing. 

## Limitations

The project still has some technical debt. I don't really know how to unit test in C yet, so maybe I'll revisit this repo once I learn. I also want to add asynchronous I/O (async I/O) at some point. Why? Well, even though my server can queue up to 10 connections (in the listen() call), it still processes them one at a time sequentially. When a client connects, the server blocks until that entire request is fully processed before moving to the next one in the queue. With async I/O, the server could start handling a new connection while waiting for disk operations to complete on another. This would dramatically improve performance under load, especially when serving large files. But implementing this properly in C requires platform-specific shenanigans that just looking at stack-overflow gave me a headache so let's skip this one I'm sure it's a challenge worth its own project.

I didn't want to manage auth with C (seems like a security disaster waiting to happen), so I put this inside an nginx server that has auth. Being able to switch ports was really useful for this setup.

## Wrapping up

Overall, I'm pretty satisfied with how this turned out. The 181KB executable is doing exactly what I need - serving files over HTTP with minimal overhead. I can access it from my home network or over my private IP anywhere in the world. 

Just for fun I added github actions to build the project on linux and windows. I'm sure it's not the best way to do it but it works for now.

## License

MIT License - See the LICENSE file for details.

## Contributing

Contributions are welcome! I'm a C noob, but if you have any suggestions or improvements, please feel free to submit a Pull Request.