#!/bin/bash
set -euo pipefail

API_URL="http://localhost:8080/api/login.cgi"
CSRF_URL="http://localhost:8080/api/csrf.cgi"

# Fetch CSRF token
get_csrf_token() {
  curl --silent "$CSRF_URL" | jq -r '.message'
}

send_login_request() {
  local username="$1"
  local password="$2"
  local expected_code="$3"
  local expected_field="$4"

  local csrf_token
  csrf_token=$(get_csrf_token)

  PAYLOAD=$(printf '{"csrf": "%s", "username": "%s", "password": "%s"}' "$csrf_token" "$username" "$password")

  echo -n "Test: username='$username', password='$password' ... "

  local temp_file="/tmp/login_response_$$_$RANDOM.json"

  local response_code
  response_code=$(curl --silent --show-error --output "$temp_file" \
    -H "Content-Type: application/json" \
    -d "$PAYLOAD" \
    "$API_URL" --write-out "%{http_code}")

  local result
  result=$(jq -r --arg field "$expected_field" '.[$field] // "missing"' "$temp_file" 2>/dev/null || echo "Invalid JSON")

  rm -f "$temp_file"

  if [[ "$response_code" == "$expected_code" && "$result" != "missing" ]]; then
    echo "PASS"
  else
    echo "FAIL"
    echo "  Expected HTTP $expected_code and field '$expected_field'"
    echo "  Got HTTP $response_code and field value: '$result'"
  fi
}

if ! command -v jq >/dev/null 2>&1; then
  echo "Error: jq is required but not installed. Please install jq."
  exit 1
fi

echo "Starting login tests..."

# Assumes this user is already registered
send_login_request "testuser1" "SuperSecret123" 200 "token"

# Wrong password
send_login_request "testuser1" "wrongpassword" 401 "message"

# Invalid user
send_login_request "nonexistent" "doesntmatter" 401 "message"

# Empty username
send_login_request "" "somepass" 400 "message"

# Empty password
send_login_request "validuser" "" 400 "message"

