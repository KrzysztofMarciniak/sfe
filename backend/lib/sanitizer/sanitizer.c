#include "sanitizer.h"

#include <ctype.h>
#include <string.h>

/**
 * @brief Sanitize a string for safe usage
 *
 * Trims leading and trailing whitespace (including multibyte-safe)
 * and escapes single quotes by doubling them to prevent SQL injection.
 * Writes sanitized string into dest buffer.
 *
 * @param dest Destination buffer to store sanitized string.
 * @param src Source string to sanitize.
 * @param dest_size Size of destination buffer in bytes.
 * @return Pointer to dest on success, NULL if input invalid or buffer too small.
 */
char *sanitize(char *dest, const char *src, size_t dest_size) {
        if (!dest || !src || dest_size == 0) return NULL;

        // Skip leading whitespace
        while (*src && isspace((unsigned char)*src)) src++;

        // Find end of string without trailing whitespace
        const char *end = src + strlen(src);
        while (end > src && isspace((unsigned char)*(end - 1))) end--;

        size_t j = 0;
        for (const char *p = src; p < end; p++) {
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
