#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
int sum(int a, int b) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "method_args.cc", "sum", "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));
  xtrace->LocalVarUpdate(xtrace_mrid, "b", base::ToString(b));
  xtrace->LogLineRun(xtrace_mrid, 1);
  xtrace->FlushAllEventsToJSONFile();
  return a + b;
}

void print(int a) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "method_args.cc", "print", "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));
  xtrace->LogLineRun(xtrace_mrid, 5);
  xtrace->FlushAllEventsToJSONFile();
  cout << a;
}

void localVars() {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "method_args.cc", "localVars", "00000000-0000-0000-0000-000000000000");
  xtrace->LogLineRun(xtrace_mrid, 9);
  int a = 5;
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));

  xtrace->LogLineRun(xtrace_mrid, 10);
  xtrace->FlushAllEventsToJSONFile();
  int b = a;
  xtrace->LocalVarUpdate(xtrace_mrid, "b", base::ToString(b));
}
