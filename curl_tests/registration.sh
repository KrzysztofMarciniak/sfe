#!/bin/bash

API_URL="http://localhost:8080/api/register.cgi"

read -r -d '' PAYLOAD << EOF
{
  "username": "testuser1",
  "password": "SuperSecret123"
}
EOF

echo "Registering user..."

curl --fail --show-error  --verbose "$API_URL" \
  -H "Content-Type: application/json" \
  -d "$PAYLOAD"

echo -e "\nDone."
