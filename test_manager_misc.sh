#!/bin/sh
set -eu

BASE_URL="http://localhost:8080/api"

run_get() {
    url="$1"
    expected_body="$2"
    expected_status="$3"

    echo ">>> GET $BASE_URL/$url"

    resp=$(curl -s -X GET \
        -w "\n[STATUS]%{http_code}" \
        "$BASE_URL/$url")

    # Extract body and status
    body=$(printf '%s' "$resp" | sed '$d')
    status=$(printf '%s' "$resp" | tail -n1 | sed 's/^\[STATUS]//')

    echo "Expected body:   $expected_body"
    echo "Gotten body:     $body"
    echo "Expected status: $expected_status"
    echo "Gotten status:   $status"

    if [ "$body" = "$expected_body" ] && [ "$status" = "$expected_status" ]; then
        echo "[PASS]"
    else
        echo "[FAIL]"
    fi
    echo
}

run_post() {
    url="$1"
    data="$2"
    expected_messages="$3"
    expected_status="$4"

    echo ">>> POST $BASE_URL/$url"

    resp=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$data" \
        -w "\n[STATUS]%{http_code}" \
        "$BASE_URL/$url")

    body=$(printf '%s' "$resp" | sed '$d')
    status=$(printf '%s' "$resp" | tail -n1 | sed 's/^\[STATUS]//')

    # Extract messages array if present
    messages=$(printf '%s' "$body" | jq -c '.messages' 2>/dev/null || echo "(no messages)")

    echo "Expected messages: $expected_messages"
    echo "Gotten messages:  $messages"
    echo "Expected status:   $expected_status"
    echo "Gotten status:     $status"

    if [ "$messages" = "$expected_messages" ] && [ "$status" = "$expected_status" ]; then
        echo "[PASS]"
    else
        echo "[FAIL]"
    fi
    echo
}

# Helper function to get a fresh CSRF token and set the global variable
get_csrf_token() {
    # Fetch token from the CSRF endpoint
    resp=$(curl -s -X GET "$BASE_URL/csrf.cgi")

    # Extract the token using the format specified in your previous test
    local temp_token=$(printf '%s' "$resp" | jq -r '.messages[0] // empty')

    if [ -z "$temp_token" ]; then
        echo "[ERROR] Failed to get fresh CSRF token from /csrf.cgi" >&2
        exit 1
    fi
    csrf_token="$temp_token"
    echo ">>> Acquired fresh CSRF token: $csrf_token"
}
