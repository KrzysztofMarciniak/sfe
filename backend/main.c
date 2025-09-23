#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>

int main(void) {
    sqlite3 *db;
    int rc;

    rc = sqlite3_open("/data/alpsc.db", &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create table if missing
    const char *sql_create = "CREATE TABLE IF NOT EXISTS visits (id INTEGER PRIMARY KEY AUTOINCREMENT, ts DATETIME DEFAULT CURRENT_TIMESTAMP);";
    sqlite3_exec(db, sql_create, 0, 0, 0);

    char *query = getenv("QUERY_STRING");
    int increase = 0;
    if(query && strstr(query, "action=increase")) increase = 1;

    printf("Content-Type: text/plain\n\n");

    if(increase){
        sqlite3_exec(db, "INSERT INTO visits DEFAULT VALUES;", 0, 0, 0);
        printf("Added visit\n");
    } else {
        // Show latest visit count and timestamp
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v2(db, "SELECT COUNT(*), MAX(ts) FROM visits;", -1, &stmt, 0);
        if(rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW){
            int count = sqlite3_column_int(stmt, 0);
            const char* last_ts = (const char*)sqlite3_column_text(stmt, 1);
            printf("%d %s\n", count, last_ts ? last_ts : "N/A");
        } else {
            printf("0 N/A\n");
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return 0;
}

