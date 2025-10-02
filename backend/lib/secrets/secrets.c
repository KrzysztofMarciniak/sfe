#include "secrets.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file secrets.c
 * @brief Functions for reading and managing secret files (CSRF and JWT tokens)
 */

#define CSRF_PATH "/app/backend/.secrets/csrf.txt"
#define JWT_PATH "/app/backend/.secrets/jwt.txt"

/**
 * @brief Reads a secret from a file
 * @param path File path to read from
 * @param out_secret Pointer to store the read secret
 * @return result_t indicating success or failure
 */

static result_t read_secret_file(const char* path, char** out_secret) {
        if (!path || !out_secret) {
                result_t res = result_failure("Invalid input parameters", NULL,
                                              ERR_INVALID_INPUT);
                result_add_extra(&res, "path=%s, out_secret=%p",
                                 path ? path : "(null)", (void*)out_secret);
                return res;
        }
        *out_secret = NULL;

        FILE* f = fopen(path, "r");
        if (!f) {
                result_t res = result_failure("Failed to open secret file",
                                              NULL, ERR_FILE_OPEN);
                result_add_extra(&res, "path=%s, errno=%d", path, errno);
                return res;
        }

        if (fseek(f, 0, SEEK_END) != 0) {
                result_t res = result_failure("Failed to seek end of file",
                                              NULL, ERR_FILE_SEEK);
                result_add_extra(&res, "errno=%d", errno);
                fclose(f);
                return res;
        }
        long size = ftell(f);
        if (size < 0 || size > 1024) {
                result_t res = result_failure("Invalid file size for secret",
                                              NULL, ERR_INVALID_SIZE);
                result_add_extra(&res, "size=%ld", size);
                fclose(f);
                return res;
        }
        rewind(f);

        char* buffer = malloc(size + 1);
        if (!buffer) {
                result_t res = result_critical_failure(
                    "Memory allocation failed for secret", NULL,
                    ERR_MEMORY_ALLOC);
                result_add_extra(&res, "errno=%d", errno);
                fclose(f);
                return res;
        }

        size_t read_bytes = fread(buffer, 1, size, f);
        fclose(f);

        if (read_bytes != (size_t)size) {
                result_t res = result_failure("Failed to read entire file",
                                              NULL, ERR_FILE_READ);
                result_add_extra(&res, "read_bytes=%zu, errno=%d", read_bytes,
                                 errno);
                free(buffer);
                return res;
        }

        buffer[size] = '\0';
        while (size > 0 &&
               (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) {
                buffer[size - 1] = '\0';
                size--;
        }

        *out_secret = buffer;
        return result_success();
}

/**
 * @brief Retrieves the CSRF secret
 * @param out_secret Pointer to store the CSRF secret
 * @return result_t indicating success or failure
 */
result_t get_csrf_secret(char** out_secret) {
        if (!out_secret) {
                return result_failure("Invalid output parameter",
                                      "get_csrf_secret: null check",
                                      ERR_INVALID_INPUT);
        }
        static char* csrf_secret = NULL;
        if (!csrf_secret) {
                result_t res = read_secret_file(CSRF_PATH, &csrf_secret);
                if (res.code != RESULT_SUCCESS) {
                        *out_secret = NULL;
                        return res;
                }
        }
        *out_secret = csrf_secret;
        return result_success();
}

/**
 * @brief Retrieves the JWT secret
 * @param out_secret Pointer to store the JWT secret
 * @return result_t indicating success or failure
 */
result_t get_jwt_secret(char** out_secret) {
        if (!out_secret) {
                return result_failure("Invalid output parameter",
                                      "get_jwt_secret: null check",
                                      ERR_INVALID_INPUT);
        }
        static char* jwt_secret = NULL;
        if (!jwt_secret) {
                result_t res = read_secret_file(JWT_PATH, &jwt_secret);
                if (res.code != RESULT_SUCCESS) {
                        *out_secret = NULL;
                        return res;
                }
        }
        *out_secret = jwt_secret;
        return result_success();
}
