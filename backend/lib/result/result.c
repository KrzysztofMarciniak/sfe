/**
 * @file result.c
 * @brief Implementation of result handling and error reporting utilities.
 *
 * Provides constructors, destructors, and JSON serialization
 * for result_t objects, which encapsulate success/failure states
 * and detailed error information.
 */

#include "result.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Duplicate a string safely.
 *
 * Wrapper around strdup() that handles NULL inputs gracefully.
 *
 * @param s Input string.
 * @return Newly allocated copy of @p s, or NULL if @p s is NULL or allocation
 * fails.
 */
static char* safe_strdup(const char* s) {
        if (!s) return NULL;
        char* d = strdup(s);
        return d;
}

/**
 * @brief Allocate and initialize a success result.
 *
 * @return Pointer to a heap-allocated result_t representing success, or NULL on
 * allocation failure.
 */
result_t* result_new_success(void) {
        result_t* r = (result_t*)malloc(sizeof(result_t));
        if (!r) return NULL;
        r->code = RESULT_SUCCESS;
        return r;
}

/**
 * @brief Internal helper to construct result objects.
 *
 * Shared logic used by failure and critical failure constructors.
 *
 * @param rc          Result code (failure or critical failure).
 * @param message     Error message.
 * @param extra_info  Additional context or details.
 * @param error_code  Library-specific error code.
 * @param failed_file File name where the error occurred.
 * @param failed_func Function name where the error occurred.
 * @return Pointer to a new heap-allocated result_t, or NULL if allocation
 * fails.
 */
static result_t* result_new_impl(result_code_t rc, const char* message,
                                 const char* extra_info, int error_code,
                                 const char* failed_file,
                                 const char* failed_func) {
        result_t* r = (result_t*)malloc(sizeof(result_t));
        if (!r) return NULL;

        r->code                   = rc;
        r->data.error.code        = error_code;
        r->data.error.message     = safe_strdup(message);
        r->data.error.failed_file = safe_strdup(failed_file);
        r->data.error.failed_func = safe_strdup(failed_func);
        r->data.error.extra_info  = safe_strdup(extra_info);

        return r;
}

/**
 * @brief Create a new failure result.
 *
 * @param message     Error message.
 * @param extra_info  Optional context information.
 * @param error_code  Library-specific error code.
 * @param failed_file Source file name.
 * @param failed_func Function name.
 * @return Pointer to a heap-allocated failure result_t.
 */
result_t* result_new_failure(const char* message, const char* extra_info,
                             int error_code, const char* failed_file,
                             const char* failed_func) {
        return result_new_impl(RESULT_FAILURE, message, extra_info, error_code,
                               failed_file, failed_func);
}

/**
 * @brief Create a new critical failure result.
 *
 * @param message     Error message.
 * @param extra_info  Optional context information.
 * @param error_code  Library-specific error code.
 * @param failed_file Source file name.
 * @param failed_func Function name.
 * @return Pointer to a heap-allocated critical failure result_t.
 */
result_t* result_new_critical_failure(const char* message,
                                      const char* extra_info, int error_code,
                                      const char* failed_file,
                                      const char* failed_func) {
        return result_new_impl(RESULT_CRITICAL_FAILURE, message, extra_info,
                               error_code, failed_file, failed_func);
}

/**
 * @brief Release memory owned by a result_t structure.
 *
 * Frees all dynamically allocated strings and the result_t itself.
 *
 * @param res Pointer to the result_t object. Safe to pass NULL.
 */
void result_free(result_t* res) {
        if (!res) return;
        free(res->data.error.message);
        free(res->data.error.failed_file);
        free(res->data.error.failed_func);
        free(res->data.error.extra_info);
        res->data.error.message     = NULL;
        res->data.error.failed_file = NULL;
        res->data.error.failed_func = NULL;
        res->data.error.extra_info  = NULL;
        free(res);
}

/**
 * @brief Append formatted extra information to a result.
 *
 * Replaces any existing extra_info. Ignored if the result represents success.
 *
 * @param res    Target result_t.
 * @param format printf-style format string.
 * @param ...    Arguments for the format string.
 */
void result_add_extra(result_t* res, const char* format, ...) {
        if (!res || !format) return;
        if (res->code == RESULT_SUCCESS) return;

        va_list args;
        va_start(args, format);

        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(NULL, 0, format, args_copy) + 1;
        va_end(args_copy);

        char* buffer = (char*)malloc((size_t)size);
        if (buffer) {
                vsnprintf(buffer, size, format, args);
                free(res->data.error.extra_info);
                res->data.error.extra_info = buffer;
        }

        va_end(args);
}

/**
 * @brief Convert a result_t object to a JSON representation.
 *
 * Returns a json_object with "code", "code_value", and (for failures)
 * an "error" object containing fields for code, message, file, function,
 * and extra_info.
 *
 * Caller must free the returned object using json_object_put().
 *
 * @param res Pointer to result_t.
 * @return Newly created json_object*, or NULL if res is NULL or success (no
 * error).
 */
struct json_object* result_to_json(const result_t* res) {
        if (!res) return NULL;

        if (res->code != RESULT_SUCCESS) {
                struct json_object* obj = json_object_new_object();
                if (!obj) return NULL;

                const char* code_str = res->code == RESULT_SUCCESS ? "success"
                                       : res->code == RESULT_FAILURE
                                           ? "failure"
                                           : "critical_failure";

                json_object_object_add(obj, "code",
                                       json_object_new_string(code_str));
                json_object_object_add(obj, "code_value",
                                       json_object_new_int((int)res->code));

                struct json_object* error_obj = json_object_new_object();
                if (!error_obj) {
                        json_object_put(obj);
                        return NULL;
                }

                json_object_object_add(
                    error_obj, "code",
                    json_object_new_int(res->data.error.code));
                json_object_object_add(
                    error_obj, "message",
                    json_object_new_string(res->data.error.message
                                               ? res->data.error.message
                                               : ""));
                json_object_object_add(
                    error_obj, "failed_file",
                    json_object_new_string(res->data.error.failed_file
                                               ? res->data.error.failed_file
                                               : ""));
                json_object_object_add(
                    error_obj, "failed_func",
                    json_object_new_string(res->data.error.failed_func
                                               ? res->data.error.failed_func
                                               : ""));
                json_object_object_add(
                    error_obj, "extra_info",
                    json_object_new_string(res->data.error.extra_info
                                               ? res->data.error.extra_info
                                               : ""));

                json_object_object_add(obj, "error", error_obj);
                return obj;
        }

        return NULL;
}
