#ifndef CSRF_H
#define CSRF_H

#include "/app/backend/lib/result/result.h"

/**
 * @file csrf.h
 * @brief Functions for generating and validating CSRF tokens
 */

/**
 * @brief Generate a CSRF token
 * @param out_token Pointer to store the generated token (caller must free)
 * @return result_t indicating success or failure
 */
result_t csrf_generate_token(char** out_token);

/**
 * @brief Validate a CSRF token
 * @param token The token to validate
 * @return result_t indicating success or failure
 */
result_t csrf_validate_token(const char* token);

// Library-specific error codes (1500-1599)
#define ERR_RAND_BYTES_FAIL 1501
#define ERR_CSRF_SECRET_FAIL 1502
#define ERR_CSRF_SECRET_EMPTY 1503
#define ERR_HMAC_GENERATION_FAIL 1504
#define ERR_HMAC_LENGTH_MISMATCH 1505
#define ERR_NULL_TOKEN 1506
#define ERR_CSRF_SANITIZATION_FAIL 1507
#define ERR_TOKEN_LENGTH_MISMATCH 1508
#define ERR_TOKEN_FUTURE_TIMESTAMP 1509
#define ERR_TOKEN_EXPIRED 1510
#define ERR_HMAC_MISMATCH 1511
#define ERR_INVALID_TOKEN 1512

#define CSRF_TOKEN_RANDOM_SIZE 32
#define CSRF_TOKEN_HMAC_SIZE 32
#define CSRF_TOKEN_TIMESTAMP_SIZE 8
#define CSRF_TOKEN_EXPIRE_SECONDS (24 * 60 * 60)
#define CSRF_TOKEN_RAW_SIZE                                   \
        (CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE + \
         CSRF_TOKEN_HMAC_SIZE)
#define CSRF_TOKEN_HEX_SIZE (CSRF_TOKEN_RAW_SIZE * 2)

#endif// CSRF_H
