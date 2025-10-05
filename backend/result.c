#include "/app/backend/lib/result/result.h"

#include "/app/backend/lib/response/response.h"
#include "/app/backend/lib/result_test/result_test.h"

#define DEBUG 1

#if DEBUG
int main(void) {
        result_t* rc          = test_fail();
        json_object* res_json = result_to_json(rc);

        response_t resp;
        response_init(&resp, 200);
        response_append_json(&resp, res_json);
        json_object_put(res_json);

        response_send(&resp);
        response_free(&resp);

        free_result(rc);
        free(rc);
}

#else
/**
 * @brief Main function when debug endpoint is disabled
 * @return 0 on completion
 */
int main(void) {
        response_t resp;
        response_init(&resp, 404);
        response_append_str(&resp, "Debug endpoint not available");
        response_send(&resp);
        return 0;
}

#endif
