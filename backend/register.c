/**
 * @file register.c
 * @brief CGI endpoint for registering a new user.
 *
 * Accepts POST requests with JSON payload:
 * {
 *   "csrf":"...",
 *   "username": "alice",
 *   "password": "hunter2"
 * }
 *
 * Validates inputs, hashes the password, inserts user into DB,
 * and returns a JSON success or error response.
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
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
#include "lib/sanitizer/sanitizer.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 0

/**
 * @brief Validate username with max length 12.
 *
 * Returns NULL if valid, otherwise returns a static error message.
 *
 * @param str Username string to validate.
 * @return NULL if valid, error message otherwise.
 */
const char *validate_username(const char *str) {
        if (!str || *str == '\0') {
                return "username is empty";
        }

        size_t len = strlen(str);
        if (len > 12) {
                return "username too long (12 characters max)";
        }

        for (size_t i = 0; i < len; i++) {
                if (!(isalnum((unsigned char)str[i]) || str[i] == '_')) {
                        return "username is invalid (only alphanumeric or underscore allowed)";
                }
        }

        return NULL;// valid
}

int main(void) {
        // Enforce POST method
        const char *method = getenv("REQUEST_METHOD");
        if (!method || strcmp(method, "POST") != 0) {
                response(405, "Method Not Allowed");
                return 0;
        }

        // Read POST data
        char *body = read_post_data();
        if (!body) {
                response(400, "Invalid or missing JSON body");
                return 0;
        }

        // Parse JSON
        struct json_object *jobj = json_tokener_parse(body);
        free(body);// free raw POST data immediately

        if (!jobj) {
                response(400, "Malformed JSON");
                return 0;
        }

        struct json_object *j_csrf     = NULL;
        struct json_object *j_username = NULL;
        struct json_object *j_password = NULL;

        // Proper json_object_object_get_ex calls to extract all fields
        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                json_object_put(jobj);
                response(400, "Missing csrf, username, or password field.");
                return 0;
        }

        // Extract strings from JSON objects
        const char *csrf_token   = json_object_get_string(j_csrf);
        const char *username_raw = json_object_get_string(j_username);
        const char *password     = json_object_get_string(j_password);

        if (!csrf_token) {
                json_object_put(jobj);
                response(400, "CSRF token not present or invalid.");
                return 0;
        }

        // Sanitize CSRF token
        char csrf_token_sanitized[256];
        if (!sanitize(csrf_token_sanitized, csrf_token, sizeof(csrf_token_sanitized))) {
                json_object_put(jobj);
                response(400, "Failed to sanitize CSRF token.");
                return 0;
        }

        // Validate CSRF token
        bool valid = csrf_validate_token(csrf_token_sanitized);
        json_object_put(jobj);// free JSON object as we no longer need it

        if (!valid) {
                response(400, "Invalid CSRF token.");
                return 0;
        }

        if (!username_raw) {
                response(400, "Username must be a string.");
                return 0;
        }

        if (!password || strlen(password) < 6) {
                response(400, "Password must be at least 6 characters.");
                return 0;
        }

        // Validate username format
        const char *validation_err = validate_username(username_raw);
        if (validation_err) {
                response(400, validation_err);
                return 0;
        }

        // Sanitize username
        char username_sanitized[64];// safely large buffer
        if (!sanitize(username_sanitized, username_raw, sizeof(username_sanitized))) {
                response(400, "Failed to sanitize username.");
                return 0;
        }

        // Hash password
        char *password_hash = hash_password(password);
        if (!password_hash) {
                response(500, "Password hashing failed.");
                return 0;
        }

        // Build user object
        user_t user = {.id = -1, .username = username_sanitized, .password_hash = password_hash};

        // Open DB
        sqlite3 *db = NULL;
        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
#if DEBUG
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Failed to open database: %s",
                         sqlite3_errmsg(db));
                response(500, err_msg);
#else
                response(500, "Failed to open database.");
#endif
                sqlite3_close(db);
                free(password_hash);
                return 0;
        }

        // Insert user
        char errbuf[256] = {0};
        int rc           = user_insert(db, &user, errbuf, sizeof(errbuf));

        sqlite3_close(db);
        free(password_hash);

        if (rc != 0) {
                if (rc == -3 && strstr(errbuf, "UNIQUE constraint failed")) {
                        response(400, "Username already exists.");
                } else {
#if DEBUG
                        response(400, errbuf);
#else
                        response(400, "Failed to register user.");
#endif
                }
                return 0;
        }

        // Success
        response(201, "User registered successfully.");

        return 0;
}
