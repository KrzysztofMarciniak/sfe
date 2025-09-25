#include <stdlib.h>
#include <string.h>

#include "lib/response/response.h"

int main(void) {
        const char *method = getenv("REQUEST_METHOD");

        if (!method) {
                response_init(400);
                response_append("Bad Request: Missing REQUEST_METHOD");
                response_send();
                return 1;
        }

        if (strcmp(method, "GET") == 0) {
                response_init(200);
                response_append("Hello, World");
                response_send();
        } else {
                response_init(405);
                response_append("Method Not Allowed");
                response_send();
        }

        return 0;
}
