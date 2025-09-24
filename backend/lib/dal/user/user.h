/**
 * @file user.h
 * @brief Data access functions for user persistence.
 */

#ifndef DAL_USER_H
#define DAL_USER_H

#include <sqlite3.h>

#include "/app/backend/lib/models/user_model/user_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Insert a new user into the database.
 *
 * @param db SQLite database connection.
 * @param user Pointer to user_t with username and password_hash filled.
 * @return 0 on success, non-zero on failure.
 */
int user_insert(sqlite3 *db, const user_t *user, char *errbuf,
                size_t errbuf_len);

/**
 * @brief Fetch a user from the database by username.
 *
 * @param db SQLite database connection.
 * @param username Username to search for.
 * @return Allocated user_t struct (caller must free), or NULL on not found or
 * error.
 */
user_t *user_fetch_by_username(sqlite3 *db, const char *username);

#ifdef __cplusplus
}
#endif

#endif  // DAL_USER_H
