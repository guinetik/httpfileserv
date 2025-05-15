#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

/**
 * HTTP status code constants
 */
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_NOT_FOUND 404
#define HTTP_STATUS_INTERNAL_SERVER_ERROR 500

/**
 * Sends a 404 Not Found response to the client.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_404(int client_fd);

/**
 * Sends a 400 Bad Request response to the client.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_400(int client_fd);

/**
 * Sends a 500 Internal Server Error response to the client.
 * 
 * @param client_fd The client socket file descriptor
 */
void send_500(int client_fd);

/**
 * Sends a generic HTTP response with the specified status code and message.
 * 
 * @param client_fd The client socket file descriptor
 * @param status_code The HTTP status code
 * @param status_text The status text (e.g., "Not Found")
 * @param content_type The content type (defaults to "text/html" if NULL)
 * @param body The response body (can be NULL for empty response)
 */
void send_http_status(int client_fd, int status_code, const char* status_text, 
                     const char* content_type, const char* body);

#endif /* HTTP_RESPONSE_H */ 