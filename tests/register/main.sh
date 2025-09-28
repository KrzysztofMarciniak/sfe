#!/usr/bin/env sh

set -eu

. ./test_manager_misc.sh

# 1. Get a valid CSRF token
csrf_resp=$(curl -s -X GET "$BASE_URL/csrf.cgi")
csrf_token=$(printf '%s' "$csrf_resp" | jq -r '.token')

echo ">>> CSRF token for registration tests: $csrf_token"

# Helper to make JSON payloads
make_payload() {
    csrf="$1"
    username="$2"
    password="$3"
    printf '{"csrf":"%s","username":"%s","password":"%s"}' \
           "$csrf" "$username" "$password"
}

# 1. Successful registration
echo ">>> Test 1: Successful registration"
payload=$(make_payload "$csrf_token" "alice" "secure123")
run_post "register.cgi" "$payload" \
         '["User registered successfully."]' "201"

# 2. Username too long
echo ">>> Test 2: Username too long"
payload=$(make_payload "$csrf_token" "averylongusername" "secure123")
run_post "register.cgi" "$payload" \
         '["Username too long (12 characters max)."]' "400"

# 3. Password too short
echo ">>> Test 3: Password too short"
payload=$(make_payload "$csrf_token" "bob" "123")
run_post "register.cgi" "$payload" \
         '["Password must be at least 6 characters."]' "400"

# 4. Missing CSRF
echo ">>> Test 4: Missing CSRF"
payload='{"username":"charlie","password":"secure123"}'
run_post "register.cgi" "$payload" \
         '["Missing csrf, username, or password field."]' "400"

# 5. Invalid CSRF
echo ">>> Test 5: Invalid CSRF"
payload=$(make_payload "INVALIDTOKEN" "dave" "secure123")
run_post "register.cgi" "$payload" \
         '["Invalid CSRF token."]' "400"

# 6. Duplicate username (alice already registered)
echo ">>> Test 6: Duplicate username"
payload=$(make_payload "$csrf_token" "alice" "secure123")
run_post "register.cgi" "$payload" \
         '["Username already exists."]' "400"

# 7. Non-alphanumeric username
echo ">>> Test 7: Invalid username chars"
payload=$(make_payload "$csrf_token" "bob!@#" "secure123")
run_post "register.cgi" "$payload" \
         '["Username must be alphanumeric."]' "400"
