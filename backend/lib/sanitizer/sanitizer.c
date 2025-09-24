#include "sanitizer.h"

char* str_trim(char *str) {
    if (!str) return NULL;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces
        return str;

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = '\0';

    return str;
}

bool validate_username(const char *str) {
    if (!str || *str == '\0') return false;

    while (*str) {
        if (!(isalnum((unsigned char)*str) || *str == '_')) return false;
        str++;
    }
    return true;
}

bool validate_json(const char *str) {
    if (!str) return false;

    // Skip leading whitespace
    while (isspace((unsigned char)*str)) str++;

    return (*str == '{' || *str == '[');
}

bool sql_escape(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src) return false;

    size_t j = 0;
    for (size_t i = 0; src[i] != '\0'; i++) {
        if (j + 2 >= dest_size) // Need room for char + possible extra quote + null
            return false;

        if (src[i] == '\'') {
            dest[j++] = '\'';
            dest[j++] = '\'';
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
    return true;
}

bool validate_token(const char *token, size_t max_len) {
    if (!token) return false;

    size_t len = strlen(token);
    if (len == 0 || len > max_len) return false;

    for (size_t i = 0; i < len; i++) {
        char c = token[i];
        if (!(isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.')) {
            return false;
        }
    }
    return true;
}
