#!/bin/bash

API_URL="http://localhost:8080/api/register.cgi"

send_request() {
  local username="$1"
  local password="$2"
  local expected_code="$3"
  local expected_msg="$4"

  PAYLOAD=$(printf '{"username": "%s", "password": "%s"}' "$username" "$password")

  echo -n "Test: username='$username', password='$password' ... "

  local temp_file="/tmp/response_$$_$RANDOM.json"

  # Capture body to file, code to variable
  response_code=$(curl --silent --show-error --output "$temp_file" \
    -H "Content-Type: application/json" \
    -d "$PAYLOAD" \
    "$API_URL" --write-out "%{http_code}" 2>/dev/null)

  msg=$(jq -r '.message // "No message field"' "$temp_file" 2>/dev/null || echo "Invalid JSON response")

  rm -f "$temp_file"

  if [[ "$response_code" == "$expected_code" && "$msg" == *"$expected_msg"* ]]; then
    echo "PASS"
  else
    echo "FAIL"
    echo "  Expected HTTP $expected_code and message containing: '$expected_msg'"
    echo "  Got HTTP $response_code and message: '$msg'"
  fi
}

if ! command -v jq >/dev/null 2>&1; then
  echo "Error: jq is required but not installed. Please install jq."
  exit 1
fi

echo "Starting tests..."

send_request "testuser1" "SuperSecret123" 201 "User registered successfully"
send_request "" "SuperSecret123" 400 "username is empty"
send_request "thisusernameistoolong" "SuperSecret123" 400 "username too long"
send_request "invalid\$user" "SuperSecret123" 400 "username is invalid"
send_request "validuser" "" 400 "Password must be at least 6 characters"
send_request "validuser" "12345" 400 "Password must be at least 6 characters"
send_request "testuser1" "AnotherPass123" 400 "Username already exists"

echo "Tests completed."
