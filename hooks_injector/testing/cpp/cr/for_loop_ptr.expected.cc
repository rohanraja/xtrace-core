#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
template <typename Strategy>
Node *StyledMarkupTraverser<Strategy>::Traverse(Node *start_node,
                                                Node *past_end) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "for_loop_ptr.cc", "StyledMarkupTraverser<Strategy>::Traverse",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "start_node",
                         start_node ? base::ToString(*start_node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "past_end",
                         past_end ? base::ToString(*past_end) : "");
  xtrace->LogLineRun(xtrace_mrid, 3);
  HeapVector<Member<ContainerNode>> ancestors_to_close;
  xtrace->LogLineRun(xtrace_mrid, 4);
  Node *next;
  xtrace->LogLineRun(xtrace_mrid, 5);
  Node *last_closed = nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "last_closed",
                         last_closed ? base::ToString(*last_closed) : "");

  xtrace->LogLineRun(xtrace_mrid, 6);
  for (Node *n = start_node; n && n != past_end; n = next) {
  }

  xtrace->LogLineRun(xtrace_mrid, 9);
  xtrace->FlushAllEventsToJSONFile();
  return last_closed;
}
