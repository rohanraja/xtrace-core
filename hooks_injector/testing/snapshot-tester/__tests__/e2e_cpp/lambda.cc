#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // Define a lambda function that prints a message
    auto printMessage = []() {
        std::cout << "Hello from the lambda function!" << std::endl;
    };

    // Call the lambda function
    printMessage();

    // Define a vector of integers
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // Use a lambda function to print each number in the vector
    // std::for_each(numbers.begin(), numbers.end(), [](int number) {
    //     std::cout << "Number: " << number << std::endl;
    // });

    return 0;
}