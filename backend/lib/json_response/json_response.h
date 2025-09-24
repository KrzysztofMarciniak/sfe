/**
 * @file json_response.h
 * @brief JSON response helper using json-c for web applications.
 *
 * This header provides the JsonResponse "object" — a small struct that
 * encapsulates an HTTP status code, an optional message, and a json-c
 * json_object. It exposes method-like function pointers for:
 *  - setting a message,
 *  - building a JSON string representation,
 *  - freeing the instance.
 *
 * Example:
 * @code
 * JsonResponse *r = return_json(200);
 * const char *json = r->set_message(r, "OK")->build(r);
 * // send json ...
 * r->free(r);
 * @endcode
 */

#ifndef JSON_RESPONSE_H
#define JSON_RESPONSE_H

#include <json-c/json.h>

/// Opaque typedef for the JsonResponse structure.
typedef struct JsonResponse JsonResponse;

/**
 * @struct JsonResponse
 * @brief Structure representing a JSON response for a web application.
 *
 * The struct contains an HTTP status code, an optional message string,
 * a json-c json_object used to construct the payload, and function pointers
 * that behave as methods for common operations. Function pointers are
 * arranged to allow method chaining.
 */
struct JsonResponse {
    /** HTTP status code of the response (e.g., 200, 400). */
    int status;

    /** Optional message string included in the response JSON.
     *
     * Ownership: the JsonResponse instance owns this buffer and will free it
     * when the instance's free method is called. May be NULL if unset.
     */
    char *msg;

    /** json-c object used to build the JSON payload. Managed by the instance. */
    json_object *json_obj;

    /**
     * @brief Set the message for the JSON response.
     * @param self Pointer to the JsonResponse instance.
     * @param msg Null-terminated message string to copy into the instance.
     * @return Pointer to the same JsonResponse instance to allow chaining.
     *
     * The function should copy the provided @p msg (or set to NULL) and update
     * the internal json_obj accordingly.
     */
    JsonResponse *(*set_message)(JsonResponse *self, const char *msg);

    /**
     * @brief Build the JSON string from the status and message.
     * @param self Pointer to the JsonResponse instance.
     * @return Const pointer to a null-terminated JSON string.
     *
     * The returned string is owned by the JsonResponse instance and remains
     * valid until the instance is modified or freed.
     */
    const char *(*build)(JsonResponse *self);

    /**
     * @brief Free the JsonResponse and all associated resources.
     * @param self Pointer to the JsonResponse instance to free.
     *
     * This function must free the msg buffer, free/destroy the json_obj,
     * and finally free the JsonResponse struct itself.
     */
    void (*free)(JsonResponse *self);
};

/**
 * @brief Create a new JsonResponse with the specified HTTP status code.
 * @param status HTTP status code to use for the response (e.g., 200, 400).
 * @return Pointer to a newly allocated JsonResponse on success; NULL on failure.
 *
 * The returned instance has function pointers initialized for set_message,
 * build, and free. The caller is responsible for calling the free method
 * when finished.
 */
JsonResponse *return_json(int status);

#endif /* JSON_RESPONSE_H */
