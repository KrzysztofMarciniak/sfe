#!/bin/bash

API_URL="http://localhost:8080/api/csrf.cgi"

if ! command -v jq >/dev/null 2>&1; then
  echo "Error: jq is required but not installed. Please install jq."
  exit 1
fi

echo
echo "Starting CSRF token generation test..."

# Step 1: GET the token
temp_file_get="/tmp/csrf_get_response_$$_$RANDOM.json"

response_code=$(curl --silent --show-error --output "$temp_file_get" \
  -H "Accept: application/json" \
  "$API_URL" --write-out "%{http_code}" 2>/dev/null)

# Extract token from 'message' field (based on your code's JSON)
csrf_token=$(jq -r '.message // empty' "$temp_file_get" 2>/dev/null || echo "")

rm -f "$temp_file_get"

if [[ "$response_code" == "200" && "$csrf_token" =~ ^[a-f0-9]{144}$ ]]; then
  echo "PASS: Received valid CSRF token."
else
  echo "FAIL: Invalid response during token generation."
  echo "  Expected HTTP 200 and 144-char hex token in 'message'"
  echo "  Got HTTP $response_code and token: '$csrf_token'"
  exit 1
fi

echo
echo "Starting CSRF token validation test..."

# Step 2: POST the token back for validation
temp_file_post="/tmp/csrf_post_response_$$_$RANDOM.json"

validation_code=$(curl --silent --show-error --output "$temp_file_post" \
  -H "Content-Type: application/json" \
  -d "{\"message\": \"$csrf_token\"}" \
  -X POST "$API_URL" --write-out "%{http_code}" 2>/dev/null)

validation_msg=$(jq -r '.message // .error // "Invalid response"' "$temp_file_post" 2>/dev/null || echo "")

rm -f "$temp_file_post"

if [[ "$validation_code" == "200" && "$validation_msg" == "Token is valid" ]]; then
  echo "PASS: Token validated successfully."
else
  echo "FAIL: Token validation failed."
  echo "  Expected HTTP 200 and message: 'Token is valid'"
  echo "  Got HTTP $validation_code and message: '$validation_msg'"
  exit 1
fi

echo
echo "All tests completed successfully."

