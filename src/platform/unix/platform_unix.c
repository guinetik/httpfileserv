#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>  /* For S_ISDIR */
#include <limits.h>    /* For PATH_MAX */
#include <signal.h>    /* For signal handling */
#include <errno.h>

/**
 * Unix-specific implementation of platform functions
 */

int platform_init(void) {
    // Set up signal handling to ignore SIGPIPE
    // This prevents the program from crashing when writing to a closed socket
    signal(SIGPIPE, SIG_IGN);
    return 0;
}

void platform_cleanup(void) {
    // No cleanup needed on Unix-like systems
}

// On Unix, we need to implement the platform sendfile function
// We need a custom implementation to avoid conflicts with the system sendfile
ssize_t platform_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    // For WSL and other Unix systems, we'll use a simple read/write implementation
    // to avoid issues with the system sendfile function
    char buffer[8192];
    ssize_t total_sent = 0;
    size_t remaining = count;
    ssize_t bytes_read, bytes_sent;

    if (offset && *offset) {
        if (lseek(in_fd, *offset, SEEK_SET) == -1) {
            return -1;
        }
    }

    while (remaining > 0) {
        bytes_read = read(in_fd, buffer, sizeof(buffer) < remaining ? sizeof(buffer) : remaining);
        if (bytes_read <= 0) {
            if (bytes_read < 0) printf("[ERROR] Read error: %s\n", strerror(errno));
            break;
        }

        bytes_sent = write(out_fd, buffer, bytes_read);
        if (bytes_sent <= 0) {
            if (bytes_sent < 0) printf("[ERROR] Write error: %s\n", strerror(errno));
            return -1;
        }

        total_sent += bytes_sent;
        remaining -= bytes_sent;

        if (offset) {
            *offset += bytes_sent;
        }
    }

    return total_sent;
}

int platform_list_directory(const char* path, dir_entry_callback callback, void* user_data) {
    DIR* dir;
    struct dirent* entry;
    struct stat stat_buf;
    char full_path[PATH_MAX];
    
    printf("[DEBUG] Unix listing directory: %s\n", path);
    
    // Open directory
    if ((dir = opendir(path)) == NULL) {
        printf("[ERROR] opendir failed: %s\n", strerror(errno));
        return 1;  // Error
    }
    
    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Get full path for the entry
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // Get file info
        if (stat(full_path, &stat_buf) != 0) {
            printf("[WARNING] stat failed for '%s': %s\n", full_path, strerror(errno));
            continue;  // Skip if can't get info
        }
        
        int is_dir = S_ISDIR(stat_buf.st_mode);
        
        printf("[DEBUG] Found: %s (%s, %zu bytes)\n", 
               entry->d_name, 
               is_dir ? "directory" : "file", 
               (size_t)stat_buf.st_size);
        
        // Call the callback
        if (callback(entry->d_name, is_dir, stat_buf.st_size, stat_buf.st_mtime, user_data) != 0) {
            printf("[DEBUG] Callback requested to stop directory listing\n");
            closedir(dir);
            return 0;  // Callback requested stop
        }
    }
    
    if (errno != 0) {
        printf("[ERROR] readdir failed: %s\n", strerror(errno));
    }
    
    closedir(dir);
    printf("[DEBUG] Unix directory listing completed\n");
    return 0;
}

void platform_set_socket_blocking(int socket, int blocking) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        printf("[ERROR] fcntl(F_GETFL) failed: %s\n", strerror(errno));
        return;
    }
    
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    
    if (fcntl(socket, F_SETFL, flags) < 0) {
        printf("[ERROR] fcntl(F_SETFL) failed: %s\n", strerror(errno));
    } else {
        printf("[DEBUG] Socket %d set to %s mode\n", socket, blocking ? "blocking" : "non-blocking");
    }
}

void platform_set_socket_timeouts(int socket, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        printf("[ERROR] setsockopt(SO_RCVTIMEO) failed: %s\n", strerror(errno));
    }
    
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        printf("[ERROR] setsockopt(SO_SNDTIMEO) failed: %s\n", strerror(errno));
    }
    
    printf("[DEBUG] Socket %d timeouts set to %d seconds\n", socket, seconds);
}

void platform_sleep_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

const char* platform_get_error_string() {
    return strerror(errno);
}