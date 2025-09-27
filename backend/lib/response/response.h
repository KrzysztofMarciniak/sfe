#ifndef RESPONSE_H
#define RESPONSE_H

#include <json-c/json.h>
#include <stdbool.h>

/**
 * @struct response_t
 * @brief Represents a single HTTP response.
 *
 * This struct encapsulates all data for a response, including its
 * status code, a flag indicating if it's been sent, and the JSON array
 * containing messages.
 */
typedef struct {
        unsigned int response_code;
        bool response_sent;
        struct json_object* response_array;
} response_t;

/**
 * @brief Initializes a response object with a given HTTP code.
 *
 * Frees any existing messages in the response and sets up a new
 * JSON array for messages.
 *
 * @param resp Pointer to the response object.
 * @param http_code HTTP status code to set.
 */
void response_init(response_t* resp, unsigned int http_code);

/**
 * @brief Appends a message to the response's JSON array.
 *
 * @param resp Pointer to the response object.
 * @param msg Message string to append.
 * @return true if the message was successfully appended, false otherwise.
 */
bool response_append(response_t* resp, const char* msg);

/**
 * @brief Sends the HTTP response, printing headers and JSON messages.
 *
 * Marks the response as sent and prevents further modifications.
 *
 * @param resp Pointer to the response object.
 */
void response_send(response_t* resp);

#endif// RESPONSE_H
