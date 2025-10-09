#include "result.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper: safe strdup wrapper */
static char* safe_strdup(const char* s) {
        if (!s) return NULL;
        char* d = strdup(s);
        return d;
}

/* Create a blank result on heap initialized to success */
result_t* result_new_success(void) {
        result_t* r = (result_t*)malloc(sizeof(result_t));
        if (!r) return NULL;
        r->code = RESULT_SUCCESS;
        return r;
}

/* Internal constructor helper */
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

result_t* result_new_failure(const char* message, const char* extra_info,
                             int error_code, const char* failed_file,
                             const char* failed_func) {
        return result_new_impl(RESULT_FAILURE, message, extra_info, error_code,
                               failed_file, failed_func);
}

result_t* result_new_critical_failure(const char* message,
                                      const char* extra_info, int error_code,
                                      const char* failed_file,
                                      const char* failed_func) {
        return result_new_impl(RESULT_CRITICAL_FAILURE, message, extra_info,
                               error_code, failed_file, failed_func);
}

/* Free internal strings and the struct itself */
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

/* Append formatted extra info (replaces previous extra_info) */
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

/* Convert result to json (caller must json_object_put returned object) */
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
