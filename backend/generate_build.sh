#!/bin/sh

# Output build.ninja file
output_file="build.ninja"

# Start writing build.ninja
cat > "$output_file" << 'EOF'
rule compile
  command = tcc -L./ $in -o $out -lsqlite3 -ljson-c -lcrypto 
  description = Compiling $in to $out

EOF

# Find top-level .c files for CGIs (exclude scripts)
cgi_sources=$(find . -maxdepth 1 -name "*.c" ! -name "*_entrypoint.sh")

# Find all .c files in ./lib, excluding test_*.c
lib_sources=$(find ./lib -type f -name "*.c" ! -name "test_*.c")

# Convert lib_sources to space-separated list
lib_sources_list=$(echo "$lib_sources" | tr '\n' ' ')

# Generate build rules for each CGI
for cgi_src in $cgi_sources; do
    # Get CGI name without .c extension
    cgi_name=$(basename "$cgi_src" .c)
    cgi_out="${cgi_name}.cgi"
    # Combine CGI source with all lib sources
    all_inputs="$cgi_src $lib_sources_list"
    echo "build $cgi_out: compile $all_inputs" >> "$output_file"
done
