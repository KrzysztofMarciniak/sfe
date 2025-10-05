#include "result_test.h"

result_t* test_fail(void) {
        result_t* rc = result_failure("test failuire", NULL, ERR_TEST_FAIL);
        if (!rc) return NULL;

        int test_var = 5;
        result_add_extra(rc, "internal variable test_var = %i", test_var);
        return rc;
}
