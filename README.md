# HTTPFileServ

A lightweight C library for serving files over HTTP. Zero dependencies, minimal footprint.

## Features

- Serve files and directory listings over HTTP
- Cross-platform support (Linux, macOS, Windows) with clean separation of platform-specific code
- Directory listing with file sizes and modification dates
- Proper MIME type detection for common file types
- URL decoding for proper handling of special characters in URLs
- Basic security features (path traversal prevention)
- Extensive debugging and error handling
- Simple API for integration into other applications

## Project Structure

```
httpfileserv/
├── bin/                  # Compiled binaries
├── examples/             # Example code using the library
├── include/              # Header files
│   ├── httpfileserv.h    # Main header
│   ├── httpfileserv_lib.h # Library API
│   └── platform.h        # Platform abstraction layer
├── src/                  # Source files
│   ├── httpfileserv.c    # Main server implementation
│   ├── httpfileserv_lib.c # Library API implementation
│   ├── platform/         # Platform-specific code
│   │   ├── platform.c    # Platform selection
│   │   ├── windows/      # Windows implementation
│   │   │   └── platform_windows.c
│   │   └── unix/         # Unix implementation
│   │       └── platform_unix.c
│   └── utils.c           # Utility functions
├── build.bat             # Windows build script
├── Makefile              # Unix/Linux build file
└── README.md             # This file
```

## Building

### Prerequisites

- C compiler (GCC, Clang, or MSVC)
- Make (or equivalent build system)

### Compilation

```bash
# Clone the repository
git clone https://github.com/yourusername/httpfileserv.git
cd httpfileserv

# Build
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
run_server.bat

# Run the server with a specific directory
run_server.bat C:\Files
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

```bash
httpfileserv /path/to/directory
```

Then open your browser to http://localhost:8080/

## Integration

To use as a library in your own C project:

```c
#include "httpfileserv.h"

int main() {
    // Your code here
    // ...
    
    // Start the server with a specific directory
    return start_server("/path/to/serve");
}
```

## License

MIT License - See the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.