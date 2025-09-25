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
 * @param out_user Pointer to user_t* where the inserted user (with generated ID)
 *                 will be stored on success (caller must free).
 * @return NULL on success, error string on failure.
 */
const char *user_insert(sqlite3 *db, const user_t *user, user_t **out_user);

/**
 * @brief Fetch a user from the database by ID.
 *
 * @param db SQLite database connection.
 * @param id ID to search for.
 * @param out_user Pointer to user_t* where the fetched user will be stored on
 *                 success (caller must free), or NULL on not found.
 * @return NULL on success, error string on failure.
 */
const char *user_fetch_by_id(sqlite3 *db, int id, user_t **out_user);

const char *user_fetch_by_username(sqlite3 *db, const char *username, user_t **out_user);

#ifdef __cplusplus
}
#endif

#endif// DAL_USER_H
