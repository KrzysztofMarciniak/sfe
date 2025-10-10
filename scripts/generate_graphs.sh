#!/bin/bash
# portable callgraph generator: cflow -> gprof2dot -> dot -> docs/graphs/*.svg
set -eu

export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:${PATH:-}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUTDIR="$ROOT/docs/graphs"
mkdir -p "$OUTDIR"

# Find sources
BACKEND_C=$(find "$ROOT/backend" -maxdepth 1 -name '*.c' -print)
LIB_C=$(find "$ROOT/backend/lib" -name '*.c' -print 2>/dev/null || true)

# Tools (prefer PATH first, fallback to common locations)
command -v cflow >/dev/null 2>&1 || { echo "ERROR: cflow not found in PATH"; exit 2; }
command -v dot >/dev/null 2>&1 || { echo "ERROR: dot (graphviz) not found in PATH"; exit 2; }

# gprof2dot may be installed in $HOME/.local/bin by pip --user; check both
GPROF2DOT=$(command -v gprof2dot 2>/dev/null || true)
if [ -z "$GPROF2DOT" ] && [ -x "${HOME}/.local/bin/gprof2dot" ]; then
    GPROF2DOT="${HOME}/.local/bin/gprof2dot"
fi
[ -n "$GPROF2DOT" ] || { echo "ERROR: gprof2dot not found (pip install --user gprof2dot)"; exit 2; }

echo "Using tools: cflow=$(command -v cflow) gprof2dot=$GPROF2DOT dot=$(command -v dot)"
echo "Output directory: $OUTDIR"

# Helper: convert a cflow text file to dot + svg
convert_cflow_to_svg() {
    local base="$1"   # basename without extension
    local txt="$2"    # path to .cflow.txt
    local dot="$OUTDIR/${base}.dot"
    local svg="$OUTDIR/${base}.svg"

    "$GPROF2DOT" -f cflow "$txt" -o "$dot"
    dot -Tsvg "$dot" -o "$svg"
    echo "Wrote $svg"
}

# Per-backend files
for src in $BACKEND_C; do
    # If no backend C files, the glob may expand to literal string; guard
    [ -f "$src" ] || continue

    bn=$(basename "$src" .c)
    echo "Generating callgraph for $bn"

    TXT="$OUTDIR/${bn}.cflow.txt"

    # Prefer using --main=main if present in that source
    if grep -q -E 'int[[:space:]]+main[[:space:]]*\(' "$src" 2>/dev/null; then
        # include lib sources for context
        if [ -n "$LIB_C" ]; then
            cflow --main=main $LIB_C "$src" > "$TXT" 2>/dev/null || cflow $LIB_C "$src" > "$TXT"
        else
            cflow --main=main "$src" > "$TXT" 2>/dev/null || cflow "$src" > "$TXT"
        fi
    else
        if [ -n "$LIB_C" ]; then
            cflow $LIB_C "$src" > "$TXT" 2>/dev/null || cflow "$src" > "$TXT"
        else
            cflow "$src" > "$TXT" 2>/dev/null || cflow "$src" > "$TXT"
        fi
    fi

    convert_cflow_to_svg "$bn" "$TXT"
done

# Combined library graph (if any)
if [ -n "$LIB_C" ]; then
    echo "Generating combined library callgraph"
    LIBTXT="$OUTDIR/lib_all.cflow.txt"
    cflow $LIB_C > "$LIBTXT" 2>/dev/null || cflow $LIB_C > "$LIBTXT"
    convert_cflow_to_svg "lib_all" "$LIBTXT"
fi

echo "All call graphs generated in $OUTDIR"
