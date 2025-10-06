f!/bin/sh
set -eu

. ./test_manager_misc.sh

# Global variable to hold the token
csrf_token=""

# Helper to make JSON payloads
make_payload() {
    csrf="$1"
    username="$2"
    password="$3"
    printf '{"csrf":"%s","username":"%s","password":"%s"}' \
           "$csrf" "$username" "$password"
}

# 1. Successful registration
get_csrf_token # Get fresh token for this test
echo ">>> Test 1: Successful registration (Token consumed)"
payload=$(make_payload "$csrf_token" "alice" "secure123")
run_post "register.cgi" "$payload" \
          '["User registered successfully."]' "201"

# 2. Username too long
get_csrf_token # Get fresh token for this test
echo ">>> Test 2: Username too long"
payload=$(make_payload "$csrf_token" "averylongusername" "secure123")
run_post "register.cgi" "$payload" \
          '["Username too long (12 characters max)."]' "400"

# 3. Password too short
get_csrf_token # Get fresh token for this test
echo ">>> Test 3: Password too short"
payload=$(make_payload "$csrf_token" "bob" "123")
run_post "register.cgi" "$payload" \
          '["Password must be at least 6 characters."]' "400"

# 4. Missing CSRF (No token used, but we'll refresh before the next test)
echo ">>> Test 4: Missing CSRF"
payload='{"username":"charlie","password":"secure123"}'
run_post "register.cgi" "$payload" \
          '["Missing csrf, username, or password field."]' "400"

# 5. Invalid CSRF
get_csrf_token # Get fresh, throwaway token for this test
echo ">>> Test 5: Invalid CSRF"
payload=$(make_payload "INVALIDTOKEN" "dave" "secure123")
# The expected output is based on the actual error message in register.c
run_post "register.cgi" "$payload" \
          '["Invalid CSRF token"]' "400"

# 6. Duplicate username (alice already registered)
get_csrf_token # Get fresh token for this test
echo ">>> Test 6: Duplicate username"
payload=$(make_payload "$csrf_token" "alice" "secure123")
run_post "register.cgi" "$payload" \
          '["Username already exists."]' "400"

# 7. Non-alphanumeric username
get_csrf_token # Get fresh token for this test
echo ">>> Test 7: Invalid username chars"
payload=$(make_payload "$csrf_token" "bob!@#" "secure123")
run_post "register.cgi" "$payload" \
          '["Username must be alphanumeric."]' "400"
