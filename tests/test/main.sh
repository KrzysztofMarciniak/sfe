#!/usr/bin/env sh

set -eu

. ./test_manager_misc.sh

expected_body='{ "message": "test works!" }'
expected_status="200"

run_get "test.cgi" "$expected_body" "$expected_status"
