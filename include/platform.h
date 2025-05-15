#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>
#include <time.h>

#ifdef __unix__
#include <sys/types.h>  /* For ssize_t and off_t */
#endif

/**
 * Platform-independent layer for the HTTP file server.
 * This header defines the interface that platform-specific
 * implementations should provide.
 */

/* Common defines */
#define BUFFER_SIZE 1024
#define MAX_PATH_SIZE 1024

/* Platform detection */
#ifdef _WIN32
    /* Path separator for Windows */
    #define PATH_SEPARATOR '\\'
    
    /* Types that might be missing on Windows */
    #if !defined(ssize_t)
    typedef int ssize_t;
    #endif
    
    #if !defined(off_t)
    typedef long off_t;
    #endif
    
    /* For socket compatibility */
    typedef int socklen_t;
    
    /* Socket API compatibility */
    /* Don't redefine close as closesocket, use closesocket directly */
    
    /* String comparison compatibility */
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    
    /* Binary mode for files */
    #ifndef O_BINARY
    #define O_BINARY 0x8000
    #endif
#else
    /* Path separator for Unix */
    #define PATH_SEPARATOR '/'
    
    /* Add Unix-specific definitions */
    #ifndef O_BINARY
    #define O_BINARY 0
    #endif
    
    /* Include required Unix headers */
    #include <unistd.h>    /* For standard Unix functions */
    #include <sys/types.h>  /* For data types */
    #include <sys/stat.h>   /* For file status and S_ISDIR */
#endif

/**
 * Initialize platform-specific resources (e.g. Winsock on Windows).
 * Must be called before using any network functions.
 *
 * @return 0 on success, non-zero on failure
 */
int platform_init(void);

/**
 * Clean up platform-specific resources.
 * Should be called when the application exits.
 */
void platform_cleanup(void);

/**
 * Send a file over a socket.
 * 
 * @param out_fd The socket to send to
 * @param in_fd The file descriptor to read from
 * @param offset The offset to start from (can be NULL)
 * @param count The number of bytes to send
 * @return The number of bytes sent, or -1 on error
 */
ssize_t platform_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

/**
 * Callback function for directory listing.
 * Called for each entry in a directory.
 * 
 * @param name The name of the entry
 * @param is_dir 1 if the entry is a directory, 0 if it's a file
 * @param size The size of the file in bytes
 * @param mtime The last modification time
 * @param user_data User-defined data passed to platform_list_directory
 * @return 0 to continue listing, non-zero to stop
 */
typedef int (*dir_entry_callback)(const char* name, int is_dir, size_t size, 
                               time_t mtime, void* user_data);

/**
 * List the contents of a directory.
 * 
 * @param path The directory to list
 * @param callback The callback to call for each entry
 * @param user_data User-defined data to pass to the callback
 * @return 0 on success, non-zero on failure
 */
int platform_list_directory(const char* path, dir_entry_callback callback, void* user_data);

/**
 * Set a socket's blocking mode.
 * 
 * @param socket The socket descriptor
 * @param blocking 1 for blocking mode, 0 for non-blocking
 */
void platform_set_socket_blocking(int socket, int blocking);

/**
 * Set timeouts for socket operations.
 * 
 * @param socket The socket descriptor
 * @param seconds The timeout in seconds
 */
void platform_set_socket_timeouts(int socket, int seconds);

/**
 * Sleep for a specified number of milliseconds.
 * 
 * @param milliseconds The number of milliseconds to sleep
 */
void platform_sleep_ms(int milliseconds);

/**
 * Get a string describing the last error that occurred.
 * 
 * @return A string describing the error
 */
const char* platform_get_error_string(void);

#endif /* PLATFORM_H */