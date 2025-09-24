/**
 * @file csrf.c
 * @brief CGI endpoint to generate and validate CSRF tokens via HTTP requests.
 *
 * This CGI program supports two HTTP methods:
 *
 * - GET: Generates a new CSRF token.
 *        Responds with JSON:
 *        {
 *          "message": "<token>"
 *        }
 *
 * - POST: Validates a CSRF token provided in the JSON request body.
 *         Expects JSON request:
 *         {
 *           "message": "<token>"
 *         }
 *         Responds with JSON:
 *         - On success:
 *           {
 *             "message": "Token is valid"
 *           }
 *         - On failure:
 *           {
 *             "message": "Invalid token"
 *           }
 *
 * The program reads the request method from the environment variable
 * REQUEST_METHOD and behaves accordingly.
 *
 * For POST requests, the request body is read from stdin based on the
 * CONTENT_LENGTH environment variable.
 *
 * Responses are sent as JSON with appropriate HTTP status codes via
 * the response() function.
 *
 * Dependencies:
 * - lib/csrf/csrf.h : Provides csrf_generate_token() and csrf_validate_token()
 * - lib/response/response.h : Provides response() function for sending JSON responses
 * - json-c library : For parsing and creating JSON objects
 */

#include "lib/csrf/csrf.h"

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

/**
 * @brief Main entry point of the CGI program.
 *
 * Determines the HTTP request method from the environment variable REQUEST_METHOD.
 *
 * - For GET requests, generates a new CSRF token using csrf_generate_token() and
 *   returns it as a JSON response with HTTP status 200.
 *
 * - For POST requests, reads the JSON request body, parses it, extracts the "message"
 *   field (expected to be the CSRF token), validates it using csrf_validate_token(),
 *   and returns a JSON response indicating whether the token is valid.
 *
 * - For unsupported methods, responds with HTTP status 405 (Method Not Allowed).
 *
 * Error handling includes appropriate HTTP status codes and JSON messages for:
 * - Missing or invalid REQUEST_METHOD
 * - Missing or invalid POST body
 * - Malformed JSON
 * - Missing or invalid "message" field
 * - Token validation failure
 *
 * @return Always returns 0.
 */
int main(void) {
        const char *method = getenv("REQUEST_METHOD");

        if (!method) {
                response(400, "Missing request method");
                return 0;
        }

        // --- GET: Generate CSRF token ---
        if (strcmp(method, "GET") == 0) {
                char *token = csrf_generate_token();
                if (!token) {
                        response(500, "Failed to generate CSRF token");
                        return 0;
                }

                response(200, token);
                free(token);
                return 0;
        }

        // --- POST: Validate CSRF token ---
        if (strcmp(method, "POST") == 0) {
                char *body = read_post_data();
                if (!body) {
                        response(400, "Missing or invalid POST body");
                        return 0;
                }

                struct json_object *jobj = json_tokener_parse(body);
                free(body);

                if (!jobj) {
                        response(400, "Malformed JSON");
                        return 0;
                }

                struct json_object *j_token = NULL;
                if (!json_object_object_get_ex(jobj, "message", &j_token)) {
                        json_object_put(jobj);
                        response(400, "Missing 'message' field");
                        return 0;
                }

                const char *token = json_object_get_string(j_token);
                if (!token) {
                        json_object_put(jobj);
                        response(400, "'message' must be a string");
                        return 0;
                }

                bool valid = csrf_validate_token(token);
                json_object_put(jobj);

                if (valid) {
                        response(200, "Token is valid");
                } else {
                        response(400, "Invalid token");
                }

                return 0;
        }

        // Method not allowed
        response(405, "Method Not Allowed");
        return 0;
}
