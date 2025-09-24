#include "read_post_data.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Reads the entire POST data from stdin based on CONTENT_LENGTH.
 *
 * @return Pointer to allocated null-terminated buffer containing POST data,
 *         or NULL on failure.
 *         Caller is responsible for freeing the returned buffer.
 */
char *read_post_data(void) {
        const char *len_str = getenv("CONTENT_LENGTH");
        if (!len_str) return NULL;

        long len = strtol(len_str, NULL, 10);
        if (len <= 0 || len > 65536) return NULL;

        char *body = malloc(len + 1);
        if (!body) return NULL;

        size_t read_len = fread(body, 1, len, stdin);
        if (read_len != (size_t)len) {
                free(body);
                return NULL;
        }

        body[len] = '\0';
        return body;
}
