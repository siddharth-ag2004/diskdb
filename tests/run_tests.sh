#!/bin/bash

# Configuration
TEST_DIR="tests/cases"
DATA_DIR="data"
SERVER_BIN="./src/server"

# Compile Server
echo "Compiling server..."
cd src
make -j > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi
cd ..

passed=0
failed=0

# Ensure data directory exists
mkdir -p "$DATA_DIR"

echo "Running tests..."

for ra_file in "$TEST_DIR"/*.ra; do
    [ -e "$ra_file" ] || continue
    
    test_name=$(basename "$ra_file" .ra)
    echo -n "Test $test_name: "
    
    # 1. Setup Data
    cp "$TEST_DIR/${test_name}_Nodes_D.csv" "$DATA_DIR/"
    cp "$TEST_DIR/${test_name}_Edges_D.csv" "$DATA_DIR/"
    
    # 2. Run Server
    # Filter out prompts and empty lines
    pushd src > /dev/null
    ./server < "../$ra_file" 2>&1 | sed 's/^[[:space:]]*>[[:space:]]*//' | grep -v "^\s*$" | grep -v "Reading New Command" | sed 's/^[ \t]*//;s/[ \t]*$//' > "../$TEST_DIR/${test_name}.out"
    popd > /dev/null
    
    # 3. Compare
    # Use diff -w -B (ignore whitespace and blank lines)
    if diff -w -B "$TEST_DIR/${test_name}.expected" "$TEST_DIR/${test_name}.out" > /dev/null; then
        echo "PASS"
        ((passed++))
    else
        echo "FAIL"
        echo "--- Expected ---"
        cat "$TEST_DIR/${test_name}.expected"
        echo "--- Actual ---"
        cat "$TEST_DIR/${test_name}.out"
        echo "----------------"
        ((failed++))
    fi
    
    # 4. Cleanup Data
    rm "$DATA_DIR/${test_name}_Nodes_D.csv"
    rm "$DATA_DIR/${test_name}_Edges_D.csv"
    
done

echo "Summary: Passed: $passed, Failed: $failed"

if [ $failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
