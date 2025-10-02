/**
 * @file register.c
 * @brief CGI endpoint for user registration (signup).
 *
 * Handles user registration requests by validating the request method, parsing
 * and validating JSON input, verifying CSRF tokens, checking username and
 * password validity, hashing the password, and inserting a new user record into
 * the database. Returns appropriate HTTP responses with error details.
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
#include "lib/die/die.h"
#include "lib/hash_password/hash_password.h"
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
#include "lib/result/result.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 0

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
 * @brief Main entry point for the registration CGI program.
 *
 * Workflow:
 *   1. Enforce POST method.
 *   2. Parse and validate JSON body (csrf, username, password).
 *   3. Validate CSRF token.
 *   4. Validate password length and username format.
 *   5. Sanitize username and reject if modified.
 *   6. Hash the password securely.
 *   7. Insert the new user record into the database.
 *   8. Return appropriate HTTP response codes and messages.
 *
 * @return Exit code (0 on normal completion).
 */
int main(void) {
        const char* method       = getenv("REQUEST_METHOD");
        char* username_sanitized = NULL;
        char* password_hash      = NULL;
        response_t resp;
        char* body = NULL;

        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append(&resp, "Method Not Allowed");
#if DEBUG
                response_append(&resp, "\n[DEBUG] Method: ");
                response_append(&resp, method ? method : "(null)");
#endif
                response_send(&resp);
                return 0;
        }

        result_t res = read_post_data(&body);
        if (res.code != RESULT_SUCCESS) {
                response_init(
                    &resp,
                    res.error.code == ERR_INVALID_CONTENT_LENGTH ? 400 : 500);
#if DEBUG
                struct json_object* res_json = result_to_json(&res);
                char* json_str = res_json ? json_object_to_json_string(res_json)
                                          : "JSON conversion failed";
                response_append(&resp, json_str);
                if (res_json) json_object_put(res_json);
#else
                switch (res.error.code) {
                        case ERR_INVALID_CONTENT_LENGTH:
                                response_append(
                                    &resp, "Invalid Content Length for POST");
                                break;
                        case ERR_MEMORY_ALLOC_FAIL:
                        case ERR_READ_FAIL:
                                response_append(&resp, "Internal Server Error");
                                break;
                        default:
                                response_append(&resp, "Internal Server Error");
                                break;
                }
#endif
                free_result(&res);
                response_send(&resp);
                return 0;
        }

        struct json_object* jobj = json_tokener_parse(body);
#if DEBUG
        response_append(&resp, "\n[DEBUG] Raw JSON body: ");
        response_append(&resp, body);
#endif
        free(body);

        if (!jobj) {
                response_init(&resp, 400);
                response_append(&resp, "Malformed JSON");
#if DEBUG
                response_append(&resp, "\n[DEBUG] json_tokener_parse failed");
#endif
                response_send(&resp);
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL,
                           *j_password = NULL;

        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response_init(&resp, 400);
                response_append(&resp,
                                "Missing csrf, username, or password field.");
#if DEBUG
                response_append(&resp, "\n[DEBUG] Extracted fields:\n");
                response_append(&resp, "csrf: ");
                response_append(
                    &resp, j_csrf ? json_object_get_string(j_csrf) : "(null)");
                response_append(&resp, "\nusername: ");
                response_append(&resp, j_username
                                           ? json_object_get_string(j_username)
                                           : "(null)");
                response_append(&resp, "\npassword: ");
                response_append(&resp, j_password
                                           ? json_object_get_string(j_password)
                                           : "(null)");
#endif
                response_send(&resp);
                return 0;
        }

        const char* csrf_token_raw = json_object_get_string(j_csrf);
        const char* username_raw   = json_object_get_string(j_username);
        const char* password       = json_object_get_string(j_password);

        if (!csrf_token_raw || !username_raw || !password) {
                json_object_put(jobj);
                response_init(&resp, 400);
                response_append(
                    &resp, "Missing or invalid csrf, username, or password.");
#if DEBUG
                response_append(&resp, "\n[DEBUG] csrf: ");
                response_append(&resp,
                                csrf_token_raw ? csrf_token_raw : "(null)");
                response_append(&resp, "\nusername: ");
                response_append(&resp, username_raw ? username_raw : "(null)");
                response_append(&resp, "\npassword: ");
                response_append(&resp, password ? password : "(null)");
#endif
                response_send(&resp);
                return 0;
        }

        result_t csrf_res = csrf_validate_token(csrf_token_raw);
        if (csrf_res.code != RESULT_SUCCESS) {
                json_object_put(jobj);
                response_init(
                    &resp,
                    csrf_res.error.code == ERR_CSRF_SECRET_EMPTY ? 500 : 400);
#if DEBUG
                struct json_object* res_json = result_to_json(&csrf_res);
                char* json_str = res_json ? json_object_to_json_string(res_json)
                                          : "JSON conversion failed";
                response_append(&resp, json_str);
                if (res_json) json_object_put(res_json);
#else
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
#endif
                free_result(&csrf_res);
                response_send(&resp);
                return 0;
        }

        json_object_put(jobj);

        if (strlen(password) < 6) {
                response_init(&resp, 400);
                response_append(&resp,
                                "Password must be at least 6 characters.");
#if DEBUG
                response_append(&resp, "\n[DEBUG] Username: ");
                response_append(&resp, username_raw);
                response_append(&resp, "\n[DEBUG] Password: ");
                response_append(&resp, password);
#endif
                response_send(&resp);
                return 0;
        }

        const char* validation_err = validate_username(username_raw);
        if (validation_err) {
                response_init(&resp, 400);
                response_append(&resp, validation_err);
#if DEBUG
                response_append(&resp, "\n[DEBUG] Username: ");
                response_append(&resp, username_raw);
#endif
                response_send(&resp);
                return 0;
        }

        username_sanitized = sanitizec_apply(
            username_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, NULL);
        if (!username_sanitized) {
                response_init(&resp, 400);
                response_append(&resp, "Username sanitization failed");
#if DEBUG
                response_append(&resp, "\n[DEBUG] Raw username: ");
                response_append(&resp, username_raw);
#endif
                response_send(&resp);
                return 0;
        }

        if (strcmp(username_raw, username_sanitized) != 0) {
                response_init(&resp, 400);
                response_append(&resp, "Username must be alphanumeric.");
#if DEBUG
                response_append(&resp, "\n[DEBUG] Raw username: ");
                response_append(&resp, username_raw);
                response_append(&resp, "\n[DEBUG] Sanitized username: ");
                response_append(&resp, username_sanitized);
#endif
                free(username_sanitized);
                response_send(&resp);
                return 0;
        }

        result_t hash_res = hash_password(password, &password_hash);
        if (hash_res.code != RESULT_SUCCESS) {
                response_init(&resp, 500);
#if DEBUG
                struct json_object* res_json = result_to_json(&hash_res);
                char* json_str = res_json ? json_object_to_json_string(res_json)
                                          : "JSON conversion failed";
                response_append(&resp, json_str);
                if (res_json) json_object_put(res_json);
#else
                switch (hash_res.error.code) {
                        case ERR_NULL_INPUT:
                                response_append(&resp,
                                                "Password cannot be empty");
                                break;
                        case ERR_SALT_GENERATION_FAIL:
                        case ERR_HASHING_FAIL:
                        case ERR_MEMORY_ALLOC_FAIL:
                        case ERR_INVALID_HASH_FORMAT:
                        case ERR_INVALID_ITERATION_COUNT:
                        case ERR_HEX_DECODE_FAIL:
                                response_append(&resp, "Internal Server Error");
                                break;
                        default:
                                response_append(&resp, "Internal Server Error");
                                break;
                }
#endif
                free(username_sanitized);
                free_result(&hash_res);
                response_send(&resp);
                return 0;
        }

        user_t user = {
            .id            = -1,
            .username      = username_sanitized,
            .password_hash = password_hash,
        };

        sqlite3* db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                response_init(&resp, 500);
                response_append(&resp, "Internal Server Error");
#if DEBUG
                response_append(&resp, "\n[DEBUG] DB error: ");
                response_append(&resp, sqlite3_errmsg(db));
                response_append(&resp, "\n[DEBUG] DB_PATH: " DB_PATH);
#endif
                sqlite3_close(db);
                free(password_hash);
                free(username_sanitized);
                response_send(&resp);
                return 0;
        }

        user_t* inserted_user = NULL;
        result_t user_res     = user_insert(db, &user, &inserted_user);
        sqlite3_close(db);

        free(password_hash);
        free(username_sanitized);

        if (user_res.code != RESULT_SUCCESS) {
                response_init(
                    &resp, user_res.error.code == ERR_SQL_PREPARE_FAIL ||
                                   user_res.error.code == ERR_SQL_STEP_FAIL ||
                                   user_res.error.code == ERR_SQL_BIND_FAIL
                               ? 500
                               : 400);
#if DEBUG
                struct json_object* res_json = result_to_json(&user_res);
                char* json_str = res_json ? json_object_to_json_string(res_json)
                                          : "JSON conversion failed";
                response_append(&resp, json_str);
                if (res_json) json_object_put(res_json);
#else
                switch (user_res.error.code) {
                        case ERR_SQL_PREPARE_FAIL:
                        case ERR_SQL_STEP_FAIL:
                        case ERR_SQL_BIND_FAIL:
                                response_append(&resp, "Internal Server Error");
                                break;
                        case ERR_USER_NOT_FOUND:
                                response_append(&resp,
                                                "User registration failed");
                                break;
                        default:
                                if (user_res.error.message &&
                                    strstr(user_res.error.message,
                                           "UNIQUE constraint failed")) {
                                        response_append(
                                            &resp, "Username already exists");
                                } else {
                                        response_append(
                                            &resp, "User registration failed");
                                }
                                break;
                }
#endif
                if (inserted_user) user_free(inserted_user);
                free_result(&user_res);
                response_send(&resp);
                return 0;
        }

        if (inserted_user) user_free(inserted_user);

        response_init(&resp, 201);
        response_append(&resp, "User registered successfully.");
#if DEBUG
        response_append(&resp, "\n[DEBUG] Username: ");
        response_append(&resp, user.username);
#endif
        response_send(&resp);
        return 0;
}
