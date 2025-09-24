/**
 * @file die.c
 * @brief Implementation of fatal error handling for JSON-based error responses
 *
 * Provides a standardized mechanism to output JSON-formatted error responses
 * and terminate program execution in critical error scenarios.
 */
#include "die.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Terminates the program with a structured JSON error response
 *
 * Outputs an HTTP JSON error response with specified status code and error
 * message. Immediately terminates program execution with a non-zero exit
 * status.
 *
 * @param status HTTP status code to include in the error response
 * @param error Descriptive error message string
 *
 * @note Prints Content-Type header and JSON to stdout
 * @note Always exits with status code 1 regardless of input status
 */
void die(int status, const char *error) {
    printf("Content-Type: application/json\r\n\r\n");
    printf("{\"status\":%d,\"error\":\"%s\"}\n", status, error);
    exit(1);
}
