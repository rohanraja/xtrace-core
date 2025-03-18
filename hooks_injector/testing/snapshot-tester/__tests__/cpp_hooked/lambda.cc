#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  // Define a lambda function that prints a message
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("lambda.cc", "main", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 6);
  auto printMessage = []() {
    std::cout << "Hello from the lambda function!" << std::endl;
  };
  xtrace->LocalVarUpdate(xtrace_mrid, "printMessage",
                         base::ToString(printMessage));

  // Call the lambda function
  xtrace->LogLineRun(xtrace_mrid, 11);
  printMessage();

  // Define a vector of integers
  xtrace->LogLineRun(xtrace_mrid, 14);
  std::vector<int> numbers = {1, 2, 3, 4, 5};
  xtrace->LocalVarUpdate(xtrace_mrid, "numbers", base::ToString(numbers));

  // Use a lambda function to print each number in the vector
  xtrace->LogLineRun(xtrace_mrid, 17);
  std::for_each(numbers.begin(), numbers.end(), [](int number) {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("lambda.cc", "lambda", "GUID_FROM_TEST");
    xtrace->LocalVarUpdate(xtrace_mrid, "number", base::ToString(number));
    xtrace->LogLineRun(xtrace_mrid, 18);
    std::cout << "Number: " << number << std::endl;
  });

  xtrace->LogLineRun(xtrace_mrid, 21);
  return 0;
}
