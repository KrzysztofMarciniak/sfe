csrf=$(curl -s http://localhost:8080/api/csrf.cgi | jq -r '.message'); \
curl -s -X POST http://localhost:8080/api/register.cgi -H "Content-Type: application/json" -d "{\"csrf\": \"$csrf\", \"username\": \"testuser1\", \"password\": \"SuperSecret123\"}"; \
curl -s -X POST http://localhost:8080/api/login.cgi -H "Content-Type: application/json" -d "{\"csrf\": \"$csrf\", \"username\": \"testuser1\", \"password\": \"SuperSecret123\"}"

