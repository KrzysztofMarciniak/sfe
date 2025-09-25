#ifndef SECRETS_H_
#define SECRETS_H_

const char *get_csrf_secret(void);
const char *get_jwt_secret(void);

#endif// SECRETS_H_
