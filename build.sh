#!/bin/bash

# SIMSCRIPT II.5 Compiler Build Script

set -e

echo "=== SIMSCRIPT II.5 Compiler Build Script ==="

# Check dependencies
check_dependencies() {
    echo "Checking dependencies..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        echo "Error: CMake not installed"
        echo "Please run: sudo apt-get install cmake"
        exit 1
    fi
    
    # Check Flex
    if ! command -v flex &> /dev/null; then
        echo "Error: Flex not installed"
        echo "Please run: sudo apt-get install flex"
        exit 1
    fi
    
    # Check Bison
    if ! command -v bison &> /dev/null; then
        echo "Error: Bison not installed"
        echo "Please run: sudo apt-get install bison"
        exit 1
    fi
    
    # Check LLVM
    if ! command -v llvm-config &> /dev/null; then
        echo "Warning: llvm-config not found"
        echo "Please run: sudo apt-get install llvm-16-dev"
        echo "or manually specify LLVM path"
    fi
    
    echo "Dependencies check complete"
}

# Create build directory
create_build_dir() {
    echo "Creating build directory..."
    mkdir -p build
    cd build
}

# Configure project
configure_project() {
    echo "Configuring project..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
}

# Build project
build_project() {
    echo "Building project..."
    make -j"$(nproc)"
}

# Run tests
run_tests() {
    echo "Running tests..."
    if [ -f "./tests/test_simscript" ]; then
        ./tests/test_simscript
    else
        echo "Test executable not found, skipping tests."
    fi
}

# Test the compiler
test_compiler() {
    echo "Testing the compiler..."
    if [ -f "./simscript_compiler" ]; then
        echo "Testing the compiler with a test program..."
        ./simscript_compiler ../tests/basic/test_simple.sim -o test_output.ll > /dev/null 2>&1
        
        if [ -f "test_output.ll" ]; then
            echo "Compiler test successful!"
            echo "Generated LLVM IR file: test_output.ll"
            rm -f test_output.ll  # Clean up test file
        else
            echo "Compiler test failed"
            exit 1
        fi
    else
        echo "Compiler executable not found"
        exit 1
    fi
}

# Main function
main() {
    check_dependencies
    create_build_dir
    configure_project
    build_project
    run_tests
    test_compiler
    
    echo ""
    echo "=== Build complete ==="
    echo "Compiler executable: $(pwd)/simscript_compiler"
    echo ""
    echo "Usage:"
    echo "  ./build/simscript_compiler input.sim -o output.ll"
    echo "  llc output.ll -o output.s"
    echo "  gcc output.s -o executable"
}

# Execute main function
main "$@"

