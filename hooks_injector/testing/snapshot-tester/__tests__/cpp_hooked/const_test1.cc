#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"

void TestMethodWithConst() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "const_test1.cc", "TestMethodWithConst", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 2);
  const SelectionState &selection_state = GetSelectionStateFor(layout_text);
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_state",
                         base::ToString(selection_state));
}
