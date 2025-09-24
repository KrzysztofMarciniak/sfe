#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

typedef struct {
    sqlite3 *db;
} sfe_db_t;

// Insert user JSON, return inserted ID or -1
int sfe_db_insert_user(sfe_db_t *sfe, const char *json_data) {
    const char *sql = "INSERT INTO users (data) VALUES (?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, json_data, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return -1;
    }
    int id = (int)sqlite3_last_insert_rowid(sfe->db);
    sqlite3_finalize(stmt);
    return id;
}

// Get user JSON by id, caller must free result
char* sfe_db_get_user_json(sfe_db_t *sfe, int user_id) {
    const char *sql = "SELECT data FROM users WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_int(stmt, 1, user_id);
    char *res = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text) res = strdup((const char*)text);
    }
    sqlite3_finalize(stmt);
    return res;
}

// Delete user by id, return 0 success, non-zero fail
int sfe_db_delete_user(sfe_db_t *sfe, int user_id) {
    const char *sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, user_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Insert session JSON with token, return 0 success, non-zero fail
int sfe_db_insert_session(sfe_db_t *sfe, const char *token, const char *json_data) {
    const char *sql = "INSERT INTO sessions (token, data) VALUES (?, ?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, json_data, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

// Get session JSON by token, caller must free result
char* sfe_db_get_session_json(sfe_db_t *sfe, const char *token) {
    const char *sql = "SELECT data FROM sessions WHERE token = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;
    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
    char *res = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text) res = strdup((const char*)text);
    }
    sqlite3_finalize(stmt);
    return res;
}

// Delete session by token, return 0 success, non-zero fail
int sfe_db_delete_session(sfe_db_t *sfe, const char *token) {
    const char *sql = "DELETE FROM sessions WHERE token = ?;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(sfe->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, token, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}
