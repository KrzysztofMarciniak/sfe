#include "user.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

int user_insert(sqlite3 *db, const user_t *user, char *errbuf,
                size_t errbuf_len) {
    if (!db || !user || !user->username || !user->password_hash) {
        snprintf(errbuf, errbuf_len, "Invalid input parameters.");
        return -1;
    }

    const char *sql =
        "INSERT INTO users (username, password_hash) VALUES (?, ?);";
    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(errbuf, errbuf_len, "Failed to prepare statement: %s",
                 sqlite3_errmsg(db));
        return -2;
    }

    sqlite3_bind_text(stmt, 1, user->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->password_hash, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        snprintf(errbuf, errbuf_len, "Failed to execute statement: %s",
                 sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -3;
    }

    sqlite3_finalize(stmt);
    return 0;
}

user_t *user_fetch_by_username(sqlite3 *db, const char *username) {
    if (!db || !username) return NULL;

    const char *sql =
        "SELECT id, username, password_hash FROM users WHERE username = ? "
        "LIMIT 1;";
    sqlite3_stmt *stmt = NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    user_t *user = NULL;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = malloc(sizeof(user_t));
        if (!user) {
            sqlite3_finalize(stmt);
            return NULL;
        }

        user->id = sqlite3_column_int(stmt, 0);
        const char *uname = (const char *)sqlite3_column_text(stmt, 1);
        const char *pwhash = (const char *)sqlite3_column_text(stmt, 2);

        user->username = strdup(uname);
        user->password_hash = strdup(pwhash);

        if (!user->username || !user->password_hash) {
            user_free(user);
            user = NULL;
        }
    }

    sqlite3_finalize(stmt);
    return user;
}
