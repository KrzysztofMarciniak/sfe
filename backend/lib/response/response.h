#ifndef RESPONSE_H
#define RESPONSE_H

#include <json-c/json.h>
#include <stdbool.h>

/**
 * @struct response_t
 * @brief Represents a single HTTP response.
 *
 * Encapsulates response metadata and JSON payload:
 * - status code
 * - sent flag
 * - root JSON object (contains status + messages)
 * - messages array
 */
typedef struct {
        unsigned int response_code;   /**< HTTP status code */
        bool response_sent;           /**< Flag if response already sent */
        struct json_object* root;     /**< Root JSON object */
        struct json_object* messages; /**< "messages" JSON array */
} response_t;

/**
 * @brief Initializes a response object with a given HTTP code.
 *
 * Creates a new JSON root object with "status" and "messages".
 * If the response already has allocated JSON, it will be freed.
 *
 * @param resp Pointer to the response object.
 * @param http_code HTTP status code to set.
 */
void response_init(response_t* resp, unsigned int http_code);

/**
 * @brief Appends a string message to the response's JSON array.
 *
 * Example: ["some message"]
 *
 * @param resp Pointer to the response object.
 * @param msg Message string to append.
 */
void response_append_str(response_t* resp, const char* msg);

/**
 * @brief Appends a JSON object directly into the response's JSON array.
 *
 * Example: [{"key": "value"}]
 *
 * @param resp Pointer to the response object.
 * @param obj JSON object to append. Its reference count is incremented.
 */
void response_append_json(response_t* resp, struct json_object* obj);

/**
 * @brief Sends the HTTP response, printing headers and the JSON payload.
 *
 * Marks the response as sent and prevents further modifications.
 * Typically prints to stdout for CGI-style apps.
 *
 * @param resp Pointer to the response object.
 */
void response_send(response_t* resp);

/**
 * @brief Frees all resources held by a response object.
 *
 * Releases JSON objects (root + messages).
 *
 * @param resp Pointer to the response object.
 */
void response_free(response_t* resp);

#endif// RESPONSE_H
