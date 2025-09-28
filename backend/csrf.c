#include "lib/csrf/csrf.h"

#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DEBUG 0

int main(void) {
        response_t resp;
        const char* errmsg = NULL;
        const char* method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(&resp, 400);
                response_append(&resp, "Missing request method.");
                response_send(&resp);
                return 0;
        }

        if (strcmp(method, "GET") == 0) {
                char* token = csrf_generate_token(&errmsg);
                if (!token) {
                        response_init(&resp, 500);
#if DEBUG
                        if (errmsg) {
                                response_append(&resp, errmsg);
                        } else {
                                response_append(
                                    &resp, "Failed to generate CSRF token.");
                        }
#else
                        response_append(&resp,
                                        "Failed to generate CSRF token.");
#endif
                        response_send(&resp);
                        return 0;
                }

                response_init(&resp, 200);
                struct json_object* token_obj = json_object_new_string(token);
                struct json_object* response_obj = json_object_new_object();
                json_object_object_add(response_obj, "token", token_obj);
                printf("Status: 200 OK\r\n");
                printf("Content-Type: application/json\r\n\r\n");
                printf("%s\n", json_object_to_json_string(response_obj));

                json_object_put(response_obj);
                free(token);
                return 0;
        }

        if (strcmp(method, "POST") == 0) {
                char* body = read_post_data();
                if (!body) {
                        response_init(&resp, 400);
                        response_append(&resp, "Missing or invalid POST body.");
                        response_send(&resp);
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

                bool valid = csrf_validate_token(token, &errmsg);
                json_object_put(jobj);

                if (valid) {
                        response_init(&resp, 200);
                        response_append(&resp, "CSRF token is valid.");
                        response_send(&resp);
                } else {
                        response_init(&resp, 400);
#if DEBUG
                        if (errmsg) {
                                response_append(&resp, errmsg);
                        } else {
                                response_append(
                                    &resp, "CSRF token validation failed.");
                        }
#else
                        response_append(&resp, "CSRF token validation failed.");
#endif
                        response_send(&resp);
                }
                return 0;
        }

        response_init(&resp, 405);
        response_append(&resp, "Method Not Allowed");
        response_send(&resp);
        return 0;
}
