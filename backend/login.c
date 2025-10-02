/**
 * @file login.c
 * @brief CGI endpoint for user login with full debug output.
 *
 * Handles user login requests by validating the request method, parsing
 * and validating JSON input, verifying CSRF tokens, checking username
 * and password validity, verifying the password against stored hash,
 * issuing a JWT token, and returning appropriate HTTP responses with
 * debug messages in the response array.
 */

#include <ctype.h>
#include <json-c/json.h>
#include <sanitizec.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/csrf/csrf.h"
#include "lib/dal/user/user.h"
#include "lib/hash_password/hash_password.h"
#include "lib/jwt/jwt.h"
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
#include "lib/result/result.h"

#define DB_PATH "/data/sfe.db"
#define BUFFER_SIZE 1024

/**
 * @brief Validate a username string.
 *
 * Checks that the username is not empty and does not exceed the maximum
 * allowed length.
 *
 * @param str The input username string.
 * @return NULL if the username is valid, otherwise an error message string.
 */
const char* validate_username(const char* str) {
        if (!str || *str == '\0') {
                return "Username is empty.";
        }

        size_t len = strlen(str);
        if (len > 12) {
                return "Username too long (12 characters max).";
        }

        return NULL;
}

/**
 * @brief Main entry point for the login CGI program.
 *
 * Workflow:
 *   1. Enforce POST method.
 *   2. Parse and validate JSON body (csrf, username, password).
 *   3. Validate CSRF token.
 *   4. Validate username format and password length.
 *   5. Sanitize username and reject if modified.
 *   6. Fetch user from database and verify password.
 *   7. Issue JWT token upon successful authentication.
 *   8. Return appropriate HTTP response codes and messages with debug output.
 *
 * @return Exit code (0 on normal completion).
 */
int main(void) {
        char debug_buf[BUFFER_SIZE];
        const char* method       = getenv("REQUEST_METHOD");
        char* username_sanitized = NULL;
        char* token              = NULL;
        response_t resp;
        char* body               = NULL;
        sqlite3* db              = NULL;
        struct json_object* jobj = NULL;

        response_init(&resp, 200);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] REQUEST_METHOD: %s",
                 method ? method : "(null)");
        response_append(&resp, debug_buf);

        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append(&resp, "Method Not Allowed");
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Method check failed: %s",
                         method ? method : "(null)");
                response_append(&resp, debug_buf);
                response_send(&resp);
                return 0;
        }

        result_t res = read_post_data(&body);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] read_post_data result: %d",
                 res.code);
        response_append(&resp, debug_buf);
        if (res.code != RESULT_SUCCESS) {
                response_init(
                    &resp,
                    res.error.code == ERR_INVALID_CONTENT_LENGTH ? 400 : 500);
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] read_post_data failed: %s",
                         res.error.message ? res.error.message : "(null)");
                response_append(&resp, debug_buf);
                switch (res.error.code) {
                        case ERR_INVALID_CONTENT_LENGTH:
                                response_append(
                                    &resp, "Invalid Content Length for POST");
                                break;
                        case ERR_MEMORY_ALLOC_FAIL:
                        case ERR_READ_FAIL:
                        default:
                                response_append(&resp, "Internal Server Error");
                                break;
                }
                free_result(&res);
                response_send(&resp);
                return 0;
        }

        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] Raw POST body: %s",
                 body ? body : "(null)");
        response_append(&resp, debug_buf);
        jobj = json_tokener_parse(body);
        snprintf(debug_buf, BUFFER_SIZE,
                 "[DEBUG] json_tokener_parse result: %p", (void*)jobj);
        response_append(&resp, debug_buf);
        free(body);
        if (!jobj) {
                response_init(&resp, 400);
                response_append(&resp, "Malformed JSON");
                snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] JSON parsing failed");
                response_append(&resp, debug_buf);
                response_send(&resp);
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL,
                           *j_password = NULL;
        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                response_init(&resp, 400);
                response_append(&resp,
                                "Missing csrf, username, or password field.");
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Missing JSON fields: csrf=%p, username=%p, "
                         "password=%p",
                         (void*)j_csrf, (void*)j_username, (void*)j_password);
                response_append(&resp, debug_buf);
                json_object_put(jobj);
                response_send(&resp);
                return 0;
        }

        const char* csrf_token_raw = json_object_get_string(j_csrf);
        const char* username_raw   = json_object_get_string(j_username);
        const char* password       = json_object_get_string(j_password);
        snprintf(debug_buf, BUFFER_SIZE,
                 "[DEBUG] Extracted: csrf=%s, username=%s, password=%s",
                 csrf_token_raw ? csrf_token_raw : "(null)",
                 username_raw ? username_raw : "(null)",
                 password ? "(hidden)" : "(null)");
        response_append(&resp, debug_buf);

        if (!csrf_token_raw || !username_raw || !password) {
                response_init(&resp, 400);
                response_append(
                    &resp, "Missing or invalid csrf, username, or password.");
                snprintf(
                    debug_buf, BUFFER_SIZE,
                    "[DEBUG] Invalid input: csrf=%s, username=%s, password=%s",
                    csrf_token_raw ? csrf_token_raw : "(null)",
                    username_raw ? username_raw : "(null)",
                    password ? "(hidden)" : "(null)");
                response_append(&resp, debug_buf);
                json_object_put(jobj);
                response_send(&resp);
                return 0;
        }

        result_t csrf_res = csrf_validate_token(csrf_token_raw);
        snprintf(debug_buf, BUFFER_SIZE,
                 "[DEBUG] csrf_validate_token result: %d", csrf_res.code);
        response_append(&resp, debug_buf);
        if (csrf_res.code != RESULT_SUCCESS) {
                response_init(
                    &resp,
                    csrf_res.error.code == ERR_CSRF_SECRET_EMPTY ? 500 : 400);
                snprintf(
                    debug_buf, BUFFER_SIZE,
                    "[DEBUG] CSRF validation failed: %s",
                    csrf_res.error.message ? csrf_res.error.message : "(null)");
                response_append(&resp, debug_buf);
                switch (csrf_res.error.code) {
                        case ERR_NULL_TOKEN:
                                response_append(&resp, "CSRF token is null");
                                break;
                        case ERR_SANITIZATION_FAIL:
                                response_append(
                                    &resp, "CSRF token sanitization failed");
                                break;
                        case ERR_TOKEN_LENGTH_MISMATCH:
                                response_append(&resp,
                                                "CSRF token length mismatch");
                                break;
                        case ERR_HEX_DECODE_FAIL:
                                response_append(
                                    &resp, "CSRF token hex decoding failed");
                                break;
                        case ERR_TOKEN_FUTURE_TIMESTAMP:
                                response_append(
                                    &resp,
                                    "CSRF token timestamp is in the future");
                                break;
                        case ERR_TOKEN_EXPIRED:
                                response_append(&resp,
                                                "CSRF token has expired");
                                break;
                        case ERR_CSRF_SECRET_EMPTY:
                                response_append(&resp, "Internal Server Error");
                                break;
                        case ERR_HMAC_GENERATION_FAIL:
                        case ERR_HMAC_LENGTH_MISMATCH:
                        case ERR_HMAC_MISMATCH:
                                response_append(
                                    &resp, "CSRF token HMAC validation failed");
                                break;
                        default:
                                response_append(&resp, "Internal Server Error");
                                break;
                }
                free_result(&csrf_res);
                json_object_put(jobj);
                response_send(&resp);
                return 0;
        }

        json_object_put(jobj);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] JSON object freed");
        response_append(&resp, debug_buf);

        if (strlen(password) < 6) {
                response_init(&resp, 400);
                response_append(&resp,
                                "Password must be at least 6 characters.");
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Password too short: %zu characters",
                         strlen(password));
                response_append(&resp, debug_buf);
                response_send(&resp);
                return 0;
        }

        const char* validation_err = validate_username(username_raw);
        if (validation_err) {
                response_init(&resp, 400);
                response_append(&resp, validation_err);
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Username validation failed: %s",
                         validation_err);
                response_append(&resp, debug_buf);
                response_send(&resp);
                return 0;
        }

        username_sanitized = sanitizec_apply(
            username_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, NULL);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] Username sanitized: %s",
                 username_sanitized ? username_sanitized : "(null)");
        response_append(&resp, debug_buf);
        if (!username_sanitized) {
                response_init(&resp, 400);
                response_append(&resp, "Username sanitization failed");
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Username sanitization failed");
                response_append(&resp, debug_buf);
                response_send(&resp);
                return 0;
        }

        if (strcmp(username_raw, username_sanitized) != 0) {
                response_init(&resp, 400);
                response_append(&resp, "Username must be alphanumeric.");
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Username modified by sanitization: raw=%s, "
                         "sanitized=%s",
                         username_raw, username_sanitized);
                response_append(&resp, debug_buf);
                free(username_sanitized);
                response_send(&resp);
                return 0;
        }

        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                response_init(&resp, 500);
                response_append(&resp, "Internal Server Error");
                snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] DB open failed: %s",
                         sqlite3_errmsg(db));
                response_append(&resp, debug_buf);
                sqlite3_close(db);
                free(username_sanitized);
                response_send(&resp);
                return 0;
        }

        user_t* user = NULL;
        result_t user_res =
            user_fetch_by_username(db, username_sanitized, &user);
        snprintf(debug_buf, BUFFER_SIZE,
                 "[DEBUG] user_fetch_by_username result: %d, user=%p",
                 user_res.code, (void*)user);
        response_append(&resp, debug_buf);
        sqlite3_close(db);
        if (user_res.code != RESULT_SUCCESS) {
                response_init(
                    &resp, user_res.error.code == ERR_SQL_PREPARE_FAIL ||
                                   user_res.error.code == ERR_SQL_STEP_FAIL ||
                                   user_res.error.code == ERR_SQL_BIND_FAIL
                               ? 500
                               : 401);
                snprintf(
                    debug_buf, BUFFER_SIZE, "[DEBUG] User fetch failed: %s",
                    user_res.error.message ? user_res.error.message : "(null)");
                response_append(&resp, debug_buf);
                switch (user_res.error.code) {
                        case ERR_SQL_PREPARE_FAIL:
                        case ERR_SQL_STEP_FAIL:
                        case ERR_SQL_BIND_FAIL:
                                response_append(&resp, "Internal Server Error");
                                break;
                        case ERR_USER_NOT_FOUND:
                                response_append(&resp,
                                                "Invalid username or password");
                                break;
                        default:
                                response_append(&resp, "Internal Server Error");
                                break;
                }
                free_result(&user_res);
                free(username_sanitized);
                response_send(&resp);
                return 0;
        }

        result_t verify_res = verify_password(password, user->password_hash);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] verify_password result: %d",
                 verify_res.code);
        response_append(&resp, debug_buf);
        if (verify_res.code != RESULT_SUCCESS) {
                response_init(&resp, verify_res.error.code == ERR_NULL_INPUT ||
                                             verify_res.error.code ==
                                                 ERR_INVALID_HASH_FORMAT
                                         ? 400
                                         : 500);
                snprintf(debug_buf, BUFFER_SIZE,
                         "[DEBUG] Password verification failed: %s",
                         verify_res.error.message ? verify_res.error.message
                                                  : "(null)");
                response_append(&resp, debug_buf);
                switch (verify_res.error.code) {
                        case ERR_NULL_INPUT:
                                response_append(&resp,
                                                "Password cannot be empty");
                                break;
                        case ERR_INVALID_HASH_FORMAT:
                                response_append(&resp,
                                                "Invalid password hash format");
                                break;
                        case ERR_HASHING_FAIL:
                        case ERR_MEMORY_ALLOC_FAIL:
                        case ERR_SALT_GENERATION_FAIL:
                        case ERR_INVALID_ITERATION_COUNT:
                        case ERR_HEX_DECODE_FAIL:
                                response_append(&resp, "Internal Server Error");
                                break;
                        default:
                                response_append(&resp,
                                                "Invalid username or password");
                                break;
                }
                free_result(&verify_res);
                free(username_sanitized);
                user_free(user);
                response_send(&resp);
                return 0;
        }

        char uid_str[16];
        snprintf(uid_str, sizeof(uid_str), "%d", user->id);
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] User ID: %s", uid_str);
        response_append(&resp, debug_buf);

        result_t jwt_res = issue_jwt(uid_str, &token);
        snprintf(debug_buf, BUFFER_SIZE,
                 "[DEBUG] issue_jwt result: %d, token=%s", jwt_res.code,
                 token ? token : "(null)");
        response_append(&resp, debug_buf);
        free(username_sanitized);
        user_free(user);
        if (jwt_res.code != RESULT_SUCCESS) {
                response_init(&resp, 500);
                snprintf(
                    debug_buf, BUFFER_SIZE, "[DEBUG] JWT issuance failed: %s",
                    jwt_res.error.message ? jwt_res.error.message : "(null)");
                response_append(&resp, debug_buf);
                switch (jwt_res.error.code) {
                        case ERR_JWT_INVALID_ID:
                                response_append(&resp, "Invalid user ID");
                                break;
                        case ERR_JWT_SECRET_FAIL:
                        case ERR_JWT_JSON_FAIL:
                        case ERR_JWT_GENERATE_FAIL:
                        case ERR_JWT_SECRET_EMPTY:
                                response_append(&resp, "Internal Server Error");
                                break;
                        default:
                                response_append(&resp, "Failed to issue JWT");
                                break;
                }
                free_result(&jwt_res);
                response_send(&resp);
                return 0;
        }

        response_init(&resp, 200);
        response_append(&resp, "Login successful");
        snprintf(debug_buf, BUFFER_SIZE, "[DEBUG] JWT issued: %s", token);
        response_append(&resp, debug_buf);
        free(token);
        response_send(&resp);
        return 0;
}
