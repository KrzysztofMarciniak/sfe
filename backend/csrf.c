#include "lib/csrf/csrf.h"

#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
int main(void) {
        const char *method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(400);
                response_append("Missing request method.");
                response_send();
                return 0;
        }

        if (strcmp(method, "GET") == 0) {
                char *token = csrf_generate_token();
                if (!token) {
                        response_init(500);
                        response_append("Failed to generate CSRF token.");
                        response_send();
                        return 0;
                }
                free(token);
                return 0;
        }

        if (strcmp(method, "POST") == 0) {
                char *body = read_post_data();
                if (!body) {
                        response_init(400);
                        response_append("Missing or invalid POST body.");
                        response_send();
                        return 0;
                }

                struct json_object *jobj = json_tokener_parse(body);
                free(body);
                if (!jobj) {
                        response_init(400);
                        response_append("Malformed JSON.");
                        response_send();
                        return 0;
                }

                struct json_object *j_token = NULL;
                if (!json_object_object_get_ex(jobj, "message", &j_token)) {
                        response_init(400);
                        response_append("Missing 'message' field.");
                        response_send();
                        return 0;
                }

                const char *token = json_object_get_string(j_token);
                if (!token) {
                        response_init(400);
                        response_append("'message' field must be a string.");
                        response_send();
                        return 0;
                }
                bool valid = csrf_validate_token(token);
                json_object_put(jobj);
                if (valid) {
                        response_init(200);
                        response_send();
                } else {
                        response_init(400);
                        response_send();
                }
                return 0;
        }
        response_init(405);
        response_append("Method Not Allowed");
        return 0;
}
