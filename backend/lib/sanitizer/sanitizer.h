#ifndef SANITIZER_H
#define SANITIZER_H

#include <stddef.h>

/**
 * @brief Sanitize an input string for safe embedding in SQL string literals.
 *
 * This function:
 *  - trims leading and trailing whitespace,
 *  - collapses/control-removes non-printable/control characters,
 *  - neutralizes common SQL comment/statement tokens
 *    by replacing them with single spaces,
 *  - escapes single quotes by doubling them (''), and
 *  - escapes backslashes by doubling them (\\).
 *
 * The sanitized string is written into the provided `dest` buffer
 * (NUL-terminated).
 *
 * @note This helper is defensive and useful when you absolutely must build an
 * SQL literal by string concatenation. The **preferred** approach is to use
 *       parameterized/prepared statements (e.g., sqlite3_bind_text) which avoid
 *       SQL injection entirely.
 *
 * @param dest      Destination buffer to receive the sanitized string.
 *                  Must be writable and at least `dest_size` bytes long.
 * @param src       Source NUL-terminated input string.
 * @param dest_size Size of the destination buffer in bytes (including NUL).
 * @return Pointer to dest on success, or NULL on error (invalid args or
 * insufficient buffer).
 */
char* sanitize(char* dest, const char* src, size_t dest_size);

#endif /* SANITIZER_H */
