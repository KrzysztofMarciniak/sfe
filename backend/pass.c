#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "/app/backend/lib/result/result.h"
#include "lib/hash_password/hash_password.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DEBUG 1

#if DEBUG

int main(void) {
        response_t resp;
        response_init(&resp, 200);

        const char* method = getenv("REQUEST_METHOD");
        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append_str(&resp, "Method Not Allowed");
                response_send(&resp);
                response_free(&resp);
                return 0;
        }

        char* body     = NULL;
        result_t* pd_r = read_post_data(&body);
        if (pd_r->code != RESULT_SUCCESS) {
                response_init(&resp, 400);
                response_append_str(&resp, "Missing or invalid POST body");
                struct json_object* res_json = result_to_json(pd_r);
                if (res_json) {
                        response_append_json(&resp, res_json);
                        json_object_put(res_json);
                }
                response_send(&resp);
                response_free(&resp);
                result_free(pd_r);
                free(body);
                return 0;
        }

        result_free(pd_r);

        struct json_object* jobj = json_tokener_parse(body);
        free(body);
        if (!jobj) {
                response_init(&resp, 400);
                response_append_str(&resp, "Malformed JSON");
                response_send(&resp);
                response_free(&resp);
                return 0;
        }

        struct json_object *j_val = NULL, *j_hash = NULL;

        if (json_object_object_get_ex(jobj, "val_password", &j_val) &&
            json_object_object_get_ex(jobj, "hash", &j_hash)) {
                const char* pw   = json_object_get_string(j_val);
                const char* hash = json_object_get_string(j_hash);

                if (!pw || !hash || !*pw || !*hash) {
                        response_init(&resp, 400);
                        response_append_str(
                            &resp, "Password and hash cannot be empty");
                } else {
                        result_t* res = verify_password(pw, hash);
                        response_init(&resp,
                                      res->code == RESULT_SUCCESS ? 200 : 400);
                        if (res->code != RESULT_SUCCESS) {
                                struct json_object* res_json =
                                    result_to_json(res);
                                if (res_json) {
                                        response_append_json(&resp, res_json);
                                        json_object_put(res_json);
                                } else {
                                        response_append_str(
                                            &resp, "Verification failed");
                                }
                        } else {
                                response_append_str(&resp, "Password is valid");
                        }
                        result_free(res);
                }

        } else {
                response_init(&resp, 400);
                response_append_str(&resp,
                                    "Missing val_password or hash fields");
        }

        json_object_put(jobj);
        response_send(&resp);
        response_free(&resp);

        return 0;
}

#else

int main(void) {
        response_t resp;
        response_init(&resp, 404);
        response_append_str(&resp, "Debug endpoint not available");
        response_send(&resp);
        response_free(&resp);
        return 0;
}

#endif
