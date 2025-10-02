#ifndef RESULT_H_
#define RESULT_H_

#include <json-c/json.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum result_code {
        RESULT_SUCCESS          = 0,
        RESULT_FAILURE          = 1,
        RESULT_CRITICAL_FAILURE = 2
} result_code_t;

/**
 * @struct error_t
 * @brief Structure to hold error information for library-specific errors
 */
typedef struct {
        int code;          /**< Library-specific error code */
        char* message;     /**< Error message */
        char* failed_file; /**< File where error occurred */
        char* failed_func; /**< Function where error occurred */
        char* extra_info;  /**< Additional error information */
} error_t;

/**
 * @struct result_t
 * @brief Structure to hold result status and error details
 */
typedef struct {
        result_code_t code; /**< Result status code */
        error_t error;      /**< Error details */
} result_t;

/**
 * @brief Create a successful result
 * @return result_t with success status
 */
static inline result_t result_success(void) {
        result_t res = {.code  = RESULT_SUCCESS,
                        .error = {.code        = 0,
                                  .message     = NULL,
                                  .failed_file = NULL,
                                  .failed_func = NULL,
                                  .extra_info  = NULL}};
        return res;
}

/**
 * @brief Add formatted extra information to a result
 * @param res Pointer to result_t to modify
 * @param format Format string for extra info
 * @param ... Variable arguments for format string
 */
static void result_add_extra(result_t* res, const char* format, ...) {
        if (!res || !format) return;

        va_list args;
        va_start(args, format);

        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(NULL, 0, format, args_copy) + 1;
        va_end(args_copy);

        char* buffer = malloc(size);
        if (buffer) {
                vsnprintf(buffer, size, format, args);
                free(res->error.extra_info);
                res->error.extra_info = buffer;
        }

        va_end(args);
}

/**
 * @brief Create a failure result
 * @param message Error message
 * @param extra_info Additional error information
 * @param error_code Library-specific error code
 * @return result_t with failure status
 */
static inline result_t result_failure(const char* message,
                                      const char* extra_info, int error_code) {
        result_t res = {
            .code  = RESULT_FAILURE,
            .error = {.code        = error_code,
                      .message     = message ? strdup(message) : NULL,
                      .failed_file = strdup(__FILE__),
                      .failed_func = strdup(__func__),
                      .extra_info  = extra_info ? strdup(extra_info) : NULL}};
        return res;
}

/**
 * @brief Create a critical failure result
 * @param message Error message
 * @param extra_info Additional error information
 * @param error_code Library-specific error code
 * @return result_t with critical failure status
 */
static inline result_t result_critical_failure(const char* message,
                                               const char* extra_info,
                                               int error_code) {
        result_t res = {
            .code  = RESULT_CRITICAL_FAILURE,
            .error = {.code        = error_code,
                      .message     = message ? strdup(message) : NULL,
                      .failed_file = strdup(__FILE__),
                      .failed_func = strdup(__func__),
                      .extra_info  = extra_info ? strdup(extra_info) : NULL}};
        return res;
}

/**
 * @brief Free resources allocated in a result
 * @param res Pointer to result_t to free
 */
static inline void free_result(result_t* res) {
        if (res) {
                free(res->error.message);
                free(res->error.failed_file);
                free(res->error.failed_func);
                free(res->error.extra_info);
                res->error.message     = NULL;
                res->error.failed_file = NULL;
                res->error.failed_func = NULL;
                res->error.extra_info  = NULL;
        }
}

/**
 * @brief Convert result to JSON object
 * @param res Pointer to result_t to convert
 * @return JSON object representing the result, or NULL on failure
 */
static inline struct json_object* result_to_json(const result_t* res) {
        if (!res) return NULL;

        struct json_object* obj = json_object_new_object();
        if (!obj) return NULL;

        const char* code_str = res->code == RESULT_SUCCESS ? "success"
                               : res->code == RESULT_FAILURE
                                   ? "failure"
                                   : "critical_failure";

        json_object_object_add(obj, "code", json_object_new_string(code_str));
        json_object_object_add(obj, "code_value",
                               json_object_new_int((int)res->code));

        struct json_object* error_obj = json_object_new_object();
        if (!error_obj) {
                json_object_put(obj);
                return NULL;
        }

        json_object_object_add(error_obj, "code",
                               json_object_new_int(res->error.code));
        json_object_object_add(
            error_obj, "message",
            json_object_new_string(res->error.message ? res->error.message
                                                      : ""));
        json_object_object_add(
            error_obj, "failed_file",
            json_object_new_string(
                res->error.failed_file ? res->error.failed_file : ""));
        json_object_object_add(
            error_obj, "failed_func",
            json_object_new_string(
                res->error.failed_func ? res->error.failed_func : ""));
        json_object_object_add(
            error_obj, "extra_info",
            json_object_new_string(res->error.extra_info ? res->error.extra_info
                                                         : ""));

        json_object_object_add(obj, "error", error_obj);
        return obj;
}

#define ERR_MEMORY_ALLOC_FAIL 999
#define ERR_HEX_DECODE_FAIL 998
#endif /* RESULT_H_ */
