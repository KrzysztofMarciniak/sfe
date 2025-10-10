#!/usr/bin/env sh
set -eu

# Pre-commit: format, run clang-tidy, print per-file failures with output & suggested fix.
# Usage: copy to .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit

echo "[pre-commit] Running clang-format and static analysis..."

# 1) Format (use repo script if present)
if [ -x "./clang-format.sh" ]; then
    ./clang-format.sh
else
    find backend \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) -exec clang-format -i {} +
fi

# Restage formatted files
git add backend || true

# 2) Find staged C/C++ files
FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(c|cpp|h|hpp)$' || true)
[ -z "$FILES" ] && {
    echo "[pre-commit] No C/C++ files staged. Skipping clang-tidy."
    exit 0
}

# 3) Run clang-tidy per-file and capture output. Fail on any non-zero exit.
TIDY=$(command -v clang-tidy 2>/dev/null || true)
if [ -z "$TIDY" ]; then
    echo "[pre-commit] clang-tidy not found. Skipping static analysis."
    exit 0
fi

# Temp dir for per-file logs (portable)
if command -v mktemp >/dev/null 2>&1; then
    TMPDIR=$(mktemp -d /tmp/precommit-tidy.XXXXXX)
elif command -v mkdtemp >/dev/null 2>&1; then
    TMPDIR=$(mkdtemp -t precommit-tidy)
else
    echo "No temporary directory command found. Aborting."
    exit 1
fi
trap 'rm -rf "$TMPDIR"' EXIT

FAIL=0

# Optional: customize checks here to be strict or lenient
TIDY_CHECKS='clang-analyzer-*,bugprone-*,clang-analyzer-security.*'

for f in $FILES; do
    [ -f "$f" ] || continue

    LOG="$TMPDIR/$(echo "$f" | sed 's/[^A-Za-z0-9_.-]/_/g').log"
    # Run clang-tidy; capture both stdout and stderr
    $TIDY -checks="$TIDY_CHECKS" "$f" -- -Ibackend -Ibackend/lib -std=c17 >"$LOG" 2>&1 || RC=$? && RC=${RC:-0}
    # If clang-tidy exits non-zero OR the log contains "warning:"/ "error:" we treat as failure.
    if [ "${RC:-0}" -ne 0 ] || grep -E "warning:|error:" "$LOG" >/dev/null 2>&1; then
        FAIL=1
        echo
        printf '%s\n' "======== clang-tidy FAILED: $f ========"
        cat "$LOG"
        echo "-------- end of clang-tidy output for $f --------"
        echo "Suggested remediation:"
        printf '  %s\n' "clang-tidy -fix -checks=\"$TIDY_CHECKS\" \"$f\" -- -Ibackend -Ibackend/lib -std=c17"
        echo "After fix: clang-format -i \"$f\" && git add \"$f\""
        echo
    fi
done

if [ $FAIL -ne 0 ]; then
    echo "[pre-commit] clang-tidy reported issues. Commit aborted."
    exit 1
fi

echo "[pre-commit] All checks passed."
exit 0
