/**
 * @file register.c
 * @brief CGI endpoint for user registration (signup).
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

#define DB_PATH "/data/sfe.db"
#define DEBUG 1

/**
 * @brief Validates username length and character set.
 *
 * @param str The raw username string.
 * @return NULL on success, or an error message string on failure.
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

int main(void) {
        const char* method         = getenv("REQUEST_METHOD");
        char* csrf_token_sanitized = NULL;
        char* username_sanitized   = NULL;
        char* password_hash        = NULL;
        char* error_message = NULL;// Used by sanitizec_apply for debug info
        response_t resp;           // Correctly declare the response struct

        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append(&resp, "Method Not Allowed");
                response_send(&resp);
                return 0;
        }

        char* body = read_post_data();
        if (!body) {
                response_init(&resp, 400);
                response_append(&resp, "Invalid or missing JSON body");
                response_send(&resp);
                return 0;
        }

        struct json_object* jobj = json_tokener_parse(body);
        free(body);// Cleanup POST body

        if (!jobj) {
                response_init(&resp, 400);
                response_append(&resp, "Malformed JSON");
                response_send(&resp);
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL,
                           *j_password = NULL;

        // 1. Extract JSON fields
        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response_init(&resp, 400);
                response_append(&resp,
                                "Missing csrf, username, or password field.");
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
                response_send(&resp);
                return 0;
        }

        // 2. CSRF Sanitization and Validation
        csrf_token_sanitized = sanitizec_apply(
            csrf_token_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, &error_message);

        if (!csrf_token_sanitized) {
                json_object_put(jobj);
                response_init(&resp, 400);
#if DEBUG
                response_append(&resp, error_message
                                           ? error_message
                                           : "Unknown sanitization error.");
#else
                response_append(&resp, "Failed to sanitize CSRF token.");
#endif
                response_send(&resp);
                return 0;
        }

        const char* csrf_err = NULL;
        if (!csrf_validate_token(csrf_token_sanitized, &csrf_err)) {
                json_object_put(jobj);
                response_init(&resp, 400);
#if DEBUG
                response_append(&resp,
                                csrf_err ? csrf_err : "Invalid CSRF token.");
#else
                response_append(&resp, "Invalid CSRF token.");
#endif
                free(csrf_token_sanitized);// Cleanup
                response_send(&resp);
                return 0;
        }

        // CSRF is valid. Cleanup the input JSON and the sanitized token.
        json_object_put(jobj);
        free(csrf_token_sanitized);
        csrf_token_sanitized = NULL;

        // 3. Username and Password Validation (Business Logic)
        if (strlen(password) < 6) {
                response_init(&resp, 400);
                response_append(&resp,
                                "Password must be at least 6 characters.");
                response_send(&resp);
                return 0;
        }

        const char* validation_err = validate_username(username_raw);
        if (validation_err) {
                response_init(&resp, 400);
                response_append(&resp, validation_err);
                response_send(&resp);
                return 0;
        }

        // 4. Final Username Sanitization
        username_sanitized = sanitizec_apply(
            username_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, &error_message);

        if (!username_sanitized) {
                response_init(&resp, 400);
#if DEBUG
                response_append(&resp, error_message
                                           ? error_message
                                           : "Unknown sanitization error.");
#else
                response_append(&resp, "Failed to sanitize username.");
#endif
                response_send(&resp);
                return 0;
        }

        // 5. Password Hashing
        const char* hash_err = NULL;
        password_hash        = hash_password(password, &hash_err);

        if (!password_hash) {
                response_init(&resp, 500);
#if DEBUG
                response_append(
                    &resp, hash_err ? hash_err : "Password hashing failed.");
#else
                response_append(&resp, "Password hashing failed.");
#endif
                free(username_sanitized);// Cleanup
                response_send(&resp);
                return 0;
        }

        // 6. Database Operations
        user_t user = {
            .id            = -1,
            .username      = username_sanitized,
            .password_hash = password_hash,
        };

        sqlite3* db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                const char* db_err = sqlite3_errmsg(db);
                sqlite3_close(db);
                response_init(&resp, 500);
#if DEBUG
                response_append(&resp, db_err);
#else
                response_append(&resp, "Failed to open database.");
#endif
                free(password_hash);     // Cleanup
                free(username_sanitized);// Cleanup
                response_send(&resp);
                return 0;
        }

        user_t* inserted_user       = NULL;
        const char* user_insert_err = NULL;// Corrected type and initialization
        const char* err =
            user_insert(db, &user, &inserted_user, &user_insert_err);

        sqlite3_close(db);

        free(password_hash);
        free(username_sanitized);

        if (err != NULL) {
                response_t resp;
                response_init(&resp, 400);

                const char* response_msg = "User registration failed.";

#if DEBUG
                if (user_insert_err != NULL) {
                        response_msg = user_insert_err;
                }
#endif

                response_append(&resp, response_msg);

                if (inserted_user) {
                        user_free(inserted_user);
                }

                // Free the allocated error message string
                if (user_insert_err) {
                        free((void*)user_insert_err);
                }

                response_send(&resp);
                return 0;
        }
        if (err != NULL) {
                response_init(&resp, 400);
                const char* response_msg = err;

                if (strstr(err, "UNIQUE constraint failed")) {
                        response_msg = "Username already exists.";
                } else if (!DEBUG) {
                        response_msg = "User registration failed.";
                }

                response_append(&resp, response_msg);

                if (inserted_user) {
                        user_free(inserted_user);
                }

                response_send(&resp);
                return 0;
        }

        // 7. Success
        if (inserted_user) {
                user_free(inserted_user);
        }

        response_init(&resp, 201);
        response_append(&resp, "User registered successfully.");
        response_send(&resp);
        return 0;
}
