#include "csrf.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "/app/backend/lib/secrets/secrets.h"

#define CSRF_TOKEN_RANDOM_SIZE 32
#define CSRF_TOKEN_HMAC_SIZE 32
#define CSRF_TOKEN_TIMESTAMP_SIZE 8
#define CSRF_TOKEN_EXPIRE_SECONDS (24 * 60 * 60)
#define CSRF_TOKEN_RAW_SIZE \
        (CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE + CSRF_TOKEN_HMAC_SIZE)
#define CSRF_TOKEN_HEX_SIZE (CSRF_TOKEN_RAW_SIZE * 2)

static char hex_char(unsigned char c) { return "0123456789abcdef"[c & 0x0f]; }

static void to_hex(const unsigned char *src, size_t len, char *dest) {
        for (size_t i = 0; i < len; ++i) {
                unsigned char hi = (unsigned char)(src[i] >> 4);
                unsigned char lo = (unsigned char)(src[i] & 0x0f);
                dest[i * 2]      = hex_char(hi);
                dest[i * 2 + 1]  = hex_char(lo);
        }
        dest[len * 2] = '\0';
}

static bool from_hex(const char *src, unsigned char *dest, size_t len, const char **errmsg) {
        char buf[3] = {0, 0, 0};
        for (size_t i = 0; i < len; ++i) {
                buf[0]            = src[i * 2];
                buf[1]            = src[i * 2 + 1];
                unsigned int byte = 0;
                if (sscanf(buf, "%2x", &byte) != 1) {
                        if (errmsg) *errmsg = "Hex decoding failed.";
                        return false;
                }
                dest[i] = (unsigned char)byte;
        }
        return true;
}

static int secure_memcmp(const void *a, const void *b, size_t len) {
        const unsigned char *p1 = (const unsigned char *)a;
        const unsigned char *p2 = (const unsigned char *)b;
        unsigned char diff      = 0;
        for (size_t i = 0; i < len; ++i) {
                diff |= p1[i] ^ p2[i];
        }
        return diff;
}

char *csrf_generate_token(const char **errmsg) {
        if (errmsg) *errmsg = NULL;

        unsigned char rand_bytes[CSRF_TOKEN_RANDOM_SIZE];
        if (RAND_bytes(rand_bytes, sizeof(rand_bytes)) != 1) {
                if (errmsg) *errmsg = "RAND_bytes failed.";
                return NULL;
        }

        uint64_t now   = (uint64_t)time(NULL);
        uint64_t ts_be = now;

        unsigned char timestamp_bytes[CSRF_TOKEN_TIMESTAMP_SIZE];
        for (int i = CSRF_TOKEN_TIMESTAMP_SIZE - 1; i >= 0; --i) {
                timestamp_bytes[i] = (unsigned char)(ts_be & 0xFF);
                ts_be >>= 8;
        }

        const char *secret = get_csrf_secret(errmsg);
        if (!secret) {
                return NULL;
        }
        size_t key_len = strlen(secret);
        if (key_len == 0) {
                if (errmsg) *errmsg = "CSRF secret is empty.";
                return NULL;
        }

        unsigned char data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int hmac_len = 0;
        if (!HMAC(EVP_sha256(), (const unsigned char *)secret, (int)key_len, data_to_mac,
                  sizeof(data_to_mac), hmac, &hmac_len)) {
                if (errmsg) *errmsg = "HMAC generation failed.";
                return NULL;
        }
        if (hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                if (errmsg) *errmsg = "HMAC length mismatch.";
                return NULL;
        }

        unsigned char token_raw[CSRF_TOKEN_RAW_SIZE];
        memcpy(token_raw, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE, hmac,
               CSRF_TOKEN_HMAC_SIZE);

        char *token_hex = malloc(CSRF_TOKEN_HEX_SIZE + 1);
        if (!token_hex) {
                if (errmsg) *errmsg = "Memory allocation failed.";
                return NULL;
        }

        to_hex(token_raw, CSRF_TOKEN_RAW_SIZE, token_hex);
        return token_hex;
}

bool csrf_validate_token(const char *token, const char **errmsg) {
        if (errmsg) *errmsg = NULL;

        if (!token) {
                if (errmsg) *errmsg = "Token is null.";
                return false;
        }
        if (strlen(token) != CSRF_TOKEN_HEX_SIZE) {
                if (errmsg) *errmsg = "Token length mismatch.";
                return false;
        }

        unsigned char token_raw[CSRF_TOKEN_RAW_SIZE];
        if (!from_hex(token, token_raw, CSRF_TOKEN_RAW_SIZE, errmsg)) {
                return false;
        }

        unsigned char *rand_bytes      = token_raw;
        unsigned char *timestamp_bytes = token_raw + CSRF_TOKEN_RANDOM_SIZE;
        unsigned char *token_hmac = token_raw + CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE;

        uint64_t token_ts = 0;
        for (size_t i = 0; i < CSRF_TOKEN_TIMESTAMP_SIZE; ++i) {
                token_ts = (token_ts << 8) | (uint64_t)timestamp_bytes[i];
        }

        uint64_t now = (uint64_t)time(NULL);
        if (token_ts > now) {
                if (errmsg) *errmsg = "Token timestamp is in the future.";
                return false;
        }
        if (now - token_ts > CSRF_TOKEN_EXPIRE_SECONDS) {
                if (errmsg) *errmsg = "Token has expired.";
                return false;
        }

        const char *secret = get_csrf_secret(errmsg);
        if (!secret) {
                return false;
        }
        size_t key_len = strlen(secret);
        if (key_len == 0) {
                if (errmsg) *errmsg = "CSRF secret is empty.";
                return false;
        }

        unsigned char data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char expected_hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int expected_hmac_len = 0;
        if (!HMAC(EVP_sha256(), (const unsigned char *)secret, (int)key_len, data_to_mac,
                  sizeof(data_to_mac), expected_hmac, &expected_hmac_len)) {
                if (errmsg) *errmsg = "HMAC recomputation failed.";
                return false;
        }
        if (expected_hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                if (errmsg) *errmsg = "Recomputed HMAC length mismatch.";
                return false;
        }

        if (secure_memcmp(token_hmac, expected_hmac, CSRF_TOKEN_HMAC_SIZE) != 0) {
                if (errmsg) *errmsg = "HMACs do not match.";
                return false;
        }

        return true;
}
