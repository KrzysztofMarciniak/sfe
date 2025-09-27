/**
 * @file response.c
 * @brief Provides a set of functions for building and sending HTTP responses.
 *
 * This file implements a module for creating structured JSON responses. It
 * uses a `response_t` struct to manage the state of a single response,
 * ensuring that data is correctly prepared and sent. The functions are
 * thread-safe due to the use of a global mutex.
 */

#include "response.h"

#include <json-c/json.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Global mutex to ensure thread-safe access to the response functions.
 *
 * This mutex protects the global state and shared resources.
 */
static pthread_mutex_t response_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Initializes a `response_t` struct for a new HTTP response.
 *
 * This function prepares a response object by setting the HTTP status code,
 * marking the response as not yet sent, and initializing a new JSON array
 * for messages. If a previous array exists in the struct, it is freed.
 *
 * @param resp A pointer to the `response_t` struct to initialize.
 * @param http_code The HTTP status code to use for the response (e.g., 200 for
 * OK).
 */
void response_init(response_t* resp, unsigned int http_code) {
        pthread_mutex_lock(&response_mutex);
        if (resp->response_array) {
                json_object_put(resp->response_array);
        }
        resp->response_code  = http_code;
        resp->response_sent  = false;
        resp->response_array = json_object_new_array();
        pthread_mutex_unlock(&response_mutex);
}

/**
 * @brief Appends a new message string to the response's JSON array.
 *
 * This function adds a new JSON string object to the `messages` array
 * within the response struct. The operation fails if the response has
 * already been sent or if the message is NULL.
 *
 * @param resp A pointer to the `response_t` struct.
 * @param msg The string message to append.
 * @return Returns true on success, false on failure.
 */
bool response_append(response_t* resp, const char* msg) {
        pthread_mutex_lock(&response_mutex);
        if (!msg || resp->response_sent || !resp->response_array) {
                pthread_mutex_unlock(&response_mutex);
                return false;
        }

        struct json_object* jmsg = json_object_new_string(msg);
        if (!jmsg) {
                pthread_mutex_unlock(&response_mutex);
                return false;
        }

        json_object_array_add(resp->response_array, jmsg);
        pthread_mutex_unlock(&response_mutex);
        return true;
}

/**
 * @brief Sends the complete HTTP response to standard output.
 *
 * This function finalizes the response by creating a JSON object containing
 * the message array, printing the HTTP headers and the JSON body, and then
 * freeing the resources. The response is marked as sent after this operation.
 *
 * @param resp A pointer to the `response_t` struct.
 */
void response_send(response_t* resp) {
        pthread_mutex_lock(&response_mutex);
        if (resp->response_sent || !resp->response_array) {
                pthread_mutex_unlock(&response_mutex);
                return;
        }
        resp->response_sent = true;

        struct json_object* response_obj = json_object_new_object();
        json_object_object_add(response_obj, "messages", resp->response_array);

        printf("Status: %d\r\n", resp->response_code);
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", json_object_to_json_string(response_obj));

        json_object_put(response_obj);
        resp->response_array = NULL;// Explicitly set to NULL after freeing
        pthread_mutex_unlock(&response_mutex);
}
