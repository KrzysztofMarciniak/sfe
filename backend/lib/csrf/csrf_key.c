#include "csrf_key.h"

#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CSRF_KEY_PATH "/tmp/csrf_secret.key"
#define CSRF_KEY_SIZE 32

static unsigned char csrf_key[CSRF_KEY_SIZE];
static int csrf_key_loaded = 0;

static int load_key_from_file() {
    FILE *fp = fopen(CSRF_KEY_PATH, "rb");
    if (!fp) return 0;

    size_t read = fread(csrf_key, 1, CSRF_KEY_SIZE, fp);
    fclose(fp);

    if (read != CSRF_KEY_SIZE) return 0;
    csrf_key_loaded = 1;
    return 1;
}

static int generate_and_store_key() {
    if (!RAND_bytes(csrf_key, CSRF_KEY_SIZE)) return 0;

    FILE *fp = fopen(CSRF_KEY_PATH, "wb");
    if (!fp) return 0;

    fwrite(csrf_key, 1, CSRF_KEY_SIZE, fp);
    fclose(fp);

    // Restrict file permissions
    chmod(CSRF_KEY_PATH, 0600);

    csrf_key_loaded = 1;
    return 1;
}

const unsigned char *get_csrf_secret_key(size_t *len) {
    if (!csrf_key_loaded) {
        if (!load_key_from_file()) {
            if (!generate_and_store_key()) {
                return NULL;
            }
        }
    }

    if (len) *len = CSRF_KEY_SIZE;
    return csrf_key;
}
