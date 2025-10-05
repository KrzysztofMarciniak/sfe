#ifndef JWT_H
#define JWT_H

#include <json-c/json.h>
#include <stdbool.h>

#include "/app/backend/lib/result/result.h"

result_t* issue_jwt(const char* id, char** out_token);
result_t* val_jwt(const char* token, struct json_object** claims_out);

// Library-specific error codes (1100-1199)
#define ERR_JWT_INVALID_ID 1101
#define ERR_JWT_SECRET_FAIL 1102
#define ERR_JWT_JSON_FAIL 1103
#define ERR_JWT_GENERATE_FAIL 1104
#define ERR_JWT_INVALID_ARGS 1105
#define ERR_JWT_SANITIZE_FAIL 1106
#define ERR_JWT_SECRET_EMPTY 1107
#define ERR_JWT_VALIDATE_FAIL 1108

#endif// JWT_H
