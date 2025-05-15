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
    char* listing;           /**< Buffer containing the HTML listing */
    const char* url_path;    /**< URL path being listed */
    size_t listing_size;     /**< Current size of the listing content */
    size_t listing_capacity; /**< Total capacity of the listing buffer */
} dir_listing_data;

/**
 * @brief Callback function for processing directory entries during directory listing
 *
 * This function is called by platform_list_directory for each entry in a directory.
 * It formats the entry as HTML and appends it to the directory listing being built.
 * The function handles both files and directories with appropriate styling and icons.
 *
 * @param name The name of the directory entry (file or subdirectory)
 * @param is_dir Flag indicating if the entry is a directory (1) or a file (0)
 * @param size Size of the file in bytes (ignored for directories)
 * @param mtime Last modification time of the entry
 * @param user_data Pointer to a dir_listing_data structure containing the listing buffer
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
    if (data->listing_size + entry_len + 1 > data->listing_capacity) {
        data->listing_capacity = data->listing_capacity * 2 + entry_len;
        char* new_listing = realloc(data->listing, data->listing_capacity);
        if (!new_listing) {
            return 1; // Error, stop listing
        }
        data->listing = new_listing;
    }
    
    // Append to the listing
    strcat(data->listing, entry_html);
    data->listing_size += entry_len;
    
    return 0; // Continue listing
}

// Main function
int main(int argc, char* argv[]) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char* base_path;
    
    // Check if base path is provided
    if (argc < 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }
    
    base_path = argv[1];
    
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
    address.sin_port = htons(PORT);
    
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
    
    printf("Server started at http://localhost:%d\n", PORT);
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
    
    // Initialize the listing buffer
    dir_listing_data data;
    data.client_fd = client_fd;
    data.url_path = url_path;
    data.listing_capacity = BUFFER_SIZE * 16;
    data.listing = malloc(data.listing_capacity);
    data.listing_size = 0;
    
    if (!data.listing) {
        printf("[ERROR] Failed to allocate memory for directory listing\n");
        send_500(client_fd);
        return;
    }
    
    printf("[DEBUG] Building HTML for directory listing\n");
    
    // Initialize with HTML header - modernized version
    strcpy(data.listing, "");
    strcat(data.listing, "<!DOCTYPE html>");
    strcat(data.listing, "<html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
    strcat(data.listing, "<title>Directory: ");
    strcat(data.listing, url_path);
    strcat(data.listing, "</title>");
    strcat(data.listing, "<style>");
    strcat(data.listing, ":root{--bg-color:#f9f9f9;--container-bg:#fff;--text-color:#333;--header-color:#444;--border-color:#eee;--hover-color:#f8f8f8;--th-bg:#f5f5f5;--th-color:#666;--link-color:#2563eb;--icon-color:#666;--footer-color:#999;}");
    strcat(data.listing, ".dark-mode{--bg-color:#121212;--container-bg:#1e1e1e;--text-color:#e0e0e0;--header-color:#f0f0f0;--border-color:#333;--hover-color:#252525;--th-bg:#252525;--th-color:#aaa;--link-color:#90caf9;--icon-color:#aaa;--footer-color:#777;}");
    strcat(data.listing, "body{font-family:system-ui,-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,Oxygen,Ubuntu,Cantarell,sans-serif;margin:0;padding:20px;color:var(--text-color);background-color:var(--bg-color);transition:background-color 0.3s ease;}");
    strcat(data.listing, ".container{max-width:1000px;margin:0 auto;background-color:var(--container-bg);border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.05);padding:20px;transition:background-color 0.3s ease;}");
    strcat(data.listing, ".header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;padding-bottom:10px;border-bottom:1px solid var(--border-color);}");
    strcat(data.listing, "h1{color:var(--header-color);font-size:24px;margin:0;}");
    strcat(data.listing, ".theme-toggle{background:none;border:none;cursor:pointer;display:flex;align-items:center;font-size:14px;color:var(--text-color);padding:5px 10px;border-radius:4px;background-color:var(--hover-color);}");
    strcat(data.listing, ".theme-toggle:hover{opacity:0.9;}");
    strcat(data.listing, ".theme-icon{font-size:16px;margin-right:5px;}");
    strcat(data.listing, "table{width:100%;border-collapse:collapse;}");
    strcat(data.listing, "th{text-align:left;padding:12px 15px;background-color:var(--th-bg);font-weight:500;color:var(--th-color);border-bottom:2px solid var(--border-color);}");
    strcat(data.listing, "td{padding:10px 15px;border-bottom:1px solid var(--border-color);}");
    strcat(data.listing, "tr:hover{background-color:var(--hover-color);}");
    strcat(data.listing, "a{color:var(--link-color);text-decoration:none;}");
    strcat(data.listing, "a:hover{text-decoration:underline;}");
    strcat(data.listing, ".parent{margin-bottom:15px;display:inline-block;}");
    strcat(data.listing, ".icon{margin-right:5px;color:var(--icon-color);}");
    strcat(data.listing, ".size{color:var(--th-color);white-space:nowrap;}");
    strcat(data.listing, ".date{color:var(--th-color);white-space:nowrap;}");
    strcat(data.listing, ".footer{margin-top:20px;font-size:12px;color:var(--footer-color);text-align:center;}");
    strcat(data.listing, "</style>");
    strcat(data.listing, "<script>");
    strcat(data.listing, "function toggleTheme(){");
    strcat(data.listing, "  document.body.classList.toggle('dark-mode');");
    strcat(data.listing, "  localStorage.setItem('darkMode',document.body.classList.contains('dark-mode'));");
    strcat(data.listing, "  updateToggleText();");
    strcat(data.listing, "}");
    strcat(data.listing, "function updateToggleText(){");
    strcat(data.listing, "  const isDark=document.body.classList.contains('dark-mode');");
    strcat(data.listing, "  const toggle=document.getElementById('theme-toggle');");
    strcat(data.listing, "  if(toggle) toggle.innerHTML=isDark?'<span class=\"theme-icon\">☀️</span> Light Mode':'<span class=\"theme-icon\">🌙</span> Dark Mode';");
    strcat(data.listing, "}");
    strcat(data.listing, "window.onload=function(){");
    strcat(data.listing, "  const prefersDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme:dark)').matches;");
    strcat(data.listing, "  const storedTheme=localStorage.getItem('darkMode');");
    strcat(data.listing, "  if(storedTheme==='true'){");
    strcat(data.listing, "    document.body.classList.add('dark-mode');");
    strcat(data.listing, "  }else if(storedTheme===null&&prefersDark){");
    strcat(data.listing, "    document.body.classList.add('dark-mode');");
    strcat(data.listing, "  }");
    strcat(data.listing, "  updateToggleText();");
    strcat(data.listing, "  document.getElementById('theme-toggle').addEventListener('click',toggleTheme);");
    strcat(data.listing, "};");
    strcat(data.listing, "</script>");
    strcat(data.listing, "</head>");
    strcat(data.listing, "<body>");
    strcat(data.listing, "<div class=\"container\">");
    strcat(data.listing, "<div class=\"header\">");
    strcat(data.listing, "<h1>Directory: ");
    strcat(data.listing, url_path);
    strcat(data.listing, "</h1>");
    strcat(data.listing, "<button id=\"theme-toggle\" class=\"theme-toggle\" type=\"button\"><span class=\"theme-icon\">🌙</span> Dark Mode</button>");
    strcat(data.listing, "</div>");
    
    // Add parent directory link if not at root
    if (strcmp(url_path, "/") != 0) {
        strcat(data.listing, "<div class=\"parent\"><a href=\"..\"><span class=\"icon\">⬆️</span> Parent Directory</a></div>");
    }
    
    strcat(data.listing, "<table><tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>");
    
    data.listing_size = strlen(data.listing);
    
    printf("[DEBUG] Calling platform_list_directory for '%s'\n", path);
    
    // List directory contents
    if (platform_list_directory(path, dir_listing_callback, &data) != 0) {
        printf("[ERROR] Failed to list directory: '%s'\n", path);
        free(data.listing);
        send_500(client_fd);
        return;
    }
    
    printf("[DEBUG] Directory listing retrieved successfully\n");
    
    // Finish HTML
    strcat(data.listing, "</table>");
    strcat(data.listing, "<div class=\"footer\">Powered by httpfileserv</div>");
    strcat(data.listing, "</div></body></html>");
    data.listing_size = strlen(data.listing);
    
    printf("[DEBUG] Generated %zu bytes of HTML\n", data.listing_size);
    
    // Send HTTP response header
    snprintf(response, BUFFER_SIZE, 
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n", 
             (long)data.listing_size);
    
    printf("[DEBUG] Sending HTTP header (%zu bytes)\n", strlen(response));
    int bytes_sent = send(client_fd, response, strlen(response), 0);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTTP header: %d - %s\n", bytes_sent, platform_get_error_string());
        free(data.listing);
        return;
    }
    
    printf("[DEBUG] Sending directory listing HTML (%zu bytes)\n", data.listing_size);
    bytes_sent = send(client_fd, data.listing, data.listing_size, 0);
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTML content: %d - %s\n", bytes_sent, platform_get_error_string());
    } else {
        printf("[DEBUG] Successfully sent %d bytes of HTML\n", bytes_sent);
    }
    
    free(data.listing);
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





