#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
int main() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("cpp_attribute.cc", "main", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1);
  bool preserve_newlines = new_style.ShouldPreserveBreaks();
  xtrace->LocalVarUpdate(xtrace_mrid, "preserve_newlines",
                         base::ToString(preserve_newlines));

  xtrace->LogLineRun(xtrace_mrid, 2);
  if (preserve_newlines && is_text_combine_) [[unlikely]] {
    xtrace->LogLineRun(xtrace_mrid, 3);
    preserve_newlines = false;
    xtrace->LocalVarUpdate(xtrace_mrid, "preserve_newlines",
                           base::ToString(preserve_newlines));
  }
}
