/**
 * @file response.h
 * @brief Simple JSON HTTP response helper for CGI programs.
 *
 * Provides a single function to send a JSON response with an HTTP status code
 * and optional message by printing HTTP headers and JSON body to stdout.
 */

#ifndef RESPONSE_H
#define RESPONSE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sends a JSON response in a CGI environment.
 *
 * Prints the HTTP status header, Content-Type, and JSON body.
 *
 * @param http_code HTTP status code (e.g., 200, 404).
 * @param message Optional null-terminated message string to include.
 */
void response(unsigned int http_code, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* RESPONSE_H */
