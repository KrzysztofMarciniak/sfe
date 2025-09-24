#include "user_model.h"
#include <stdlib.h>
#include <string.h>

char* user_to_json(const user_t *user) {
    if (!user) return NULL;

    struct json_object *jobj = json_object_new_object();
    if (!jobj) return NULL;

    json_object_object_add(jobj, "username", json_object_new_string(user->username));
    json_object_object_add(jobj, "password_hash", json_object_new_string(user->password_hash));

    if (user->id >= 0) {
        json_object_object_add(jobj, "id", json_object_new_int(user->id));
    }

    const char *json_str = json_object_to_json_string(jobj);
    char *result = strdup(json_str);

    json_object_put(jobj);
    return result;
}

user_t* json_to_user(const char *json_str) {
    if (!json_str) return NULL;

    struct json_object *jobj = json_tokener_parse(json_str);
    if (!jobj) return NULL;

    user_t *user = malloc(sizeof(user_t));
    if (!user) {
        json_object_put(jobj);
        return NULL;
    }

    user->id = -1;
    user->username = NULL;
    user->password_hash = NULL;

    struct json_object *jfield = NULL;

    if (json_object_object_get_ex(jobj, "id", &jfield)) {
        user->id = json_object_get_int(jfield);
    }

    if (json_object_object_get_ex(jobj, "username", &jfield)) {
        const char *uname = json_object_get_string(jfield);
        if (uname) user->username = strdup(uname);
    }

    if (json_object_object_get_ex(jobj, "password_hash", &jfield)) {
        const char *pwhash = json_object_get_string(jfield);
        if (pwhash) user->password_hash = strdup(pwhash);
    }

    json_object_put(jobj);

    // Validate mandatory fields
    if (!user->username || !user->password_hash) {
        free(user->username);
        free(user->password_hash);
        free(user);
        return NULL;
    }

    return user;
}

void user_free(user_t *user) {
    if (!user) return;
    free(user->username);
    free(user->password_hash);
    free(user);
}
