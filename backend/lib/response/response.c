#include <json-c/json.h>
#include <stdbool.h>
#include <stdio.h>

static struct json_object *response_array = NULL;
static unsigned int response_code         = 200;
static bool response_sent                 = false;

void response_init(unsigned int http_code) {
        response_code = http_code;
        response_sent = false;

        if (response_array) {
                json_object_put(response_array);
        }
        response_array = json_object_new_array();
}

bool response_append(const char *msg) {
        if (!msg || response_sent || !response_array) return false;

        struct json_object *jmsg = json_object_new_string(msg);
        if (!jmsg) return false;

        json_object_array_add(response_array, jmsg);
        return true;
}

void response_send(void) {
        if (response_sent || !response_array) return;
        response_sent = true;

        struct json_object *response_obj = json_object_new_object();
        json_object_object_add(response_obj, "messages", response_array);

        printf("Status: %d\r\n", response_code);
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", json_object_to_json_string(response_obj));

        json_object_put(response_obj);
        response_array = NULL;
}
