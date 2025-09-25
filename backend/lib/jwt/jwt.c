#include "jwt.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "/app/backend/lib/secrets/secrets.h"
#include "jwtc.h"

/**
 * @brief Issues a JWT token with a single "id" claim, valid for one week.
 *
 * Creates a JWT containing the provided id as the "id" claim. The token is
 * signed using the secret obtained from get_jwt_secret() and expires in one week.
 *
 * @param id The user or entity ID to include in the token.

 * @return Returns a malloc'd JWT string on success. Caller must free().
 *         Returns NULL on failure.
 */
char *issue_jwt(const char *id) {
        if (!id) return NULL;

        const char *secret = get_jwt_secret();
        if (!secret) return NULL;

        json_object *claims = json_object_new_object();
        if (!claims) return NULL;

        json_object_object_add(claims, "id", json_object_new_string(id));

        // Expiration time: now + 1 week (604800 seconds)
        time_t now = time(NULL);
        json_object_object_add(claims, "iat", json_object_new_int64(now));
        json_object_object_add(claims, "exp", json_object_new_int64(now + 604800));

        char *error = NULL;
        char *token = jwtc_generate(secret, 604800, claims, &error);

        json_object_put(claims);

        if (!token && error) {
                free(error);
        }

        return token;
}

/**
 * @brief Validates a JWT token.
 *
 * This function validates the given JWT token using the secret obtained from
 * get_jwt_secret(). It also verifies the expiration (`exp`) claim internally.
 *
 * @param token The JWT token string to validate.
 * @param claims_out Pointer to a json_object* which will be set to the claims
 *                   if the token is valid. Caller must call json_object_put().
 *
 * @return Returns NULL if the token is valid (and claims_out is set).
 *         Returns a malloc'd error string describing the validation failure if invalid.
 *         Caller must free() this error string.
 */
char *val_jwt(const char *token, json_object **claims_out) {
        if (!token || !claims_out) return strdup("Invalid arguments");

        *claims_out = NULL;
        char *error = NULL;

        const char *secret = get_jwt_secret();
        if (!secret) {
                return strdup("No secret available");
        }

        int valid = jwtc_validate(token, secret, 0, claims_out, &error);
        if (!valid) {
                if (*claims_out) {
                        json_object_put(*claims_out);
                        *claims_out = NULL;
                }
                return error;
        }

        return NULL;
}
