#!/bin/bash

# SIMSCRIPT Compiler Test Suite

echo "=== SIMSCRIPT Compiler Tests ==="

# Check if compiler is built
if [ ! -f "./build/simscript_compiler" ]; then
    echo "Error: Compiler not found. Please run ./build.sh first"
    exit 1
fi

# Test basic functionality
echo "1. Testing basic output..."
./build/simscript_compiler tests/basic/test_simple.sim > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   PASS: Basic output test"
else
    echo "   FAIL: Basic output test"
fi

# Test math operations
echo "2. Testing math operations..."
./build/simscript_compiler tests/basic/test_math.sim > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   PASS: Math operations test"
else
    echo "   FAIL: Math operations test"
fi

# Test comments
echo "3. Testing comments..."
./build/simscript_compiler tests/basic/test_comment.sim > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   PASS: Comments test"
else
    echo "   FAIL: Comments test"
fi

# Test PREAMBLE
echo "4. Testing PREAMBLE..."
./build/simscript_compiler tests/basic/test_preamble.sim > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   PASS: PREAMBLE test"
else
    echo "   FAIL: PREAMBLE test"
fi

# Test runtime execution
echo "5. Testing code execution..."
./build/simscript_compiler tests/basic/test_simple.sim -o test_output.ll > /dev/null 2>&1
if [ -f test_output.ll ]; then
    llc test_output.ll -o test_output.s 2>/dev/null
    if [ -f test_output.s ]; then
        gcc -no-pie test_output.s -o test_executable 2>/dev/null
        if [ -f test_executable ]; then
            RESULT=$(./test_executable)
            if [ "$RESULT" = "42" ]; then
                echo "   PASS: Code execution test"
            else
                echo "   FAIL: Code execution test - Expected: 42, Got: $RESULT"
            fi
            rm -f test_executable
        else
            echo "   FAIL: Executable generation failed"
        fi
        rm -f test_output.s
    else
        echo "   FAIL: Assembly generation failed"
    fi
    rm -f test_output.ll
else
    echo "   FAIL: LLVM IR generation failed"
fi

echo ""
echo "=== Tests Complete ==="
