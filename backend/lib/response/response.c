#include "response.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Initializes a response object with a given HTTP code.
 */
void response_init(response_t* resp, unsigned int http_code) {
        if (!resp) return;
        if (resp->root) {
                json_object_put(resp->root);
        }

        resp->response_code = http_code;
        resp->response_sent = false;
        resp->root          = json_object_new_object();
        if (!resp->root) {
                resp->messages = NULL;
                return;
        }
        json_object_object_add(resp->root, "status",
                               json_object_new_int(http_code));
        resp->messages = json_object_new_array();
        json_object_object_add(resp->root, "messages", resp->messages);
}

/**
 * @brief Appends a string message to the response's JSON array.
 */
void response_append_str(response_t* resp, const char* msg) {
        if (!resp || !resp->messages || !msg) return;
        json_object* jstr = json_object_new_string(msg);
        if (!jstr) return;
        json_object_array_add(resp->messages, jstr);
}

/**
 * @brief Appends a JSON object to the response's JSON array.
 */
void response_append_json(response_t* resp, struct json_object* obj) {
        if (!resp || !resp->messages || !obj) return;
        json_object_array_add(resp->messages, json_object_get(obj));
}

/**
 * @brief Sends the HTTP response (prints JSON payload).
 */
void response_send(response_t* resp) {
        if (!resp || resp->response_sent) return;

        printf("Status: %d\r\n", resp->response_code);
        printf("Content-Type: application/json\r\n\r\n");

        const char* payload =
            json_object_to_json_string_ext(resp->root, JSON_C_TO_STRING_PLAIN);
        printf("%s\n", payload);

        resp->response_sent = true;
}

/**
 * @brief Frees all resources held by a response object.
 */
void response_free(response_t* resp) {
        if (!resp) return;

        if (resp->root) {
                json_object_put(resp->root);
                resp->root     = NULL;
                resp->messages = NULL;
        }
}
