/**
 * @file test_sanitizer.c
 * @brief Unit tests for sanitizer functions using Check framework.
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "sanitizer.h"

START_TEST(test_str_trim_basic) {
        char input[]  = "  hello world  ";
        char *trimmed = str_trim(input);
        ck_assert_str_eq(trimmed, "hello world");
}
END_TEST

START_TEST(test_str_trim_all_spaces) {
        char input[]  = "     ";
        char *trimmed = str_trim(input);
        ck_assert_str_eq(trimmed, "");
}
END_TEST

START_TEST(test_validate_username_valid) { ck_assert(validate_username("user_name123")); }
END_TEST

START_TEST(test_validate_username_invalid) { ck_assert(!validate_username("user name!")); }
END_TEST

START_TEST(test_validate_json_valid_object) {
        ck_assert(validate_json("   { \"key\": \"value\" }"));
}
END_TEST

START_TEST(test_validate_json_valid_array) { ck_assert(validate_json("\n\t [1, 2, 3]")); }
END_TEST

START_TEST(test_validate_json_invalid) { ck_assert(!validate_json("not json")); }
END_TEST

START_TEST(test_sql_escape_simple) {
        const char *src = "O'Hara";
        char dest[32];
        bool result = sql_escape(dest, src, sizeof(dest));
        ck_assert(result);
        ck_assert_str_eq(dest, "O''Hara");
}
END_TEST

START_TEST(test_sql_escape_insufficient_buffer) {
        const char *src = "O'Hara";
        char dest[5];// Too small
        bool result = sql_escape(dest, src, sizeof(dest));
        ck_assert(!result);
}
END_TEST

START_TEST(test_validate_token_valid) { ck_assert(validate_token("abc-123_XYZ.ok", 32)); }
END_TEST

START_TEST(test_validate_token_invalid_char) { ck_assert(!validate_token("abc$123", 32)); }
END_TEST

START_TEST(test_validate_token_too_long) {
        char token[65];
        memset(token, 'a', 64);
        token[64] = '\0';
        ck_assert(!validate_token(token, 32));
}
END_TEST

Suite *sanitizer_suite(void) {
        Suite *s;
        TCase *tc_core;

        s = suite_create("Sanitizer");

        tc_core = tcase_create("Core");

        tcase_add_test(tc_core, test_str_trim_basic);
        tcase_add_test(tc_core, test_str_trim_all_spaces);
        tcase_add_test(tc_core, test_validate_username_valid);
        tcase_add_test(tc_core, test_validate_username_invalid);
        tcase_add_test(tc_core, test_validate_json_valid_object);
        tcase_add_test(tc_core, test_validate_json_valid_array);
        tcase_add_test(tc_core, test_validate_json_invalid);
        tcase_add_test(tc_core, test_sql_escape_simple);
        tcase_add_test(tc_core, test_sql_escape_insufficient_buffer);
        tcase_add_test(tc_core, test_validate_token_valid);
        tcase_add_test(tc_core, test_validate_token_invalid_char);
        tcase_add_test(tc_core, test_validate_token_too_long);

        suite_add_tcase(s, tc_core);
        return s;
}

int main(void) {
        int number_failed;
        Suite *s;
        SRunner *sr;

        s  = sanitizer_suite();
        sr = srunner_create(s);

        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
