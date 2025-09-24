#ifndef USER_MODEL_H
#define USER_MODEL_H

#include <json-c/json.h>

/**
 * @struct user_t
 * @brief Represents a user with ID, username, and password hash.
 */
typedef struct {
        int id;              /**< User ID, -1 if unset */
        char *username;      /**< Username string (dynamically allocated) */
        char *password_hash; /**< Password hash string (dynamically allocated) */
} user_t;

/**
 * @brief Serialize a user_t struct to a JSON string.
 *
 * The caller is responsible for freeing the returned string.
 *
 * @param user Pointer to user_t struct.
 * @return char* JSON string on success, NULL on failure.
 */
char *user_to_json(const user_t *user);

/**
 * @brief Parse a JSON string into a dynamically allocated user_t struct.
 *
 * The caller is responsible for freeing the returned user using user_free().
 *
 * @param json_str JSON-formatted string representing a user.
 * @return user_t* Pointer to allocated user_t on success, NULL on failure.
 */
user_t *json_to_user(const char *json_str);

/**
 * @brief Free a dynamically allocated user_t struct and its fields.
 *
 * @param user Pointer to user_t to free.
 */
void user_free(user_t *user);

#endif// USER_MODEL_H
