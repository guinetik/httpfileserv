#include "httpfileserv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Load a template file into memory
 *
 * Reads the entire contents of a template file into a dynamically allocated string.
 * The caller is responsible for freeing the returned string.
 *
 * @param template_path Path to the template file
 * @return Dynamically allocated string containing the template content, or NULL on error
 */
char* load_template(const char* template_path) {
    FILE* file = fopen(template_path, "rb");
    if (!file) {
        printf("[ERROR] Failed to open template file: %s\n", template_path);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        printf("[ERROR] Empty or invalid template file: %s\n", template_path);
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer for the entire file content plus null terminator
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        printf("[ERROR] Failed to allocate memory for template\n");
        fclose(file);
        return NULL;
    }
    
    // Read the entire file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        printf("[ERROR] Failed to read template file: %s\n", template_path);
        free(buffer);
        return NULL;
    }
    
    // Null terminate the buffer
    buffer[file_size] = '\0';
    
    return buffer;
}

/**
 * @brief Replace all occurrences of a placeholder in a string with replacement text
 *
 * @param str The string containing the placeholder
 * @param placeholder The placeholder to replace
 * @param replacement The text to replace the placeholder with
 * @return A new dynamically allocated string with all replacements made
 */
static char* replace_placeholder(const char* str, const char* placeholder, const char* replacement) {
    // Find first occurrence to check if placeholder exists
    const char* pos = strstr(str, placeholder);
    if (!pos) {
        return strdup(str); // No placeholder found, duplicate the original string
    }
    
    size_t placeholder_len = strlen(placeholder);
    size_t replacement_len = strlen(replacement);
    size_t str_len = strlen(str);
    
    // Count occurrences to calculate new size
    int count = 0;
    const char* temp = str;
    while ((temp = strstr(temp, placeholder)) != NULL) {
        count++;
        temp += placeholder_len;
    }
    
    // Calculate the size of the new string
    size_t new_len = str_len + count * (replacement_len - placeholder_len);
    char* result = (char*)malloc(new_len + 1); // +1 for null terminator
    if (!result) {
        return NULL;
    }
    
    // Copy with replacements
    const char* src = str;
    char* dest = result;
    
    while ((pos = strstr(src, placeholder)) != NULL) {
        // Copy part before placeholder
        size_t prefix_len = pos - src;
        memcpy(dest, src, prefix_len);
        dest += prefix_len;
        
        // Copy replacement
        memcpy(dest, replacement, replacement_len);
        dest += replacement_len;
        
        // Move source pointer past placeholder
        src = pos + placeholder_len;
    }
    
    // Copy remaining part after last placeholder
    strcpy(dest, src);
    
    return result;
}

/**
 * @brief Process a template by replacing placeholders with actual content
 *
 * @param template_content The template content with placeholders
 * @param url_path The URL path to display
 * @param entries The HTML for directory entries
 * @param has_parent Whether to include a parent directory link
 * @return A new dynamically allocated string with the processed template
 */
char* process_template(const char* template_content, const char* url_path, const char* entries, int has_parent) {
    char* result = strdup(template_content);
    if (!result) return NULL;
    
    // Replace directory path placeholder
    char* temp = replace_placeholder(result, "{{DIRECTORY_PATH}}", url_path);
    free(result);
    result = temp;
    
    // Replace directory entries placeholder
    temp = replace_placeholder(result, "{{DIRECTORY_ENTRIES}}", entries);
    free(result);
    result = temp;
    
    // Replace parent directory link placeholder
    const char* parent_link = "";
    if (has_parent) {
        parent_link = "<div class=\"parent\"><a href=\"..\"><span class=\"icon\">⬆️</span> Parent Directory</a></div>";
    }
    
    temp = replace_placeholder(result, "{{PARENT_DIRECTORY_LINK}}", parent_link);
    free(result);
    result = temp;
    
    return result;
} 