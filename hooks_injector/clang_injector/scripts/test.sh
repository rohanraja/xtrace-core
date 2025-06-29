#!/bin/bash

# Test the Clang injector with sample input

set -e

echo "Testing xTrace Clang Injector..."

# Check if binary exists
if [ ! -f "build/xtrace-clang-injector" ]; then
    echo "Binary not found. Building first..."
    npm run build
fi

# Create test directory
mkdir -p test
cd test

# Create a simple test file
cat > simple_test.cpp << 'EOF'
#include <iostream>

int add(int a, int b) {
    return a + b;
}

class Calculator {
public:
    int multiply(int x, int y) {
        int result = x * y;
        return result;
    }
};

int main() {
    int result = add(5, 3);
    std::cout << "Result: " << result << std::endl;
    
    Calculator calc;
    int product = calc.multiply(4, 7);
    std::cout << "Product: " << product << std::endl;
    
    return 0;
}
EOF

echo "Created test file: simple_test.cpp"
echo ""
echo "Original code:"
echo "=============="
cat simple_test.cpp
echo ""
echo ""

echo "Running xTrace injection..."
echo "=========================="

# Run the injector
../build/xtrace-clang-injector simple_test.cpp -- -std=c++17 > injected_test.cpp

echo "Injected code:"
echo "=============="
cat injected_test.cpp

echo ""
echo "✓ Test completed successfully!"
echo "  Original file: test/simple_test.cpp"
echo "  Injected file: test/injected_test.cpp"

cd ..
