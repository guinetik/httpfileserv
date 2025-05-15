#ifndef UTILS_H
#define UTILS_H

/**
 * Decodes a URL-encoded string.
 * 
 * @param str The URL-encoded string to decode
 * @return A newly allocated string with the decoded result (must be freed by caller)
 */
char* url_decode(const char* str);

/**
 * Determines the MIME type for a file based on its extension.
 * 
 * @param path The file path
 * @return A string containing the MIME type
 */
const char* get_mime_type(const char* path);

#endif /* UTILS_H */ 