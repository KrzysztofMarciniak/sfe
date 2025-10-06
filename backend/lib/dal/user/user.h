#ifndef DAL_USER_H
#define DAL_USER_H

#include <sqlite3.h>

#include "/app/backend/lib/models/user_model/user_model.h"
#include "/app/backend/lib/result/result.h"

/**
 * @file user.h
 * @brief Data access functions for user persistence
 */

/**
 * @brief Insert a new user into the database
 * @param db SQLite database connection
 * @param user Pointer to user_t with username and password_hash filled
 * @param out_user Pointer to store inserted user with generated ID (caller must
 * free)
 * @return result_t indicating success or failure
 */
result_t* user_insert(sqlite3* db, const user_t* user, user_t** out_user);

/**
 * @brief Fetch a user from the database by ID
 * @param db SQLite database connection
 * @param id ID to search for
 * @param out_user Pointer to store fetched user (caller must free)
 * @return result_t indicating success or failure
 */
result_t* user_fetch_by_id(sqlite3* db, int id, user_t** out_user);

/**
 * @brief Fetch a user from the database by username
 * @param db SQLite database connection
 * @param username Username to search for
 * @param out_user Pointer to store fetched user (caller must free)
 * @return result_t indicating success or failure
 */
result_t* user_fetch_by_username(sqlite3* db, const char* username,
                                 user_t** out_user);
// Library-specific error codes (1300-1399)
#define ERR_INVALID_INPUT 1301
#define ERR_SQL_PREPARE_FAIL 1302
#define ERR_SQL_BIND_FAIL 1303
#define ERR_SQL_STEP_FAIL 1304
#define ERR_USER_NOT_FOUND 1305
#define ERR_USER_DUPLICATE 1306

#endif// DAL_USER_H
