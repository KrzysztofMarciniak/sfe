#include "read_get_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/app/backend/lib/result/result.h"

/**
 * @brief Reads GET query string from environment and allocates a buffer.
 * @param out_query Pointer to store allocated null-terminated buffer (caller
 * must free)
 * @return result_t* indicating success or failure
 */
result_t* read_get_data(char** out_query) {
        if (!out_query)
                return result_failure("Output pointer NULL", NULL,
                                      ERR_GET_NULL_INPUT);

        *out_query        = NULL;
        const char* query = getenv("QUERY_STRING");
        if (!query) {
                return result_failure("QUERY_STRING not set", NULL,
                                      ERR_GET_NULL_INPUT);
        }

        size_t len = strlen(query);
        if (len == 0) {
                return result_failure("QUERY_STRING empty", NULL,
                                      ERR_GET_NULL_INPUT);
        }

        char* buffer = malloc(len + 1);
        if (!buffer) {
                return result_critical_failure("Memory allocation failed", NULL,
                                               ERR_MEMORY_ALLOC_FAIL);
        }

        memcpy(buffer, query, len);
        buffer[len] = '\0';
        *out_query  = buffer;

        return result_success();
}
