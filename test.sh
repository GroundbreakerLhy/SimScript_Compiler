#!/bin/bash

echo "=== SIMSCRIPT Compiler Tests ==="

# 构建项目
./build.sh > /dev/null 2>&1

# 检查编译器是否存在
if [ ! -f "build/simscript_compiler" ]; then
    echo "ERROR: Compiler not found. Build failed."
    exit 1
fi

test_count=0
pass_count=0

# 测试函数
run_test() {
    local test_name="$1"
    local test_file="$2"
    local expected_pattern="$3"
    
    test_count=$((test_count + 1))
    echo -n "$test_count. Testing $test_name..."
    
    # 编译测试文件
    ./build/simscript_compiler "$test_file" -o temp_test.ll > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "   FAIL: Compilation failed"
        return 1
    fi
    
    # 如果提供了预期模式，则编译并执行
    if [ -n "$expected_pattern" ]; then
        llc temp_test.ll -o temp_test.s > /dev/null 2>&1
        gcc -no-pie temp_test.s -o temp_test > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            echo "   FAIL: Linking failed"
            return 1
        fi
        
        output=$(./temp_test 2>&1)
        if echo "$output" | grep -q "$expected_pattern"; then
            echo "   PASS: $test_name test"
            pass_count=$((pass_count + 1))
            rm -f temp_test.ll temp_test.s temp_test
            return 0
        else
            echo "   FAIL: Expected pattern '$expected_pattern' not found in output: '$output'"
            rm -f temp_test.ll temp_test.s temp_test
            return 1
        fi
    else
        echo "   PASS: $test_name test"
        pass_count=$((pass_count + 1))
        rm -f temp_test.ll
        return 0
    fi
}

# 运行所有测试
run_test "basic output" "tests/basic/test_simple.sim" "42"
run_test "math operations" "tests/basic/test_math.sim" "15"
run_test "comments" "tests/basic/test_comment.sim" "123"
run_test "PREAMBLE" "tests/basic/test_preamble.sim" "42"
run_test "code execution" "tests/basic/test_functions.sim" "8"
run_test "ELSEIF" "tests/basic/test_elseif.sim" "1"
run_test "simulation" "tests/basic/test_simulation.sim" "1"
run_test "file I/O" "tests/basic/test_fileio.sim" "1"
run_test "FOR EACH" "tests/basic/test_foreach.sim" "1"

echo
echo "=== Tests Complete ==="
echo "Passed: $pass_count/$test_count tests"

if [ $pass_count -eq $test_count ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
