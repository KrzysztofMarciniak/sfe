#include "/app/backend/lib/result/result.h"

#include "/app/backend/lib/response/response.h"

#define DEBUG 0

#if DEBUG
int main(void) {
        int test_var = 5;

        result_t rc = result_failure("test failure", NULL, ERR_TEST_FAIL);
        result_add_extra(&rc, "test_var = %i", test_var);

        json_object* res_json = result_to_json(&rc);

        response_t resp;
        response_init(&resp, 200);

        response_append_json(&resp, res_json);

        response_send(&resp);
        response_free(&resp);

        free_result(&rc);
        json_object_put(res_json);

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
        response_append_str(&resp, "Debug endpoint not available");
        response_send(&resp);
        return 0;
}

#endif
