#include "httpfileserv.h"
#include "utils.h"

/**
 * This file contains utility functions for the HTTP file server.
 */

char* url_decode(const char* str) {
    if (str == NULL) return NULL;
    
    size_t len = strlen(str);
    char* decoded = malloc(len + 1);
    size_t i, j = 0;
    
    for (i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            int value;
            sscanf(str + i + 1, "%2x", &value);
            decoded[j++] = value;
            i += 2;
        } else if (str[i] == '+') {
            decoded[j++] = ' ';
        } else {
            decoded[j++] = str[i];
        }
    }
    
    decoded[j] = '\0';
    return decoded;
}

const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (ext == NULL) {
        return "application/octet-stream";
    }
    
    ext++;  // Skip the dot
    
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) {
        return "text/html";
    } else if (strcasecmp(ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(ext, "css") == 0) {
        return "text/css";
    } else if (strcasecmp(ext, "js") == 0) {
        return "application/javascript";
    } else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(ext, "png") == 0) {
        return "image/png";
    } else if (strcasecmp(ext, "gif") == 0) {
        return "image/gif";
    } else if (strcasecmp(ext, "pdf") == 0) {
        return "application/pdf";
    } else if (strcasecmp(ext, "json") == 0) {
        return "application/json";
    } else {
        return "application/octet-stream";
    }
}