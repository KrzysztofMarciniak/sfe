#ifndef CSRF_H
#define CSRF_H

#include <stdbool.h>

char* csrf_generate_token(void);
bool csrf_validate_token(const char *token);

#endif // CSRF_H
