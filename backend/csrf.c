#include "lib/csrf/csrf.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

#include "/app/backend/lib/result/result.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DEBUG 0

int main(void) {
        response_t resp;
        const char* method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(&resp, 400);
                response_append_str(&resp, "Missing request method.");
                response_send(&resp);
                response_free(&resp);
                return 0;
        }

        if (strcmp(method, "GET") == 0) {
                char* token   = NULL;
                result_t* res = csrf_generate_token(&token);
                if (res->code != RESULT_SUCCESS) {
                        response_init(&resp, 500);
#if DEBUG
                        struct json_object* res_json = result_to_json(&res);
                        if (res_json) {
                                response_append_json(&resp, res_json);
                                json_object_put(res_json);
                        } else {
                                response_append_str(
                                    &resp, "Failed to generate CSRF token.");
                        }
#else
                        response_append_str(&resp,
                                            "Failed to generate CSRF token.");
#endif
                        response_send(&resp);
                        response_free(&resp);
                        result_free(res);
                        return 0;
                }

                response_init(&resp, 200);
                response_append_str(&resp, token ? token : "");
                free(token);
                result_free(res);
                response_send(&resp);
                response_free(&resp);
                return 0;
        }

        if (strcmp(method, "POST") == 0) {
                char* body   = NULL;
                result_t* rc = read_post_data(&body);
                if (rc->code != RESULT_SUCCESS) {
                        response_init(
                            &resp, rc->error.code == ERR_INVALID_CONTENT_LENGTH
                                       ? 400
                                       : 500);
#if DEBUG
                        struct json_object* json_err = result_to_json(&rc);
                        if (json_err) {
                                response_append_json(&resp, json_err);
                                json_object_put(json_err);
                        } else {
                                response_append_str(&resp,
                                                    "Error reading POST data.");
                        }
#else
                        if (rc->error.code == ERR_INVALID_CONTENT_LENGTH) {
                                response_append_str(
                                    &resp, "Invalid Content Length for POST");
                        } else {
                                response_append_str(&resp,
                                                    "Internal Server Error");
                        }
#endif
                        response_send(&resp);
                        response_free(&resp);
                        result_free(rc);
                        return 0;
                }

                struct json_object* jobj = json_tokener_parse(body);
                free(body);
                if (!jobj) {
                        response_init(&resp, 400);
                        response_append_str(&resp, "Malformed JSON.");
                        response_send(&resp);
                        response_free(&resp);
                        return 0;
                }

                struct json_object* j_token = NULL;
                if (!json_object_object_get_ex(jobj, "token", &j_token)) {
                        response_init(&resp, 400);
                        response_append_str(&resp, "Missing 'token' field.");
                        json_object_put(jobj);
                        response_send(&resp);
                        response_free(&resp);
                        return 0;
                }

                const char* token = json_object_get_string(j_token);
                if (!token) {
                        response_init(&resp, 400);
                        response_append_str(&resp,
                                            "'token' field must be a string.");
                        json_object_put(jobj);
                        response_send(&resp);
                        response_free(&resp);
                        return 0;
                }

                result_t* res = csrf_validate_token(token);
                json_object_put(jobj);

                if (res->code == RESULT_SUCCESS) {
                        response_init(&resp, 200);
                        response_append_str(&resp, "CSRF token is valid.");
                        response_send(&resp);
                        response_free(&resp);
                } else {
#if DEBUG
                        struct json_object* res_json = result_to_json(&res);
                        response_init(&resp, 400);
                        if (res_json) {
                                response_append_json(&resp, res_json);
                                json_object_put(res_json);
                        } else {
                                response_append_str(&resp,
                                                    "JSON conversion failed.");
                        }
#else
                        switch (res->error.code) {
                                case ERR_TOKEN_LENGTH_MISMATCH:
                                        response_init(&resp, 400);
                                        response_append_str(
                                            &resp, "Token Length Mismatch.");
                                case ERR_NULL_TOKEN:
                                case ERR_TOKEN_EXPIRED:
                                case ERR_TOKEN_FUTURE_TIMESTAMP:
                                case ERR_CSRF_SECRET_EMPTY:
                                        response_init(&resp, 400);
                                        response_append_str(
                                            &resp, "Invalid csrf Token.");

                                        break;
                                default:
                                        response_init(&resp, 500);
                                        response_append_str(
                                            &resp, "Internal Server Error");
                                        break;
                        }
#endif
                        response_send(&resp);
                        response_free(&resp);
                }

                result_free(res);
                return 0;
        }

        response_init(&resp, 405);
        response_append_str(&resp, "Method Not Allowed");
        response_send(&resp);
        response_free(&resp);
        return 0;
}
