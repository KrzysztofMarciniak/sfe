#include "lib/csrf/csrf.h"

#include <json-c/json.h>

#include "/app/backend/lib/result/result.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DEBUG 0

/**
 * @brief Main function to handle CSRF token generation and validation
 * @return 0 on completion
 */
int main(void) {
        response_t resp;
        const char* method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(&resp, 400);
                response_append(&resp, "Missing request method.");
                response_send(&resp);
                return 0;
        }

        if (strcmp(method, "GET") == 0) {
                char* token  = NULL;
                result_t res = csrf_generate_token(&token);
                if (res.code != RESULT_SUCCESS) {
                        response_init(&resp, 500);
#if DEBUG
                        struct json_object* res_json = result_to_json(&res);
                        if (res_json) {
                                response_append(
                                    &resp,
                                    json_object_to_json_string(res_json));
                                json_object_put(res_json);
                        } else {
                                response_append(
                                    &resp, "Failed to generate CSRF token.");
                        }
#else
                        response_append(&resp,
                                        "Failed to generate CSRF token.");
#endif
                        response_send(&resp);
                        free_result(&res);
                        return 0;
                }

                response_init(&resp, 200);
                struct json_object* response_obj = json_object_new_object();
                json_object_object_add(
                    response_obj, "token",
                    json_object_new_string(token ? token : ""));
                response_append(&resp,
                                json_object_to_json_string(response_obj));

                json_object_put(response_obj);
                free(token);
                free_result(&res);
                response_send(&resp);
                return 0;
        }

        if (strcmp(method, "POST") == 0) {
                char* body  = NULL;
                result_t rc = read_post_data(&body);
                if (rc.code != RESULT_SUCCESS) {
#if DEBUG
                        struct json_object* json_err = result_to_json(&rc);
                        char* json_str = json_object_to_json_string(json_err);
                        response_init(&resp, 400);
                        response_append(&resp, json_str);
                        json_object_put(json_err);
#else
                        if (rc.error.code == ERR_INVALID_CONTENT_LENGTH) {
                                response_init(&resp, 400);
                                response_append(
                                    &resp, "Invalid Content Length for POST");
                        } else if (rc.error.code == ERR_MEMORY_ALLOC_FAIL ||
                                   rc.error.code == ERR_READ_FAIL) {
                                response_init(&resp, 500);
                                response_append(&resp, "Internal Server Error");
                        }
#endif
                        response_send(&resp);
                        free_result(&rc);
                        return 0;
                }
                struct json_object* jobj = json_tokener_parse(body);
                free(body);
                if (!jobj) {
                        response_init(&resp, 400);
                        response_append(&resp, "Malformed JSON.");
                        response_send(&resp);
                        return 0;
                }

                struct json_object* j_token = NULL;
                if (!json_object_object_get_ex(jobj, "token", &j_token)) {
                        response_init(&resp, 400);
                        response_append(&resp, "Missing 'token' field.");
                        json_object_put(jobj);
                        response_send(&resp);
                        return 0;
                }

                const char* token = json_object_get_string(j_token);
                if (!token) {
                        response_init(&resp, 400);
                        response_append(&resp,
                                        "'token' field must be a string.");
                        json_object_put(jobj);
                        response_send(&resp);
                        return 0;
                }

                result_t res = csrf_validate_token(token);
                json_object_put(jobj);
                if (res.code == RESULT_SUCCESS) {
                        response_init(&resp, 200);
                        response_append(&resp, "CSRF token is valid.");
                } else {
#if DEBUG
                        struct json_object* res_json = result_to_json(&res);
                        char* json_str =
                            res_json ? json_object_to_json_string(res_json)
                                     : "JSON conversion failed";
                        response_init(&resp, 400);
                        response_append(&resp, json_str);
                        if (res_json) json_object_put(res_json);
#else
                        switch (res.error.code) {
                                case ERR_NULL_TOKEN:
                                        response_init(&resp, 400);
                                        response_append(&resp,
                                                        "CSRF token is null");
                                        break;
                                case ERR_HEX_DECODE_FAIL:
                                        response_init(&resp, 500);
                                        response_append(
                                            &resp, "Internal Server Error");
                                        break;
                                case ERR_TOKEN_FUTURE_TIMESTAMP:
                                        response_init(&resp, 400);
                                        response_append(&resp,
                                                        "CSRF token timestamp "
                                                        "is in the future");
                                        break;
                                case ERR_TOKEN_EXPIRED:
                                        response_init(&resp, 400);
                                        response_append(
                                            &resp, "CSRF token has expired");
                                        break;
                                case ERR_CSRF_SECRET_EMPTY:
                                        response_init(&resp, 500);
                                        response_append(
                                            &resp, "Internal Server Error");
                                        break;
                                case ERR_HMAC_GENERATION_FAIL:
                                case ERR_HMAC_LENGTH_MISMATCH:
                                case ERR_HMAC_MISMATCH:
                                case ERR_SANITIZATION_FAIL:
                                case ERR_TOKEN_LENGTH_MISMATCH:
                                        response_init(&resp, 500);
                                        response_append(
                                            &resp, "Internal Server Error");
                                        break;
                                default:
                                        response_init(&resp, 500);
                                        response_append(
                                            &resp, "Internal Server Error");
                                        break;
                        }
#endif
                        response_send(&resp);
                }
                free_result(&res);
                return 0;
        }

        response_init(&resp, 405);
        response_append(&resp, "Method Not Allowed");
        response_send(&resp);
        return 0;
}
