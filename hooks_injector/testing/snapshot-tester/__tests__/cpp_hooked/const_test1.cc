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

  xtrace->LogLineRun(xtrace_mrid, 4);
  const unsigned start_in_block2 = paint_range_->start_offset.value();
  xtrace->LocalVarUpdate(xtrace_mrid, "start_in_block2",
                         base::ToString(start_in_block2));

  xtrace->LogLineRun(xtrace_mrid, 5);
  int y = 34;
  xtrace->LocalVarUpdate(xtrace_mrid, "y", base::ToString(y));

  xtrace->LogLineRun(xtrace_mrid, 6);
  y = y + 1;
  xtrace->LocalVarUpdate(xtrace_mrid, "y", base::ToString(y));

  xtrace->LogLineRun(xtrace_mrid, 7);
  if (true) {
    xtrace->LogLineRun(xtrace_mrid, 8);
    int x = 23;
    xtrace->LocalVarUpdate(xtrace_mrid, "x", base::ToString(x));

    xtrace->LogLineRun(xtrace_mrid, 9);
    const unsigned start_in_block = paint_range_->start_offset.value();
    xtrace->LocalVarUpdate(xtrace_mrid, "start_in_block",
                           base::ToString(start_in_block));

    xtrace->LogLineRun(xtrace_mrid, 10);
    y = y + 1;
    xtrace->LocalVarUpdate(xtrace_mrid, "y", base::ToString(y));
  }
}
