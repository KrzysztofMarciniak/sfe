#include "user.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro to allocate the error message string on failure. The caller is
// responsible for freeing the memory pointed to by *errmsg.
#define ALLOCATE_ERR_MSG(errmsg_ptr, msg)                                     \
        do {                                                                  \
                if (errmsg_ptr) {                                             \
                        *errmsg_ptr = strdup(msg);                            \
                        if (!(*errmsg_ptr)) {                                 \
                                /* Handle strdup failure if possible, but for \
                                 * simplicity, proceed with what we have */   \
                        }                                                     \
                }                                                             \
        } while (0)

// Helper macro to allocate SQLite error message
#define ALLOCATE_SQLITE_ERR_MSG(errmsg_ptr, db) \
        ALLOCATE_ERR_MSG(errmsg_ptr, sqlite3_errmsg(db))

// ------------------------------------

int user_insert(sqlite3* db, const user_t* user, user_t** out_user,
                char** errmsg) {
        if (errmsg) *errmsg = NULL;

        if (!db || !user || !user->username || !user->password_hash ||
            !out_user) {
                ALLOCATE_ERR_MSG(errmsg, "Invalid input parameters.");
                return USER_ERROR;
        }

        const char* sql =
            "INSERT INTO users (username, password_hash) VALUES (?, ?);";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                ALLOCATE_SQLITE_ERR_MSG(errmsg, db);
                return USER_ERROR;
        }

        // Bind parameters
        sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, user->password_hash, -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
                ALLOCATE_SQLITE_ERR_MSG(errmsg, db);
                sqlite3_finalize(stmt);
                return USER_ERROR;
        }

        sqlite3_finalize(stmt);

        int new_id = (int)sqlite3_last_insert_rowid(db);

        // user_fetch_by_id now returns int status and takes errmsg
        return user_fetch_by_id(db, new_id, out_user, errmsg);
}

int user_fetch_by_id(sqlite3* db, int id, user_t** out_user, char** errmsg) {
        if (errmsg) *errmsg = NULL;// Initialize errmsg to NULL
        if (!db || !out_user) {
                ALLOCATE_ERR_MSG(errmsg, "Invalid arguments.");
                return USER_ERROR;
        }

        *out_user = NULL;

        const char* sql =
            "SELECT id, username, password_hash FROM users WHERE id = ? LIMIT "
            "1;";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                ALLOCATE_SQLITE_ERR_MSG(errmsg, db);
                return USER_ERROR;
        }

        sqlite3_bind_int(stmt, 1, id);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t* user = malloc(sizeof(user_t));
                if (!user) {
                        sqlite3_finalize(stmt);
                        ALLOCATE_ERR_MSG(
                            errmsg,
                            "Out of memory (failed to allocate user_t).");
                        return USER_ERROR;
                }

                user->id           = sqlite3_column_int(stmt, 0);
                const char* uname  = (const char*)sqlite3_column_text(stmt, 1);
                const char* pwhash = (const char*)sqlite3_column_text(stmt, 2);

                user->username      = uname ? strdup(uname) : NULL;
                user->password_hash = pwhash ? strdup(pwhash) : NULL;

                if (!user->username || !user->password_hash) {
                        // Assuming user_free exists to clean up partially
                        // allocated user_t user_free(user);
                        free(user->username);// Cleanup allocated parts
                        free(user->password_hash);
                        free(user);

                        sqlite3_finalize(stmt);
                        ALLOCATE_ERR_MSG(
                            errmsg,
                            "Out of memory (failed to duplicate strings).");
                        return USER_ERROR;
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return USER_SUCCESS;
        }

        sqlite3_finalize(stmt);
        // User not found is still treated as an error state for this function.
        ALLOCATE_ERR_MSG(errmsg, "User not found.");
        return USER_ERROR;
}

int user_fetch_by_username(sqlite3* db, const char* username, user_t** out_user,
                           char** errmsg) {
        if (errmsg) *errmsg = NULL;// Initialize errmsg to NULL
        if (!db || !username || !out_user) {
                ALLOCATE_ERR_MSG(errmsg, "Invalid arguments.");
                return USER_ERROR;
        }

        *out_user = NULL;

        const char* sql =
            "SELECT id, username, password_hash FROM users WHERE username = ? "
            "COLLATE NOCASE LIMIT "
            "1;";
        sqlite3_stmt* stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                ALLOCATE_SQLITE_ERR_MSG(errmsg, db);
                return USER_ERROR;
        }

        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
                ALLOCATE_SQLITE_ERR_MSG(errmsg, db);
                sqlite3_finalize(stmt);
                return USER_ERROR;
        }

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t* user = malloc(sizeof(user_t));
                if (!user) {
                        sqlite3_finalize(stmt);
                        ALLOCATE_ERR_MSG(
                            errmsg,
                            "Out of memory (failed to allocate user_t).");
                        return USER_ERROR;
                }

                user->id           = sqlite3_column_int(stmt, 0);
                const char* uname  = (const char*)sqlite3_column_text(stmt, 1);
                const char* pwhash = (const char*)sqlite3_column_text(stmt, 2);

                user->username      = uname ? strdup(uname) : NULL;
                user->password_hash = pwhash ? strdup(pwhash) : NULL;

                if (!user->username || !user->password_hash) {
                        // Assuming user_free exists, otherwise manual free
                        // user_free(user);
                        free(user->username);
                        free(user->password_hash);
                        free(user);

                        sqlite3_finalize(stmt);
                        ALLOCATE_ERR_MSG(
                            errmsg,
                            "Out of memory (failed to duplicate strings).");
                        return USER_ERROR;
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return USER_SUCCESS;
        }

        sqlite3_finalize(stmt);
        // User not found
        ALLOCATE_ERR_MSG(errmsg, "User not found.");
        return USER_ERROR;
}
