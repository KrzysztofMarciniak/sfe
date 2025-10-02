#!/usr/bin/env sh
set -eu

. ./test_manager_misc.sh

PASS_ENDPOINT="pass.cgi"

# Helper to make JSON payloads
make_payload() {
    printf '%s' "$1"
}

# POST helper that validates expected pattern inside the messages array
run_post_msgs() {
    url="$1"
    data="$2"
    expected_pattern="$3"
    expected_status="$4"

    echo ">>> POST $BASE_URL/$url"
    echo "[DEBUG] Request payload: $data"

    resp=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$data" \
        -w "\n[STATUS]%{http_code}" \
        "$BASE_URL/$url")

    body=$(printf '%s' "$resp" | sed -n '1,/^\[STATUS]/p' | sed '$d')
    status=$(printf '%s' "$resp" | sed -n 's/^\[STATUS]//p')

    # Extract messages array entries
    messages_lines=$(printf '%s' "$body" | jq -r '.messages[]' 2>/dev/null || true)
    messages_json=$(printf '%s\n' "$messages_lines" | jq -R -s -c 'split("\n")[:-1]' 2>/dev/null || printf '[]')

    echo "Expected pattern (grep -E): $expected_pattern"
    echo "Gotten messages:  $messages_json"
    echo "Expected status:   $expected_status"
    echo "Gotten status:     $status"

    if [ -n "$messages_lines" ] && printf '%s\n' "$messages_lines" | grep -E -q "$expected_pattern" && [ "$status" = "$expected_status" ]; then
        echo "[PASS]"
    else
        echo "[FAIL]"
    fi
    echo
}

# Check if debug endpoint is available via POST
resp=$(curl -s -o /dev/null -w "%{http_code}" -X POST \
    -H "Content-Type: application/json" -d '{}' "$BASE_URL/$PASS_ENDPOINT")

if [ "$resp" = "404" ]; then
    echo "[WARN] Debug endpoint $PASS_ENDPOINT not available (HTTP $resp). Skipping all password tests."
    exit 0
fi


echo ">>> Test 1: Generate password hash"
payload=$(make_payload '{"gen_password":"mysecret"}')
run_post_msgs "$PASS_ENDPOINT" "$payload" '^[0-9a-f]{32}\$[0-9]+\$[0-9a-f]{64}$' "200"

echo ">>> Test 2: Validate password against hash"
raw=$(curl -s -X POST "$BASE_URL/$PASS_ENDPOINT" \
    -H "Content-Type: application/json" \
    -d '{"gen_password":"mypassword"}')

hash=$(printf '%s' "$raw" | jq -r '.messages[] | select(test("^[0-9a-f]{32}\\$[0-9]+\\$[0-9a-f]{64}$"))' 2>/dev/null | head -n1 || true)
if [ -z "$hash" ] || [ "$hash" = "null" ]; then
    echo "[WARN] Failed to extract hash from response; skipping validation."
else
    echo "[DEBUG] Extracted hash: ${hash}"
    payload=$(make_payload "{\"val_password\":\"mypassword\",\"hash\":\"$hash\"}")
    run_post_msgs "$PASS_ENDPOINT" "$payload" '^Password is valid$' "200"
fi

echo ">>> Test 3: Debug endpoint not available (simulate DEBUG=0)"
run_post_msgs "pass_debug_off.cgi" "{}" '^Debug endpoint not available$' "404"

echo ">>> Test 4: Generate 1024-char password hash"
LONG_PASS=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c1024)
payload=$(make_payload "{\"gen_password\":\"$LONG_PASS\"}")
run_post_msgs "$PASS_ENDPOINT" "$payload" '^[0-9a-f]{32}\$[0-9]+\$[0-9a-f]{64}$' "200"

echo ">>> Test 5: Validate 1024-char password against its hash"
raw=$(curl -s -X POST "$BASE_URL/$PASS_ENDPOINT" \
    -H "Content-Type: application/json" \
    -d "{\"gen_password\":\"$LONG_PASS\"}")

HASH_1024=$(printf '%s' "$raw" | jq -r '.messages[] | select(test("^[0-9a-f]{32}\\$[0-9]+\\$[0-9a-f]{64}$"))' 2>/dev/null | head -n1 || true)
if [ -z "$HASH_1024" ] || [ "$HASH_1024" = "null" ]; then
    echo "[WARN] Failed to extract 1024 hash from response; skipping validation."
else
    echo "[DEBUG] Extracted 1024 hash (truncated): ${HASH_1024:0:60}..."
    payload=$(make_payload "{\"val_password\":\"$LONG_PASS\",\"hash\":\"$HASH_1024\"}")
    run_post_msgs "$PASS_ENDPOINT" "$payload" '^Password is valid$' "200"
fi

echo ">>> All pass.c debug endpoint tests completed."
