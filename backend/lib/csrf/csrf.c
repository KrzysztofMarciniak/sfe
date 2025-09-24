#include "csrf.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "csrf_key.h"

#define CSRF_TOKEN_RANDOM_SIZE 32  // Random bytes length
#define CSRF_TOKEN_HMAC_SIZE 32    // SHA-256 HMAC length
#define CSRF_TOKEN_TIMESTAMP_SIZE 8// 64-bit UNIX timestamp (uint64_t)

#define CSRF_TOKEN_EXPIRE_SECONDS (24 * 60 * 60)// 24 hours token validity

// Token layout: [ random(32) | timestamp(8) | hmac(32) ]
// Total bytes = 72 -> hex = 72 * 2 = 144 chars + null terminator

#define CSRF_TOKEN_RAW_SIZE \
        (CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE + CSRF_TOKEN_HMAC_SIZE)
#define CSRF_TOKEN_HEX_SIZE (CSRF_TOKEN_RAW_SIZE * 2)

static char hex_char(unsigned char c) { return "0123456789abcdef"[c & 0xf]; }

static void to_hex(const unsigned char *src, size_t len, char *dest) {
        for (size_t i = 0; i < len; ++i) {
                dest[i * 2]     = hex_char(src[i] >> 4);
                dest[i * 2 + 1] = hex_char(src[i]);
        }
        dest[len * 2] = '\0';
}

static bool from_hex(const char *src, unsigned char *dest, size_t len) {
        for (size_t i = 0; i < len; i++) {
                unsigned int byte;
                if (sscanf(src + i * 2, "%2x", &byte) != 1) {
                        return false;
                }
                dest[i] = (unsigned char)byte;
        }
        return true;
}

// Constant-time memcmp to avoid timing attacks
static int secure_memcmp(const void *a, const void *b, size_t len) {
        const unsigned char *p1 = a;
        const unsigned char *p2 = b;
        unsigned char diff      = 0;
        for (size_t i = 0; i < len; i++) {
                diff |= p1[i] ^ p2[i];
        }
        return diff;// 0 if equal, nonzero otherwise
}

char *csrf_generate_token(void) {
        unsigned char rand_bytes[CSRF_TOKEN_RANDOM_SIZE];
        if (!RAND_bytes(rand_bytes, sizeof(rand_bytes))) {
                return NULL;
        }

        // Get current UNIX timestamp (seconds)
        uint64_t timestamp = (uint64_t)time(NULL);

        // Convert timestamp to big-endian bytes
        unsigned char timestamp_bytes[CSRF_TOKEN_TIMESTAMP_SIZE];
        for (int i = CSRF_TOKEN_TIMESTAMP_SIZE - 1; i >= 0; i--) {
                timestamp_bytes[i] = timestamp & 0xFF;
                timestamp >>= 8;
        }

        // Get secret key
        size_t key_len;
        const unsigned char *key = get_csrf_secret_key(&key_len);
        if (!key) {
                return NULL;
        }

        // Prepare data to HMAC: random_bytes || timestamp_bytes
        unsigned char data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int hmac_len = 0;
        if (!HMAC(EVP_sha256(), key, (int)key_len, data_to_mac, sizeof(data_to_mac), hmac,
                  &hmac_len)) {
                return NULL;
        }
        if (hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                return NULL;
        }

        // Build final raw token: random + timestamp + hmac
        unsigned char token_raw[CSRF_TOKEN_RAW_SIZE];
        memcpy(token_raw, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);
        memcpy(token_raw + CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE, hmac,
               CSRF_TOKEN_HMAC_SIZE);

        // Allocate hex string (2 chars per byte + null)
        char *token_hex = malloc(CSRF_TOKEN_HEX_SIZE + 1);
        if (!token_hex) return NULL;

        to_hex(token_raw, CSRF_TOKEN_RAW_SIZE, token_hex);

        return token_hex;
}

bool csrf_validate_token(const char *token) {
        if (!token || strlen(token) != CSRF_TOKEN_HEX_SIZE) {
                return false;
        }

        unsigned char token_raw[CSRF_TOKEN_RAW_SIZE];
        if (!from_hex(token, token_raw, CSRF_TOKEN_RAW_SIZE)) {
                return false;
        }

        // Extract components
        unsigned char *rand_bytes      = token_raw;
        unsigned char *timestamp_bytes = token_raw + CSRF_TOKEN_RANDOM_SIZE;
        unsigned char *token_hmac = token_raw + CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE;

        // Parse timestamp (big-endian)
        uint64_t token_timestamp = 0;
        for (int i = 0; i < CSRF_TOKEN_TIMESTAMP_SIZE; i++) {
                token_timestamp = (token_timestamp << 8) | timestamp_bytes[i];
        }

        // Check expiration
        uint64_t now = (uint64_t)time(NULL);
        if (token_timestamp > now || now - token_timestamp > CSRF_TOKEN_EXPIRE_SECONDS) {
                // Token expired or timestamp invalid
                return false;
        }

        // Get secret key
        size_t key_len;
        const unsigned char *key = get_csrf_secret_key(&key_len);
        if (!key) {
                return false;
        }

        // Recalculate HMAC
        unsigned char data_to_mac[CSRF_TOKEN_RANDOM_SIZE + CSRF_TOKEN_TIMESTAMP_SIZE];
        memcpy(data_to_mac, rand_bytes, CSRF_TOKEN_RANDOM_SIZE);
        memcpy(data_to_mac + CSRF_TOKEN_RANDOM_SIZE, timestamp_bytes, CSRF_TOKEN_TIMESTAMP_SIZE);

        unsigned char expected_hmac[CSRF_TOKEN_HMAC_SIZE];
        unsigned int expected_hmac_len = 0;
        if (!HMAC(EVP_sha256(), key, (int)key_len, data_to_mac, sizeof(data_to_mac), expected_hmac,
                  &expected_hmac_len)) {
                return false;
        }
        if (expected_hmac_len != CSRF_TOKEN_HMAC_SIZE) {
                return false;
        }

        // Constant-time compare HMACs
        if (secure_memcmp(token_hmac, expected_hmac, CSRF_TOKEN_HMAC_SIZE) != 0) {
                return false;
        }

        return true;
}
