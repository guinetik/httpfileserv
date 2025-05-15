#include "httpfileserv_lib.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific includes for file operations
#ifdef _WIN32
#include <io.h>
#define close _close
#else
#include <unistd.h>
#endif

// Global variables for the server state
static int server_running = 0;
static int server_socket = -1;
static request_callback user_callback = NULL;
static char server_base_path[MAX_PATH_SIZE] = {0};
static int server_port = PORT;

// Custom MIME type handling
#define MAX_CUSTOM_MIME_TYPES 50
static struct {
    char extension[32];
    char mime_type[64];
} custom_mime_types[MAX_CUSTOM_MIME_TYPES];
static int num_custom_mime_types = 0;

int start_server(const char* directory_path, int port) {
    if (server_running) {
        fprintf(stderr, "Server is already running\n");
        return 1;
    }
    
    // Save the base path
    strncpy(server_base_path, directory_path, MAX_PATH_SIZE - 1);
    server_base_path[MAX_PATH_SIZE - 1] = '\0';
    
    // Use the provided port or default
    server_port = (port > 0) ? port : PORT;
    
    // Initialize platform-specific functionality
    if (platform_init() != 0) {
        fprintf(stderr, "Platform initialization failed\n");
        return 1;
    }
    
    // TODO: Implement actual server starting code here (from main())
    // This is a placeholder for now
    server_running = 1;
    printf("Server started (simulated) on port %d serving %s\n", 
           server_port, server_base_path);
    
    return 0;
}

void stop_server(void) {
    if (!server_running) {
        return;
    }
    
    // TODO: Implement proper server shutdown
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
    
    server_running = 0;
    platform_cleanup();
    printf("Server stopped\n");
}

void set_request_callback(request_callback callback) {
    user_callback = callback;
}

void set_mime_type(const char* extension, const char* mime_type) {
    if (num_custom_mime_types >= MAX_CUSTOM_MIME_TYPES) {
        fprintf(stderr, "Maximum number of custom MIME types reached\n");
        return;
    }
    
    // Remove leading dot if present
    const char* ext = extension;
    if (ext[0] == '.') {
        ext++;
    }
    
    // Check if this extension already exists
    for (int i = 0; i < num_custom_mime_types; i++) {
        if (strcasecmp(custom_mime_types[i].extension, ext) == 0) {
            // Update existing entry
            strncpy(custom_mime_types[i].mime_type, mime_type, 
                    sizeof(custom_mime_types[i].mime_type) - 1);
            custom_mime_types[i].mime_type[sizeof(custom_mime_types[i].mime_type) - 1] = '\0';
            return;
        }
    }
    
    // Add new entry
    strncpy(custom_mime_types[num_custom_mime_types].extension, ext, 
            sizeof(custom_mime_types[num_custom_mime_types].extension) - 1);
    custom_mime_types[num_custom_mime_types].extension[
        sizeof(custom_mime_types[num_custom_mime_types].extension) - 1] = '\0';
    
    strncpy(custom_mime_types[num_custom_mime_types].mime_type, mime_type, 
            sizeof(custom_mime_types[num_custom_mime_types].mime_type) - 1);
    custom_mime_types[num_custom_mime_types].mime_type[
        sizeof(custom_mime_types[num_custom_mime_types].mime_type) - 1] = '\0';
    
    num_custom_mime_types++;
}

int set_server_option(const char* option_name, const char* option_value) {
    // TODO: Implement server options
    printf("Setting server option: %s = %s\n", option_name, option_value);
    return 0;
}

// Function to be called when a request is processed
// This would be called from handle_connection() in the actual implementation
void invoke_request_callback(const char* method, const char* path, int status_code) {
    if (user_callback != NULL) {
        user_callback(method, path, status_code);
    }
}