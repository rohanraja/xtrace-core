#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
#include <algorithm>
#include <iostream>
#include <vector>

// Function which takes in a lamda function
void executeLambda(const std::function<void()> &lambda) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("lambda.cc", "executeLambda", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "lambda", base::ToString(lambda));
  xtrace->LogLineRun(xtrace_mrid, 6);
  lambda();
}

int main() {
  // Define a lambda function that prints a message
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("lambda.cc", "main", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 11);
  auto printMessage = []() {
    std::cout << "Hello from the lambda function!" << std::endl;
  };
  xtrace->LocalVarUpdate(xtrace_mrid, "printMessage",
                         base::ToString(printMessage));

  // Call the lambda function
  xtrace->LogLineRun(xtrace_mrid, 16);
  printMessage();

  // Define a vector of integers
  xtrace->LogLineRun(xtrace_mrid, 19);
  std::vector<int> numbers = {1, 2, 3, 4, 5};
  xtrace->LocalVarUpdate(xtrace_mrid, "numbers", base::ToString(numbers));

  xtrace->LogLineRun(xtrace_mrid, 21);
  executeLambda([]() {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("lambda.cc", "lambda", "GUID_FROM_TEST");
    xtrace->LogLineRun(xtrace_mrid, 22);
    std::cout << "Hello from the lambda function! INLINE" << std::endl;
  });

  xtrace->LogLineRun(xtrace_mrid, 25);
  return 0;
}
