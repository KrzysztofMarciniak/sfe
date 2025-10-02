#!/usr/bin/env sh

set -eu

. ./test_manager_misc.sh

# 1. GET csrf.cgi -> expect JSON with "messages" array containing token
echo ">>> Test 1: GET /csrf.cgi (generate token)"
resp=$(curl -s -X GET "$BASE_URL/csrf.cgi")
token=$(printf '%s' "$resp" | jq -r '.messages[0]')
if [ -z "$token" ] || [ "$token" = "null" ]; then
    echo "[FAIL] No token returned"
    exit 1
fi
echo "Got token: $token"
echo "[PASS] Token received"
echo

# 2. POST csrf.cgi with the correct token -> expect "CSRF token is valid."
echo ">>> Test 2: POST /csrf.cgi (valid token)"
run_post "csrf.cgi" "{\"token\":\"$token\"}" \
         'CSRF token is valid.' "200"

# 3. POST csrf.cgi with an incorrect token -> expect validation failure
echo ">>> Test 3: POST /csrf.cgi (invalid token)"
run_post "csrf.cgi" '{"token":"INVALIDTOKEN"}' \
         'CSRF token is null' "400"

# 4. POST csrf.cgi with a really long token -> expect validation failure
echo ">>> Test 4: POST /csrf.cgi (too long token)"
long_token="$(head -c 2048 </dev/zero | tr '\0' 'A')"
run_post "csrf.cgi" "{\"token\":\"$long_token\"}" \
         'CSRF token is null' "400"
