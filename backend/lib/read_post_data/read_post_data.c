#include "read_post_data.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Reads POST data from stdin based on CONTENT_LENGTH.
 * @param out_body Pointer to store allocated null-terminated buffer (caller
 * must free).
 * @return result_t indicating success or failure with details.
 */
result_t* read_post_data(char** out_body) {
        if (out_body) {
                *out_body = NULL;
        }

        const char* len_str = getenv("CONTENT_LENGTH");
        if (!len_str) {
                result_t* res = result_failure("CONTENT_LENGTH not set", NULL,
                                               ERR_INVALID_CONTENT_LENGTH);
                result_add_extra(res, "len_str=%p", (const void*)len_str);
                return res;
        }

        char* endptr;
        errno    = 0;
        long len = strtol(len_str, &endptr, 10);
        if (errno != 0 || *endptr != '\0' || len <= 0 || len > 65536) {
                result_t* res = result_failure("Invalid CONTENT_LENGTH", NULL,
                                               ERR_INVALID_CONTENT_LENGTH);
                result_add_extra(res, "len_str=%s, len=%ld, errno=%d", len_str,
                                 len, errno);
                return res;
        }

        char* body = malloc(len + 1);
        if (!body) {
                result_t* res = result_critical_failure(
                    "Memory allocation failed", NULL, ERR_MEMORY_ALLOC_FAIL);
                result_add_extra(res, "requested_size=%ld", len + 1);
                return res;
        }

        size_t read_len = fread(body, 1, len, stdin);
        if (read_len != (size_t)len) {
                result_t* res = result_failure("Failed to read POST data", NULL,
                                               ERR_READ_FAIL);
                result_add_extra(res, "read_len=%zu, expected=%ld, errno=%d",
                                 read_len, len, errno);
                free(body);
                return res;
        }

        body[len] = '\0';
        *out_body = body;
        return result_success();
}
