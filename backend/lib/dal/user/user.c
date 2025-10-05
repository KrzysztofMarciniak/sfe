#include "user.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Insert a new user into the database
 * @param db SQLite database connection
 * @param user Pointer to user_t with username and password_hash filled
 * @param out_user Pointer to store inserted user with generated ID (caller must
 * free)
 * @return result_t indicating success or failure
 */
result_t* user_insert(sqlite3* db, const user_t* user, user_t** out_user) {
        if (out_user) {
                *out_user = NULL;
        }

        if (!db || !user || !user->username || !user->password_hash ||
            !out_user) {
                result_t* res = result_failure("Invalid input parameters", NULL,
                                               ERR_INVALID_INPUT);
                result_add_extra(
                    res,
                    "db=%p, user=%p, username=%p, password_hash=%p, "
                    "out_user=%p",
                    (const void*)db, (const void*)user,
                    (const void*)user ? (const void*)user->username : NULL,
                    (const void*)user ? (const void*)user->password_hash : NULL,
                    (const void*)out_user);
                return res;
        }

        const char* sql =
            "INSERT INTO users (username, password_hash) VALUES (?, ?);";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                result_t* res =
                    result_critical_failure("Failed to prepare SQL statement",
                                            NULL, ERR_SQL_PREPARE_FAIL);
                result_add_extra(res, "sqlite_error=%s", sqlite3_errmsg(db));
                return res;
        }

        sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, user->password_hash, -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
                result_t* res = result_failure(
                    "Failed to execute SQL statement", NULL, ERR_SQL_STEP_FAIL);
                result_add_extra(res, "sqlite_error=%s", sqlite3_errmsg(db));
                sqlite3_finalize(stmt);
                return res;
        }

        user_t* new_user = malloc(sizeof(user_t));
        if (!new_user) {
                result_t* res = result_critical_failure(
                    "Failed to allocate memory for user", NULL,
                    ERR_MEMORY_ALLOC_FAIL);
                sqlite3_finalize(stmt);
                return res;
        }

        new_user->id            = sqlite3_last_insert_rowid(db);
        new_user->username      = strdup(user->username);
        new_user->password_hash = strdup(user->password_hash);

        if (!new_user->username || !new_user->password_hash) {
                result_t* res = result_critical_failure(
                    "Failed to allocate memory for user fields", NULL,
                    ERR_MEMORY_ALLOC_FAIL);
                user_free(new_user);
                sqlite3_finalize(stmt);
                return res;
        }

        *out_user = new_user;
        sqlite3_finalize(stmt);
        return result_success();
}

/**
 * @brief Fetch a user from the database by ID
 * @param db SQLite database connection
 * @param id ID to search for
 * @param out_user Pointer to store fetched user (caller must free)
 * @return result_t indicating success or failure
 */
result_t* user_fetch_by_id(sqlite3* db, int id, user_t** out_user) {
        if (out_user) {
                *out_user = NULL;
        }

        if (!db || !out_user) {
                result_t* res = result_failure("Invalid arguments", NULL,
                                               ERR_INVALID_INPUT);
                result_add_extra(res, "db=%p, out_user=%p", (const void*)db,
                                 (const void*)out_user);
                return res;
        }

        const char* sql =
            "SELECT id, username, password_hash FROM users WHERE id = ? LIMIT "
            "1;";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                result_t* res =
                    result_failure("Failed to prepare SQL statement", NULL,
                                   ERR_SQL_PREPARE_FAIL);
                result_add_extra(res, "sqlite_error=%s", sqlite3_errmsg(db));
                return res;
        }

        sqlite3_bind_int(stmt, 1, id);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t* user = malloc(sizeof(user_t));
                if (!user) {
                        result_t* res = result_critical_failure(
                            "Failed to allocate memory for user", NULL,
                            ERR_MEMORY_ALLOC_FAIL);
                        sqlite3_finalize(stmt);
                        return res;
                }

                user->id           = sqlite3_column_int(stmt, 0);
                const char* uname  = (const char*)sqlite3_column_text(stmt, 1);
                const char* pwhash = (const char*)sqlite3_column_text(stmt, 2);

                user->username      = uname ? strdup(uname) : NULL;
                user->password_hash = pwhash ? strdup(pwhash) : NULL;

                if (!user->username || !user->password_hash) {
                        result_t* res = result_critical_failure(
                            "Failed to allocate memory for user fields", NULL,
                            ERR_MEMORY_ALLOC_FAIL);
                        user_free(user);
                        sqlite3_finalize(stmt);
                        return res;
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return result_success();
        }

        result_t* res =
            result_failure("User not found", NULL, ERR_USER_NOT_FOUND);
        result_add_extra(res, "id=%d", id);
        sqlite3_finalize(stmt);
        return res;
}

/**
 * @brief Fetch a user from the database by username
 * @param db SQLite database connection
 * @param username Username to search for
 * @param out_user Pointer to store fetched user (caller must free)
 * @return result_t indicating success or failure
 */
result_t* user_fetch_by_username(sqlite3* db, const char* username,
                                 user_t** out_user) {
        if (out_user) {
                *out_user = NULL;
        }

        if (!db || !username || !out_user) {
                result_t* res = result_failure("Invalid arguments", NULL,
                                               ERR_INVALID_INPUT);
                result_add_extra(res, "db=%p, username=%p, out_user=%p",
                                 (const void*)db, (const void*)username,
                                 (const void*)out_user);
                return res;
        }

        const char* sql =
            "SELECT id, username, password_hash FROM users WHERE username = ? "
            "COLLATE NOCASE LIMIT 1;";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                result_t* res =
                    result_failure("Failed to prepare SQL statement", NULL,
                                   ERR_SQL_PREPARE_FAIL);
                result_add_extra(res, "sqlite_error=%s", sqlite3_errmsg(db));
                return res;
        }

        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
                result_t* res = result_failure("Failed to bind SQL parameters",
                                               NULL, ERR_SQL_BIND_FAIL);
                result_add_extra(res, "sqlite_error=%s", sqlite3_errmsg(db));
                sqlite3_finalize(stmt);
                return res;
        }

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t* user = malloc(sizeof(user_t));
                if (!user) {
                        result_t* res = result_critical_failure(
                            "Failed to allocate memory for user", NULL,
                            ERR_MEMORY_ALLOC_FAIL);
                        sqlite3_finalize(stmt);
                        return res;
                }

                user->username      = NULL;
                user->password_hash = NULL;

                user->id           = sqlite3_column_int(stmt, 0);
                const char* uname  = (const char*)sqlite3_column_text(stmt, 1);
                const char* pwhash = (const char*)sqlite3_column_text(stmt, 2);

                if (uname) {
                        user->username = strdup(uname);
                        if (!user->username) {
                                result_t* res = result_critical_failure(
                                    "Failed to allocate memory for username",
                                    NULL, ERR_MEMORY_ALLOC_FAIL);
                                user_free(user);
                                sqlite3_finalize(stmt);
                                return res;
                        }
                }

                if (pwhash) {
                        user->password_hash = strdup(pwhash);
                        if (!user->password_hash) {
                                result_t* res = result_critical_failure(
                                    "Failed to allocate memory for "
                                    "password_hash",
                                    NULL, ERR_MEMORY_ALLOC_FAIL);
                                user_free(user);
                                sqlite3_finalize(stmt);
                                return res;
                        }
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return result_success();
        }

        result_t* res =
            result_failure("User not found", NULL, ERR_USER_NOT_FOUND);
        result_add_extra(res, "username=%s", username);
        sqlite3_finalize(stmt);
        return res;
}
