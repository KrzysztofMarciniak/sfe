#ifndef DIE_H
#define DIE_H

/**
 * @brief Terminates the program with a JSON error response
 *
 * Outputs a standardized JSON error response to stdout and exits.
 * Useful for critical error handling in CGI and other contexts.
 *
 * @param status HTTP status code to include in response
 * @param error Error message string
 */
void die(int status, const char *error);

#endif
