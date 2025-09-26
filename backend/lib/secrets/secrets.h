#ifndef SECRETS_H
#define SECRETS_H

const char *get_csrf_secret(const char **errmsg);
const char *get_jwt_secret(const char **errmsg);

#endif// SECRETS_H
