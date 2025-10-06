#ifndef HASH_PASSWORD_H
#define HASH_PASSWORD_H

#include <sodium.h>

#include "/app/backend/lib/result/result.h"

// Library-specific error codes (1400-1499)
#define ERR_NULL_INPUT 1401
#define ERR_NULL_OUTPUT_PTR 1407
#define ERR_SALT_GENERATION_FAIL 1402
#define ERR_HASHING_FAIL 1403
#define ERR_INVALID_HASH_FORMAT 1404
#define ERR_INVALID_ITERATION_COUNT 1405
#define ERR_HASH_MISMATCH 1406
#define ERR_HASH_OUTPUT_PTR_NULL 1407
#define ERR_LIBSODIUM_FAIL 1408

/**
 * @brief The maximum length required to store the full encoded hash string
 * (including salt, algorithm, cost factors, and hash value).
 * Libsodium ensures this is sufficient for its current password hashing
 * scheme (Argon2id).
 */
#define PWHASH_STR_LEN crypto_pwhash_STRBYTES

/**
 * @brief Hash a password using libsodium's recommended Argon2id algorithm.
 * @param password Input password to hash
 * @param out_hash Pointer to store the resulting encoded hash string (caller
 * must free)
 * @return result_t indicating success or failure
 */
result_t* hash_password(const char* password, char** out_hash);

/**
 * @brief Verify a password against a stored hash using libsodium's function.
 * @param password Input password to verify
 * @param stored_hash Stored hash to compare against (must be in libsodium's
 * format)
 * @return result_t indicating success (match) or failure (mismatch or error)
 */
result_t* verify_password(const char* password, const char* stored_hash);

#endif
