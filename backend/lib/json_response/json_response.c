/**
 * @file json_response.c
 * @brief Implementation of JsonResponse helper using json-c.
 *
 * This file provides functions for creating a JsonResponse, setting its
 * message, building a JSON string, and freeing resources. The JsonResponse
 * object owns its message buffer and json-c json_object and exposes
 * method-like function pointers for chaining.
 */

#include "json_response.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Sets the message for the JSON response.
 *
 * Frees any existing message and sets a new one by duplicating the input
 * string. If msg is NULL, the existing message is freed and set to NULL.
 *
 * @param resp Pointer to the JsonResponse instance.
 * @param msg The message string to set (may be NULL).
 * @return Pointer to the JsonResponse for method chaining.
 */
static JsonResponse *set_message(JsonResponse *resp, const char *msg) {
    if (!resp) return NULL;
    if (resp->msg) {
        free(resp->msg);
        resp->msg = NULL;
    }
    if (msg) {
        resp->msg = strdup(msg);
    }
    return resp;
}

/**
 * @brief Builds the JSON string from the status and message.
 *
 * Constructs a JSON string using json-c, in the format {"status": <status>}
 * or {"status": <status>, "message": "<msg>"}.
 *
 * The generated JSON string is owned by the internal json_object and remains
 * valid until resp->build is called again or resp is freed.
 *
 * @param resp Pointer to the JsonResponse instance.
 * @return Const pointer to the generated JSON string, or a static error JSON on
 * failure.
 */

static const char *build_json(JsonResponse *resp) {
    if (!resp) return "{\"error\":\"invalid response\"}";

    /* Create a new JSON object for each build to ensure clean state */
    json_object *obj = json_object_new_object();
    if (!obj) {
        return "{\"error\":\"json object creation failed\"}";
    }

    /* Add status */
    json_object_object_add(obj, "status", json_object_new_int(resp->status));

    /* Add message if present */
    if (resp->msg) {
        json_object_object_add(obj, "message",
                               json_object_new_string(resp->msg));
    }

    /* Replace previous json_obj with the new one (decrement refcount of old) */
    if (resp->json_obj) {
        json_object_put(resp->json_obj);
    }
    resp->json_obj = obj;

    return json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
}

/**
 * @brief Frees the JsonResponse and its associated resources.
 *
 * Safely deallocates the message, JSON object, and the JsonResponse struct
 * itself.
 *
 * @param resp Pointer to the JsonResponse instance to free.
 */
static void free_response(JsonResponse *resp) {
    if (!resp) return;
    if (resp->msg) free(resp->msg);
    if (resp->json_obj) json_object_put(resp->json_obj);
    free(resp);
}

/**
 * @brief Creates a new JSON response with the specified status code.
 *
 * Allocates and initializes a JsonResponse instance and wires up method
 * pointers.
 *
 * @param status The HTTP status code (e.g., 200, 400).
 * @return Pointer to the newly created JsonResponse, or NULL on allocation
 * failure.
 */
JsonResponse *return_json(int status) {
    JsonResponse *resp = malloc(sizeof(JsonResponse));
    if (!resp) {
        return NULL;
    }
    resp->status = status;
    resp->msg = NULL;
    resp->json_obj = NULL;
    resp->set_message = set_message;
    resp->build = build_json;
    resp->free = free_response;
    return resp;
}
