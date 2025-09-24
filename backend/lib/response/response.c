/**
 * @file response.c
 * @brief Implementation of CGI-based JSON HTTP response.
 *
 * This implementation is intended for use in CGI programs that write directly
 * to stdout. It emits standard HTTP headers and a JSON-encoded body.
 */

#include "response.h"

#include <stdio.h>

/**
 * @brief Sends a JSON response in a CGI environment.
 *
 * Prints the HTTP status, Content-Type, and JSON body to stdout.
 *
 * @param http_code HTTP status code (e.g., 200, 404).
 * @param message Optional message string to include in JSON.
 */
void response(unsigned int http_code, const char *message) {
        // Print Status and headers for CGI
        printf("Status: %d\r\n", http_code);
        printf("Content-Type: application/json\r\n\r\n");

        if (message) {
                printf("{\"message\":\"%s\"}\n", message);
        } else {
                printf("{}\n");
        }
}
