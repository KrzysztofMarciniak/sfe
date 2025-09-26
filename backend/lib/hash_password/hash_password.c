#include "hash_password.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdarg.h>// Added for va_list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SALT_LEN 16
#define HASH_LEN 32
#define DEBUG 1

// Append formatted string to *err (reallocates *err buffer)
static void err_append(const char **err, const char *fmt, ...) {
        if (!err) return;

        va_list args;
        va_start(args, fmt);

        // Calculate needed size for new message
        va_list args_copy;
        va_copy(args_copy, args);
        int needed = vsnprintf(NULL, 0, fmt, args_copy);
        va_end(args_copy);
        if (needed < 0) {
                va_end(args);
                return;
        }

        size_t old_len = *err ? strlen(*err) : 0;
        size_t new_len = old_len + (size_t)needed + 1;

        char *new_err = realloc(*err ? (char *)(*err) : NULL, new_len);
        if (!new_err) {
                va_end(args);
                return;
        }

        // Append formatted string at the end
        vsnprintf(new_err + old_len, new_len - old_len, fmt, args);
        va_end(args);

        // Free the old pointer and assign the new one to prevent memory leaks
        if (*err) {
                free((void *)*err);
        }
        *err = new_err;
}

static void bin_to_hex(const unsigned char *in, size_t len, char *out) {
        for (size_t i = 0; i < len; ++i) {
                sprintf(out + i * 2, "%02x", in[i]);
        }
        out[len * 2] = '\0';
}

static int hex_to_bin(const char *hex, unsigned char *out, size_t out_len) {
        size_t hex_len = strlen(hex);
        if (hex_len != out_len * 2) return -1;
        for (size_t i = 0; i < out_len; ++i) {
                if (sscanf(hex + 2 * i, "%2hhx", &out[i]) != 1) return -1;
        }
        return 0;
}

char *hash_password(const char *password, const char **err) {
        if (err) *err = NULL;

        if (!password) {
                if (err) *err = "Password is NULL.";
                return NULL;
        }

        unsigned char salt[SALT_LEN];
        unsigned char hash[HASH_LEN];
        const int iterations = 100000;

        if (RAND_bytes(salt, sizeof(salt)) != 1) {
                if (err) *err = "Failed to generate random salt.";
                return NULL;
        }

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), iterations,
                              EVP_sha256(), HASH_LEN, hash) != 1) {
                if (err) *err = "Password hashing failed.";
                return NULL;
        }

        char salt_hex[SALT_LEN * 2 + 1];
        char hash_hex[HASH_LEN * 2 + 1];
        bin_to_hex(salt, SALT_LEN, salt_hex);
        bin_to_hex(hash, HASH_LEN, hash_hex);

        // Dynamically calculate the size of the result string
        size_t iter_str_len = snprintf(NULL, 0, "%d", iterations);
        size_t result_len   = strlen(salt_hex) + 1 + iter_str_len + 1 + strlen(hash_hex) + 1;

        char *result = malloc(result_len);
        if (!result) {
                if (err) *err = "Out of memory.";
                return NULL;
        }

        snprintf(result, result_len, "%s$%d$%s", salt_hex, iterations, hash_hex);
        return result;
}

int verify_password(const char *password, const char *stored_hash, const char **err) {
        if (err) *err = NULL;
        const char *debug_err = NULL;// Use a temporary buffer for debug messages

#define APPEND_ERR(...) err_append(&debug_err, __VA_ARGS__)

        APPEND_ERR("[DEBUG] Starting verify_password\n");
        APPEND_ERR("[DEBUG] Input password: '%s'\n", password ? password : "NULL");
        APPEND_ERR("[DEBUG] Stored hash: '%s'\n", stored_hash ? stored_hash : "NULL");

        if (!password || !stored_hash) {
                APPEND_ERR("[ERROR] Password or stored hash is NULL.\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }

        char *copy = strdup(stored_hash);
        if (!copy) {
                APPEND_ERR("[ERROR] Out of memory.\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }

        char *salt_hex = copy;
        char *iter_str = strchr(salt_hex, '$');
        if (!iter_str) {
                free(copy);
                APPEND_ERR("[ERROR] Invalid hash format (no iteration delimiter).\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }
        *iter_str++ = '\0';

        char *hash_hex = strchr(iter_str, '$');
        if (!hash_hex) {
                free(copy);
                APPEND_ERR("[ERROR] Invalid hash format (no hash delimiter).\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }
        *hash_hex++ = '\0';

        APPEND_ERR("[DEBUG] Parsed salt_hex: '%s'\n", salt_hex);
        APPEND_ERR("[DEBUG] Parsed iterations string: '%s'\n", iter_str);
        APPEND_ERR("[DEBUG] Parsed hash_hex: '%s'\n", hash_hex);

        int iterations = atoi(iter_str);
        if (iterations <= 0) {
                free(copy);
                APPEND_ERR("[ERROR] Invalid iteration count: %d\n", iterations);
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }
        APPEND_ERR("[DEBUG] Iterations count: %d\n", iterations);

        unsigned char salt[SALT_LEN];
        unsigned char expected_hash[HASH_LEN];
        unsigned char actual_hash[HASH_LEN];

        if (hex_to_bin(salt_hex, salt, SALT_LEN) != 0) {
                free(copy);
                APPEND_ERR("[ERROR] Failed to decode salt hex string.\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }
        if (hex_to_bin(hash_hex, expected_hash, HASH_LEN) != 0) {
                free(copy);
                APPEND_ERR("[ERROR] Failed to decode expected hash hex string.\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }

        APPEND_ERR("[DEBUG] Salt (bin): ");
        for (int i = 0; i < SALT_LEN; ++i) {
                APPEND_ERR("%02x", salt[i]);
        }
        APPEND_ERR("\n");

        APPEND_ERR("[DEBUG] Expected hash (bin): ");
        for (int i = 0; i < HASH_LEN; ++i) {
                APPEND_ERR("%02x", expected_hash[i]);
        }
        APPEND_ERR("\n");

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, SALT_LEN, iterations, EVP_sha256(),
                              HASH_LEN, actual_hash) != 1) {
                free(copy);
                APPEND_ERR("[ERROR] Password hashing failed during verification.\n");
                int ret = -1;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }

        APPEND_ERR("[DEBUG] Actual hash (bin): ");
        for (int i = 0; i < HASH_LEN; ++i) {
                APPEND_ERR("%02x", actual_hash[i]);
        }
        APPEND_ERR("\n");

        free(copy);

        unsigned char diff = 0;
        for (int i = 0; i < HASH_LEN; ++i) {
                diff |= actual_hash[i] ^ expected_hash[i];
        }
        APPEND_ERR("[DEBUG] Hash diff result: %d\n", diff);

        if (diff != 0) {
                char expected_hash_hex[HASH_LEN * 2 + 1];
                char actual_hash_hex[HASH_LEN * 2 + 1];
                bin_to_hex(expected_hash, HASH_LEN, expected_hash_hex);
                bin_to_hex(actual_hash, HASH_LEN, actual_hash_hex);

                APPEND_ERR("Password mismatch!\nExpected hash: %s\nActual hash: %s\n",
                           expected_hash_hex, actual_hash_hex);
                int ret = 0;
                if (err)
                        *err = debug_err;
                else
                        free((void *)debug_err);
                return ret;
        }

        APPEND_ERR("[DEBUG] Password verification succeeded.\n");
        int ret = 1;
        if (err)
                *err = debug_err;
        else
                free((void *)debug_err);
        return ret;

#undef APPEND_ERR
}
