#ifndef HTTPFILESERV_LIB_H
#define HTTPFILESERV_LIB_H

#include "httpfileserv.h"

/**
 * Initialize and start the HTTP file server.
 * 
 * @param directory_path The directory to serve files from
 * @param port The port to listen on (0 for default port)
 * @return 0 on success, non-zero on failure
 */
int start_server(const char* directory_path, int port);

/**
 * Stop the HTTP file server.
 */
void stop_server(void);

/**
 * Set a callback function to be called for each request.
 * This allows for custom request handling and logging.
 * 
 * @param callback The callback function to be called
 */
typedef void (*request_callback)(const char* method, const char* path, int status_code);
void set_request_callback(request_callback callback);

/**
 * Set custom MIME types for file extensions.
 * 
 * @param extension The file extension (without the dot)
 * @param mime_type The MIME type to use for this extension
 */
void set_mime_type(const char* extension, const char* mime_type);

/**
 * Configure the server with additional options.
 * 
 * @param option_name The name of the option to set
 * @param option_value The value for the option
 * @return 0 on success, non-zero on failure
 */
int set_server_option(const char* option_name, const char* option_value);

#endif /* HTTPFILESERV_LIB_H */