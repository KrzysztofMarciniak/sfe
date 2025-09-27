/**
 * @file login.c
 * @brief CGI endpoint for user login.
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
#include "lib/jwt/jwt.h"
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 1

int main(void) {
        const char* method         = getenv("REQUEST_METHOD");
        char* error_message        = NULL;
        char* csrf_token_sanitized = NULL;
        char* username_sanitized   = NULL;
        sqlite3* db                = NULL;
        struct json_object* jobj   = NULL;
        char* body                 = NULL;

#if DEBUG
        response_t debug_resp;
        response_init(&debug_resp, 200);
        response_append(&debug_resp,
                        "[DEBUG] Handling request with method: POST");
#endif

        if (!method || strcmp(method, "POST") != 0) {
                response_t resp;
                response_init(&resp, 405);
                response_append(&resp, "Method Not Allowed");
                response_send(&resp);
                return 0;
        }

        body = read_post_data();
        if (!body) {
                response_t resp;
                response_init(&resp, 400);
                response_append(&resp, "Invalid or missing JSON body");
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(&debug_resp, "[DEBUG] POST data read successfully.");
#endif

        jobj = json_tokener_parse(body);
        free(body);// Free the raw POST body immediately after parsing attempt

        if (!jobj) {
                response_t resp;
                response_init(&resp, 400);
                response_append(&resp, "Malformed JSON");
                response_send(&resp);
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL,
                           *j_password = NULL;

        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response_t resp;
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
                response_t resp;
                response_init(&resp, 400);
                response_append(&resp, "All fields must be non-empty strings.");
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(
            &debug_resp,
            "[DEBUG] Extracted fields: csrf, username, and password.");
#endif

        // --- SANITIZATION START ---
        csrf_token_sanitized = sanitizec_apply(
            csrf_token_raw, SANITIZEC_RULE_HEX_ONLY, &error_message);
        if (!csrf_token_sanitized) {
                json_object_put(jobj);
                response_t resp;
                response_init(&resp, 400);
#if DEBUG
                response_append(
                    &resp, error_message
                               ? error_message
                               : "Failed to sanitize CSRF (unknown error).");
#else
                response_append(&resp, "Failed to sanitize CSRF.");
#endif
                response_send(&resp);
                return 0;
        }

        username_sanitized = sanitizec_apply(
            username_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, &error_message);
        if (!username_sanitized) {
                json_object_put(jobj);
                free(csrf_token_sanitized);// Cleanup token
                response_t resp;
                response_init(&resp, 400);
#if DEBUG
                response_append(
                    &resp,
                    error_message
                        ? error_message
                        : "Failed to sanitize username (unknown error).");
#else
                response_append(&resp, "Failed to sanitize username.");
#endif
                response_send(&resp);
                return 0;
        }
        // --- SANITIZATION END ---

        // CSRF Validation
        const char* csrf_err = NULL;
        if (!csrf_validate_token(csrf_token_sanitized, &csrf_err)) {
                json_object_put(jobj);
                free(csrf_token_sanitized);// Cleanup token
                free(username_sanitized);  // Cleanup username
                response_t resp;
                response_init(&resp, 400);
#if DEBUG
                response_append(
                    &resp, csrf_err ? csrf_err
                                    : "Invalid CSRF token (unknown reason).");
#else
                response_append(&resp, "Invalid CSRF token.");
#endif
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(&debug_resp,
                        "[DEBUG] CSRF token validated successfully.");
#endif

        // Cleanup parsed JSON object now that all values are extracted
        json_object_put(jobj);
        // Cleanup the *raw* sanitized token as validation is complete
        free(csrf_token_sanitized);
        csrf_token_sanitized = NULL;

        // Open database
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                const char* db_err = sqlite3_errmsg(db);
                sqlite3_close(db);
                free(username_sanitized);// Cleanup username
                response_t resp;
                response_init(&resp, 500);
#if DEBUG
                response_append(
                    &resp, db_err ? db_err
                                  : "Failed to open database (unknown error).");
#else
                response_append(&resp, "Failed to open database.");
#endif
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(&debug_resp, "[DEBUG] Database opened. Fetching user.");
#endif

        // Fetch user
        user_t* user     = NULL;
        int user_fetched = user_fetch_by_username(db, username_sanitized, &user,
                                                  &error_message);
        sqlite3_close(db);

        // Cleanup sanitized username after use
        free(username_sanitized);
        username_sanitized = NULL;

        if (error_message || !user || !user_fetched) {
                response_t resp;
                response_init(&resp, 401);
#if DEBUG
                response_append(
                    &resp, error_message ? error_message : "User not found.");
#else
                response_append(&resp, "Invalid username or password.");
#endif
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(
            &debug_resp,
            "[DEBUG] User fetched successfully. Verifying password.");
#endif

        const char* pw_err = NULL;
        int match = verify_password(password, user->password_hash, &pw_err);

        if (match != 1) {
                user_free(user);
                response_t resp;
                response_init(&resp, 401);
#if DEBUG
                response_append(&resp,
                                pw_err ? pw_err : "Invalid credentials.");
#else
                response_append(&resp, "Invalid username or password.");
#endif
                response_send(&resp);
                return 0;
        }

#if DEBUG
        response_append(&debug_resp, "[DEBUG] Password verified. Issuing JWT.");
#endif

        // Issue JWT
        char user_id_str[16];
        snprintf(user_id_str, sizeof(user_id_str), "%d", user->id);
        const char* jwt_err = NULL;// Variable to capture the JWT error message
        char* token         = issue_jwt(user_id_str, &jwt_err);
        user_free(user);

        if (!token) {
                response_t resp;
                response_init(&resp, 500);
#if DEBUG
                // Use the specific error message for debugging
                response_append(
                    &resp,
                    jwt_err ? jwt_err : "Failed to issue JWT (unknown error).");
#else
                // Print a generic error message for production
                response_append(&resp, "Couldn't generate JWT.");
#endif
                response_send(&resp);
                return 0;
        }
        // Build JSON response
        struct json_object* res = json_object_new_object();
        json_object_object_add(res, "token", json_object_new_string(token));
        free(token);// Free the allocated token string

        // Send success response
        printf("Status: 200\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", json_object_to_json_string(res));
        json_object_put(res);

        return 0;
}
