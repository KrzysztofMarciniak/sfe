/**
 * @file response.h
 * @brief Header file for the HTTP response functions.
 *
 * This header defines the `response_t` struct and declares the
 * public functions for initializing, appending, and sending a response.
 */

#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdbool.h>

/**
 * @brief Represents the state of an HTTP response.
 *
 * This struct encapsulates all the necessary data for building a JSON-based
 * HTTP response, including the message array, status code, and a flag to
 * track if the response has been sent.
 */
typedef struct {
        struct json_object* response_array;
        unsigned int response_code;
        bool response_sent;
} response_t;

/**
 * @brief Initializes a `response_t` struct for a new HTTP response.
 *
 * @param resp A pointer to the `response_t` struct to initialize.
 * @param http_code The HTTP status code to use for the response.
 */
void response_init(response_t* resp, unsigned int http_code);

/**
 * @brief Appends a new message string to the response's JSON array.
 *
 * @param resp A pointer to the `response_t` struct.
 * @param msg The string message to append.
 * @return Returns true on success, false on failure.
 */
bool response_append(response_t* resp, const char* msg);

/**
 * @brief Sends the complete HTTP response to standard output.
 *
 * @param resp A pointer to the `response_t` struct.
 */
void response_send(response_t* resp);

#endif// RESPONSE_H
