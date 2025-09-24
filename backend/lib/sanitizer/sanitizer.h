#ifndef SANITIZER_H
#define SANITIZER_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>


/**
 * @brief Trim leading and trailing whitespace from a string (in place).
 *
 * @param str String to trim.
 * @return Pointer to trimmed string (may be different from input if leading whitespace removed).
 */
char* str_trim(char *str);

/**
 * @brief Check if the string contains only alphanumeric characters and underscores.
 *
 * @param str Input string.
 * @return true if valid username, false otherwise.
 */
bool validate_username(const char *str);

/**
 * @brief Validate if the string looks like valid JSON (simple check: must start with '{' or '[').
 *
 * @param str JSON string.
 * @return true if valid JSON format (basic), false otherwise.
 */
bool validate_json(const char *str);

/**
 * @brief Basic SQL string sanitizer (placeholder).
 * Escapes single quotes by doubling them.
 *
 * @param dest Destination buffer to store escaped string.
 * @param src Source string to escape.
 * @param dest_size Size of destination buffer.
 * @return true on success, false if dest_size is insufficient.
 */
bool sql_escape(char *dest, const char *src, size_t dest_size);

/**
 * @brief Validate a token string (e.g., session token) for allowed characters and length.
 *
 * @param token Token string.
 * @param max_len Maximum allowed length.
 * @return true if token is valid, false otherwise.
 */
bool validate_token(const char *token, size_t max_len);

#endif // SANITIZER_H
