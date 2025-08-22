#!/bin/bash
# Usage: ./run.sh <source.aur>

if [ $# -eq 0 ]; then
    echo "Usage: $0 <source.aur>"
    exit 1
fi

SOURCE=$1
BASENAME=$(basename "$SOURCE" .aur)

# Compile Aurora source to object file
./build/aurorac "$SOURCE" -o "build/${BASENAME}.o" --emit-ll "build/${BASENAME}.ll" || exit 1

# Link with runtime
clang -no-pie "build/${BASENAME}.o" build/CMakeFiles/aurora_runtime.dir/stdlib/aurora_runtime.c.o -o "build/${BASENAME}" || exit 1

# Run the program
echo "Running ${BASENAME}:"
"./build/${BASENAME}"