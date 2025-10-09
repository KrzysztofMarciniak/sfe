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
#include "lib/hash_password/hash_password.h"
#include "lib/models/user_model/user_model.h"
#include "lib/read_post_data/read_post_data.h"
#include "lib/response/response.h"
#include "lib/result/result.h"

#define DB_PATH "/data/sfe.db"
#define DEBUG 0

static void free_memory(sqlite3* db, char* body, struct json_object* jobj,
                        char* username_sanitized, char* password_hash,
                        user_t* inserted_user, result_t* res,
                        result_t* csrf_res, result_t* hash_res,
                        result_t* user_res) {
        if (db) sqlite3_close(db);
        if (body) free(body);
        if (jobj) json_object_put(jobj);
        if (username_sanitized) free(username_sanitized);
        if (password_hash) free(password_hash);
        if (inserted_user) user_free(inserted_user);
        if (res) result_free(res);
        if (csrf_res) result_free(csrf_res);
        if (hash_res) result_free(hash_res);
        if (user_res) result_free(user_res);
}

const char* validate_username(const char* str) {
        if (!str || *str == '\0') return "Username is empty.";
        size_t len = strlen(str);
        if (len > 12) return "Username too long (12 characters max).";
        return NULL;
}

int main(void) {
        const char* method = getenv("REQUEST_METHOD");

        char *username_sanitized = NULL, *password_hash = NULL, *body = NULL;
        struct json_object* jobj = NULL;
        user_t* inserted_user    = NULL;
        sqlite3* db              = NULL;

        result_t *res = NULL, *csrf_res = NULL, *hash_res = NULL,
                 *user_res = NULL;

        response_t resp;
        response_init(&resp, 200);

        if (!method || strcmp(method, "POST") != 0) {
                response_init(&resp, 405);
                response_append_str(&resp, "Method Not Allowed");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        res = read_post_data(&body);
        if (res->code != RESULT_SUCCESS) {
                response_init(&resp,
                              res->data.error.code == ERR_INVALID_CONTENT_LENGTH
                                  ? 400
                                  : 500);
                switch (res->data.error.code) {
                        case ERR_INVALID_CONTENT_LENGTH:
                                response_append_str(
                                    &resp, "Invalid Content Length for POST");
                                break;
                        default:
                                response_append_str(&resp,
                                                    "Internal Server Error");
                                break;
                }
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        jobj = json_tokener_parse(body);
        if (!jobj) {
                response_init(&resp, 400);
                response_append_str(&resp, "Malformed JSON");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        struct json_object *j_csrf = NULL, *j_username = NULL,
                           *j_password = NULL;
        if (!json_object_object_get_ex(jobj, "csrf", &j_csrf) ||
            !json_object_object_get_ex(jobj, "username", &j_username) ||
            !json_object_object_get_ex(jobj, "password", &j_password)) {
                response_init(&resp, 400);
                response_append_str(
                    &resp, "Missing csrf, username, or password field.");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        const char* csrf_token_raw = json_object_get_string(j_csrf);
        const char* username_raw   = json_object_get_string(j_username);
        const char* password       = json_object_get_string(j_password);

        if (!csrf_token_raw || !username_raw || !password) {
                response_init(&resp, 400);
                response_append_str(
                    &resp, "Missing or invalid csrf, username, or password.");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        csrf_res = csrf_validate_token(csrf_token_raw);
        if (csrf_res->code != RESULT_SUCCESS) {
                response_init(&resp, 400);
                response_append_str(&resp, "Invalid CSRF token");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        if (strlen(password) < 6) {
                response_init(&resp, 400);
                response_append_str(&resp,
                                    "Password must be at least 6 characters.");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        const char* validation_err = validate_username(username_raw);
        if (validation_err) {
                response_init(&resp, 400);
                response_append_str(&resp, validation_err);
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        username_sanitized = sanitizec_apply(
            username_raw, SANITIZEC_RULE_ALPHANUMERIC_ONLY, NULL);
        if (!username_sanitized) {
                response_init(&resp, 400);
                response_append_str(&resp, "Username sanitization failed");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        if (strcmp(username_raw, username_sanitized) != 0) {
                response_init(&resp, 400);
                response_append_str(&resp, "Username must be alphanumeric.");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        hash_res = hash_password(password, &password_hash);
        if (hash_res->code != RESULT_SUCCESS) {
                response_init(&resp, 500);
                response_append_str(&resp, "Internal Server Error");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        user_t user = {
            .id            = -1,
            .username      = username_sanitized,
            .password_hash = password_hash,
        };

        if (sqlite3_open(DB_PATH, &db) != SQLITE_OK) {
                response_init(&resp, 500);
                response_append_str(&resp, "Internal Server Error");
                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        user_res = user_insert(db, &user, &inserted_user);
        if (user_res->code != RESULT_SUCCESS) {
                response_init(
                    &resp, (user_res->data.error.code == ERR_SQL_PREPARE_FAIL ||
                            user_res->data.error.code == ERR_SQL_STEP_FAIL ||
                            user_res->data.error.code == ERR_SQL_BIND_FAIL)
                               ? 500
                               : 400);

                switch (user_res->data.error.code) {
                        case ERR_USER_DUPLICATE:
                                response_append_str(&resp,
                                                    "Username already exists.");
                                break;
                        case ERR_SQL_PREPARE_FAIL:
                        case ERR_SQL_STEP_FAIL:
                        case ERR_SQL_BIND_FAIL:
                                response_append_str(&resp,
                                                    "Internal Server Error");
                                break;
                        case ERR_USER_NOT_FOUND:
                                response_append_str(&resp,
                                                    "User registration failed");
                                break;
                        default:
                                if (user_res->data.error.message &&
                                    strstr(user_res->data.error.message,
                                           "UNIQUE constraint failed")) {
                                        response_append_str(
                                            &resp, "Username already exists.");
                                } else {
                                        response_append_str(
                                            &resp, "User registration failed");
                                }
                                break;
                }

                free_memory(db, body, jobj, username_sanitized, password_hash,
                            inserted_user, res, csrf_res, hash_res, user_res);
                response_send(&resp);
                return 0;
        }

        response_init(&resp, 201);
        response_append_str(&resp, "User registered successfully.");

        free_memory(db, body, jobj, username_sanitized, password_hash,
                    inserted_user, res, csrf_res, hash_res, user_res);
        response_send(&resp);
        return 0;
}
