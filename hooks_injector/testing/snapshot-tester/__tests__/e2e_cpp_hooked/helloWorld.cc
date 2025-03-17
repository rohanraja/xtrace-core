#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
#include <iostream>

int main() {

  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("helloWorld.cc", "main", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 4);
  int x = 34;
  xtrace->LocalVarUpdate(xtrace_mrid, "x", base::ToString(x));

  xtrace->LogLineRun(xtrace_mrid, 5);
  std::cout << "Hello, World!" << std::endl;
  xtrace->LogLineRun(xtrace_mrid, 6);
  x = x + 1;
  xtrace->LocalVarUpdate(xtrace_mrid, "x", base::ToString(x));

  xtrace->LogLineRun(xtrace_mrid, 7);
  std::cout << "x is " << x << std::endl;
  xtrace->LogLineRun(xtrace_mrid, 8);
  xtrace->FlushAllEventsToJSONFile();
  return 0;
}
