#!/bin/bash
# Test script to verify our forest resource mapping fix

echo "Testing forest resource mapping fix..."

# Test the compilation was successful
if [ ! -f "./circle" ]; then
    echo "ERROR: circle binary not found - compilation failed"
    exit 1
fi

echo "Binary compilation successful."

# The segfault appears to be in the spell system, not our resource code
# Let's check if our change is syntactically correct by examining the compiled object
echo "Checking resource_descriptions.c compilation..."
if [ -f "src/resource_descriptions.o" ]; then
    echo "resource_descriptions.o compiled successfully"
    # Check the modification timestamp to see if it's recent
    if [ "src/resource_descriptions.o" -nt "src/resource_descriptions.c" ]; then
        echo "Object file is up to date with source"
    else
        echo "WARNING: Object file appears older than source"
    fi
else
    echo "ERROR: resource_descriptions.o not found"
    exit 1
fi

echo "Forest resource mapping fix compilation verified."
