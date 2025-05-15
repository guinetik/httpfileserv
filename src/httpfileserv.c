#include "httpfileserv.h"
#include "platform.h"
#include "http_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific includes for file operations
#ifdef _WIN32
#include <io.h>
#define open _open
#define close _close
#else
#include <unistd.h>
#include <fcntl.h>
#endif

// Structure to hold directory listing data for callback
/**
 * @brief Structure to hold directory listing data during generation
 *
 * This structure maintains the state of the HTML directory listing being built,
 * including the buffer for the HTML content, its size and capacity, and context
 * information like the client file descriptor and URL path.
 */
typedef struct {
    int client_fd;           /**< Client socket file descriptor */
    char* entries;           /**< Buffer containing the HTML entries */
    const char* url_path;    /**< URL path being listed */
    size_t entries_size;     /**< Current size of the entries content */
    size_t entries_capacity; /**< Total capacity of the entries buffer */
} dir_listing_data;

/**
 * @brief Callback function for processing directory entries during directory listing
 *
 * This function is called by platform_list_directory for each entry in a directory.
 * It formats the entry as an HTML table row and appends it to the entries buffer.
 * The function handles both files and directories with appropriate styling and icons.
 *
 * @param name The name of the directory entry (file or subdirectory)
 * @param is_dir Flag indicating if the entry is a directory (1) or a file (0)
 * @param size Size of the file in bytes (ignored for directories)
 * @param mtime Last modification time of the entry
 * @param user_data Pointer to a dir_listing_data structure containing the entries buffer
 *
 * @return 0 on success to continue listing, 1 on error to stop listing
 */
int dir_listing_callback(const char* name, int is_dir, size_t size, time_t mtime, void* user_data) {
    dir_listing_data* data = (dir_listing_data*)user_data;
    char entry_html[BUFFER_SIZE];
    char timestr[80];
    
    // Format last modified time
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&mtime));
    
    // Format the HTML for this entry with improved styling
    if (is_dir) {
        snprintf(entry_html, BUFFER_SIZE, 
                 "<tr><td><a href=\"%s/\"><span class=\"icon\">📁</span> %s/</a></td>"
                 "<td class=\"size\">-</td><td class=\"date\">%s</td></tr>", 
                 name, name, timestr);
    } else {
        // Format file size nicely
        char size_str[32];
        if (size < 1024) {
            snprintf(size_str, sizeof(size_str), "%ld B", (long)size);
        } else if (size < 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1f KB", size / 1024.0);
        } else if (size < 1024 * 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1f MB", size / (1024.0 * 1024.0));
        } else {
            snprintf(size_str, sizeof(size_str), "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
        }
        
        snprintf(entry_html, BUFFER_SIZE, 
                 "<tr><td><a href=\"%s\"><span class=\"icon\">📄</span> %s</a></td>"
                 "<td class=\"size\">%s</td><td class=\"date\">%s</td></tr>", 
                 name, name, size_str, timestr);
    }
    
    // Check if we need to resize the buffer
    size_t entry_len = strlen(entry_html);
    if (data->entries_size + entry_len + 1 > data->entries_capacity) {
        data->entries_capacity = data->entries_capacity * 2 + entry_len;
        char* new_entries = realloc(data->entries, data->entries_capacity);
        if (!new_entries) {
            return 1; // Error, stop listing
        }
        data->entries = new_entries;
    }
    
    // Append to the entries
    strcat(data->entries, entry_html);
    data->entries_size += entry_len;
    
    return 0; // Continue listing
}

// Main function
int main(int argc, char* argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char* base_path;
    int port = DEFAULT_PORT;
    
    // Check command-line arguments
    if (argc < 2) {
        printf("Usage: %s <directory_path> [port]\n", argv[0]);
        printf("  directory_path: Directory to serve files from\n");
        printf("  port: Optional port number (default: %d)\n", DEFAULT_PORT);
        return 1;
    }
    
    base_path = argv[1];
    
    // Check if port is provided as an argument
    if (argc >= 3) {
        int custom_port = atoi(argv[2]);
        if (custom_port > 0 && custom_port < 65536) {
            port = custom_port;
        } else {
            printf("Warning: Invalid port number '%s', using default port %d\n", 
                  argv[2], DEFAULT_PORT);
        }
    }
    
    // Initialize platform-specific functionality
    if (platform_init() != 0) {
        perror("Platform initialization failed");
        exit(EXIT_FAILURE);
    }
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt))) {
        perror("setsockopt");
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        platform_cleanup();
        exit(EXIT_FAILURE);
    }
    
    printf("Server started at http://localhost:%d\n", port);
    printf("Serving directory: %s\n", base_path);
    
    // Accept and handle connections
    while (1) {
        printf("Waiting for connections...\n");
        
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, 
                                (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        printf("Connection accepted (fd=%d)\n", client_fd);
        
        // Set socket options to reuse address and prevent "Address already in use" errors
        int reuse = 1;
        if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) {
            perror("setsockopt:SO_REUSEADDR on client socket");
        }
        
        // Disable Nagle's algorithm to improve responsiveness
        int nodelay = 1;
        if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay)) < 0) {
            perror("setsockopt:TCP_NODELAY");
        }

        // Set socket to blocking mode explicitly
        platform_set_socket_blocking(client_fd, 1);
        
        // Set socket timeout to prevent stalled connections
        platform_set_socket_timeouts(client_fd, 60);  // 60 seconds timeout
        
        // Keep-alive settings
        int keepalive = 1;
        if (setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&keepalive, sizeof(keepalive)) < 0) {
            perror("setsockopt:SO_KEEPALIVE");
        }
        
        printf("[DEBUG] Socket options set, handling connection...\n");
        
        // We wrap handle_connection in a simple error handler
        // to prevent a bad request from crashing the server
        handle_connection(client_fd, base_path);
        
        // Add delay before closing to ensure all data is sent
        platform_sleep_ms(500); // 500ms

        printf("[DEBUG] Closing connection (fd=%d)...\n", client_fd);
        
        #ifdef _WIN32
        if (closesocket(client_fd) != 0) {
            printf("[ERROR] Failed to close client socket: %d\n", WSAGetLastError());
        }
        #else
        if (close(client_fd) < 0) {
            perror("Failed to close client socket");
        }
        #endif
        
        printf("Connection closed.\n");
        
        // Small delay between connections
        platform_sleep_ms(100); // 100ms
    }
    
    // Cleanup (this code is never reached in this simple version)
    #ifdef _WIN32
    closesocket(server_fd);
    #else
    close(server_fd);
    #endif
    platform_cleanup();
    
    return 0;
}

void handle_connection(int client_fd, const char* base_path) {
    char buffer[BUFFER_SIZE] = {0};
    char method[32] = {0};
    char url[MAX_PATH_SIZE] = {0};
    char path[MAX_PATH_SIZE] = {0};
    
    printf("[DEBUG] Reading request from client_fd=%d...\n", client_fd);
    
    // Read request
    int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        // Connection closed or error
        printf("[ERROR] Failed to read from socket or connection closed: %d\n", bytes_read);
        if (bytes_read < 0) {
            printf("[ERROR] recv error: %s\n", platform_get_error_string());
        }
        return;
    }
    buffer[bytes_read] = '\0';
    
    printf("[DEBUG] Read %d bytes from client\n", bytes_read);
    printf("Request:\n%s\n", buffer);
    
    // Parse request line
    if (sscanf(buffer, "%31s %1023s", method, url) != 2) {
        printf("[ERROR] Failed to parse request: '%s'\n", buffer);
        send_400(client_fd);
        return;
    }
    
    printf("[DEBUG] Parsed request: method='%s', url='%s'\n", method, url);
    
    // Handle only GET requests
    if (strcmp(method, "GET") != 0) {
        printf("[ERROR] Unsupported method: '%s'\n", method);
        send_404(client_fd);
        return;
    }
    
    // URL decode the path
    char* decoded_url = url_decode(url);
    if (!decoded_url) {
        printf("[ERROR] Failed to decode URL: '%s'\n", url);
        send_500(client_fd);
        return;
    }
    
    printf("[DEBUG] Decoded URL: '%s'\n", decoded_url);
    
    // Construct file path (skipping the leading '/')
    const char* request_path = strcmp(decoded_url, "/") == 0 ? "" : decoded_url + 1;
    
    // Use correct path separator for the platform
    snprintf(path, MAX_PATH_SIZE, "%s%c%s", base_path, PATH_SEPARATOR, request_path);
    
    printf("[DEBUG] Raw path: '%s'\n", path);
    
    // Remove any potential path traversal vulnerabilities
    char* p;
    while ((p = strstr(path, "..")) != NULL) {
        printf("[WARNING] Path traversal attempt detected and blocked\n");
        memmove(p, p + 2, strlen(p + 2) + 1);
    }
    
    printf("[DEBUG] Accessing path: '%s'\n", path);
    
    // Check if path exists
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        printf("[ERROR] File not found: '%s' - %s\n", path, platform_get_error_string());
        send_404(client_fd);
        free(decoded_url);
        return;
    }
    
    // Check if it's a directory (portable way for Windows and Unix)
    int is_directory = (path_stat.st_mode & S_IFDIR) != 0;
    printf("[DEBUG] Path exists. Is directory: %s\n", is_directory ? "Yes" : "No");
    
    // If directory, send listing
    if (is_directory) {
        printf("[DEBUG] Sending directory listing for: '%s'\n", path);
        send_directory_listing(client_fd, path, decoded_url);
        printf("[DEBUG] Directory listing sent\n");
    } else {
        // If file, send the file
        printf("[DEBUG] Sending file: '%s' (size: %ld bytes)\n", path, (long)path_stat.st_size);
        send_file(client_fd, path);
        printf("[DEBUG] File sent\n");
    }
    
    printf("[DEBUG] Freeing decoded URL\n");
    free(decoded_url);
    printf("[DEBUG] Connection handling complete\n");
}

/**
 * @brief Generates and sends an HTML directory listing to the client
 *
 * This function creates a modern, responsive HTML page that displays the contents
 * of a directory with file/folder icons, sizes, and modification times. It handles
 * dynamic memory allocation for the listing and sends the complete HTML response
 * to the client.
 *
 * @param client_fd Socket file descriptor for the client connection
 * @param path Filesystem path to the directory being listed
 * @param url_path URL path corresponding to the directory (for display purposes)
 */
void send_directory_listing(int client_fd, const char* path, const char* url_path) {
    char response[BUFFER_SIZE];
    
    printf("[DEBUG] Preparing directory listing for '%s'\n", path);
    
    // Initialize the entries buffer
    dir_listing_data data;
    data.client_fd = client_fd;
    data.url_path = url_path;
    data.entries_capacity = BUFFER_SIZE * 16;
    data.entries = malloc(data.entries_capacity);
    data.entries_size = 0;
    
    if (!data.entries) {
        printf("[ERROR] Failed to allocate memory for directory listing\n");
        send_500(client_fd);
        return;
    }
    
    // Initialize with empty string
    strcpy(data.entries, "");
    
    printf("[DEBUG] Calling platform_list_directory for '%s'\n", path);
    
    // List directory contents
    if (platform_list_directory(path, dir_listing_callback, &data) != 0) {
        printf("[ERROR] Failed to list directory: '%s'\n", path);
        free(data.entries);
        send_500(client_fd);
        return;
    }
    
    printf("[DEBUG] Directory listing retrieved successfully\n");
    
    // Load the template file
    char template_path[MAX_PATH_SIZE];
    snprintf(template_path, MAX_PATH_SIZE, "src/directory_template.html");
    
    char* template_content = load_template(template_path);
    if (!template_content) {
        printf("[ERROR] Failed to load template file: %s\n", template_path);
        free(data.entries);
        send_500(client_fd);
        return;
    }
    
    // Process the template
    int has_parent = strcmp(url_path, "/") != 0;
    
    // Make sure we display the directory path correctly
    char display_path[MAX_PATH_SIZE];
    if (strcmp(url_path, "/") == 0) {
        strcpy(display_path, "/");
    } else {
        // Remove the leading slash for display if present
        const char* display_url = url_path;
        if (display_url[0] == '/') {
            display_url++;
        }
        snprintf(display_path, MAX_PATH_SIZE, "%s", display_url);
    }
    
    printf("[DEBUG] Using display path: '%s'\n", display_path);
    
    char* html_content = process_template(template_content, display_path, data.entries, has_parent);
    free(template_content);
    free(data.entries);
    
    if (!html_content) {
        printf("[ERROR] Failed to process template\n");
        send_500(client_fd);
        return;
    }
    
    size_t content_length = strlen(html_content);
    printf("[DEBUG] Generated %zu bytes of HTML\n", content_length);
    
    // Send HTTP response header
    snprintf(response, BUFFER_SIZE, 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n", 
             (long)content_length);
    
    printf("[DEBUG] Sending HTTP header (%zu bytes)\n", strlen(response));
    int bytes_sent = send(client_fd, response, strlen(response), 0);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTTP header: %d - %s\n", bytes_sent, platform_get_error_string());
        free(html_content);
        return;
    }
    
    printf("[DEBUG] Sending directory listing HTML (%zu bytes)\n", content_length);
    bytes_sent = send(client_fd, html_content, content_length, 0);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTML content: %d - %s\n", bytes_sent, platform_get_error_string());
    } else {
        printf("[DEBUG] Successfully sent %d bytes of HTML\n", bytes_sent);
    }
    
    free(html_content);
    printf("[DEBUG] Directory listing complete\n");
}

// Sends a file to the client with appropriate headers.
void send_file(int client_fd, const char* path) {
    int fd;
    struct stat file_stat;
    off_t offset = 0;
    char response[BUFFER_SIZE];
    ssize_t bytes_sent;
    
    printf("[DEBUG] Preparing to send file: '%s'\n", path);
    
    // Get file info
    if (stat(path, &file_stat) != 0) {
        printf("[ERROR] File does not exist: '%s' - %s\n", path, platform_get_error_string());
        send_404(client_fd);
        return;
    }
    
    printf("[DEBUG] File size: %ld bytes\n", (long)file_stat.st_size);
    
    // Open the file
    fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0) {
        printf("[ERROR] Failed to open file: '%s' - %s\n", path, platform_get_error_string());
        send_404(client_fd);
        return;
    }
    
    printf("[DEBUG] File opened successfully (fd=%d)\n", fd);
    
    // Get MIME type
    const char* mime_type = get_mime_type(path);
    printf("[DEBUG] MIME type: %s\n", mime_type);
    
    // Send HTTP response header
    snprintf(response, BUFFER_SIZE, 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n", 
             mime_type, (long)file_stat.st_size);
    
    printf("[DEBUG] Sending HTTP header (%zu bytes)\n", strlen(response));
    bytes_sent = send(client_fd, response, strlen(response), 0);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTTP header: %d - %s\n", bytes_sent, platform_get_error_string());
        close(fd);
        return;
    }
    
    // Send file content using sendfile
    printf("[DEBUG] Sending file content (%ld bytes)\n", (long)file_stat.st_size);
    bytes_sent = sendfile(client_fd, fd, &offset, file_stat.st_size);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send file content: %d - %s\n", bytes_sent, platform_get_error_string());
    } else {
        printf("[DEBUG] Successfully sent %ld bytes of file content\n", (long)bytes_sent);
    }
    
    if (close(fd) < 0) {
        printf("[ERROR] Failed to close file (fd=%d) - %s\n", fd, platform_get_error_string());
    } else {
        printf("[DEBUG] File closed successfully\n");
    }
}





