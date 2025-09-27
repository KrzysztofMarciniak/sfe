/**

    @file user.h

    @brief Data access functions for user persistence.
    */

#ifndef DAL_USER_H
#define DAL_USER_H

#include <sqlite3.h>

#include "/app/backend/lib/models/user_model/user_model.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USER_SUCCESS 0
#define USER_ERROR 1

/**

    @brief Insert a new user into the database.

    @param db SQLite database connection.

    @param user Pointer to user_t with username and password_hash filled.

    @param out_user Pointer to user_t* where the inserted user (with generated
   ID)

    will be stored on success (caller must free).

    @param errmsg Pointer to char* where a newly allocated error message will be

    stored on failure (caller MUST free), or NULL on success.

    @return 0 on success (USER_SUCCESS), 1 on failure (USER_ERROR).
    */
int user_insert(sqlite3* db, const user_t* user, user_t** out_user,
                char** errmsg);

/**

    @brief Fetch a user from the database by ID.

    @param db SQLite database connection.

    @param id ID to search for.

    @param out_user Pointer to user_t* where the fetched user will be stored on

    success (caller must free), or NULL on not found.

    @param errmsg Pointer to char* where a newly allocated error message will be

    stored on failure (caller MUST free), or NULL on success.

    @return 0 on success (USER_SUCCESS), 1 on failure or if user is not found
   (USER_ERROR).
    */
int user_fetch_by_id(sqlite3* db, int id, user_t** out_user, char** errmsg);

/**

    @brief Fetch a user from the database by username.

    @param db SQLite database connection.

    @param username Username to search for.

    @param out_user Pointer to user_t* where the fetched user will be stored on

    success (caller must free), or NULL on not found.

    @param errmsg Pointer to char* where a newly allocated error message will be

    stored on failure (caller MUST free), or NULL on success.

    @return 0 on success (USER_SUCCESS), 1 on failure or if user is not found
   (USER_ERROR).
    */
int user_fetch_by_username(sqlite3* db, const char* username, user_t** out_user,
                           char** errmsg);

#ifdef __cplusplus
}
#endif

#endif// DAL_USER_H
