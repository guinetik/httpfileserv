#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <errno.h>

/**
 * Unix-specific implementation of platform functions
 */

int platform_init(void) {
    // No initialization needed on Unix-like systems
    return 0;
}

void platform_cleanup(void) {
    // No cleanup needed on Unix-like systems
}

// Note: sendfile is already provided by system headers on Unix platforms

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