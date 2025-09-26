#ifndef CSRF_H
#define CSRF_H

#include <stdbool.h>

char* csrf_generate_token(const char** errmsg);
bool csrf_validate_token(const char* token, const char** errmsg);

#endif// CSRF_H
