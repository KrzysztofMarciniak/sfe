/**
 * @file sanitizer.c
 * @brief Implementation of string sanitization and validation utilities.
 */

#include "sanitizer.h"

/**
 * @brief Trim leading and trailing whitespace from a string (in place).
 *
 * This function removes whitespace from the beginning and end of the string.
 * Leading whitespace is skipped by incrementing the string pointer, and
 * trailing whitespace is removed by inserting a null terminator at the new end.
 *
 * @param str Input string to trim. Must be writable.
 * @return Pointer to the trimmed string (possibly shifted from original
 * pointer).
 */
char *str_trim(char *str) {
        if (!str) return NULL;

        // Trim leading whitespace
        while (isspace((unsigned char)*str)) str++;

        if (*str == '\0')// All spaces
                return str;

        // Trim trailing whitespace
        char *end = str + strlen(str) - 1;
        while (end > str && isspace((unsigned char)*end)) end--;

        *(end + 1) = '\0';

        return str;
}

/**
 * @brief Check if a string is a valid username.
 *
 * A valid username must only contain alphanumeric characters or underscores.
 * Empty strings or strings with invalid characters are rejected.
 *
 * @param str Null-terminated input string.
 * @return true if valid, false otherwise.
 */
bool validate_username(const char *str) {
        if (!str || *str == '\0') return false;

        while (*str) {
                if (!(isalnum((unsigned char)*str) || *str == '_')) return false;
                str++;
        }
        return true;
}

/**
 * @brief Perform a basic check for JSON format.
 *
 * Skips leading whitespace and checks whether the first non-whitespace
 * character is '{' (for objects) or '[' (for arrays).
 *
 * @param str Null-terminated JSON string.
 * @return true if it appears to be JSON, false otherwise.
 */
bool validate_json(const char *str) {
        if (!str) return false;

        while (isspace((unsigned char)*str)) str++;

        return (*str == '{' || *str == '[');
}

/**
 * @brief Escape single quotes in a SQL string.
 *
 * Escapes `'` characters by replacing them with `''` (doubling the quote),
 * writing the result into `dest`. This is useful when constructing SQL
 * statements manually.
 *
 * @param dest Destination buffer to write escaped string.
 * @param src Source string to escape.
 * @param dest_size Size of destination buffer in bytes.
 * @return true if escaping succeeded, false if buffer was too small.
 */
bool sql_escape(char *dest, const char *src, size_t dest_size) {
        if (!dest || !src) return false;

        size_t j = 0;
        for (size_t i = 0; src[i] != '\0'; i++) {
                if (j + 2 >= dest_size)// room for current char + extra quote + null
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

/**
 * @brief Validate a token string for allowed characters and length.
 *
 * A valid token:
 * - is non-empty
 * - does not exceed `max_len`
 * - contains only alphanumeric characters or the symbols `-`, `_`, `.`
 *
 * @param token Input token string.
 * @param max_len Maximum allowed length (not including null terminator).
 * @return true if token is valid, false otherwise.
 */
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
