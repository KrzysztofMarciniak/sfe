#!/bin/sh

DOXYFILE="/app/Doxyfile"

echo "Generating minimal Doxyfile at $DOXYFILE..."

cat > "$DOXYFILE" <<EOF
PROJECT_NAME = "simple forum engine"
INPUT = /app/backend
RECURSIVE = YES
GENERATE_LATEX = NO
EOF

echo "Running doxygen..."
doxygen "$DOXYFILE"
echo "Doxygen generation complete."
