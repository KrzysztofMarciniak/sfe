#include "sanitizer.h"

#include <ctype.h>
#include <string.h>

/**
 * @brief Sanitizes a string for safe usage in contexts like SQL literals.
 *
 * This function:
 * - Removes **leading and trailing ASCII whitespace**.
 * - Escapes **single quotes (`'`)** by doubling them (`''`), which helps
 * prevent SQL injection when inserting untrusted strings into SQL queries.
 *
 * The sanitized result is written into the `dest` buffer. If the buffer is too
 * small to hold the sanitized string (including escaped quotes and
 * null-terminator), the function returns `NULL` and does not modify `dest`.
 *
 * ### Example:
 * @code
 * char buffer[100];
 * if (sanitize(buffer, "  O'Reilly  ", sizeof(buffer))) {
 *     printf("Sanitized: %s\n", buffer);  // Output: O''Reilly
 * } else {
 *     // Handle error
 * }
 * @endcode
 *
 * @param[out] dest       Destination buffer to write the sanitized string.
 * @param[in]  src        Input string to sanitize.
 * @param[in]  dest_size  Size of the destination buffer in bytes.
 *
 * @return Pointer to `dest` on success, or `NULL` on error:
 *         - if any argument is invalid,
 *         - if the sanitized string would exceed `dest_size`.
 *
 * @note This function operates on ASCII input. It is not Unicode-aware.
 * @note This is not a substitute for proper parameterized SQL queries.
 */
char* sanitize(char* dest, const char* src, size_t dest_size) {
        if (!dest || !src || dest_size == 0) return NULL;

        // Skip leading whitespace
        while (*src && isspace((unsigned char)*src)) src++;

        // Find end of string without trailing whitespace
        const char* end = src + strlen(src);
        while (end > src && isspace((unsigned char)*(end - 1))) end--;

        size_t j = 0;
        for (const char* p = src; p < end; p++) {
                unsigned char c = (unsigned char)*p;

                if (c == '\'') {
                        // Escape single quote by doubling
                        if (j + 2 >= dest_size) return NULL;
                        dest[j++] = '\'';
                        dest[j++] = '\'';
                } else {
                        if (j + 1 >= dest_size) return NULL;
                        dest[j++] = *p;
                }
        }

        dest[j] = '\0';
        return dest;
}
