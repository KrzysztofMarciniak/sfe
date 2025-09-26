#ifndef JWT_H
#define JWT_H

#include <json-c/json.h>
#include <stdbool.h>

char *issue_jwt(const char *id, const char **errmsg);

bool val_jwt(const char *token, struct json_object **claims_out, const char **errmsg);

#endif// JWT_H
