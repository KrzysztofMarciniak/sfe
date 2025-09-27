#include <json-c/json.h>
#include <stdio.h>
#include <string.h>

int main(void) {
        struct json_object* response_obj = json_object_new_object();
        json_object_object_add(response_obj, "message",
                               json_object_new_string("test works!"));

        const char* json_str = json_object_to_json_string(response_obj);

        // CGI headers
        printf("Status: 200\r\n");
        printf("Content-Type: application/json\r\n");
        printf("Content-Length: %zu\r\n", strlen(json_str));
        printf("\r\n");

        // Body
        printf("%s\n", json_str);

        json_object_put(response_obj);
        return 0;
}
