#ifndef JWT_H
#define JWT_H

#include <json-c/json.h>

char *issue_jwt(const char *id);

char *val_jwt(const char *token, json_object **claims_out);

#endif // JWT_H
