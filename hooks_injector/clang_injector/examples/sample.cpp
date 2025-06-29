#include <iostream>
#include <vector>
#include <memory>

// Simple function
int add(int a, int b) {
    return a + b;
}

// Function with pointers
void processArray(int* arr, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        arr[i] *= 2;
    }
}

// Function with references
void increment(int& value) {
    value++;
}

// Class with methods
class Calculator {
private:
    std::string name_;
    
public:
    Calculator(const std::string& name) : name_(name) {
    }
    
    int multiply(int x, int y) {
        int result = x * y;
        return result;
    }
    
    double divide(double numerator, double denominator) {
        if (denominator == 0.0) {
            return 0.0;
        }
        return numerator / denominator;
    }
    
    // Method with complex types
    std::vector<int> generateSequence(int start, int count) {
        std::vector<int> sequence;
        for (int i = 0; i < count; ++i) {
            sequence.push_back(start + i);
        }
        return sequence;
    }
};

// Template function (advanced C++ feature)
template<typename T>
T maximum(const T& a, const T& b) {
    return (a > b) ? a : b;
}

// Function with lambda (C++11 feature)
void processData() {
    auto lambda = [](int x) -> int {
        return x * x;
    };
    
    int result = lambda(5);
    std::cout << "Lambda result: " << result << std::endl;
}

// Main function
int main() {
    // Test simple function
    int sum = add(5, 3);
    std::cout << "Sum: " << sum << std::endl;
    
    // Test pointer function
    int numbers[] = {1, 2, 3, 4, 5};
    processArray(numbers, 5);
    
    // Test reference function
    int value = 10;
    increment(value);
    std::cout << "Incremented value: " << value << std::endl;
    
    // Test class methods
    Calculator calc("MyCalculator");
    int product = calc.multiply(4, 7);
    double quotient = calc.divide(15.0, 3.0);
    
    std::cout << "Product: " << product << std::endl;
    std::cout << "Quotient: " << quotient << std::endl;
    
    // Test complex method
    std::vector<int> sequence = calc.generateSequence(10, 5);
    std::cout << "Sequence: ";
    for (int num : sequence) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    
    // Test template function
    int maxInt = maximum(10, 20);
    double maxDouble = maximum(3.14, 2.71);
    std::cout << "Max int: " << maxInt << std::endl;
    std::cout << "Max double: " << maxDouble << std::endl;
    
    // Test lambda function
    processData();
    
    return 0;
}
