#include <json-c/json.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

static __thread struct json_object *response_array = NULL;
static __thread int response_code                  = 200;
static __thread bool response_sent                 = false;
static pthread_mutex_t response_mutex              = PTHREAD_MUTEX_INITIALIZER;

void response_init(int http_code) {
        pthread_mutex_lock(&response_mutex);
        if (response_array) {
                json_object_put(response_array);
        }
        response_code  = http_code;
        response_sent  = false;
        response_array = json_object_new_array();
        pthread_mutex_unlock(&response_mutex);
}

bool response_append(const char *msg) {
        pthread_mutex_lock(&response_mutex);
        if (!msg || response_sent || !response_array) {
                pthread_mutex_unlock(&response_mutex);
                return false;
        }

        struct json_object *jmsg = json_object_new_string(msg);
        if (!jmsg) {
                pthread_mutex_unlock(&response_mutex);
                return false;
        }

        json_object_array_add(response_array, jmsg);
        pthread_mutex_unlock(&response_mutex);
        return true;
}

void response_send(void) {
        pthread_mutex_lock(&response_mutex);
        if (response_sent || !response_array) {
                pthread_mutex_unlock(&response_mutex);
                return;
        }
        response_sent = true;

        struct json_object *response_obj = json_object_new_object();
        json_object_object_add(response_obj, "messages", response_array);

        printf("Status: %d\r\n", response_code);
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", json_object_to_json_string(response_obj));

        json_object_put(response_obj);
        response_array = NULL;
        pthread_mutex_unlock(&response_mutex);
}
