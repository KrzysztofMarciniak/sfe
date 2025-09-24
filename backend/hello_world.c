/**
 * @file main.c
 * @brief CGI handler that returns a JSON response based on HTTP method.
 *
 * @details
 * This CGI program demonstrates the use of the JsonResponse library.
 * It checks the HTTP request method and responds with a JSON-formatted
 * message. It supports only the GET method and returns appropriate
 * HTTP status codes and messages for supported and unsupported methods.
 *
 * Features:
 * - Checks HTTP request method via the REQUEST_METHOD environment variable
 * - Sends JSON responses with appropriate HTTP status codes
 * - Demonstrates method chaining using JsonResponse
 *
 * @note This is a minimal example showcasing JsonResponse in a CGI context.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/die/die.h"
#include "lib/json_response/json_response.h"

/**
 * @brief Entry point for the CGI program.
 *
 * @details
 * Handles the HTTP request by:
 * 1. Retrieving the request method from the environment
 * 2. Creating a JsonResponse with an appropriate status code
 * 3. Setting a response message
 * 4. Outputting the JSON response to stdout
 * 5. Freeing allocated resources
 *
 * - Returns 200 OK with a greeting message for GET requests
 * - Returns 405 Method Not Allowed for all other request types
 *
 * @return int Exit status
 * @retval 0 Success
 * @retval 1 Failure due to memory allocation error
 */
int main(void) {
    // Retrieve HTTP request method from environment
    const char *method = getenv("REQUEST_METHOD");

    JsonResponse *resp = method && strcmp(method, "GET") == 0
                             ? return_json(200)
                             : return_json(405);

    // Check for allocation failure
    if (!resp) {
        die(500, "Memory allocation failed.");
        return 1;
    }

    // Set HTTP headers for JSON response
    printf("Content-Type: application/json\r\n\r\n");

    // Set appropriate message
    resp->set_message(resp, method && strcmp(method, "GET") == 0
                                ? "Hello, World"
                                : "Method Not Allowed");

    // Output JSON response
    printf("%s\n", resp->build(resp));

    // Clean up allocated resources
    resp->free(resp);
    return 0;
}
