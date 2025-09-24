/**
 * @file main.c
 * @brief Simple CGI handler that responds to HTTP GET requests.
 *
 * This program checks the `REQUEST_METHOD` environment variable (set by the CGI environment)
 * and returns a 200 OK response for GET requests. For all other methods, it returns
 * a 405 Method Not Allowed. If the method is missing, it returns a 400 Bad Request.
 */

#include <stdlib.h>
#include <string.h>

#include "lib/response/response.h"

/**
 * @brief Entry point for the CGI program.
 *
 * Uses the `REQUEST_METHOD` environment variable to determine the HTTP method.
 * - If the method is `GET`, returns a `200 OK` response with a "Hello, World" message.
 * - If the method is anything else, returns a `405 Method Not Allowed`.
 * - If the method is not set, returns a `400 Bad Request`.
 *
 * @return Exit status code (0 on success, non-zero on error).
 */
int main(void) {
        const char *method = getenv("REQUEST_METHOD");

        if (!method) {
                response(400, "Bad Request");
                return 1;
        }

        if (strcmp(method, "GET") == 0) {
                response(200, "Hello, World");
        } else {
                response(405, "Method Not Allowed");
        }

        return 0;
}
