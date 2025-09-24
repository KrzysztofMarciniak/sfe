/**
 * @file sanitizer.h
 * @brief String sanitization and validation utilities.
 *
 * Provides basic sanitization and validation functions for common use cases
 * such as trimming whitespace, validating usernames or tokens, escaping SQL
 * strings, and checking basic JSON format.
 */

#ifndef SANITIZER_H
#define SANITIZER_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trim leading and trailing whitespace from a string (in place).
 *
 * Modifies the input string to remove leading and trailing whitespace
 * characters (spaces, tabs, newlines, etc.). The operation is performed in
 * place, so no new memory is allocated.
 *
 * @param str String to trim. Must be null-terminated and writable.
 * @return Pointer to the trimmed string. This may point to a different location
 *         within the original string if leading whitespace is removed.
 */
char *str_trim(char *str);

/**
 * @brief Check if a string is a valid username.
 *
 * A valid username contains only alphanumeric characters (`a-z`, `A-Z`, `0-9`)
 * and underscores (`_`), and must not be empty.
 *
 * @param str Null-terminated input string.
 * @return `true` if the string is a valid username, `false` otherwise.
 */
bool validate_username(const char *str);

/**
 * @brief Perform a basic JSON format validation.
 *
 * This is a lightweight check that only verifies if the string starts with
 * a `{` (object) or `[` (array), indicating possible JSON structure. This does
 * not perform full JSON parsing or validation.
 *
 * @param str Null-terminated input string.
 * @return `true` if the string appears to be JSON, `false` otherwise.
 */
bool validate_json(const char *str);

/**
 * @brief Escape single quotes in a SQL string.
 *
 * Escapes single quotes (`'`) by doubling them (`''`) in the source string
 * and stores the result in the destination buffer. Useful for preventing
 * basic SQL injection when inserting values directly into SQL statements.
 *
 * @param dest Destination buffer where the escaped string is written.
 * @param src Source string to escape.
 * @param dest_size Size of the destination buffer (in bytes).
 * @return `true` on success, `false` if `dest_size` is insufficient.
 */
bool sql_escape(char *dest, const char *src, size_t dest_size);

/**
 * @brief Validate a token string (e.g., API/session token).
 *
 * Checks that the token contains only allowed characters (alphanumeric and
 * optionally a limited set of safe symbols), and that its length does not
 * exceed `max_len`.
 *
 * @param token Null-terminated token string to validate.
 * @param max_len Maximum allowed length (not including null terminator).
 * @return `true` if the token is valid, `false` otherwise.
 */
bool validate_token(const char *token, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif// SANITIZER_H
