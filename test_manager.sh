#!/usr/bin/env sh
# test_manager.sh - Simple test runner

set -eu

# List of test modules
tests="test csrf pass register login"

for t in $tests; do
    script="tests/$t/main.sh"
    if [ -f "$script" ]; then
        echo "=== Running $script ==="
        sh "$script"
        echo "=== Finished $t ==="
    else
        echo "!!! Skipping $t: $script not found" >&2
    fi
done
