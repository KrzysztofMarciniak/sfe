// response.h

#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdbool.h>

// Initialize response with a status code (e.g., 200)
void response_init(int http_code);

// Append a debug or error message
bool response_append(const char *msg);

// Send the response once (prints headers and full JSON)
void response_send(void);

#endif
