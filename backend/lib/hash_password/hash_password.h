#ifndef HASH_PASSWORD_H
#define HASH_PASSWORD_H

#include <stdbool.h>

#include "/app/backend/lib/result/result.h"
// Library-specific error codes (1400-1499)
#define ERR_NULL_INPUT 1401
#define ERR_SALT_GENERATION_FAIL 1402
#define ERR_HASHING_FAIL 1403
#define ERR_INVALID_HASH_FORMAT 1404
#define ERR_INVALID_ITERATION_COUNT 1405
#define ERR_HASH_MISMATCH 1406
#define SALT_LEN 16
#define HASH_LEN 32
#define ITERATIONS 100000

result_t hash_password(const char* password, char** out_hash);
result_t verify_password(const char* password, const char* stored_hash);

#endif
