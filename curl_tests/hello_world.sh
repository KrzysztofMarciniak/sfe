#!/bin/sh
API_URL="http://localhost:8080/api/hello_world.cgi"
echo "Registering user..."
curl --fail --show-error  --verbose "$API_URL" \
  -H "Content-Type: application/json" 

echo -e "\nDone."

