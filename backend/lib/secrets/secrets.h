#ifndef SECRETS_H
#define SECRETS_H
#include "/app/backend/lib/result/result.h"

result_t* get_csrf_secret(char** out_secret);
result_t* get_jwt_secret(char** out_secret);

// Library-specific error codes
#define ERR_INVALID_INPUT 1001
#define ERR_FILE_OPEN 1002
#define ERR_FILE_SEEK 1003
#define ERR_INVALID_SIZE 1004
#define ERR_MEMORY_ALLOC 1005
#define ERR_FILE_READ 1006

#endif// SECRETS_H
