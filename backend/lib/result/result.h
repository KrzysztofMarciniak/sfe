/**
 * @file result.h
 * @brief Provides result handling utilities with structured error information
 * and JSON serialization.
 *
 * This module defines a standardized result object for representing
 * success, failure, and critical failure states in a consistent manner.
 * It also provides JSON conversion and convenience macros that capture
 * contextual information automatically.
 */

#ifndef RESULT_H_
#define RESULT_H_

#include <json-c/json.h>
#include <stdarg.h>
#include <stddef.h>

/**
 * @enum result_code
 * @brief Result status codes.
 */
typedef enum result_code {
        RESULT_SUCCESS = 0, /**< Operation completed successfully */
        RESULT_FAILURE = 1, /**< Recoverable failure occurred */
        RESULT_CRITICAL_FAILURE =
            2 /**< Unrecoverable/critical failure occurred */
} result_code_t;

/**
 * @struct error_t
 * @brief Structure holding detailed information about an error.
 *
 * Each field is heap-allocated; caller is responsible for cleanup
 * through result_free().
 */
typedef struct {
        int code;          /**< Library-specific error code */
        char* message;     /**< Human-readable error message */
        char* failed_file; /**< Source file where error occurred */
        char* failed_func; /**< Function name where error occurred */
        char* extra_info;  /**< Optional additional context or details */
} error_t;

/**
 * @struct result_t
 * @brief Represents the result of an operation, including optional error info.
 *
 * Uses a union to minimize memory usage. For successful operations, no
 * error_t structure is allocated.
 */
typedef struct {
        result_code_t code; /**< Status code representing success or failure */

        union {
                struct {
                } success; /**< Placeholder for success (no additional data) */

                error_t error; /**< Error information for failures */
        } data;
} result_t;

/**
 * @brief Create a new success result.
 *
 * @return Pointer to a heap-allocated success result_t.
 */
result_t* result_new_success(void);

/**
 * @brief Create a new failure result with contextual information.
 *
 * @param message     Error message.
 * @param extra_info  Additional details (nullable).
 * @param error_code  Library-specific error code.
 * @param failed_file Source file where the error occurred.
 * @param failed_func Function name where the error occurred.
 * @return Pointer to a heap-allocated failure result_t.
 */
result_t* result_new_failure(const char* message, const char* extra_info,
                             int error_code, const char* failed_file,
                             const char* failed_func);

/**
 * @brief Create a new critical failure result with contextual information.
 *
 * @param message     Error message.
 * @param extra_info  Additional details (nullable).
 * @param error_code  Library-specific error code.
 * @param failed_file Source file where the error occurred.
 * @param failed_func Function name where the error occurred.
 * @return Pointer to a heap-allocated critical failure result_t.
 */
result_t* result_new_critical_failure(const char* message,
                                      const char* extra_info, int error_code,
                                      const char* failed_file,
                                      const char* failed_func);

/**
 * @brief Free memory associated with a result_t object.
 *
 * @param res Pointer to result_t to free (nullable).
 */
void result_free(result_t* res);

/**
 * @def result_success
 * @brief Convenience macro to create a success result capturing caller context.
 */
#define result_success() result_new_success()

/**
 * @def result_failure
 * @brief Convenience macro to create a failure result capturing caller context.
 */
#define result_failure(msg, extra, code) \
        result_new_failure((msg), (extra), (code), __FILE__, __func__)

/**
 * @def result_critical_failure
 * @brief Convenience macro to create a critical failure result capturing caller
 * context.
 */
#define result_critical_failure(msg, extra, code) \
        result_new_critical_failure((msg), (extra), (code), __FILE__, __func__)

/**
 * @brief Append formatted extra information to an existing result.
 *
 * Uses printf-style formatting. If res is NULL or not a failure, the call is
 * ignored.
 *
 * @param res    Result object to modify.
 * @param format Format string.
 * @param ...    Variadic arguments for formatting.
 */
void result_add_extra(result_t* res, const char* format, ...);

/**
 * @brief Convert a result_t object into a JSON representation.
 *
 * Caller takes ownership of the returned json_object* and must release it
 * with json_object_put().
 *
 * @param res Pointer to result_t.
 * @return New JSON object representing the result.
 */
struct json_object* result_to_json(const result_t* res);

/** @name Common Error Codes
 *  @brief Predefined library-level error codes.
 *  @{
 */
#define ERR_MEMORY_ALLOC_FAIL 999 /**< Memory allocation failure */
#define ERR_HEX_DECODE_FAIL 998   /**< Hex decode operation failed */
#define ERR_TEST_FAIL 9999        /**< Generic test failure */
/** @} */

#endif /* RESULT_H_ */
