#include "hash_password.h"

// Replaces OpenSSL headers
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Assuming this path is correct for your project's result type
#include "/app/backend/lib/result/result.h"

// --- Helper functions (bin_to_hex, hex_to_bin) removed as libsodium handles
// formatting ---

/**
 * @brief Hash a password using libsodium's recommended Argon2id algorithm.
 *
 * Libsodium's crypto_pwhash_str() function handles:
 * 1. Generating a cryptographically secure random salt.
 * 2. Hashing the password using the default secure algorithm (Argon2id).
 * 3. Encoding the salt, hash, and cost parameters into a single, compact
 * string.
 *
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

        if (!out_hash) {
                return result_failure("Output pointer (out_hash) is NULL", NULL,
                                      ERR_HASH_OUTPUT_PTR_NULL);
        }

        if (sodium_init() == -1) {
                return result_critical_failure(
                    "Libsodium initialization failed", NULL,
                    ERR_LIBSODIUM_FAIL);
        }

        char* encoded_hash = malloc(PWHASH_STR_LEN);
        if (!encoded_hash) {
                return result_critical_failure("Out of memory for hash string",
                                               NULL, ERR_MEMORY_ALLOC_FAIL);
        }

        if (crypto_pwhash_str(encoded_hash, password, strlen(password),
                              crypto_pwhash_OPSLIMIT_MODERATE,
                              crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
                free(encoded_hash);
                result_t* res =
                    result_critical_failure("Libsodium password hashing failed",
                                            NULL, ERR_HASHING_FAIL);
                result_add_extra(
                    res, "password_len=%zu, opslimit=%lu, memlimit=%lu",
                    strlen(password),
                    (unsigned long)crypto_pwhash_OPSLIMIT_MODERATE,
                    (unsigned long)crypto_pwhash_MEMLIMIT_MODERATE);
                return res;
        }

        *out_hash = encoded_hash;
        return result_success();
}

/**
 * @brief Verify a password against a stored hash using libsodium's function.
 *
 * Libsodium's crypto_pwhash_str_verify() handles:
 * 1. Safely parsing the stored hash string to extract the salt, parameters, and
 * hash.
 * 2. Re-hashing the input password using the extracted parameters.
 * 3. Performing a constant-time comparison against the stored hash.
 *
 * @param password Input password to verify
 * @param stored_hash Stored hash string (in libsodium's encoded format)
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

        if (sodium_init() == -1) {
                return result_critical_failure(
                    "Libsodium initialization failed", NULL,
                    ERR_LIBSODIUM_FAIL);
        }

        if (crypto_pwhash_str_verify(stored_hash, password, strlen(password)) !=
            0) {
                return result_failure(
                    "Password hash mismatch or invalid format", NULL,
                    ERR_HASH_MISMATCH);
        }

        return result_success();
}
