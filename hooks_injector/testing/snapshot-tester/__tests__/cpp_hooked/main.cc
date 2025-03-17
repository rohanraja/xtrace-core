#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
int hello() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("main.cc", "hello", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1);
  xtrace->FlushAllEventsToJSONFile();
  printf("Hello, World!");
}
