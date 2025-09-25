#include "user.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *user_insert(sqlite3 *db, const user_t *user, user_t **out_user) {
        if (!db || !user || !user->username || !user->password_hash || !out_user) {
                return "Invalid input parameters.";
        }

        const char *sql    = "INSERT INTO users (username, password_hash) VALUES (?, ?);";
        sqlite3_stmt *stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                return sqlite3_errmsg(db);
        }

        sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, user->password_hash, -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return sqlite3_errmsg(db);
        }

        sqlite3_finalize(stmt);

        int new_id = (int)sqlite3_last_insert_rowid(db);

        return user_fetch_by_id(db, new_id, out_user);
}

const char *user_fetch_by_id(sqlite3 *db, int id, user_t **out_user) {
        if (!db || !out_user) return "Invalid arguments.";

        *out_user = NULL;

        const char *sql    = "SELECT id, username, password_hash FROM users WHERE id = ? LIMIT 1;";
        sqlite3_stmt *stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                return sqlite3_errmsg(db);
        }

        sqlite3_bind_int(stmt, 1, id);

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t *user = malloc(sizeof(user_t));
                if (!user) {
                        sqlite3_finalize(stmt);
                        return "Out of memory.";
                }

                user->id           = sqlite3_column_int(stmt, 0);
                const char *uname  = (const char *)sqlite3_column_text(stmt, 1);
                const char *pwhash = (const char *)sqlite3_column_text(stmt, 2);

                user->username      = uname ? strdup(uname) : NULL;
                user->password_hash = pwhash ? strdup(pwhash) : NULL;

                if (!user->username || !user->password_hash) {
                        user_free(user);
                        sqlite3_finalize(stmt);
                        return "Out of memory.";
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return NULL;
        }

        sqlite3_finalize(stmt);
        return "User not found.";
}

const char *user_fetch_by_username(sqlite3 *db, const char *username, user_t **out_user) {
        if (!db || !username || !out_user) return "Invalid arguments.";

        *out_user = NULL;

        const char *sql =
            "SELECT id, username, password_hash FROM users WHERE username = ? COLLATE NOCASE LIMIT "
            "1;";
        sqlite3_stmt *stmt = NULL;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
                return sqlite3_errmsg(db);
        }

        rc = sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
                sqlite3_finalize(stmt);
                return sqlite3_errmsg(db);
        }

        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
                user_t *user = malloc(sizeof(user_t));
                if (!user) {
                        sqlite3_finalize(stmt);
                        return "Out of memory.";
                }

                user->id           = sqlite3_column_int(stmt, 0);
                const char *uname  = (const char *)sqlite3_column_text(stmt, 1);
                const char *pwhash = (const char *)sqlite3_column_text(stmt, 2);

                user->username      = uname ? strdup(uname) : NULL;
                user->password_hash = pwhash ? strdup(pwhash) : NULL;

                if (!user->username || !user->password_hash) {
                        user_free(user);
                        sqlite3_finalize(stmt);
                        return "Out of memory.";
                }

                *out_user = user;
                sqlite3_finalize(stmt);
                return NULL;
        }

        sqlite3_finalize(stmt);
        return "User not found.";
}
