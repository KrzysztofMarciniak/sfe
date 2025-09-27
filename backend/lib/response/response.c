#include "response.h"

#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initializes the response structure. Cleans up any existing response
 * array before creating a new one.
 *
 * @param resp The response structure pointer.
 * @param http_code The HTTP status code for the response.
 */
void response_init(response_t* resp, unsigned int http_code) {
        if (!resp) return;

        // Initialize the struct to a known state to prevent garbage values
        resp->response_array = NULL;
        resp->response_code  = http_code;
        resp->response_sent  = false;
        resp->response_array = json_object_new_array();
}

/**
 * @brief Appends a message string to the response's JSON array.
 *
 * @param resp The response structure pointer.
 * @param msg The message string to append.
 * @return true if successful, false if the response was already sent or inputs
 * are invalid.
 */
bool response_append(response_t* resp, const char* msg) {
        if (!resp || !msg) return false;

        if (resp->response_sent || !resp->response_array) {
                return false;
        }

        struct json_object* jmsg = json_object_new_string(msg);
        if (!jmsg) {
                return false;
        }

        json_object_array_add(resp->response_array, jmsg);
        return true;
}

/**
 * @brief Sends the HTTP response, including headers and the JSON body,
 * and automatically cleans up the internal JSON structures.
 * This function marks the response as sent and nullifies the array pointer.
 *
 * @param resp The response structure pointer.
 */
void response_send(response_t* resp) {
        if (!resp) return;

        if (resp->response_sent || !resp->response_array) {
                return;
        }

        struct json_object* response_obj = json_object_new_object();
        json_object_object_add(response_obj, "messages", resp->response_array);

        printf("Status: %d\r\n", resp->response_code);
        printf("Content-Type: application/json\r\n\r\n");

        printf("%s\n", json_object_to_json_string(response_obj));

        json_object_put(response_obj);

        resp->response_array = NULL;
        resp->response_sent  = true;
}
