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
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
#include "lib/sanitizer/sanitizer.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 1

const char* validate_username(const char* str) {
        if (!str || *str == '\0') {
                return "username is empty";
        }

        size_t len = strlen(str);
        if (len > 12) {
                return "username too long (12 characters max)";
        }

        for (size_t i = 0; i < len; i++) {
                if (!(isalnum((unsigned char)str[i]) || str[i] == '_')) {
                        return "username is invalid (only alphanumeric or "
                               "underscore allowed)";
                }
        }

        return NULL;
}

int main(void) {
        const char* method = getenv("REQUEST_METHOD");

#if DEBUG
        response_init(400);
        response_append(method ? method : "REQUEST_METHOD is NULL");
#else
        if (!method || strcmp(method, "POST") != 0) {
                response_init(405);
                response_append("Method Not Allowed");
                response_send();
                return 0;
        }
#endif

        if (!method || strcmp(method, "POST") != 0) {
                response_init(405);
                response_append("Method Not Allowed");
                response_send();
                return 0;
        }

        char* body = read_post_data();
        if (!body) {
                response_init(400);
                response_append("Invalid or missing JSON body");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("POST data read successfully.");
#endif

        struct json_object* jobj = json_tokener_parse(body);
        free(body);

        if (!jobj) {
                response_init(400);
                response_append("Malformed JSON");
                response_send();
                return 0;
        }

        struct json_object* j_csrf     = NULL;
        struct json_object* j_username = NULL;
        struct json_object* j_password = NULL;

        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response_init(400);
                response_append("Missing csrf, username, or password field.");
                response_send();
                return 0;
        }

        const char* csrf_token   = json_object_get_string(j_csrf);
        const char* username_raw = json_object_get_string(j_username);
        const char* password     = json_object_get_string(j_password);

        if (!csrf_token || !username_raw || !password) {
                json_object_put(jobj);
                response_init(400);
                response_append(
                    "Missing or invalid csrf, username, or password.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Extracted CSRF, username, and password.");
#endif

        char csrf_token_sanitized[256];
        if (!sanitize(csrf_token_sanitized, csrf_token,
                      sizeof(csrf_token_sanitized))) {
                json_object_put(jobj);
                response_init(400);
                response_append("Failed to sanitize CSRF token.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("CSRF token sanitized.");
#endif

        bool valid = csrf_validate_token(csrf_token_sanitized);
        json_object_put(jobj);// done with jobj

        if (!valid) {
                response_init(400);
                response_append("Invalid CSRF token.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("CSRF token validated.");
#endif

        if (strlen(password) < 6) {
                response_init(400);
                response_append("Password must be at least 6 characters.");
                response_send();
                return 0;
        }

        const char* validation_err = validate_username(username_raw);
        if (validation_err) {
                response_init(400);
                response_append(validation_err);
                response_send();
                return 0;
        }

        char username_sanitized[64];
        if (!sanitize(username_sanitized, username_raw,
                      sizeof(username_sanitized))) {
                response_init(400);
                response_append("Failed to sanitize username.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Username validated and sanitized.");
#endif

        const char* hash_err = NULL;
        char* password_hash  = hash_password(password, &hash_err);

        if (!password_hash) {
                response_init(500);
                response_append(hash_err ? hash_err
                                         : "Password hashing failed.");
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Password hashed.");
#endif

        user_t user = {
            .id            = -1,
            .username      = username_sanitized,
            .password_hash = password_hash,
        };

        sqlite3* db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                response_init(500);
                response_append(sqlite3_errmsg(db));
                sqlite3_close(db);
                free(password_hash);
                response_send();
                return 0;
        }

#if DEBUG
        response_append("Database opened.");
#endif

        user_t* inserted_user = NULL;
        const char* err       = user_insert(db, &user, &inserted_user);

        sqlite3_close(db);
        free(password_hash);

        if (err != NULL) {
                response_init(400);
                if (strstr(err, "UNIQUE constraint failed")) {
                        response_append("Username already exists.");
                } else {
                        response_append(err);
                }

                if (inserted_user) {
                        user_free(inserted_user);
                }

                response_send();
                return 0;
        }

        if (inserted_user) {
                user_free(inserted_user);
        }

        response_init(201);
        response_append("User registered successfully.");
        response_send();
        return 0;
}
