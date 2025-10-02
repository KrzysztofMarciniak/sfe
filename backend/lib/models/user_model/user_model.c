#include "user_model.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief Serialize a user_t struct to a JSON string
 * @param user Pointer to user_t struct
 * @param out_json Pointer to store the JSON string (caller must free)
 * @return result_t indicating success or failure
 */
result_t user_to_json(const user_t* user, char** out_json) {
        if (out_json) {
                *out_json = NULL;
        }

        if (!user) {
                result_t res =
                    result_failure("User cannot be NULL", NULL, ERR_USER_NULL);
                result_add_extra(&res, "user=%p", (const void*)user);
                return res;
        }

        struct json_object* jobj = json_object_new_object();
        if (!jobj) {
                result_t res = result_critical_failure(
                    "Failed to create JSON object", NULL, ERR_JSON_CREATE_FAIL);
                return res;
        }

        json_object_object_add(
            jobj, "username",
            json_object_new_string(user->username ? user->username : ""));
        json_object_object_add(
            jobj, "password_hash",
            json_object_new_string(user->password_hash ? user->password_hash
                                                       : ""));

        if (user->id >= 0) {
                json_object_object_add(jobj, "id",
                                       json_object_new_int(user->id));
        }

        const char* json_str = json_object_to_json_string(jobj);
        char* result         = strdup(json_str);

        json_object_put(jobj);

        if (!result) {
                result_t res = result_critical_failure(
                    "Failed to allocate memory for JSON string", NULL,
                    ERR_MEMORY_ALLOC_FAIL);
                return res;
        }

        *out_json = result;
        return result_success();
}

/**
 * @brief Parse a JSON string into a dynamically allocated user_t struct
 * @param json_str JSON-formatted string representing a user
 * @param out_user Pointer to store allocated user_t (caller must free with
 * user_free)
 * @return result_t indicating success or failure
 */
result_t json_to_user(const char* json_str, user_t** out_user) {
        if (out_user) {
                *out_user = NULL;
        }

        if (!json_str) {
                result_t res = result_failure("JSON string cannot be NULL",
                                              NULL, ERR_USER_NULL);
                result_add_extra(&res, "json_str=%p", (const void*)json_str);
                return res;
        }

        struct json_object* jobj = json_tokener_parse(json_str);
        if (!jobj) {
                result_t res = result_failure("Failed to parse JSON string",
                                              NULL, ERR_JSON_PARSE_FAIL);
                result_add_extra(&res, "json_str=%s", json_str);
                return res;
        }

        user_t* user = malloc(sizeof(user_t));
        if (!user) {
                result_t res = result_critical_failure(
                    "Failed to allocate memory for user", NULL,
                    ERR_MEMORY_ALLOC_FAIL);
                json_object_put(jobj);
                return res;
        }

        user->id            = -1;
        user->username      = NULL;
        user->password_hash = NULL;

        struct json_object* jfield = NULL;

        if (json_object_object_get_ex(jobj, "id", &jfield)) {
                user->id = json_object_get_int(jfield);
        }

        if (json_object_object_get_ex(jobj, "username", &jfield)) {
                const char* uname = json_object_get_string(jfield);
                if (uname) user->username = strdup(uname);
        }

        if (json_object_object_get_ex(jobj, "password_hash", &jfield)) {
                const char* pwhash = json_object_get_string(jfield);
                if (pwhash) user->password_hash = strdup(pwhash);
        }

        json_object_put(jobj);

        if (!user->username || !user->password_hash) {
                result_t res = result_failure(
                    "Mandatory fields (username or password_hash) missing",
                    NULL, ERR_MANDATORY_FIELDS_MISSING);
                result_add_extra(&res, "username=%p, password_hash=%p",
                                 (const void*)user->username,
                                 (const void*)user->password_hash);
                user_free(user);
                return res;
        }

        *out_user = user;
        return result_success();
}

/**
 * @brief Free a dynamically allocated user_t struct and its fields
 * @param user Pointer to user_t to free
 */
void user_free(user_t* user) {
        if (!user) return;
        free(user->username);
        free(user->password_hash);
        free(user);
}
