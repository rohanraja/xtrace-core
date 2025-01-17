#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
int hello() {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "main.cc", "hello", "00000000-0000-0000-0000-000000000000");
  xtrace->LogLineRun(xtrace_mrid, 1);
  xtrace->FlushAllEventsToJSONFile();
  printf("Hello, World!");
}
