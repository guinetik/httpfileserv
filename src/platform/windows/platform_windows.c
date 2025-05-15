/**
 * platform_windows.c - Windows-specific platform implementation
 * 
 * This file contains Windows implementations of the platform-specific functions
 * needed by our HTTP file server. Windows uses different APIs than Unix-like
 * systems for networking, file operations, and system calls.
 * 
 * Key Windows-specific components used in this file:
 * - Winsock2: Windows' implementation of Berkeley sockets API
 * - Win32 API: For file system operations and error handling
 * - Windows HANDLE-based I/O: Different from Unix file descriptors
 */

#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>  /* Windows Socket API - Windows' implementation of Berkeley sockets */
#include <windows.h>   /* Core Windows API functions */
#include <io.h>        /* Low-level I/O functions (_read, _lseek, etc.) */
#include <fcntl.h>     /* File control options */

/**
 * Initialize platform-specific resources
 * 
 * On Windows, we need to initialize the Winsock library before using any socket functions.
 * This is not needed on Unix-like systems where sockets are part of the standard library.
 * 
 * WSAStartup loads the Winsock DLL (ws2_32.dll) and establishes the Winsock API.
 * MAKEWORD(2,2) requests Winsock version 2.2.
 * 
 * @return 0 on success, non-zero error code on failure
 */
int platform_init(void) {
    WSADATA wsaData;  /* Structure to receive details of Winsock implementation */
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("[ERROR] WSAStartup failed: %d\n", result);
    }
    return result;
}

/**
 * Clean up platform-specific resources
 * 
 * On Windows, we need to clean up Winsock resources when we're done.
 * WSACleanup unloads the Winsock DLL and cancels any outstanding operations.
 */
void platform_cleanup(void) {
    WSACleanup();  /* Terminates use of the Winsock DLL */
}

/**
 * Windows implementation of sendfile - transfers data between file descriptor and socket
 * 
 * Unix systems have a native sendfile() system call for zero-copy file transfers,
 * but Windows doesn't have an exact equivalent. This function implements similar
 * functionality by reading from a file and writing to a socket in chunks.
 * 
 * @param out_fd Socket descriptor to send data to
 * @param in_fd File descriptor to read data from
 * @param offset Pointer to offset in file to start reading from (can be NULL)
 * @param count Number of bytes to transfer
 * @return Number of bytes sent, or -1 on error
 */
ssize_t platform_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    char buffer[8192];  /* Buffer for reading file data - 8KB is a common efficient buffer size */
    ssize_t total_sent = 0;
    size_t remaining = count;
    ssize_t to_read, bytes_read, bytes_sent;
    
    printf("[DEBUG] Windows sendfile: out_fd=%d, in_fd=%d, count=%zu\n", out_fd, in_fd, count);

    /* If offset is provided, seek to that position in the file */
    if (offset && *offset) {
        /* Windows uses _lseek instead of lseek for file positioning */
        if (_lseek(in_fd, (long)*offset, SEEK_SET) == -1) {
            printf("[ERROR] _lseek failed: %lu\n", GetLastError());
            return -1;
        }
    }

    /* Read and send data in chunks until we've sent 'count' bytes or hit EOF */
    while (remaining > 0) {
        /* Determine how much to read - either the buffer size or remaining bytes */
        to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        
        /* Windows uses _read instead of read */
        bytes_read = _read(in_fd, buffer, (unsigned int)to_read);
        
        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                printf("[ERROR] _read failed: %lu\n", GetLastError());
            }
            break;  /* End of file or error */
        }
        
        printf("[DEBUG] Read %ld bytes from file\n", (long)bytes_read);
        
        /* Windows send function takes int instead of size_t for the length */
        bytes_sent = send(out_fd, buffer, (int)bytes_read, 0);
        if (bytes_sent <= 0) {
            /* Windows uses WSAGetLastError() instead of errno for socket errors */
            printf("[ERROR] send failed: %d\n", WSAGetLastError());
            return -1;  /* Error */
        }
        
        printf("[DEBUG] Sent %ld bytes to socket\n", (long)bytes_sent);
        
        total_sent += bytes_sent;
        remaining -= bytes_sent;
        
        /* Update the offset if provided */
        if (offset) {
            *offset += bytes_sent;
        }
    }
    
    printf("[DEBUG] Windows sendfile completed: total sent=%ld\n", (long)total_sent);
    return total_sent;
}

/**
 * List directory contents and call a callback function for each entry
 * 
 * Windows uses a completely different API for directory listing compared to Unix.
 * Instead of opendir/readdir, Windows uses FindFirstFile/FindNextFile with a 
 * WIN32_FIND_DATA structure to store file information.
 * 
 * @param path Directory path to list
 * @param callback Function to call for each directory entry
 * @param user_data User data to pass to the callback
 * @return 0 on success, non-zero on error
 */
int platform_list_directory(const char* path, dir_entry_callback callback, void* user_data) {
    WIN32_FIND_DATAA find_data;  /* Structure to receive file info (ANSI version) */
    HANDLE find_handle;          /* Windows handle for the find operation */
    char search_path[MAX_PATH];  /* Buffer for the search path with wildcard */
    
    printf("[DEBUG] Windows listing directory: %s\n", path);
    
    /* Create search path with wildcard - Windows requires "\*" pattern to list directory */
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    
    /* Start finding files - Windows equivalent of opendir() */
    find_handle = FindFirstFileA(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) {
        printf("[ERROR] FindFirstFileA failed: %lu\n", GetLastError());
        return 1;  /* Error */
    }
    
    do {
        /* Skip "." and ".." entries - these are returned by Windows but often not needed */
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        /* Extract file information from the WIN32_FIND_DATA structure */
        
        /* Check if entry is a directory using file attributes */
        int is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        /* Get file size - Windows uses a high/low DWORD pair for 64-bit size */
        ULARGE_INTEGER filesize;
        filesize.HighPart = find_data.nFileSizeHigh;
        filesize.LowPart = find_data.nFileSizeLow;
        size_t size = (size_t)filesize.QuadPart;
        
        /* Convert Windows file time to Unix time_t
         * Windows time: 100-nanosecond intervals since January 1, 1601 (UTC)
         * Unix time: seconds since January 1, 1970 (UTC)
         * The conversion requires:
         * 1. Convert to 64-bit value (QuadPart)
         * 2. Convert from 100ns to seconds (divide by 10,000,000)
         * 3. Adjust for difference in epochs (subtract 11,644,473,600 seconds)
         */
        ULARGE_INTEGER ull;
        ull.LowPart = find_data.ftLastWriteTime.dwLowDateTime;
        ull.HighPart = find_data.ftLastWriteTime.dwHighDateTime;
        time_t mtime = (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
        
        printf("[DEBUG] Found: %s (%s, %zu bytes)\n", 
               find_data.cFileName, 
               is_dir ? "directory" : "file", 
               size);
        
        /* Call the callback with the file information */
        if (callback(find_data.cFileName, is_dir, size, mtime, user_data) != 0) {
            printf("[DEBUG] Callback requested to stop directory listing\n");
            FindClose(find_handle);
            return 0;  /* Callback requested stop */
        }
    } while (FindNextFileA(find_handle, &find_data) != 0);  /* Get next file, Windows equivalent of readdir() */
    
    /* Check if the loop ended because of an error or because there are no more files */
    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES) {
        printf("[ERROR] FindNextFileA failed: %lu\n", error);
    }
    
    /* Clean up the find handle - Windows equivalent of closedir() */
    FindClose(find_handle);
    printf("[DEBUG] Windows directory listing completed\n");
    return 0;
}

/**
 * Set a socket to blocking or non-blocking mode
 * 
 * Windows uses ioctlsocket() with FIONBIO flag instead of Unix's fcntl() with O_NONBLOCK.
 * 
 * @param socket The socket descriptor
 * @param blocking 1 for blocking mode, 0 for non-blocking mode
 */
void platform_set_socket_blocking(int socket, int blocking) {
    /* In Windows, mode is 0 for blocking, 1 for non-blocking (opposite of the parameter) */
    unsigned long mode = blocking ? 0 : 1;
    
    /* Windows uses ioctlsocket instead of fcntl for socket options */
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        printf("[ERROR] ioctlsocket(FIONBIO) failed: %d\n", WSAGetLastError());
    } else {
        printf("[DEBUG] Socket %d set to %s mode\n", socket, blocking ? "blocking" : "non-blocking");
    }
}

/**
 * Set socket receive and send timeouts
 * 
 * This is similar between Windows and Unix, but Windows uses DWORD for timeout
 * values instead of struct timeval directly.
 * 
 * @param socket The socket descriptor
 * @param seconds Timeout in seconds
 */
void platform_set_socket_timeouts(int socket, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    
    /* Set receive timeout */
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        printf("[ERROR] setsockopt(SO_RCVTIMEO) failed: %d\n", WSAGetLastError());
    }
    
    /* Set send timeout */
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        printf("[ERROR] setsockopt(SO_SNDTIMEO) failed: %d\n", WSAGetLastError());
    }
    
    printf("[DEBUG] Socket %d timeouts set to %d seconds\n", socket, seconds);
}

/**
 * Sleep for a specified number of milliseconds
 * 
 * Windows uses Sleep() while Unix systems typically use usleep() or nanosleep()
 * 
 * @param milliseconds Time to sleep in milliseconds
 */
void platform_sleep_ms(int milliseconds) {
    Sleep(milliseconds);  /* Windows Sleep function takes milliseconds directly */
}

/**
 * Get a string description of the last system error
 * 
 * Windows uses GetLastError() and FormatMessage() instead of errno and strerror()
 * 
 * @return String describing the last error that occurred
 */
const char* platform_get_error_string() {
    static char error_buf[256];  /* Static buffer to store the error message */
    DWORD error_code = GetLastError();  /* Get the last error code */
    
    /* Format the error message from the system */
    DWORD result = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,  /* Get message from system, don't process inserts */
        NULL,           /* No source buffer needed */
        error_code,     /* The error code to look up */
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  /* Default language */
        error_buf,      /* Buffer to store the message */
        sizeof(error_buf),  /* Size of the buffer */
        NULL            /* No arguments for inserts */
    );
    
    /* If FormatMessage failed, create a generic error message */
    if (result == 0) {
        snprintf(error_buf, sizeof(error_buf), "Unknown error (code: %lu)", error_code);
    }
    
    /* Remove trailing newline characters that Windows often adds to error messages */
    char* p = error_buf + strlen(error_buf) - 1;
    while (p >= error_buf && (*p == '\r' || *p == '\n')) {
        *p-- = '\0';
    }
    
    return error_buf;
}