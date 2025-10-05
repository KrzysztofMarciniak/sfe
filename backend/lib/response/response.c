#include "response.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initializes a response object with a given HTTP code.
 *
 * Sets up the root JSON object and messages array. If re-initializing,
 * any previous JSON objects are freed.
 *
 * @param resp Pointer to the response_t object
 * @param http_code HTTP status code to set
 */
void response_init(response_t* resp, unsigned int http_code) {
        if (!resp) return;

        if (resp->root) {
                json_object_put(resp->root);
                resp->root     = NULL;
                resp->messages = NULL;
        }

        resp->response_code = http_code;
        resp->response_sent = false;

        resp->root = json_object_new_object();
        if (!resp->root) {
                resp->messages = NULL;
                return;
        }

        json_object_object_add(resp->root, "status",
                               json_object_new_int((int)http_code));

        resp->messages = json_object_new_array();
        if (resp->messages) {
                json_object_object_add(resp->root, "messages", resp->messages);
        }
}

/**
 * @brief Appends a string message to the response's JSON array.
 *
 * @param resp Pointer to the response_t object
 * @param msg C string to append
 */
void response_append_str(response_t* resp, const char* msg) {
        if (!resp || !msg) return;

        if (!resp->messages && resp->root) {
                resp->messages = json_object_new_array();
                if (resp->messages) {
                        json_object_object_add(resp->root, "messages",
                                               resp->messages);
                }
        }

        if (!resp->messages) return;

        json_object* jstr = json_object_new_string(msg);
        if (!jstr) return;

        if (json_object_array_add(resp->messages, jstr) != 0) {
                json_object_put(jstr);
        }
}

/**
 * @brief Safely appends a JSON object to the response's JSON array.
 *
 * Increments the refcount of the object so that the response owns a reference.
 *
 * @param resp Pointer to the response_t object
 * @param obj JSON object to append
 */
void response_append_json(response_t* resp, struct json_object* obj) {
        if (!resp || !obj) return;

        if (!resp->messages && resp->root) {
                resp->messages = json_object_new_array();
                if (resp->messages) {
                        json_object_object_add(resp->root, "messages",
                                               resp->messages);
                }
        }

        if (!resp->messages) return;

        struct json_object* obj_ref = json_object_get(obj);
        if (!obj_ref) return;

        if (json_object_array_add(resp->messages, obj_ref) != 0) {
                json_object_put(obj_ref);
        }
}

/**
 * @brief Sends the HTTP response (prints JSON payload).
 *
 * Prints HTTP-style headers and the serialized JSON payload. Ensures
 * the response is only sent once.
 *
 * @param resp Pointer to the response_t object
 */
void response_send(response_t* resp) {
        if (!resp || resp->response_sent) return;

        if (!resp->root) {
                resp->root = json_object_new_object();
                if (!resp->root) {
                        resp->response_sent = true;
                        return;
                }
                json_object_object_add(
                    resp->root, "status",
                    json_object_new_int((int)resp->response_code));
        }

        if (!resp->messages) {
                resp->messages = json_object_new_array();
                if (resp->messages) {
                        json_object_object_add(resp->root, "messages",
                                               resp->messages);
                }
        }

        const char* payload =
            json_object_to_json_string_ext(resp->root, JSON_C_TO_STRING_PLAIN);
        if (!payload) {
                resp->response_sent = true;
                return;
        }

        printf("Status: %u\r\n", resp->response_code);
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", payload);

        resp->response_sent = true;
}

/**
 * @brief Frees all resources held by a response object.
 *
 * Drops all JSON object references and resets pointers to NULL.
 *
 * @param resp Pointer to the response_t object
 */
void response_free(response_t* resp) {
        if (!resp) return;

        if (resp->root) {
                json_object_put(resp->root);
                resp->root = NULL;
        }
        resp->messages      = NULL;
        resp->response_sent = false;
}
