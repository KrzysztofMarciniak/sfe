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

char *issue_jwt(const char *id, const char **errmsg) {
        if (errmsg) {
                *errmsg = NULL;
        }

        if (!id) {
                if (errmsg) {
                        *errmsg = "User ID cannot be null.";
                }
                return NULL;
        }

        const char *secret = get_jwt_secret(errmsg);
        if (!secret) {
                return NULL;
        }
        if (strlen(secret) == 0) {
                if (errmsg) {
                        *errmsg = "JWT secret is empty.";
                }
                return NULL;
        }

        struct json_object *claims = json_object_new_object();
        if (!claims) {
                if (errmsg) {
                        *errmsg = "Failed to create JSON object for claims.";
                }
                return NULL;
        }

        json_object_object_add(claims, "id", json_object_new_string(id));
        time_t now = time(NULL);
        json_object_object_add(claims, "iat", json_object_new_int64(now));
        json_object_object_add(claims, "exp", json_object_new_int64(now + 604800));

        char *jwt_error = NULL;
        char *token     = jwtc_generate(secret, 604800, claims, &jwt_error);

        json_object_put(claims);

        if (!token) {
                if (errmsg) {
                        if (jwt_error) {
                                *errmsg = "JWT generation failed.";
                        } else {
                                *errmsg = "JWT generation failed with unknown error.";
                        }
                }
        }

        if (jwt_error) {
                free(jwt_error);
        }

        return token;
}

bool val_jwt(const char *token, struct json_object **claims_out, const char **errmsg) {
        // Initialize output pointers
        if (errmsg) {
                *errmsg = NULL;
        }
        if (claims_out) {
                *claims_out = NULL;
        }

        // Check for invalid arguments.
        if (!token || !claims_out) {
                if (errmsg) {
                        *errmsg = "Invalid arguments.";
                }
                return false;
        }

        // Get the secret from the secrets management module.
        const char *secret = get_jwt_secret(errmsg);
        if (!secret) {
                // The error message is already set by get_jwt_secret().
                return false;
        }
        // Check if the secret is empty.
        if (strlen(secret) == 0) {
                if (errmsg) {
                        *errmsg = "JWT secret is empty.";
                }
                return false;
        }

        // Validate the token using the jwtc library.
        char *jwt_lib_error = NULL;
        int valid           = jwtc_validate(token, secret, 0, claims_out, &jwt_lib_error);

        // Check the validation result.
        if (!valid) {
                // Clean up the claims object if it was partially created.
                if (*claims_out) {
                        json_object_put(*claims_out);
                        *claims_out = NULL;
                }

                // Set the error message for the caller.
                if (errmsg) {
                        if (jwt_lib_error) {
                                // Use a generic static message as the library's message is
                                // malloc'd.
                                *errmsg = "JWT validation failed.";
                        } else {
                                *errmsg = "JWT validation failed with an unknown error.";
                        }
                }

                // Free the library's error message to prevent a memory leak.
                if (jwt_lib_error) {
                        free(jwt_lib_error);
                }

                return false;
        }

        return true;
}
