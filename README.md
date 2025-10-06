# sfe — Simple Forum Engine

**Status:** In development.


**Stack:** C API, SQLite, Docker, Ninja build system, lighttpd CGI, tcc for quick compilation.


**Docs:** Doxygen at `/api/docs`.


**Tests:** POSIX shell + `curl`.


**Security:** CSRF + JWT (uses `jwtc`), libsodium, OpenSSL.


**Deps:** `libsodium`, `openssl`, `json-c`, `sanitizec`, `jwtc` ([https://github.com/KrzysztofMarciniak/jwtc](https://github.com/KrzysztofMarciniak/jwtc)), `sanitizec` ([https://github.com/KrzysztofMarciniak/sanitizec](https://github.com/KrzysztofMarciniak/sanitizec)).

---

## Repository layout (high level)

```
/backend/                   # CGI endpoints (C sources)
/backend/lib/               # Core libraries (dal, hash_password, csrf, response, result, etc.)
/backend/sqlite_entrypoint.sh  # Initializes SQLite schema and tables

/tests/                     # POSIX shell + curl test scripts
/test_manager.sh             # Main test orchestrator 
/test_manager_misc.sh        # Helper functions for the test manager

/start.sh                    # Builds Docker image and starts the backend server
```

---

## Build & run

### Docker 

1. Run:

```sh
chmod +x start_server.sh ; ./start_server.sh
```

## API (example: registration)

`POST /api/register.cgi`
Request body JSON:

```json
{
  "csrf": "<csrf-token>",
  "username": "alice",
  "password": "hunter2"
}
```

Responses:

* **201** — `"User registered successfully."` (account created)
* **400** — `"Username already exists."` (duplicate username)
* **400** — validation messages (e.g. `"Password must be at least 6 characters."`)
* **405** — method not allowed
* **500** — internal server errors

Example test (POSIX shell + curl):

```sh
curl -s -X POST http://localhost:8080/api/register.cgi \
  -H "Content-Type: application/json" \
  -d '{"csrf":"<token>","username":"bob","password":"secret"}'
# assert HTTP status and body in your test harness
```

---

## Error model (`result_t`)

All library functions return `result_t*`.

Key points from `result.h`:

* `result_code_t` — `RESULT_SUCCESS`, `RESULT_FAILURE`, `RESULT_CRITICAL_FAILURE`.
* `error_t` — contains `code`, `message`, `failed_file`, `failed_func`, `extra_info`.
* Convenience macros: `result_success()`, `result_failure(msg, extra, code)`, `result_critical_failure(...)`.
* Mutator: `result_add_extra(...)`.
* `result_to_json()` for serializing errors into JSON for API responses.

