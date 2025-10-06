#ifndef RESULT_H_
#define RESULT_H_

#include <json-c/json.h>
#include <stdarg.h>
#include <stddef.h>

/** Result status codes */
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
        char* message;     /**< Error message (heap) */
        char* failed_file; /**< File where error occurred (heap) */
        char* failed_func; /**< Function where error occurred (heap) */
        char* extra_info;  /**< Additional error information (heap) */
} error_t;

/**
 * @struct result_t
 * @brief Structure to hold result status and error details (heap allocated)
 */
typedef struct {
        result_code_t code; /**< Result status code */
        error_t error;      /**< Error details */
} result_t;

/* Creation / destruction */
result_t* result_new_success(void);
result_t* result_new_failure(const char* message, const char* extra_info,
                             int error_code, const char* failed_file,
                             const char* failed_func);
result_t* result_new_critical_failure(const char* message,
                                      const char* extra_info, int error_code,
                                      const char* failed_file,
                                      const char* failed_func);
void result_free(result_t* res);

/* Convenience macros that capture caller file/func */
#define result_success() result_new_success()
#define result_failure(msg, extra, code) \
        result_new_failure((msg), (extra), (code), __FILE__, __func__)
#define result_critical_failure(msg, extra, code) \
        result_new_critical_failure((msg), (extra), (code), __FILE__, __func__)

/* Mutators */
void result_add_extra(result_t* res, const char* format, ...);

/* Convert to json (caller owns returned json_object* and must json_object_put
 * it) */
struct json_object* result_to_json(const result_t* res);

/* Error codes */
#define ERR_MEMORY_ALLOC_FAIL 999
#define ERR_HEX_DECODE_FAIL 998
#define ERR_TEST_FAIL 9999

#endif /* RESULT_H_ */
