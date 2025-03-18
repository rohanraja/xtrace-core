#include <iostream>
#include <vector>
#include <algorithm>

// Function which takes in a lamda function
void executeLambda(const std::function<void()>& lambda) {
    lambda();
}

int main() {
    // Define a lambda function that prints a message
    auto printMessage = []() {
        std::cout << "Hello from the lambda function!" << std::endl;
    };

    // Call the lambda function
    printMessage();

    // Define a vector of integers
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    executeLambda([](){
        std::cout << "Hello from the lambda function! INLINE" << std::endl;
    });

    return 0;
}