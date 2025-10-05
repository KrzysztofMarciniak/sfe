#include "result_test.h"

result_t* test_fail(void) {
        result_t* rc = malloc(sizeof(result_t));
        if (!rc) return NULL;

        int test_var = 5;
        *rc          = result_failure("test failuire", NULL, ERR_TEST_FAIL);
        result_add_extra(rc, "internal variable test_var = %i", test_var);
        return rc;
}
