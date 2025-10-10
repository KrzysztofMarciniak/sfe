#!/bin/bash
# scripts/generate_doxygen.sh
# Generates Doxygen documentation for the backend
set -euo pipefail

ROOT="$PWD"
DOXYDIR="$ROOT/docs/doxygen"
DOXYFILE="$DOXYDIR/Doxyfile"

# Ensure output directory exists
mkdir -p "$DOXYDIR"

echo "Generating minimal Doxyfile at $DOXYFILE..."

cat > "$DOXYFILE" <<EOF
# Minimal Doxygen config
PROJECT_NAME = "simple forum engine"
OUTPUT_DIRECTORY = "$DOXYDIR"
INPUT = "$ROOT/backend"
RECURSIVE = YES
GENERATE_LATEX = NO
GENERATE_HTML = YES
QUIET = YES
EOF

# Run Doxygen
echo "Running Doxygen..."
command -v doxygen >/dev/null 2>&1 || { echo "Doxygen not installed"; exit 1; }
doxygen "$DOXYFILE"

echo "Doxygen generation complete. HTML docs at $DOXYDIR/html"
