#include "csrf.h"

#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <sanitizec.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "/app/backend/lib/memcmp/memcmp.h"
#include "/app/backend/lib/secrets/secrets.h"

/**
 * @brief Convert a nibble to a hexadecimal character
 * @param c Input nibble
 * @return Hexadecimal character
 */
static char hex_char(unsigned char c) { return "0123456789abcdef"[c & 0x0f]; }

/**
 * @brief Convert binary data to hexadecimal string
 * @param src Binary input data
 * @param len Length of binary data
 * @param dest Output buffer for hex string
 */
static void to_hex(const unsigned char* src, size_t len, char* dest) {
        for (size_t i = 0; i < len; ++i) {
                unsigned char hi = (unsigned char)(src[i] >> 4);
                unsigned char lo = (unsigned char)(src[i] & 0x0f);
                dest[i * 2]      = hex_char(hi);
                dest[i * 2 + 1]  = hex_char(lo);
        }
        dest[len * 2] = '\0';
}

/**
 * @brief Convert hexadecimal string to binary data
 * @param src Hexadecimal input string
 * @param dest Output buffer for binary data
 * @param len Expected length of output
 * @return true on success, false on failure
 */
static bool from_hex(const char* src, unsigned char* dest, size_t len) {
        char buf[3] = {0, 0, 0};
        for (size_t i = 0; i < len; ++i) {
                buf[0]            = src[i * 2];
                buf[1]            = src[i * 2 + 1];
                unsigned int byte = 0;
                if (sscanf(buf, "%2x", &byte) != 1) {
                        return false;
                }
                dest[i] = (unsigned char)byte;
        }
        return true;
}

/**
 * @brief Generate a CSRF token
 * @param out_token Pointer to store the generated token (caller must free)
 * @return result_t indicating success or failure
 */
result_t csrf_generate_token(char** out_token) {
        if (out_token) {
                *out_token = NULL;
        }

        unsigned char rand_bytes[CSRF_TOKEN_RANDOM_SIZE];
        if (RAND_bytes(rand_bytes, sizeof(rand_bytes)) != 1) {
                result_t res = result_critical_failure(
                    "RAND_bytes failed", NULL, ERR_RAND_BYTES_FAIL);
                return res;
        }

        uint64_t now   = (uint64_t)time(NULL);
        uint64_t ts_be = now;

        unsigned char timestamp_bytes[CSRF_TOKEN_TIMESTAMP_SIZE];
        for (int i = CSRF_TOKEN_TIMESTAMP_SIZE - 1; i >= 0; --i) {
                timestamp_bytes[i] = (unsigned char)(ts_be & 0xFF);
                ts_be >>= 8;
        }

        char* secret = NULL;
        result_t sc  = get_csrf_secret(&secret);
        if (sc.code != RESULT_SUCCESS) {
                return sc;
        }

        size_t key_len = strlen(secret);
        if (key_len == 0) {
                result_t res = result_critical_failure(
                    "CSRF secret is empty", NULL, ERR_CSRF_SECRET_EMPTY);
                free_result(&sc);
                return res;
        }

        unsigned char
            data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes,
               CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int hmac_len = 0;
        if (!HMAC(EVP_sha256(), (const unsigned char*)secret, (int)key_len,
                  data_to_mac, sizeof(data_to_mac), hmac, &hmac_len)) {
                result_t res = result_failure("HMAC generation failed", NULL,
                                              ERR_HMAC_GENERATION_FAIL);
                result_add_extra(&res, "key_len=%zu", key_len);
                free_result(&sc);
                return res;
        }
        if (hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                result_t res = result_critical_failure(
                    "HMAC length mismatch", NULL, ERR_HMAC_LENGTH_MISMATCH);
                result_add_extra(&res, "hmac_len=%u, expected=%d", hmac_len,
                                 CSRF_TOKEN_HMAC_SIZE);
                free_result(&sc);
                return res;
        }

        unsigned char token_raw[CSRF_TOKEN_RAW_SIZE];
        memcpy(token_raw, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes,
               CSRF_TOKEN_TIMESTAMP_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE,
               hmac, CSRF_TOKEN_HMAC_SIZE);

        char* token_hex = malloc(CSRF_TOKEN_HEX_SIZE + 1);
        if (!token_hex) {
                result_t res = result_critical_failure(
                    "Memory allocation failed", NULL, ERR_MEMORY_ALLOC_FAIL);
                free_result(&sc);
                return res;
        }

        to_hex(token_raw, CSRF_TOKEN_RAW_SIZE, token_hex);
        free_result(&sc);

        *out_token = token_hex;
        return result_success();
}

/**
 * @brief Validate a CSRF token
 * @param token The token to validate
 * @return result_t indicating success or failure
 */
result_t csrf_validate_token(const char* token) {
        if (!token) {
                result_t res =
                    result_failure("Token is null", NULL, ERR_NULL_TOKEN);
                return res;
        }

        char* token_sanitized =
            sanitizec_apply(token, SANITIZEC_RULE_HEX_ONLY, NULL);
        if (!token_sanitized) {
                result_t res =
                    result_critical_failure("CSRF token sanitization failed",
                                            NULL, ERR_SANITIZATION_FAIL);
                return res;
        }

        if (strlen(token_sanitized) != CSRF_TOKEN_HEX_SIZE) {
                result_t res = result_critical_failure(
                    "Token length mismatch", NULL, ERR_TOKEN_LENGTH_MISMATCH);
                result_add_extra(&res, "token_length=%zu, expected=%d",
                                 strlen(token_sanitized), CSRF_TOKEN_HEX_SIZE);
                free(token_sanitized);
                return res;
        }

        unsigned char token_raw_bytes[CSRF_TOKEN_RAW_SIZE];
        if (!from_hex(token_sanitized, token_raw_bytes, CSRF_TOKEN_RAW_SIZE)) {
                result_t res = result_critical_failure(
                    "Hex decoding failed", NULL, ERR_HEX_DECODE_FAIL);
                result_add_extra(&res, "token=%s", token_sanitized);
                free(token_sanitized);
                return res;
        }

        unsigned char* rand_bytes = token_raw_bytes;
        unsigned char* timestamp_bytes =
            token_raw_bytes + CSRF_TOKEN_RANDOM_SIZE;
        unsigned char* token_hmac = token_raw_bytes + CSRF_TOKEN_RANDOM_SIZE +
                                    CSRF_TOKEN_TIMESTAMP_SIZE;

        uint64_t token_ts = 0;
        for (size_t i = 0; i < CSRF_TOKEN_TIMESTAMP_SIZE; ++i) {
                token_ts = (token_ts << 8) | (uint64_t)timestamp_bytes[i];
        }
        uint64_t now = (uint64_t)time(NULL);
        if (token_ts > now) {
                result_t res =
                    result_failure("Token timestamp is in the future", NULL,
                                   ERR_TOKEN_FUTURE_TIMESTAMP);
                result_add_extra(&res, "token_ts=%llu, now=%llu", token_ts,
                                 now);
                free(token_sanitized);
                return res;
        }
        if (now - token_ts > CSRF_TOKEN_EXPIRE_SECONDS) {
                result_t res = result_failure("Token has expired", NULL,
                                              ERR_TOKEN_EXPIRED);
                result_add_extra(&res,
                                 "token_ts=%llu, now=%llu, expire_seconds=%d",
                                 token_ts, now, CSRF_TOKEN_EXPIRE_SECONDS);
                free(token_sanitized);
                return res;
        }

        char* secret = NULL;
        result_t sc  = get_csrf_secret(&secret);
        if (sc.code != RESULT_SUCCESS) {
                free(token_sanitized);
                return sc;
        }

        size_t key_len = strlen(secret);
        if (key_len == 0) {
                result_t res = result_critical_failure(
                    "CSRF secret is empty", NULL, ERR_CSRF_SECRET_EMPTY);
                free(token_sanitized);
                free_result(&sc);
                return res;
        }

        unsigned char
            data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes,
               CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char expected_hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int expected_hmac_len = 0;
        if (!HMAC(EVP_sha256(), (const unsigned char*)secret, (int)key_len,
                  data_to_mac, sizeof(data_to_mac), expected_hmac,
                  &expected_hmac_len)) {
                result_t res = result_failure("HMAC recomputation failed", NULL,
                                              ERR_HMAC_GENERATION_FAIL);
                result_add_extra(&res, "key_len=%zu", key_len);
                free(token_sanitized);
                free_result(&sc);
                return res;
        }
        if (expected_hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                result_t res = result_failure("Recomputed HMAC length mismatch",
                                              NULL, ERR_HMAC_LENGTH_MISMATCH);
                result_add_extra(&res, "hmac_len=%u, expected=%d",
                                 expected_hmac_len, CSRF_TOKEN_HMAC_SIZE);
                free(token_sanitized);
                free_result(&sc);
                return res;
        }

        if (memcmp(token_hmac, expected_hmac, CSRF_TOKEN_HMAC_SIZE) != 0) {
                result_t res = result_failure("HMACs do not match", NULL,
                                              ERR_HMAC_MISMATCH);
                free(token_sanitized);
                free_result(&sc);
                return res;
        }

        free(token_sanitized);
        free_result(&sc);
        return result_success();
}
