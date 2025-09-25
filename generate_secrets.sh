#!/bin/sh

set -e

mkdir -p ./.secrets

# Generate secure random base64url strings without padding
generate_secret() {
  local bytes=$1
  openssl rand -base64 "$bytes" | tr '+/' '-_' | tr -d '='
}

CSRF_SECRET=$(generate_secret 32)
JWT_SECRET=$(generate_secret 64)

echo "$CSRF_SECRET" > .secrets/csrf.txt
echo "$JWT_SECRET" > .secrets/jwt.txt

echo "Secrets generated:"
echo "  ./.secrets/csrf.txt"
echo "  ./.secrets/jwt.txt"
