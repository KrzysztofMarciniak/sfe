#include "secrets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSRF_PATH "/app/backend/.secrets/csrf.txt"
#define JWT_PATH "/app/backend/.secrets/jwt.txt"

static char *read_secret_file(const char *path, const char **errmsg) {
        if (errmsg) *errmsg = NULL;

        FILE *f = fopen(path, "r");
        if (!f) {
                if (errmsg) *errmsg = "Failed to open secret file.";
                return NULL;
        }

        if (fseek(f, 0, SEEK_END) != 0) {
                if (errmsg) *errmsg = "Failed to seek end of file.";
                fclose(f);
                return NULL;
        }
        long size = ftell(f);
        if (size < 0 || size > 1024) {
                if (errmsg) *errmsg = "Invalid file size for secret.";
                fclose(f);
                return NULL;
        }
        rewind(f);

        char *buffer = malloc(size + 1);
        if (!buffer) {
                if (errmsg) *errmsg = "Memory allocation failed for secret.";
                fclose(f);
                return NULL;
        }

        size_t read_bytes = fread(buffer, 1, size, f);
        fclose(f);

        if (read_bytes != (size_t)size) {
                if (errmsg) *errmsg = "Failed to read entire file.";
                free(buffer);
                return NULL;
        }

        buffer[size] = '\0';

        while (size > 0 && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) {
                buffer[size - 1] = '\0';
                size--;
        }

        return buffer;
}

const char *get_csrf_secret(const char **errmsg) {
        static char *csrf_secret = NULL;
        if (!csrf_secret) {
                csrf_secret = read_secret_file(CSRF_PATH, errmsg);
        }
        return csrf_secret;
}

const char *get_jwt_secret(const char **errmsg) {
        static char *jwt_secret = NULL;
        if (!jwt_secret) {
                jwt_secret = read_secret_file(JWT_PATH, errmsg);
        }
        return jwt_secret;
}
