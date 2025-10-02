#include "jwt.h"

#include <sanitizec.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "/app/backend/lib/secrets/secrets.h"
#include "jwtc.h"

/**
 * @file jwt.c
 * @brief Functions for issuing and validating JWT tokens
 */

/**
 * @brief Issues a JWT token with a single "id" claim, valid for one week
 * @param id The user or entity ID to include in the token
 * @param out_token Pointer to store the malloc'd JWT string (caller must free)
 * @return result_t indicating success or failure
 */
result_t issue_jwt(const char* id, char** out_token) {
        if (out_token) {
                *out_token = NULL;
        }

        if (!id) {
                result_t res = result_failure("User ID cannot be NULL", NULL,
                                              ERR_JWT_INVALID_ID);
                result_add_extra(&res, "id=%p", (const void*)id);
                return res;
        }

        char* secret = NULL;
        result_t sc  = get_jwt_secret(&secret);
        if (sc.code != RESULT_SUCCESS) {
                return sc;
        }

        if (strlen(secret) == 0) {
                result_t res = result_failure("JWT secret is empty", NULL,
                                              ERR_JWT_SECRET_EMPTY);
                free_result(&sc);
                return res;
        }

        struct json_object* claims = json_object_new_object();
        if (!claims) {
                result_t res = result_critical_failure(
                    "Failed to create JSON object for claims", NULL,
                    ERR_JWT_JSON_FAIL);
                free_result(&sc);
                return res;
        }

        json_object_object_add(claims, "id", json_object_new_string(id));
        time_t now = time(NULL);
        json_object_object_add(claims, "iat", json_object_new_int64(now));
        json_object_object_add(claims, "exp",
                               json_object_new_int64(now + 604800));

        char* jwt_error = NULL;
        char* token     = jwtc_generate(secret, 604800, claims, &jwt_error);

        json_object_put(claims);
        free_result(&sc);

        if (!token) {
                result_t res = result_failure("JWT generation failed", NULL,
                                              ERR_JWT_GENERATE_FAIL);
                if (jwt_error) {
                        result_add_extra(&res, "jwt_error=%s", jwt_error);
                        free(jwt_error);
                }
                return res;
        }

        if (jwt_error) {
                free(jwt_error);
        }

        *out_token = token;
        return result_success();
}

/**
 * @brief Validates a JWT token and extracts its claims
 * @param token The JWT token string to validate
 * @param claims_out Pointer to store extracted claims (caller must free with
 * json_object_put)
 * @return result_t indicating success or failure
 */
result_t val_jwt(const char* token, struct json_object** claims_out) {
        if (claims_out) {
                *claims_out = NULL;
        }

        if (!token || !claims_out) {
                result_t res = result_failure("Invalid arguments", NULL,
                                              ERR_JWT_INVALID_ARGS);
                result_add_extra(&res, "token=%p, claims_out=%p",
                                 (const void*)token, (const void*)claims_out);
                return res;
        }

        char* sanitized_token =
            sanitizec_apply(token, SANITIZEC_RULE_ALPHANUMERIC_ONLY, NULL);
        if (!sanitized_token) {
                result_t res = result_failure("Token sanitization failed", NULL,
                                              ERR_JWT_SANITIZE_FAIL);
                return res;
        }

        char* secret = NULL;
        result_t sc  = get_jwt_secret(&secret);
        if (sc.code != RESULT_SUCCESS) {
                free(sanitized_token);
                return sc;
        }

        if (strlen(secret) == 0) {
                result_t res = result_failure("JWT secret is empty", NULL,
                                              ERR_JWT_SECRET_EMPTY);
                free(sanitized_token);
                free_result(&sc);
                return res;
        }

        char* jwt_lib_error = NULL;
        int valid = jwtc_validate(sanitized_token, secret, 0, claims_out,
                                  &jwt_lib_error);

        free(sanitized_token);
        free_result(&sc);

        if (!valid) {
                if (*claims_out) {
                        json_object_put(*claims_out);
                        *claims_out = NULL;
                }

                result_t res = result_failure("JWT validation failed", NULL,
                                              ERR_JWT_VALIDATE_FAIL);
                if (jwt_lib_error) {
                        result_add_extra(&res, "jwt_error=%s", jwt_lib_error);
                        free(jwt_lib_error);
                }
                return res;
        }

        if (jwt_lib_error) {
                free(jwt_lib_error);
        }

        return result_success();
}
