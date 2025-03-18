#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
PaintLayerScrollableArea::FreezeScrollbarsRootScope::
    ~FreezeScrollbarsRootScope() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "name_newline.cc",
      "PaintLayerScrollableArea::FreezeScrollbarsRootScope::    "
      "~FreezeScrollbarsRootScope",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 2);
  if (scrollable_area_)
    scrollable_area_->ClearScrollbarRoot();
}
