/**
 * @file main.c
 * @brief An example of how to use the `response` module.
 *
 * This program demonstrates the typical workflow for creating and
 * sending an HTTP response using the provided functions.
 */

#include <stdlib.h>
#include <string.h>

#include "lib/response/response.h"

int main(void) {
        response_t my_response;

        const char* method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(&my_response, 400);
                response_append(&my_response,
                                "Bad Request: Missing REQUEST_METHOD");
                response_send(&my_response);
                return 1;
        }

        if (strcmp(method, "GET") == 0) {
                response_init(&my_response, 200);
                response_append(&my_response, "Hello, World");
                response_send(&my_response);
        } else {
                response_init(&my_response, 405);
                response_append(&my_response, "Method Not Allowed");
                response_send(&my_response);
        }

        return 0;
}
