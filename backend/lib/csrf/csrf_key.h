#ifndef CSRF_KEY_H
#define CSRF_KEY_H

#include <stddef.h>

const unsigned char *get_csrf_secret_key(size_t *len);

#endif
