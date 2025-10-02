#!/usr/bin/env sh

set -eu

. ./test_manager_misc.sh

# --- Helper Functions ---

# Fetch a valid CSRF token
get_csrf() {
    local csrf_resp csrf_token
    csrf_resp=$(curl -s -X GET "$BASE_URL/csrf.cgi")
    csrf_token=$(printf '%s' "$csrf_resp" | jq -r '.token')
    printf '%s' "$csrf_token"
}

# Build JSON payload
make_payload() {
    csrf="$1"
    username="$2"
    password="$3"
    printf '{"csrf":"%s","username":"%s","password":"%s"}' \
           "$csrf" "$username" "$password"
}

# --- Main Tests ---

csrf_token=$(get_csrf)
echo ">>> CSRF token for login tests: $csrf_token"

# 1. Successful login (assumes 'alice' is already registered with 'secure123')
echo ">>> Test 1: Successful login"
payload=$(make_payload "$csrf_token" "alice" "secure123")
run_post "login.cgi" "$payload" '["token"]' "200"

# 2. Wrong password
echo ">>> Test 2: Wrong password"
payload=$(make_payload "$csrf_token" "alice" "wrongpassword")
run_post "login.cgi" "$payload" '["Invalid username or password."]' "401"

# 3. Non-existent username
echo ">>> Test 3: Non-existent username"
payload=$(make_payload "$csrf_token" "nonexistent" "secure123")
run_post "login.cgi" "$payload" '["Invalid username or password."]' "401"

# 4. Missing CSRF token
echo ">>> Test 4: Missing CSRF"
payload='{"username":"alice","password":"secure123"}'
run_post "login.cgi" "$payload" '["All fields must be non-empty strings."]' "400"

# 5. Invalid CSRF token
echo ">>> Test 5: Invalid CSRF"
payload=$(make_payload "INVALIDTOKEN" "alice" "secure123")
run_post "login.cgi" "$payload" '["Invalid CSRF token."]' "400"

# 6. Empty username
echo ">>> Test 6: Empty username"
payload=$(make_payload "$csrf_token" "" "secure123")
run_post "login.cgi" "$payload" '["All fields must be non-empty strings."]' "400"

# 7. Empty password
echo ">>> Test 7: Empty password"
payload=$(make_payload "$csrf_token" "alice" "")
run_post "login.cgi" "$payload" '["All fields must be non-empty strings."]' "400"

# 8. Username with non-alphanumeric characters
echo ">>> Test 8: Invalid username chars"
payload=$(make_payload "$csrf_token" "bob!@#" "secure123")
run_post "login.cgi" "$payload" '["Invalid username or password."]' "401"
