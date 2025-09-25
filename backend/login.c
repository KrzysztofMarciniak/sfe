/**
 * @file login.c
 * @brief CGI endpoint for user login.
 */

#include <ctype.h>
#include <json-c/json.h>
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
#include "lib/sanitizer/sanitizer.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 1

int main(void) {
        const char *method = getenv("REQUEST_METHOD");

#if DEBUG
        response_init(400);
        response_append(method ? method : "REQUEST_METHOD is NULL");
#endif

        if (!method || strcmp(method, "POST") != 0) {
                response_init(405);
                response_append("Method Not Allowed");
                response_send();
                return 0;
        }

        char *body = read_post_data();
        if (!body) {
                response_init(400);
                response_append("Invalid or missing JSON body");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("POST data read.");
#endif

        struct json_object *jobj = json_tokener_parse(body);
        free(body);

        if (!jobj) {
                response_init(400);
                response_append("Malformed JSON");
                response_send();
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL, *j_password = NULL;

        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response_init(400);
                response_append("Missing csrf, username, or password field.");
                response_send();
                return 0;
        }

        const char *csrf_token   = json_object_get_string(j_csrf);
        const char *username_raw = json_object_get_string(j_username);
        const char *password     = json_object_get_string(j_password);

        if (!csrf_token || !username_raw || !password) {
                json_object_put(jobj);
                response_init(400);
                response_append("All fields must be non-empty strings.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Extracted CSRF, username, and password.");
#endif

        char csrf_token_sanitized[256];
        if (!sanitize(csrf_token_sanitized, csrf_token, sizeof(csrf_token_sanitized))) {
                json_object_put(jobj);
                response_init(400);
                response_append("Failed to sanitize CSRF token.");
                response_send();
                return 0;
        }

        if (!csrf_validate_token(csrf_token_sanitized)) {
                json_object_put(jobj);
                response_init(400);
                response_append("Invalid CSRF token.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("CSRF token validated.");
#endif

        char username_sanitized[64];
        if (!sanitize(username_sanitized, username_raw, sizeof(username_sanitized))) {
                json_object_put(jobj);
                response_init(400);
                response_append("Failed to sanitize username.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Username sanitized.");
#endif

        json_object_put(jobj);// cleanup parsed JSON

        // Open database
        sqlite3 *db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                response_init(500);
#if DEBUG
                response_append(sqlite3_errmsg(db));
#else
                response_append("Failed to open database.");
#endif
                sqlite3_close(db);
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Database opened.");
#endif

        // Fetch user
        user_t *user    = NULL;
        const char *err = user_fetch_by_username(db, username_sanitized, &user);
        sqlite3_close(db);

        if (err || !user) {
                response_init(401);
#if DEBUG
                response_append(err ? err : "User not found.");
#else
                response_append("Invalid username or password.");
#endif
                response_send();
                return 0;
        }

#if DEBUG
        response_append("User fetched from database.");
#endif

        const char *pw_err = NULL;
        int match          = verify_password(password, user->password_hash, &pw_err);

        if (match != 1) {
                user_free(user);
                response_init(401);
#if DEBUG
                response_append(pw_err ? pw_err : "Invalid credentials.");
#else
                response_append("Invalid username or password.");
#endif
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Password verified.");
#endif

        // Issue JWT
        char user_id_str[16];
        snprintf(user_id_str, sizeof(user_id_str), "%d", user->id);
        char *token = issue_jwt(user_id_str);
        user_free(user);

        if (!token) {
                response_init(500);
                response_append("Failed to issue JWT.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("JWT issued.");
#endif

        // Build JSON response
        struct json_object *res = json_object_new_object();
        json_object_object_add(res, "token", json_object_new_string(token));
        free(token);

        printf("Status: 200\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("%s\n", json_object_to_json_string(res));
        json_object_put(res);

        return 0;
}
