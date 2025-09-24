#include "hash_password.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SALT_LEN 16
#define HASH_LEN 32

/**
 * @brief Convert binary data to a hex string.
 */
static void bin_to_hex(const unsigned char *in, size_t len, char *out) {
        for (size_t i = 0; i < len; ++i) sprintf(out + i * 2, "%02x", in[i]);
        out[len * 2] = '\0';
}

/**
 * @brief Convert hex string to binary.
 */
static int hex_to_bin(const char *hex, unsigned char *out, size_t out_len) {
        size_t hex_len = strlen(hex);
        if (hex_len != out_len * 2) return -1;
        for (size_t i = 0; i < out_len; ++i) {
                if (sscanf(hex + 2 * i, "%2hhx", &out[i]) != 1) return -1;
        }
        return 0;
}

char *hash_password(const char *password) {
        if (!password) return NULL;

        unsigned char salt[SALT_LEN];
        unsigned char hash[HASH_LEN];
        const int iterations = 100000;

        if (RAND_bytes(salt, sizeof(salt)) != 1) return NULL;

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), iterations,
                              EVP_sha256(), HASH_LEN, hash) != 1)
                return NULL;

        // Convert to hex strings
        char salt_hex[SALT_LEN * 2 + 1];
        char hash_hex[HASH_LEN * 2 + 1];
        bin_to_hex(salt, SALT_LEN, salt_hex);
        bin_to_hex(hash, HASH_LEN, hash_hex);

        // Allocate final string: salt$iterations$hash
        size_t result_len = strlen(salt_hex) + 1 + 10 + 1 + strlen(hash_hex) + 1;
        char *result      = malloc(result_len);
        if (!result) return NULL;

        snprintf(result, result_len, "%s$%d$%s", salt_hex, iterations, hash_hex);
        return result;
}

int verify_password(const char *password, const char *stored_hash) {
        if (!password || !stored_hash) return -1;

        // Parse stored hash: salt$iterations$hash
        char *copy = strdup(stored_hash);
        if (!copy) return -1;

        char *salt_hex = strtok(copy, "$");
        char *iter_str = strtok(NULL, "$");
        char *hash_hex = strtok(NULL, "$");

        if (!salt_hex || !iter_str || !hash_hex) {
                free(copy);
                return -1;
        }

        int iterations = atoi(iter_str);
        if (iterations <= 0) {
                free(copy);
                return -1;
        }

        unsigned char salt[SALT_LEN];
        unsigned char expected_hash[HASH_LEN];
        unsigned char actual_hash[HASH_LEN];

        if (hex_to_bin(salt_hex, salt, SALT_LEN) != 0 ||
            hex_to_bin(hash_hex, expected_hash, HASH_LEN) != 0) {
                free(copy);
                return -1;
        }

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, SALT_LEN, iterations, EVP_sha256(),
                              HASH_LEN, actual_hash) != 1) {
                free(copy);
                return -1;
        }

        free(copy);

        // Constant-time comparison
        int result = 1;
        for (int i = 0; i < HASH_LEN; ++i) {
                if (actual_hash[i] != expected_hash[i]) result = 0;
        }

        return result;
}
