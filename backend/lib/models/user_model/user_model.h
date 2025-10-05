#ifndef USER_MODEL_H
#define USER_MODEL_H

#include <json-c/json.h>

#include "/app/backend/lib/result/result.h"

// Library-specific error codes (1200-1299)
#define ERR_USER_NULL 1201
#define ERR_JSON_CREATE_FAIL 1202
#define ERR_JSON_PARSE_FAIL 1203
#define ERR_MANDATORY_FIELDS_MISSING 1204

/**
 * @struct user_t
 * @brief Represents a user with ID, username, and password hash.
 */
typedef struct {
        int id;         /**< User ID, -1 if unset */
        char* username; /**< Username string (dynamically allocated) */
        char*
            password_hash; /**< Password hash string (dynamically allocated) */
} user_t;

/**
 * @brief Serialize a user_t struct to a JSON string
 * @param user Pointer to user_t struct
 * @param out_json Pointer to store the JSON string (caller must free)
 * @return result_t indicating success or failure
 */
result_t* user_to_json(const user_t* user, char** out_json);

/**
 * @brief Parse a JSON string into a dynamically allocated user_t struct
 * @param json_str JSON-formatted string representing a user
 * @param out_user Pointer to store allocated user_t (caller must free with
 * user_free)
 * @return result_t indicating success or failure
 */
result_t* json_to_user(const char* json_str, user_t** out_user);

/**
 * @brief Free a dynamically allocated user_t struct and its fields
 * @param user Pointer to user_t to free
 */
void user_free(user_t* user);

#endif// USER_MODEL_H
