#ifndef HTTPFILESERV_H
#define HTTPFILESERV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

// Platform-specific includes are in platform.h
#include "platform.h"
#include "utils.h"
#include "http_response.h"

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

/* Configuration constants */
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PATH_SIZE 1024

/**
 * Handles an incoming client connection.
 * 
 * @param client_fd The client socket file descriptor
 * @param base_path The base directory path to serve files from
 */
void handle_connection(int client_fd, const char* base_path);

/**
 * Sends a directory listing as HTML to the client.
 * 
 * @param client_fd The client socket file descriptor
 * @param path The filesystem path to the directory
 * @param url_path The URL path for the directory
 */
void send_directory_listing(int client_fd, const char* path, const char* url_path);

/**
 * Sends a file to the client with appropriate headers.
 * 
 * @param client_fd The client socket file descriptor
 * @param path The path to the file to send
 */
void send_file(int client_fd, const char* path);

#endif /* HTTPFILESERV_H */