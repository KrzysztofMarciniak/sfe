#include <stdlib.h>
#include <string.h>

#include "lib/response/response.h"

int main(void) {
        const char *method = getenv("REQUEST_METHOD");

        if (method && strcmp(method, "GET") == 0) {
                response(200, "Hello, World");
        } else {
                response(405, "Method Not Allowed");
        }

        return 0;
}
