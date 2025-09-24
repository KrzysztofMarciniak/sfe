/**
 * @file register.c
 * @brief CGI endpoint for registering a new user.
 *
 * Accepts POST requests with JSON payload:
 * {
 *   "username": "alice",
 *   "password": "hunter2"
 * }
 *
 * Validates inputs, hashes the password, inserts user into DB,
 * and returns a JSON success or error response.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <sqlite3.h>

#include "lib/die/die.h"
#include "lib/json_response/json_response.h"
#include "lib/sanitizer/sanitizer.h"
#include "lib/hash_password/hash_password.h"
#include "lib/models/user_model/user_model.h"
#include "lib/dal/user/user.h"

#define DB_PATH "/data/sfe.db"

// Read entire stdin body (POST data)
static char* read_post_data(void) {
    const char *len_str = getenv("CONTENT_LENGTH");
    if (!len_str) return NULL;

    long len = strtol(len_str, NULL, 10);
    if (len <= 0 || len > 65536) return NULL;

    char *body = malloc(len + 1);
    if (!body) return NULL;

    fread(body, 1, len, stdin);
    body[len] = '\0';
    return body;
}

int main(void) {
    printf("Content-Type: application/json\r\n\r\n");

    // Enforce POST method
    const char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "POST") != 0) {
        JsonResponse *res = return_json(405);
        res->set_message(res, "Method Not Allowed");
        printf("%s\n", res->build(res));
        res->free(res);
        return 0;
    }

    // Read POST data
    char *body = read_post_data();
    if (!body || !validate_json(body)) {
        JsonResponse *res = return_json(400);
        res->set_message(res, "Invalid or missing JSON body.");
        printf("%s\n", res->build(res));
        res->free(res);
        free(body);
        return 0;
    }

    // Parse JSON
    struct json_object *jobj = json_tokener_parse(body);
    struct json_object *j_username = NULL, *j_password = NULL;

    free(body); // no longer needed

    if (!jobj ||
        !json_object_object_get_ex(jobj, "username", &j_username) ||
        !json_object_object_get_ex(jobj, "password", &j_password)) {
        JsonResponse *res = return_json(400);
        res->set_message(res, "Missing username or password.");
        printf("%s\n", res->build(res));
        res->free(res);
        json_object_put(jobj);
        return 0;
    }

    const char *username = json_object_get_string(j_username);
    const char *password = json_object_get_string(j_password);

    // Validate inputs
    if (!username || !validate_username(username) ||
        !password || strlen(password) < 6) {
        JsonResponse *res = return_json(400);
        res->set_message(res, "Invalid username or password.");
        printf("%s\n", res->build(res));
        res->free(res);
        json_object_put(jobj);
        return 0;
    }

    json_object_put(jobj); // free JSON

    // Hash password
    char *password_hash = hash_password(password);
    if (!password_hash) {
        JsonResponse *res = return_json(500);
        res->set_message(res, "Password hashing failed.");
        printf("%s\n", res->build(res));
        res->free(res);
        return 0;
    }

    // Build user
    user_t user = {
        .id = -1,
        .username = (char *)username, // not freed here
        .password_hash = password_hash
    };

    // Open DB
    sqlite3 *db = NULL;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
        JsonResponse *res = return_json(500);
        res->set_message(res, "Failed to open database.");
        printf("%s\n", res->build(res));
        res->free(res);
        sqlite3_close(db);
        free(password_hash);
        return 0;
    }

    // Insert user
    int rc = user_insert(db, &user);
    sqlite3_close(db);
    free(password_hash);

    if (rc != 0) {
        JsonResponse *res = return_json(400);
        if (rc == -3) {
            res->set_message(res, "Username already exists.");
        } else {
            res->set_message(res, "Failed to register user.");
        }
        printf("%s\n", res->build(res));
        res->free(res);
        return 0;
    }

    // Success
    JsonResponse *res = return_json(201);
    res->set_message(res, "User registered successfully.");
    printf("%s\n", res->build(res));
    res->free(res);

    return 0;
}
