#include <stdio.h>

/**
 * @brief Sends a JSON response in a CGI environment.
 *
 * Prints the HTTP status header, Content-Type, and JSON body.
 *
 * @param http_code HTTP status code (e.g., 200, 404).
 * @param message Optional message string to include in JSON.
 */
void response(unsigned int http_code, const char *message) {
        // Print Status header for CGI
        printf("Status: %d\r\n", http_code);
        printf("Content-Type: application/json\r\n\r\n");

        if (message) {
                printf("{\"status\":%d,\"message\":\"%s\"}\n", http_code, message);
        } else {
                printf("{\"status\":%d}\n", http_code);
        }
}
