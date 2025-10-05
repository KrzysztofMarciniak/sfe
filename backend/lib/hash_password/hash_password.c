#include "hash_password.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/app/backend/lib/result/result.h"

/**
 * @brief Convert binary data to hexadecimal string
 * @param bin Binary input data
 * @param len Length of binary data
 * @param out Output buffer for hex string
 */
static void bin_to_hex(const unsigned char* bin, size_t len, char* out) {
        for (size_t i = 0; i < len; ++i) {
                sprintf(out + i * 2, "%02x", bin[i]);
        }
        out[len * 2] = '\0';
}

/**
 * @brief Convert hexadecimal string to binary data
 * @param hex Hexadecimal input string
 * @param out Output buffer for binary data
 * @param out_len Expected length of output
 * @return 0 on success, -1 on failure
 */
static int hex_to_bin(const char* hex, unsigned char* out, size_t out_len) {
        for (size_t i = 0; i < out_len; ++i) {
                unsigned int byte;
                if (sscanf(hex + 2 * i, "%2x", &byte) != 1) {
                        return -1;
                }
                out[i] = (unsigned char)byte;
        }
        return 0;
}

/**
 * @brief Hash a password using PBKDF2 with SHA256
 * @param password Input password to hash
 * @param out_hash Pointer to store the resulting hash string (caller must free)
 * @return result_t indicating success or failure
 */
result_t* hash_password(const char* password, char** out_hash) {
        if (out_hash) {
                *out_hash = NULL;
        }

        if (!password) {
                return result_failure("Password is NULL", NULL, ERR_NULL_INPUT);
        }

        unsigned char salt[SALT_LEN];
        unsigned char hash[HASH_LEN];

        if (RAND_bytes(salt, SALT_LEN) != 1) {
                result_t* res = result_critical_failure(
                    "Failed to generate salt", NULL, ERR_SALT_GENERATION_FAIL);
                result_add_extra(res, "salt_len=%d", SALT_LEN);
                return res;
        }

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, SALT_LEN,
                              ITERATIONS, EVP_sha256(), HASH_LEN, hash) != 1) {
                result_t* res = result_critical_failure("Hashing failed", NULL,
                                                        ERR_HASHING_FAIL);
                result_add_extra(
                    res,
                    "password_len=%zu, salt_len=%d, iterations=%d, hash_len=%d",
                    strlen(password), SALT_LEN, ITERATIONS, HASH_LEN);
                return res;
        }

        char salt_hex[SALT_LEN * 2 + 1];
        char hash_hex[HASH_LEN * 2 + 1];
        bin_to_hex(salt, SALT_LEN, salt_hex);
        bin_to_hex(hash, HASH_LEN, hash_hex);

        char* out = malloc(SALT_LEN * 2 + 1 + 6 + 1 + HASH_LEN * 2 + 1);
        if (!out) {
                return result_critical_failure("Out of memory", NULL,
                                               ERR_MEMORY_ALLOC_FAIL);
        }

        sprintf(out, "%s$%d$%s", salt_hex, ITERATIONS, hash_hex);
        *out_hash = out;
        return result_success();
}

/**
 * @brief Verify a password against a stored hash
 * @param password Input password to verify
 * @param stored_hash Stored hash to compare against
 * @return result_t indicating success (match) or failure (mismatch or error)
 */
result_t* verify_password(const char* password, const char* stored_hash) {
        if (!password || !stored_hash) {
                result_t* res = result_failure(
                    "Password or stored hash is NULL", NULL, ERR_NULL_INPUT);
                result_add_extra(res, "password=%p, stored_hash=%p",
                                 (const void*)password,
                                 (const void*)stored_hash);
                return res;
        }

        char* copy = strdup(stored_hash);
        if (!copy) {
                result_t* res = result_critical_failure("Out of memory", NULL,
                                                        ERR_MEMORY_ALLOC_FAIL);
                return res;
        }

        char* salt_hex = copy;
        char* iter_str = strchr(salt_hex, '$');
        if (!iter_str) {
                result_t* res = result_critical_failure(
                    "Invalid hash format", NULL, ERR_INVALID_HASH_FORMAT);
                result_add_extra(res, "stored_hash=%s", stored_hash);
                free(copy);
                return res;
        }
        *iter_str++ = '\0';

        char* hash_hex = strchr(iter_str, '$');
        if (!hash_hex) {
                result_t* res = result_critical_failure(
                    "Invalid hash format", NULL, ERR_INVALID_HASH_FORMAT);
                result_add_extra(res, "stored_hash=%s", stored_hash);
                free(copy);
                return res;
        }
        *hash_hex++ = '\0';

        int iterations = atoi(iter_str);
        if (iterations <= 0) {
                result_t* res =
                    result_critical_failure("Invalid iteration count", NULL,
                                            ERR_INVALID_ITERATION_COUNT);
                result_add_extra(res, "iterations=%s", iter_str);
                free(copy);
                return res;
        }

        unsigned char salt[SALT_LEN];
        unsigned char expected_hash[HASH_LEN];
        unsigned char actual_hash[HASH_LEN];

        if (hex_to_bin(salt_hex, salt, SALT_LEN) != 0 ||
            hex_to_bin(hash_hex, expected_hash, HASH_LEN) != 0) {
                result_t* res = result_critical_failure(
                    "Failed to decode hex", NULL, ERR_HEX_DECODE_FAIL);
                result_add_extra(res, "salt_hex=%s, hash_hex=%s", salt_hex,
                                 hash_hex);
                free(copy);
                return res;
        }

        if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, SALT_LEN,
                              iterations, EVP_sha256(), HASH_LEN,
                              actual_hash) != 1) {
                result_t* res = result_critical_failure(
                    "Hashing failed during verification", NULL,
                    ERR_HASHING_FAIL);
                result_add_extra(
                    res,
                    "password_len=%zu, salt_len=%d, iterations=%d, hash_len=%d",
                    strlen(password), SALT_LEN, iterations, HASH_LEN);
                free(copy);
                return res;
        }

        free(copy);

        for (int i = 0; i < HASH_LEN; ++i) {
                if (actual_hash[i] != expected_hash[i]) {
                        result_t* res = result_failure("Password hash mismatch",
                                                       NULL, ERR_HASH_MISMATCH);
                        return res;
                }
        }

        return result_success();
}
