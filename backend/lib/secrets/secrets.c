#include "secrets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CSRF_PATH "/app/backend/.secrets/csrf.txt"
#define JWT_PATH "/app/backend/.secrets/jwt.txt"

static char *read_secret_file(const char *path) {
        FILE *f = fopen(path, "r");
        if (!f) return NULL;

        // Read entire file content (assuming small secrets)
        if (fseek(f, 0, SEEK_END) != 0) {
                fclose(f);
                return NULL;
        }
        long size = ftell(f);
        if (size < 0 || size > 1024) {// sanity check max size
                fclose(f);
                return NULL;
        }
        rewind(f);

        char *buffer = malloc(size + 1);
        if (!buffer) {
                fclose(f);
                return NULL;
        }

        size_t read_bytes = fread(buffer, 1, size, f);
        fclose(f);

        if (read_bytes != (size_t)size) {
                free(buffer);
                return NULL;
        }

        buffer[size] = '\0';

        // Trim trailing newline(s)
        while (size > 0 && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) {
                buffer[size - 1] = '\0';
                size--;
        }

        return buffer;
}

const char *get_csrf_secret(void) {
        static char *csrf_secret = NULL;
        if (!csrf_secret) {
                csrf_secret = read_secret_file(CSRF_PATH);
        }
        return csrf_secret;
}

const char *get_jwt_secret(void) {
        static char *jwt_secret = NULL;
        if (!jwt_secret) {
                jwt_secret = read_secret_file(JWT_PATH);
        }
        return jwt_secret;
}
