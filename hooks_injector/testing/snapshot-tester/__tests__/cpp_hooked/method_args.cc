#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
int sum(int a, int b) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("method_args.cc", "sum", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));
  xtrace->LocalVarUpdate(xtrace_mrid, "b", base::ToString(b));
  xtrace->LogLineRun(xtrace_mrid, 1);
  return a + b;
}

void print(int a) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("method_args.cc", "print", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));
  xtrace->LogLineRun(xtrace_mrid, 5);
  cout << a;
}

void localVars() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("method_args.cc", "localVars", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 9);
  int a = 5;
  xtrace->LocalVarUpdate(xtrace_mrid, "a", base::ToString(a));

  xtrace->LogLineRun(xtrace_mrid, 10);
  int b = a;
  xtrace->LocalVarUpdate(xtrace_mrid, "b", base::ToString(b));
}
