/**
 * http_response.c - HTTP response handling module
 * 
 * This file contains functions for generating and sending HTTP responses to clients.
 * It abstracts the details of HTTP protocol formatting and provides a simple interface
 * for sending common HTTP status responses like 404 Not Found or 500 Internal Server Error.
 * 
 * Key concepts covered in this file:
 * - HTTP response format (status line, headers, body)
 * - Cross-platform socket handling (Windows vs Unix)
 * - Error handling and reporting
 * - Memory management for variable-length responses
 */

#include "http_response.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

/* Include Windows socket headers for Windows platform */
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
/* Unix socket headers */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

/**
 * Send an HTTP response with the specified status code and message
 * 
 * This function forms the core of our HTTP response system. It constructs a properly
 * formatted HTTP response with status line, headers, and optional body, then sends it
 * to the client.
 * 
 * HTTP Response Format:
 * HTTP/1.1 [STATUS_CODE] [STATUS_TEXT]    <- Status line
 * Content-Type: [MIME_TYPE]               <- Headers
 * Content-Length: [LENGTH]
 * Connection: close
 * 
 * [BODY]                                  <- Response body (optional)
 * 
 * The function handles large responses by sending the header and body separately
 * if they won't fit in our buffer together.
 * 
 * @param client_fd The client socket file descriptor
 * @param status_code The HTTP status code (e.g., 200, 404, 500)
 * @param status_text The status text (e.g., "OK", "Not Found")
 * @param content_type The MIME type of the response (e.g., "text/html")
 * @param body The response body (can be NULL for empty response)
 */
void send_http_status(int client_fd, int status_code, const char* status_text, 
                     const char* content_type, const char* body) {
    char response[BUFFER_SIZE];  /* Buffer to hold the HTTP response */
    int bytes_sent;              /* Number of bytes sent to the client */
    size_t response_len;         /* Length of the response string */
    
    /* Use default content type if none provided */
    if (content_type == NULL) {
        content_type = "text/html";  /* Default to HTML for web browsers */
    }
    
    /* Calculate content length - important for the client to know when the response ends */
    size_t content_length = (body != NULL) ? strlen(body) : 0;
    
    /* Format the HTTP response header using snprintf for safety
     * This creates the status line and headers according to HTTP/1.1 protocol */
    snprintf(response, BUFFER_SIZE,
             "HTTP/1.1 %d %s\r\n"           /* Status line: HTTP/1.1 200 OK */
             "Content-Type: %s\r\n"         /* Content-Type header: text/html */
             "Content-Length: %zu\r\n"      /* Content-Length header: size of body */
             "Connection: close\r\n\r\n",   /* Connection header + empty line to separate headers from body */
             status_code, status_text, content_type, content_length);
    
    /* Add body if provided */
    if (body != NULL && content_length > 0) {
        response_len = strlen(response);
        
        /* Check if the complete response (header + body) fits in our buffer */
        if (response_len + content_length < BUFFER_SIZE) {
            /* If it fits, append the body to the header in the buffer */
            strcat(response, body);
        } else {
            /* If it doesn't fit, send header and body separately
             * This handles large responses that exceed our buffer size */
            
            /* Send header first */
            #ifdef _WIN32
            /* Windows requires casting the length to int for the send function */
            bytes_sent = send(client_fd, response, (int)strlen(response), 0);
            #else
            /* Unix systems use size_t for the length parameter */
            bytes_sent = send(client_fd, response, strlen(response), 0);
            #endif
            
            /* Check for errors when sending the header */
            if (bytes_sent < 0) {
                printf("[ERROR] Failed to send HTTP header: %d - %s\n", bytes_sent, platform_get_error_string());
                return;
            }
            
            /* Send body separately */
            #ifdef _WIN32
            bytes_sent = send(client_fd, body, (int)content_length, 0);
            #else
            bytes_sent = send(client_fd, body, content_length, 0);
            #endif
            
            /* Check for errors when sending the body */
            if (bytes_sent < 0) {
                printf("[ERROR] Failed to send HTTP body: %d - %s\n", bytes_sent, platform_get_error_string());
            }
            return;  /* We're done - sent header and body separately */
        }
    }
    
    /* Send the complete response (header + body if it fits) */
    #ifdef _WIN32
    bytes_sent = send(client_fd, response, (int)strlen(response), 0);
    #else
    bytes_sent = send(client_fd, response, strlen(response), 0);
    #endif
    
    /* Check for errors when sending the complete response */
    if (bytes_sent < 0) {
        printf("[ERROR] Failed to send HTTP response: %d - %s\n", bytes_sent, platform_get_error_string());
    }
}

/**
 * Send a 404 Not Found response to the client
 * 
 * This is a convenience function that wraps send_http_status with pre-defined
 * parameters for a 404 Not Found response. It's used when a requested resource
 * cannot be found on the server.
 * 
 * The response includes a simple HTML page explaining the error to the user.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_404(int client_fd) {
    /* Define the HTML body for the 404 response */
    const char* body = 
        "<html><body><h1>404 Not Found</h1>"
        "<p>The requested resource could not be found.</p></body></html>";
    
    printf("[DEBUG] Sending 404 Not Found response\n");
    
    /* Call the generic function with 404-specific parameters */
    send_http_status(client_fd, HTTP_STATUS_NOT_FOUND, "Not Found", "text/html", body);
}

/**
 * Send a 400 Bad Request response to the client
 * 
 * This is a convenience function that wraps send_http_status with pre-defined
 * parameters for a 400 Bad Request response. It's used when the client sends
 * a request that the server cannot understand or process.
 * 
 * The response includes a simple HTML page explaining the error to the user.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_400(int client_fd) {
    /* Define the HTML body for the 400 response */
    const char* body = 
        "<html><body><h1>400 Bad Request</h1>"
        "<p>Your browser sent a request that this server could not understand.</p></body></html>";
    
    printf("[DEBUG] Sending 400 Bad Request response\n");
    
    /* Call the generic function with 400-specific parameters */
    send_http_status(client_fd, HTTP_STATUS_BAD_REQUEST, "Bad Request", "text/html", body);
}

/**
 * Send a 500 Internal Server Error response to the client
 * 
 * This is a convenience function that wraps send_http_status with pre-defined
 * parameters for a 500 Internal Server Error response. It's used when the server
 * encounters an unexpected condition that prevents it from fulfilling the request.
 * 
 * The response includes a simple HTML page explaining the error to the user.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_500(int client_fd) {
    /* Define the HTML body for the 500 response */
    const char* body = 
        "<html><body><h1>500 Internal Server Error</h1>"
        "<p>The server encountered an unexpected condition.</p></body></html>";
    
    printf("[DEBUG] Sending 500 Internal Server Error response\n");
    
    /* Call the generic function with 500-specific parameters */
    send_http_status(client_fd, HTTP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", "text/html", body);
} 