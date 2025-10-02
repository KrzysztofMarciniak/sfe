/**
 * @file pass.c
 * @brief CGI endpoint for debugging password hashing and validation.
 *
 * Only works if compiled with DEBUG=1.
 */

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/hash_password/hash_password.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DEBUG 0

#if DEBUG

/**
 * @brief Main function to handle password hashing and validation for debugging
 * @return 0 on completion
 */
int main(void) {
        response_t resp;
        response_init(&resp, 200);
        const char* method = getenv("REQUEST_METHOD");

        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append(&resp, "Method Not Allowed");
                response_send(&resp);
                return 0;
        }

        char* body = read_post_data();
        if (!body) {
                response_init(&resp, 400);
                response_append(&resp, "Missing JSON body");
                response_send(&resp);
                return 0;
        }

        struct json_object* jobj = json_tokener_parse(body);
        free(body);
        if (!jobj) {
                response_init(&resp, 400);
                response_append(&resp, "Malformed JSON");
                response_send(&resp);
                return 0;
        }

        response_append(&resp, "[DEBUG] Received JSON body");
        response_append(&resp, json_object_to_json_string(jobj));

        struct json_object *j_gen = NULL, *j_val = NULL, *j_hash = NULL;

        if (json_object_object_get_ex(jobj, "gen_password", &j_gen)) {
                const char* pw = json_object_get_string(j_gen);
                response_append(&resp, "[DEBUG] Password to hash:");
                response_append(&resp, pw ? pw : "(NULL)");

                if (!pw || !*pw) {
                        response_init(&resp, 400);
                        response_append(&resp, "Password cannot be empty");
                } else {
                        char* hash   = NULL;
                        result_t res = hash_password(pw, &hash);
                        response_append(&resp, "[DEBUG] Hash output:");
                        response_append(&resp, hash ? hash : "(NULL)");

                        if (res.code != success) {
                                response_init(&resp, 500);
                                struct json_object* res_json =
                                    result_to_json(&res);
                                if (res_json) {
                                        response_append(
                                            &resp, json_object_to_json_string(
                                                       res_json));
                                        json_object_put(res_json);
                                } else {
                                        response_append(&resp,
                                                        "Hashing failed");
                                }
                        } else {
                                response_append(&resp, hash);
                        }
                        free(hash);
                        free_result(&res);
                }
        } else if (json_object_object_get_ex(jobj, "val_password", &j_val) &&
                   json_object_object_get_ex(jobj, "hash", &j_hash)) {
                const char* pw   = json_object_get_string(j_val);
                const char* hash = json_object_get_string(j_hash);

                response_append(&resp, "[DEBUG] Password to validate:");
                response_append(&resp, pw ? pw : "(NULL)");
                response_append(&resp, "[DEBUG] Hash provided:");
                response_append(&resp, hash ? hash : "(NULL)");

                if (!pw || !hash || !*pw || !*hash) {
                        response_init(&resp, 400);
                        response_append(&resp,
                                        "Password and hash cannot be empty");
                } else {
                        result_t res = verify_password(pw, hash);
                        response_append(&resp, "[DEBUG] Verification result:");
                        if (res.code != success) {
                                struct json_object* res_json =
                                    result_to_json(&res);
                                if (res_json) {
                                        response_append(
                                            &resp, json_object_to_json_string(
                                                       res_json));
                                        json_object_put(res_json);
                                } else {
                                        response_append(&resp,
                                                        "Verification failed");
                                }
                        } else {
                                response_append(&resp, "Password is valid");
                        }
                        free_result(&res);
                }
        } else {
                response_init(&resp, 400);
                response_append(
                    &resp, "Missing gen_password or val_password/hash fields");
        }

        json_object_put(jobj);
        response_send(&resp);
        return 0;
}

#else

/**
 * @brief Main function when debug endpoint is disabled
 * @return 0 on completion
 */
int main(void) {
        response_t resp;
        response_init(&resp, 404);
        response_append(&resp, "Debug endpoint not available");
        response_send(&resp);
        return 0;
}

#endif
