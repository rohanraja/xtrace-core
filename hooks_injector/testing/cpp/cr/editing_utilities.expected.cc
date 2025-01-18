#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
/*
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/editing/editing_utilities.h"

#include <array>

#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/core/clipboard/clipboard_mime_types.h"
#include "third_party/blink/renderer/core/clipboard/data_object.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_access_policy.h"
#include "third_party/blink/renderer/core/clipboard/system_clipboard.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/core/editing/commands/editing_commands_utilities.h"
#include "third_party/blink/renderer/core/editing/editing_strategy.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/ime/edit_context.h"
#include "third_party/blink/renderer/core/editing/ime/input_method_controller.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/core/editing/local_caret_rect.h"
#include "third_party/blink/renderer/core/editing/plain_text_range.h"
#include "third_party/blink/renderer/core/editing/position_iterator.h"
#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/serializers/html_interchange.h"
#include "third_party/blink/renderer/core/editing/state_machines/backspace_state_machine.h"
#include "third_party/blink/renderer/core/editing/state_machines/backward_grapheme_boundary_state_machine.h"
#include "third_party/blink/renderer/core/editing/state_machines/forward_grapheme_boundary_state_machine.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/html_select_element.h"
#include "third_party/blink/renderer/core/html/forms/html_text_area_element.h"
#include "third_party/blink/renderer/core/html/forms/text_control_element.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_br_element.h"
#include "third_party/blink/renderer/core/html/html_div_element.h"
#include "third_party/blink/renderer/core/html/html_dlist_element.h"
#include "third_party/blink/renderer/core/html/html_embed_element.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/html/html_li_element.h"
#include "third_party/blink/renderer/core/html/html_object_element.h"
#include "third_party/blink/renderer/core/html/html_olist_element.h"
#include "third_party/blink/renderer/core/html/html_paragraph_element.h"
#include "third_party/blink/renderer/core/html/html_span_element.h"
#include "third_party/blink/renderer/core/html/html_table_caption_element.h"
#include "third_party/blink/renderer/core/html/html_table_cell_element.h"
#include "third_party/blink/renderer/core/html/html_table_col_element.h"
#include "third_party/blink/renderer/core/html/html_table_element.h"
#include "third_party/blink/renderer/core/html/html_table_row_element.h"
#include "third_party/blink/renderer/core/html/html_table_section_element.h"
#include "third_party/blink/renderer/core/html/html_ulist_element.h"
#include "third_party/blink/renderer/core/html/image_document.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html_element_factory.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/svg/svg_image_element.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"

namespace blink {

using mojom::blink::FormControlType;

namespace {

std::ostream &operator<<(std::ostream &os, PositionMoveType type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "std::ostream",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "os", base::ToString(os));
  xtrace->LocalVarUpdate(xtrace_mrid, "type", base::ToString(type));
  xtrace->LogLineRun(xtrace_mrid, 108);
  static const std::array<const char *, 3> kTexts = {
      "CodeUnit", "BackwardDeletion", "GraphemeCluster"};
  xtrace->LocalVarUpdate(xtrace_mrid, "kTexts", base::ToString(kTexts));

  xtrace->LogLineRun(xtrace_mrid, 110);
  DCHECK_LT(static_cast<size_t>(type), kTexts.size())
      << "Unknown PositionMoveType value";
  xtrace->LogLineRun(xtrace_mrid, 112);
  xtrace->FlushAllEventsToJSONFile();
  return os << kTexts[static_cast<size_t>(type)];
}

UChar WhitespaceRebalancingCharToAppend(const String &string,
                                        bool start_is_start_of_paragraph,
                                        bool should_emit_nbsp_before_end,
                                        wtf_size_t index, UChar previous) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "WhitespaceRebalancingCharToAppend",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "string", base::ToString(string));
  xtrace->LocalVarUpdate(xtrace_mrid, "start_is_start_of_paragraph",
                         base::ToString(start_is_start_of_paragraph));
  xtrace->LocalVarUpdate(xtrace_mrid, "should_emit_nbsp_before_end",
                         base::ToString(should_emit_nbsp_before_end));
  xtrace->LocalVarUpdate(xtrace_mrid, "index", base::ToString(index));
  xtrace->LocalVarUpdate(xtrace_mrid, "previous", base::ToString(previous));
  xtrace->LogLineRun(xtrace_mrid, 120);
  DCHECK_LT(index, string.length());

  xtrace->LogLineRun(xtrace_mrid, 122);
  if (!IsWhitespace(string[index]))
    return string[index];

  xtrace->LogLineRun(xtrace_mrid, 125);
  if (!index && start_is_start_of_paragraph)
    return kNoBreakSpaceCharacter;
  xtrace->LogLineRun(xtrace_mrid, 127);
  if (index + 1 == string.length() && should_emit_nbsp_before_end)
    return kNoBreakSpaceCharacter;

  // Generally, alternate between space and no-break space.
  xtrace->LogLineRun(xtrace_mrid, 131);
  if (previous == ' ')
    return kNoBreakSpaceCharacter;
  xtrace->LogLineRun(xtrace_mrid, 133);
  if (previous == kNoBreakSpaceCharacter)
    return ' ';

  // Run of two or more spaces starts with a no-break space (crbug.com/453042).
  xtrace->LogLineRun(xtrace_mrid, 137);
  if (index + 1 < string.length() && IsWhitespace(string[index + 1]))
    return kNoBreakSpaceCharacter;

  xtrace->LogLineRun(xtrace_mrid, 140);
  xtrace->FlushAllEventsToJSONFile();
  return ' ';
}

} // namespace

bool NeedsLayoutTreeUpdate(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NeedsLayoutTreeUpdate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 146);
  const Document &document = node.GetDocument();
  xtrace->LogLineRun(xtrace_mrid, 147);
  if (document.NeedsLayoutTreeUpdate())
    return true;
  // TODO(yosin): We should make |document::needsLayoutTreeUpdate()| to
  // check |LayoutView::needsLayout()|.
  xtrace->LogLineRun(xtrace_mrid, 151);
  xtrace->FlushAllEventsToJSONFile();
  return document.View() && document.View()->NeedsLayout();
}

template <typename PositionType>
static bool NeedsLayoutTreeUpdateAlgorithm(const PositionType &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "NeedsLayoutTreeUpdateAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 156);
  const Node *node = position.AnchorNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 157);
  if (!node)
    return false;
  xtrace->LogLineRun(xtrace_mrid, 159);
  xtrace->FlushAllEventsToJSONFile();
  return NeedsLayoutTreeUpdate(*node);
}

bool NeedsLayoutTreeUpdate(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NeedsLayoutTreeUpdate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 163);
  xtrace->FlushAllEventsToJSONFile();
  return NeedsLayoutTreeUpdateAlgorithm<Position>(position);
}

bool NeedsLayoutTreeUpdate(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NeedsLayoutTreeUpdate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 167);
  xtrace->FlushAllEventsToJSONFile();
  return NeedsLayoutTreeUpdateAlgorithm<PositionInFlatTree>(position);
}

// Atomic means that the node has no children, or has children which are ignored
// for the purposes of editing.
bool IsAtomicNode(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsAtomicNode",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 173);
  xtrace->FlushAllEventsToJSONFile();
  return node && (!node->hasChildren() || EditingIgnoresContent(*node));
}

bool IsAtomicNodeInFlatTree(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsAtomicNodeInFlatTree",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 177);
  xtrace->FlushAllEventsToJSONFile();
  return node && (!FlatTreeTraversal::HasChildren(*node) ||
                  EditingIgnoresContent(*node));
}

bool IsNodeFullyContained(const EphemeralRange &range, const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsNodeFullyContained",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 182);
  if (range.IsNull())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 185);
  if (!NodeTraversal::CommonAncestor(*range.StartPosition().AnchorNode(), node))
    return false;

  xtrace->LogLineRun(xtrace_mrid, 188);
  xtrace->FlushAllEventsToJSONFile();
  return range.StartPosition() <= Position::BeforeNode(node) &&
         Position::AfterNode(node) <= range.EndPosition();
}

// TODO(editing-dev): We should implement real version which refers
// "user-select" CSS property.
bool IsUserSelectContain(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsUserSelectContain",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 195);
  xtrace->FlushAllEventsToJSONFile();
  return IsA<HTMLTextAreaElement>(node) || IsA<HTMLInputElement>(node) ||
         IsA<HTMLSelectElement>(node);
}

enum EditableLevel { kEditable, kRichlyEditable };
static bool HasEditableLevel(const Node &node, EditableLevel editable_level) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "HasEditableLevel",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LocalVarUpdate(xtrace_mrid, "editable_level",
                         base::ToString(editable_level));
  xtrace->LogLineRun(xtrace_mrid, 201);
  DCHECK(node.GetDocument().IsActive());
  // TODO(editing-dev): We should have this check:
  // DCHECK_GE(node.document().lifecycle().state(),
  //           DocumentLifecycle::StyleClean);
  xtrace->LogLineRun(xtrace_mrid, 205);
  if (node.IsPseudoElement())
    return false;

  // Ideally we'd call DCHECK(!needsStyleRecalc()) here, but
  // ContainerNode::setFocus() calls setNeedsStyleRecalc(), so the assertion
  // would fire in the middle of Document::setFocusedNode().

  xtrace->LogLineRun(xtrace_mrid, 212);
  for (const Node &ancestor : NodeTraversal::InclusiveAncestorsOf(node)) {
    xtrace->LogLineRun(xtrace_mrid, 213);
    if (!(ancestor.IsHTMLElement() || ancestor.IsDocumentNode()))
      continue;
    // An inert subtree should not contain any content or controls which are
    // critical to understanding or using aspects of the page which are not in
    // the inert state. Content in an inert subtree will not be perceivable by
    // all users, or interactive. See
    // https://html.spec.whatwg.org/multipage/interaction.html#the-inert-attribute.
    // To prevent the invisible inert element being overlooked, the
    // inert attribute of the element is initially assessed. See
    // https://issues.chromium.org/issues/41490809.
    xtrace->LogLineRun(xtrace_mrid, 223);
    if (RuntimeEnabledFeatures::InertElementNonEditableEnabled()) {
      xtrace->LogLineRun(xtrace_mrid, 224);
      const Element *element = DynamicTo<Element>(ancestor);
      xtrace->LogLineRun(xtrace_mrid, 225);
      if (element && element->IsInertRoot()) {
        xtrace->LogLineRun(xtrace_mrid, 226);
        return false;
      }
    }
    xtrace->LogLineRun(xtrace_mrid, 229);
    if (const ComputedStyle *style =
            GetComputedStyleForElementOrLayoutObject(ancestor)) {
      xtrace->LogLineRun(xtrace_mrid, 231);
      switch (style->UsedUserModify()) {
      case EUserModify::kReadOnly:
        xtrace->LogLineRun(xtrace_mrid, 233);
        return false;
      case EUserModify::kReadWrite:
        xtrace->LogLineRun(xtrace_mrid, 235);
        return true;
      case EUserModify::kReadWritePlaintextOnly:
        xtrace->LogLineRun(xtrace_mrid, 237);
        return editable_level != kRichlyEditable;
      }
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 242);
  xtrace->FlushAllEventsToJSONFile();
  return false;
}

bool IsEditable(const Node &node) {
  // TODO(editing-dev): We shouldn't check editable style in inactive documents.
  // We should hoist this check in the call stack, replace it by a DCHECK of
  // active document and ultimately cleanup the code paths with inactive
  // documents.  See crbug.com/667681
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsEditable",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 250);
  if (!node.GetDocument().IsActive())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 253);
  xtrace->FlushAllEventsToJSONFile();
  return HasEditableLevel(node, kEditable);
}

bool IsRichlyEditable(const Node &node) {
  // TODO(editing-dev): We shouldn't check editable style in inactive documents.
  // We should hoist this check in the call stack, replace it by a DCHECK of
  // active document and ultimately cleanup the code paths with inactive
  // documents.  See crbug.com/667681
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsRichlyEditable",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 261);
  if (!node.GetDocument().IsActive())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 264);
  xtrace->FlushAllEventsToJSONFile();
  return HasEditableLevel(node, kRichlyEditable);
}

bool IsRootEditableElement(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsRootEditableElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 268);
  xtrace->FlushAllEventsToJSONFile();
  return IsEditable(node) && node.IsElementNode() &&
         (!node.parentNode() || !IsEditable(*node.parentNode()) ||
          !node.parentNode()->IsElementNode() ||
          &node == node.GetDocument().body());
}

Element *RootEditableElement(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "RootEditableElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 275);
  const Element *result = nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "result",
                         result ? base::ToString(*result) : "");

  xtrace->LogLineRun(xtrace_mrid, 276);
  for (const Node *n = &node; n && IsEditable(*n); n = n->parentNode()) {
    xtrace->LogLineRun(xtrace_mrid, 277);
    if (auto *element = DynamicTo<Element>(n))
      result = element;
    xtrace->LogLineRun(xtrace_mrid, 279);
    if (node.GetDocument().body() == n)
      break;
  }
  xtrace->LogLineRun(xtrace_mrid, 282);
  xtrace->FlushAllEventsToJSONFile();
  return const_cast<Element *>(result);
}

ContainerNode *HighestEditableRoot(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "HighestEditableRoot",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 286);
  if (position.IsNull())
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 289);
  ContainerNode *highest_root = RootEditableElementOf(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         highest_root ? base::ToString(*highest_root) : "");

  xtrace->LogLineRun(xtrace_mrid, 290);
  if (!highest_root)
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 293);
  if (IsA<HTMLBodyElement>(*highest_root))
    return highest_root;

  xtrace->LogLineRun(xtrace_mrid, 296);
  ContainerNode *node = highest_root->parentNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 297);
  while (node) {
    xtrace->LogLineRun(xtrace_mrid, 298);
    if (IsEditable(*node))
      highest_root = node;
    xtrace->LogLineRun(xtrace_mrid, 300);
    if (IsA<HTMLBodyElement>(*node))
      break;
    xtrace->LogLineRun(xtrace_mrid, 302);
    node = node->parentNode();
  }

  xtrace->LogLineRun(xtrace_mrid, 305);
  xtrace->FlushAllEventsToJSONFile();
  return highest_root;
}

ContainerNode *HighestEditableRoot(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "HighestEditableRoot",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 309);
  xtrace->FlushAllEventsToJSONFile();
  return HighestEditableRoot(ToPositionInDOMTree(position));
}

bool IsEditablePosition(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsEditablePosition",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 313);
  const Node *node = position.ComputeContainerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 314);
  if (!node)
    return false;
  xtrace->LogLineRun(xtrace_mrid, 316);
  DCHECK(node->GetDocument().IsActive());
  xtrace->LogLineRun(xtrace_mrid, 317);
  if (node->GetDocument().Lifecycle().GetState() >=
      DocumentLifecycle::kInStyleRecalc) {
    // TODO(yosin): Update the condition and DCHECK here given that
    // https://codereview.chromium.org/2665823002/ avoided this function from
    // being called during InStyleRecalc.
  } else {
    xtrace->LogLineRun(xtrace_mrid, 323);
    DCHECK(!NeedsLayoutTreeUpdate(position)) << position;
  }

  xtrace->LogLineRun(xtrace_mrid, 326);
  if (IsDisplayInsideTable(node))
    node = node->parentNode();

  xtrace->LogLineRun(xtrace_mrid, 329);
  if (node->IsDocumentNode())
    return false;
  xtrace->LogLineRun(xtrace_mrid, 331);
  xtrace->FlushAllEventsToJSONFile();
  return IsEditable(*node);
}

bool IsEditablePosition(const PositionInFlatTree &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsEditablePosition",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 335);
  xtrace->FlushAllEventsToJSONFile();
  return IsEditablePosition(ToPositionInDOMTree(p));
}

bool IsRichlyEditablePosition(const Position &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsRichlyEditablePosition",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 339);
  const Node *node = p.AnchorNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 340);
  if (!node)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 343);
  if (IsDisplayInsideTable(node))
    node = node->parentNode();

  xtrace->LogLineRun(xtrace_mrid, 346);
  xtrace->FlushAllEventsToJSONFile();
  return IsRichlyEditable(*node);
}

Element *RootEditableElementOf(const Position &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "RootEditableElementOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 350);
  Node *node = p.ComputeContainerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 351);
  if (!node)
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 354);
  if (IsDisplayInsideTable(node))
    node = node->parentNode();

  xtrace->LogLineRun(xtrace_mrid, 357);
  xtrace->FlushAllEventsToJSONFile();
  return RootEditableElement(*node);
}

Element *RootEditableElementOf(const PositionInFlatTree &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "RootEditableElementOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 361);
  xtrace->FlushAllEventsToJSONFile();
  return RootEditableElementOf(ToPositionInDOMTree(p));
}

template <typename Strategy>
PositionTemplate<Strategy>
NextCandidateAlgorithm(const PositionTemplate<Strategy> &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextCandidateAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 367);
  TRACE_EVENT0("input", "EditingUtility::nextCandidateAlgorithm");
  xtrace->LogLineRun(xtrace_mrid, 368);
  PositionIteratorAlgorithm<Strategy> p(position);

  xtrace->LogLineRun(xtrace_mrid, 370);
  p.Increment();
  xtrace->LogLineRun(xtrace_mrid, 371);
  while (!p.AtEnd()) {
    xtrace->LogLineRun(xtrace_mrid, 372);
    PositionTemplate<Strategy> candidate = p.ComputePosition();
    xtrace->LogLineRun(xtrace_mrid, 373);
    if (IsVisuallyEquivalentCandidate(candidate))
      return candidate;

    xtrace->LogLineRun(xtrace_mrid, 376);
    p.Increment();
  }

  xtrace->LogLineRun(xtrace_mrid, 379);
  xtrace->FlushAllEventsToJSONFile();
  return PositionTemplate<Strategy>();
}

Position NextCandidate(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextCandidate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 383);
  xtrace->FlushAllEventsToJSONFile();
  return NextCandidateAlgorithm<EditingStrategy>(position);
}

PositionInFlatTree NextCandidate(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextCandidate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 387);
  xtrace->FlushAllEventsToJSONFile();
  return NextCandidateAlgorithm<EditingInFlatTreeStrategy>(position);
}

// |nextVisuallyDistinctCandidate| is similar to |nextCandidate| except
// for returning position which |downstream()| not equal to initial position's
// |downstream()|.
template <typename Strategy>
static PositionTemplate<Strategy> NextVisuallyDistinctCandidateAlgorithm(
    const PositionTemplate<Strategy> &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "NextVisuallyDistinctCandidateAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 396);
  TRACE_EVENT0("input",
               "EditingUtility::nextVisuallyDistinctCandidateAlgorithm");
  xtrace->LogLineRun(xtrace_mrid, 398);
  if (position.IsNull())
    return PositionTemplate<Strategy>();

  xtrace->LogLineRun(xtrace_mrid, 401);
  PositionIteratorAlgorithm<Strategy> p(position);
  xtrace->LogLineRun(xtrace_mrid, 402);
  const PositionTemplate<Strategy> downstream_start =
      MostForwardCaretPosition(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "downstream_start",
                         base::ToString(downstream_start));

  xtrace->LogLineRun(xtrace_mrid, 404);
  const PositionTemplate<Strategy> upstream_start =
      MostBackwardCaretPosition(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "upstream_start",
                         base::ToString(upstream_start));

  xtrace->LogLineRun(xtrace_mrid, 407);
  p.Increment();
  xtrace->LogLineRun(xtrace_mrid, 408);
  while (!p.AtEnd()) {
    xtrace->LogLineRun(xtrace_mrid, 409);
    PositionTemplate<Strategy> candidate = p.ComputePosition();
    xtrace->LogLineRun(xtrace_mrid, 410);
    if (IsVisuallyEquivalentCandidate(candidate) &&
        MostForwardCaretPosition(candidate) != downstream_start &&
        MostBackwardCaretPosition(candidate) != upstream_start)
      return candidate;

    xtrace->LogLineRun(xtrace_mrid, 415);
    p.Increment();
  }

  xtrace->LogLineRun(xtrace_mrid, 418);
  xtrace->FlushAllEventsToJSONFile();
  return PositionTemplate<Strategy>();
}

Position NextVisuallyDistinctCandidate(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "NextVisuallyDistinctCandidate",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 422);
  xtrace->FlushAllEventsToJSONFile();
  return NextVisuallyDistinctCandidateAlgorithm<EditingStrategy>(position);
}

PositionInFlatTree
NextVisuallyDistinctCandidate(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "NextVisuallyDistinctCandidate",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 427);
  xtrace->FlushAllEventsToJSONFile();
  return NextVisuallyDistinctCandidateAlgorithm<EditingInFlatTreeStrategy>(
      position);
}

template <typename Strategy>
PositionTemplate<Strategy>
PreviousCandidateAlgorithm(const PositionTemplate<Strategy> &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousCandidateAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 434);
  TRACE_EVENT0("input", "EditingUtility::previousCandidateAlgorithm");
  xtrace->LogLineRun(xtrace_mrid, 435);
  PositionIteratorAlgorithm<Strategy> p(position);

  xtrace->LogLineRun(xtrace_mrid, 437);
  p.Decrement();
  xtrace->LogLineRun(xtrace_mrid, 438);
  while (!p.AtStart()) {
    xtrace->LogLineRun(xtrace_mrid, 439);
    PositionTemplate<Strategy> candidate = p.ComputePosition();
    xtrace->LogLineRun(xtrace_mrid, 440);
    if (IsVisuallyEquivalentCandidate(candidate))
      return candidate;

    xtrace->LogLineRun(xtrace_mrid, 443);
    p.Decrement();
  }

  xtrace->LogLineRun(xtrace_mrid, 446);
  xtrace->FlushAllEventsToJSONFile();
  return PositionTemplate<Strategy>();
}

Position PreviousCandidate(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PreviousCandidate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 450);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousCandidateAlgorithm<EditingStrategy>(position);
}

PositionInFlatTree PreviousCandidate(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PreviousCandidate",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 454);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousCandidateAlgorithm<EditingInFlatTreeStrategy>(position);
}

// |previousVisuallyDistinctCandidate| is similar to |previousCandidate| except
// for returning position which |downstream()| not equal to initial position's
// |downstream()|.
template <typename Strategy>
PositionTemplate<Strategy> PreviousVisuallyDistinctCandidateAlgorithm(
    const PositionTemplate<Strategy> &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousVisuallyDistinctCandidateAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 463);
  TRACE_EVENT0("input",
               "EditingUtility::previousVisuallyDistinctCandidateAlgorithm");
  xtrace->LogLineRun(xtrace_mrid, 465);
  if (position.IsNull())
    return PositionTemplate<Strategy>();

  xtrace->LogLineRun(xtrace_mrid, 468);
  PositionIteratorAlgorithm<Strategy> p(position);
  xtrace->LogLineRun(xtrace_mrid, 469);
  PositionTemplate<Strategy> downstream_start =
      MostForwardCaretPosition(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "downstream_start",
                         base::ToString(downstream_start));

  xtrace->LogLineRun(xtrace_mrid, 471);
  const PositionTemplate<Strategy> upstream_start =
      MostBackwardCaretPosition(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "upstream_start",
                         base::ToString(upstream_start));

  xtrace->LogLineRun(xtrace_mrid, 474);
  p.Decrement();
  xtrace->LogLineRun(xtrace_mrid, 475);
  while (!p.AtStart()) {
    xtrace->LogLineRun(xtrace_mrid, 476);
    PositionTemplate<Strategy> candidate = p.ComputePosition();
    xtrace->LogLineRun(xtrace_mrid, 477);
    if (IsVisuallyEquivalentCandidate(candidate) &&
        MostForwardCaretPosition(candidate) != downstream_start &&
        MostBackwardCaretPosition(candidate) != upstream_start)
      return candidate;

    xtrace->LogLineRun(xtrace_mrid, 482);
    p.Decrement();
  }

  xtrace->LogLineRun(xtrace_mrid, 485);
  xtrace->FlushAllEventsToJSONFile();
  return PositionTemplate<Strategy>();
}

Position PreviousVisuallyDistinctCandidate(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousVisuallyDistinctCandidate",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 489);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousVisuallyDistinctCandidateAlgorithm<EditingStrategy>(position);
}

PositionInFlatTree
PreviousVisuallyDistinctCandidate(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousVisuallyDistinctCandidate",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 494);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousVisuallyDistinctCandidateAlgorithm<EditingInFlatTreeStrategy>(
      position);
}

template <typename Strategy>
PositionTemplate<Strategy> FirstEditablePositionAfterPositionInRootAlgorithm(
    const PositionTemplate<Strategy> &position, const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc",
                            "FirstEditablePositionAfterPositionInRootAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 502);
  DCHECK(!NeedsLayoutTreeUpdate(highest_root))
      << position << ' ' << highest_root;
  // position falls before highestRoot.
  xtrace->LogLineRun(xtrace_mrid, 505);
  if (position.CompareTo(PositionTemplate<Strategy>::FirstPositionInNode(
          highest_root)) == -1 &&
      IsEditable(highest_root))
    return PositionTemplate<Strategy>::FirstPositionInNode(highest_root);

  xtrace->LogLineRun(xtrace_mrid, 510);
  PositionTemplate<Strategy> editable_position = position;
  xtrace->LocalVarUpdate(xtrace_mrid, "editable_position",
                         base::ToString(editable_position));

  xtrace->LogLineRun(xtrace_mrid, 512);
  if (position.AnchorNode()->GetTreeScope() != highest_root.GetTreeScope()) {
    xtrace->LogLineRun(xtrace_mrid, 513);
    Node *shadow_ancestor = highest_root.GetTreeScope().AncestorInThisScope(
        editable_position.AnchorNode());
    xtrace->LogLineRun(xtrace_mrid, 515);
    if (!shadow_ancestor)
      return PositionTemplate<Strategy>();

    xtrace->LogLineRun(xtrace_mrid, 518);
    editable_position = PositionTemplate<Strategy>::AfterNode(*shadow_ancestor);
  }

  xtrace->LogLineRun(xtrace_mrid, 521);
  Node *non_editable_node = nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "non_editable_node",
                         non_editable_node ? base::ToString(*non_editable_node)
                                           : "");

  xtrace->LogLineRun(xtrace_mrid, 522);
  while (editable_position.AnchorNode() &&
         !IsEditablePosition(editable_position) &&
         editable_position.AnchorNode()->IsDescendantOf(&highest_root)) {
    xtrace->LogLineRun(xtrace_mrid, 525);
    non_editable_node = editable_position.AnchorNode();
    xtrace->LogLineRun(xtrace_mrid, 526);
    editable_position = IsAtomicNode(editable_position.AnchorNode())
                            ? PositionTemplate<Strategy>::InParentAfterNode(
                                  *editable_position.AnchorNode())
                            : NextVisuallyDistinctCandidate(editable_position);
  }

  xtrace->LogLineRun(xtrace_mrid, 532);
  if (editable_position.AnchorNode() &&
      editable_position.AnchorNode() != &highest_root &&
      !editable_position.AnchorNode()->IsDescendantOf(&highest_root))
    return PositionTemplate<Strategy>();

  // If `non_editable_node` is the last child of
  // `editable_position.AnchorNode()`, obtain the next sibling position.
  // - If we do not obtain the next sibling position, we will be unable to
  //   access the next paragraph within the `InsertListCommand::DoApply` while
  //   loop. See http://crbug.com/571420 for more details.
  // - If `non_editable_node` is not the last child, we will bypass the next
  //   editable sibling position. See http://crbug.com/1334557 for more details.
  xtrace->LogLineRun(xtrace_mrid, 544);
  bool need_obtain_next =
      non_editable_node && editable_position.AnchorNode() &&
      non_editable_node == editable_position.AnchorNode()->lastChild();
  xtrace->LocalVarUpdate(xtrace_mrid, "need_obtain_next",
                         base::ToString(need_obtain_next));

  xtrace->LogLineRun(xtrace_mrid, 547);
  if (need_obtain_next) {
    // Make sure not to move out of |highest_root|
    xtrace->LogLineRun(xtrace_mrid, 549);
    const PositionTemplate<Strategy> boundary =
        PositionTemplate<Strategy>::LastPositionInNode(highest_root);
    // `NextVisuallyDistinctCandidate` is similar to `NextCandidate`, but
    // it skips the next visually equivalent of `editable_position`.
    // `editable_position` is already "visually distinct" relative to
    // `position`, so use `NextCandidate` here.
    // See http://crbug.com/1406207 for more details.
    xtrace->LogLineRun(xtrace_mrid, 556);
    const PositionTemplate<Strategy> next_candidate =
        NextCandidate(editable_position);
    xtrace->LogLineRun(xtrace_mrid, 558);
    editable_position = next_candidate.IsNotNull()
                            ? std::min(boundary, next_candidate)
                            : boundary;
  }
  xtrace->LogLineRun(xtrace_mrid, 562);
  xtrace->FlushAllEventsToJSONFile();
  return editable_position;
}

Position FirstEditablePositionAfterPositionInRoot(const Position &position,
                                                  const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "FirstEditablePositionAfterPositionInRoot",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 567);
  xtrace->FlushAllEventsToJSONFile();
  return FirstEditablePositionAfterPositionInRootAlgorithm<EditingStrategy>(
      position, highest_root);
}

PositionInFlatTree
FirstEditablePositionAfterPositionInRoot(const PositionInFlatTree &position,
                                         const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "FirstEditablePositionAfterPositionInRoot",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 574);
  xtrace->FlushAllEventsToJSONFile();
  return FirstEditablePositionAfterPositionInRootAlgorithm<
      EditingInFlatTreeStrategy>(position, highest_root);
}

template <typename Strategy>
PositionTemplate<Strategy> LastEditablePositionBeforePositionInRootAlgorithm(
    const PositionTemplate<Strategy> &position, const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc",
                            "LastEditablePositionBeforePositionInRootAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 582);
  DCHECK(!NeedsLayoutTreeUpdate(highest_root))
      << position << ' ' << highest_root;
  // When position falls after highestRoot, the result is easy to compute.
  xtrace->LogLineRun(xtrace_mrid, 585);
  if (position.CompareTo(
          PositionTemplate<Strategy>::LastPositionInNode(highest_root)) == 1)
    return PositionTemplate<Strategy>::LastPositionInNode(highest_root);

  xtrace->LogLineRun(xtrace_mrid, 589);
  PositionTemplate<Strategy> editable_position = position;
  xtrace->LocalVarUpdate(xtrace_mrid, "editable_position",
                         base::ToString(editable_position));

  xtrace->LogLineRun(xtrace_mrid, 591);
  if (position.AnchorNode()->GetTreeScope() != highest_root.GetTreeScope()) {
    xtrace->LogLineRun(xtrace_mrid, 592);
    Node *shadow_ancestor = highest_root.GetTreeScope().AncestorInThisScope(
        editable_position.AnchorNode());
    xtrace->LogLineRun(xtrace_mrid, 594);
    if (!shadow_ancestor)
      return PositionTemplate<Strategy>();

    xtrace->LogLineRun(xtrace_mrid, 597);
    editable_position = PositionTemplate<Strategy>::FirstPositionInOrBeforeNode(
        *shadow_ancestor);
  }

  xtrace->LogLineRun(xtrace_mrid, 601);
  while (editable_position.AnchorNode() &&
         !IsEditablePosition(editable_position) &&
         editable_position.AnchorNode()->IsDescendantOf(&highest_root))
    editable_position =
        IsAtomicNode(editable_position.AnchorNode())
            ? PositionTemplate<Strategy>::InParentBeforeNode(
                  *editable_position.AnchorNode())
            : PreviousVisuallyDistinctCandidate(editable_position);

  xtrace->LogLineRun(xtrace_mrid, 610);
  if (editable_position.AnchorNode() &&
      editable_position.AnchorNode() != &highest_root &&
      !editable_position.AnchorNode()->IsDescendantOf(&highest_root))
    return PositionTemplate<Strategy>();
  xtrace->LogLineRun(xtrace_mrid, 614);
  xtrace->FlushAllEventsToJSONFile();
  return editable_position;
}

Position LastEditablePositionBeforePositionInRoot(const Position &position,
                                                  const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "LastEditablePositionBeforePositionInRoot",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 619);
  xtrace->FlushAllEventsToJSONFile();
  return LastEditablePositionBeforePositionInRootAlgorithm<EditingStrategy>(
      position, highest_root);
}

PositionInFlatTree
LastEditablePositionBeforePositionInRoot(const PositionInFlatTree &position,
                                         const Node &highest_root) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "LastEditablePositionBeforePositionInRoot",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "highest_root",
                         base::ToString(highest_root));
  xtrace->LogLineRun(xtrace_mrid, 626);
  xtrace->FlushAllEventsToJSONFile();
  return LastEditablePositionBeforePositionInRootAlgorithm<
      EditingInFlatTreeStrategy>(position, highest_root);
}

template <typename StateMachine>
int FindNextBoundaryOffset(const String &str, int current) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "FindNextBoundaryOffset",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "str", base::ToString(str));
  xtrace->LocalVarUpdate(xtrace_mrid, "current", base::ToString(current));
  xtrace->LogLineRun(xtrace_mrid, 632);
  StateMachine machine;
  xtrace->LogLineRun(xtrace_mrid, 633);
  TextSegmentationMachineState state = TextSegmentationMachineState::kInvalid;
  xtrace->LocalVarUpdate(xtrace_mrid, "state", base::ToString(state));

  xtrace->LogLineRun(xtrace_mrid, 635);
  for (int i = current - 1; i >= 0; --i) {
    xtrace->LogLineRun(xtrace_mrid, 636);
    state = machine.FeedPrecedingCodeUnit(str[i]);
    xtrace->LogLineRun(xtrace_mrid, 637);
    if (state != TextSegmentationMachineState::kNeedMoreCodeUnit)
      break;
  }
  xtrace->LogLineRun(xtrace_mrid, 640);
  if (current == 0 || state == TextSegmentationMachineState::kNeedMoreCodeUnit)
    state = machine.TellEndOfPrecedingText();
  xtrace->LogLineRun(xtrace_mrid, 642);
  if (state == TextSegmentationMachineState::kFinished)
    return current + machine.FinalizeAndGetBoundaryOffset();
  xtrace->LogLineRun(xtrace_mrid, 644);
  const int length = str.length();
  xtrace->LocalVarUpdate(xtrace_mrid, "length", base::ToString(length));

  xtrace->LogLineRun(xtrace_mrid, 645);
  DCHECK_EQ(TextSegmentationMachineState::kNeedFollowingCodeUnit, state);
  xtrace->LogLineRun(xtrace_mrid, 646);
  for (int i = current; i < length; ++i) {
    xtrace->LogLineRun(xtrace_mrid, 647);
    state = machine.FeedFollowingCodeUnit(str[i]);
    xtrace->LogLineRun(xtrace_mrid, 648);
    if (state != TextSegmentationMachineState::kNeedMoreCodeUnit)
      break;
  }
  xtrace->LogLineRun(xtrace_mrid, 651);
  xtrace->FlushAllEventsToJSONFile();
  return current + machine.FinalizeAndGetBoundaryOffset();
}

// Explicit instantiation to avoid link error for the usage in EditContext.
template int
FindNextBoundaryOffset<BackwardGraphemeBoundaryStateMachine>(const String &str,
                                                             int current);
template int
FindNextBoundaryOffset<ForwardGraphemeBoundaryStateMachine>(const String &str,
                                                            int current);

int PreviousGraphemeBoundaryOf(const Node &node, int current) {
  // TODO(yosin): Need to support grapheme crossing |Node| boundary.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousGraphemeBoundaryOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LocalVarUpdate(xtrace_mrid, "current", base::ToString(current));
  xtrace->LogLineRun(xtrace_mrid, 664);
  DCHECK_GE(current, 0);
  xtrace->LogLineRun(xtrace_mrid, 665);
  auto *text_node = DynamicTo<Text>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "text_node",
                         text_node ? base::ToString(*text_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 666);
  if (current <= 1 || !text_node)
    return current - 1;
  xtrace->LogLineRun(xtrace_mrid, 668);
  const String &text = text_node->data();
  // TODO(yosin): Replace with DCHECK for out-of-range request.
  xtrace->LogLineRun(xtrace_mrid, 670);
  if (static_cast<unsigned>(current) > text.length())
    return current - 1;
  xtrace->LogLineRun(xtrace_mrid, 672);
  xtrace->FlushAllEventsToJSONFile();
  return FindNextBoundaryOffset<BackwardGraphemeBoundaryStateMachine>(text,
                                                                      current);
}

static int PreviousBackwardDeletionOffsetOf(const Node &node, int current) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousBackwardDeletionOffsetOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LocalVarUpdate(xtrace_mrid, "current", base::ToString(current));
  xtrace->LogLineRun(xtrace_mrid, 677);
  DCHECK_GE(current, 0);
  xtrace->LogLineRun(xtrace_mrid, 678);
  if (current <= 1)
    return 0;
  xtrace->LogLineRun(xtrace_mrid, 680);
  auto *text_node = DynamicTo<Text>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "text_node",
                         text_node ? base::ToString(*text_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 681);
  if (!text_node)
    return current - 1;

  xtrace->LogLineRun(xtrace_mrid, 684);
  const String &text = text_node->data();
  xtrace->LogLineRun(xtrace_mrid, 685);
  DCHECK_LT(static_cast<unsigned>(current - 1), text.length());
  xtrace->LogLineRun(xtrace_mrid, 686);
  xtrace->FlushAllEventsToJSONFile();
  return FindNextBoundaryOffset<BackspaceStateMachine>(text, current);
}

int NextGraphemeBoundaryOf(const Node &node, int current) {
  // TODO(yosin): Need to support grapheme crossing |Node| boundary.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextGraphemeBoundaryOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LocalVarUpdate(xtrace_mrid, "current", base::ToString(current));
  xtrace->LogLineRun(xtrace_mrid, 691);
  auto *text_node = DynamicTo<Text>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "text_node",
                         text_node ? base::ToString(*text_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 692);
  if (!text_node)
    return current + 1;
  xtrace->LogLineRun(xtrace_mrid, 694);
  const String &text = text_node->data();
  xtrace->LogLineRun(xtrace_mrid, 695);
  const int length = text.length();
  xtrace->LocalVarUpdate(xtrace_mrid, "length", base::ToString(length));

  xtrace->LogLineRun(xtrace_mrid, 696);
  DCHECK_LE(current, length);
  xtrace->LogLineRun(xtrace_mrid, 697);
  if (current >= length - 1)
    return current + 1;
  xtrace->LogLineRun(xtrace_mrid, 699);
  xtrace->FlushAllEventsToJSONFile();
  return FindNextBoundaryOffset<ForwardGraphemeBoundaryStateMachine>(text,
                                                                     current);
}

template <typename Strategy>
PositionTemplate<Strategy>
PreviousPositionOfAlgorithm(const PositionTemplate<Strategy> &position,
                            PositionMoveType move_type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PreviousPositionOfAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 707);
  Node *const node = position.AnchorNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 708);
  if (!node)
    return position;

  xtrace->LogLineRun(xtrace_mrid, 711);
  const int offset = position.ComputeEditingOffset();
  xtrace->LocalVarUpdate(xtrace_mrid, "offset", base::ToString(offset));

  xtrace->LogLineRun(xtrace_mrid, 713);
  if (offset > 0) {
    xtrace->LogLineRun(xtrace_mrid, 714);
    if (EditingIgnoresContent(*node))
      return PositionTemplate<Strategy>::BeforeNode(*node);
    xtrace->LogLineRun(xtrace_mrid, 716);
    if (Node *child = Strategy::ChildAt(*node, offset - 1)) {
      xtrace->LogLineRun(xtrace_mrid, 717);
      return PositionTemplate<Strategy>::LastPositionInOrAfterNode(*child);
    }

    // There are two reasons child might be 0:
    //   1) The node is node like a text node that is not an element, and
    //      therefore has no children. Going backward one character at a
    //      time is correct.
    //   2) The old offset was a bogus offset like (<br>, 1), and there is
    //      no child. Going from 1 to 0 is correct.
    xtrace->LogLineRun(xtrace_mrid, 726);
    switch (move_type) {
    case PositionMoveType::kCodeUnit:
      xtrace->LogLineRun(xtrace_mrid, 728);
      return PositionTemplate<Strategy>(node, offset - 1);
    case PositionMoveType::kBackwardDeletion:
      xtrace->LogLineRun(xtrace_mrid, 730);
      return PositionTemplate<Strategy>(
          node, PreviousBackwardDeletionOffsetOf(*node, offset));
    case PositionMoveType::kGraphemeCluster:
      xtrace->LogLineRun(xtrace_mrid, 733);
      return PositionTemplate<Strategy>(
          node, PreviousGraphemeBoundaryOf(*node, offset));
    default:
      xtrace->LogLineRun(xtrace_mrid, 736);
      NOTREACHED() << "Unhandled moveType: " << move_type;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 740);
  if (ContainerNode *parent = Strategy::Parent(*node)) {
    xtrace->LogLineRun(xtrace_mrid, 741);
    if (EditingIgnoresContent(*parent))
      return PositionTemplate<Strategy>::BeforeNode(*parent);
    // TODO(yosin) We should use |Strategy::index(Node&)| instead of
    // |Node::nodeIndex()|.
    xtrace->LogLineRun(xtrace_mrid, 745);
    return PositionTemplate<Strategy>(parent, node->NodeIndex());
  }
  xtrace->LogLineRun(xtrace_mrid, 747);
  xtrace->FlushAllEventsToJSONFile();
  return position;
}

Position PreviousPositionOf(const Position &position,
                            PositionMoveType move_type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PreviousPositionOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 752);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousPositionOfAlgorithm<EditingStrategy>(position, move_type);
}

PositionInFlatTree PreviousPositionOf(const PositionInFlatTree &position,
                                      PositionMoveType move_type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PreviousPositionOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 757);
  xtrace->FlushAllEventsToJSONFile();
  return PreviousPositionOfAlgorithm<EditingInFlatTreeStrategy>(position,
                                                                move_type);
}

template <typename Strategy>
PositionTemplate<Strategy>
NextPositionOfAlgorithm(const PositionTemplate<Strategy> &position,
                        PositionMoveType move_type) {
  // TODO(yosin): We should have printer for PositionMoveType.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextPositionOfAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 766);
  DCHECK(move_type != PositionMoveType::kBackwardDeletion);

  xtrace->LogLineRun(xtrace_mrid, 768);
  Node *node = position.AnchorNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 769);
  if (!node)
    return position;

  xtrace->LogLineRun(xtrace_mrid, 772);
  const int offset = position.ComputeEditingOffset();
  xtrace->LocalVarUpdate(xtrace_mrid, "offset", base::ToString(offset));

  xtrace->LogLineRun(xtrace_mrid, 774);
  if (Node *child = Strategy::ChildAt(*node, offset)) {
    xtrace->LogLineRun(xtrace_mrid, 775);
    return PositionTemplate<Strategy>::FirstPositionInOrBeforeNode(*child);
  }

  // TODO(yosin) We should use |Strategy::lastOffsetForEditing()| instead of
  // DOM tree version.
  xtrace->LogLineRun(xtrace_mrid, 780);
  if (!Strategy::HasChildren(*node) &&
      offset < EditingStrategy::LastOffsetForEditing(node)) {
    // There are two reasons child might be 0:
    //   1) The node is node like a text node that is not an element, and
    //      therefore has no children. Going forward one character at a time
    //      is correct.
    //   2) The new offset is a bogus offset like (<br>, 1), and there is no
    //      child. Going from 0 to 1 is correct.
    xtrace->LogLineRun(xtrace_mrid, 788);
    switch (move_type) {
    case PositionMoveType::kCodeUnit:
      xtrace->LogLineRun(xtrace_mrid, 790);
      return PositionTemplate<Strategy>::EditingPositionOf(node, offset + 1);
    case PositionMoveType::kBackwardDeletion:
      xtrace->LogLineRun(xtrace_mrid, 792);
      NOTREACHED() << "BackwardDeletion is only available for prevPositionOf "
                   << "functions.";
    case PositionMoveType::kGraphemeCluster:
      xtrace->LogLineRun(xtrace_mrid, 795);
      return PositionTemplate<Strategy>::EditingPositionOf(
          node, NextGraphemeBoundaryOf(*node, offset));
    default:
      xtrace->LogLineRun(xtrace_mrid, 798);
      NOTREACHED() << "Unhandled moveType: " << move_type;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 802);
  if (ContainerNode *parent = Strategy::Parent(*node))
    return PositionTemplate<Strategy>::EditingPositionOf(
        parent, Strategy::Index(*node) + 1);
  xtrace->LogLineRun(xtrace_mrid, 805);
  xtrace->FlushAllEventsToJSONFile();
  return position;
}

Position NextPositionOf(const Position &position, PositionMoveType move_type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextPositionOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 809);
  xtrace->FlushAllEventsToJSONFile();
  return NextPositionOfAlgorithm<EditingStrategy>(position, move_type);
}

PositionInFlatTree NextPositionOf(const PositionInFlatTree &position,
                                  PositionMoveType move_type) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NextPositionOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "move_type", base::ToString(move_type));
  xtrace->LogLineRun(xtrace_mrid, 814);
  xtrace->FlushAllEventsToJSONFile();
  return NextPositionOfAlgorithm<EditingInFlatTreeStrategy>(position,
                                                            move_type);
}

bool IsEnclosingBlock(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsEnclosingBlock",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 819);
  xtrace->FlushAllEventsToJSONFile();
  return node && node->GetLayoutObject() &&
         !node->GetLayoutObject()->IsInline();
}

// TODO(yosin) Deploy this in all of the places where |enclosingBlockFlow()| and
// |enclosingBlockFlowOrTableElement()| are used.
// TODO(yosin) Callers of |Node| version of |enclosingBlock()| should use
// |Position| version The enclosing block of [table, x] for example, should be
// the block that contains the table and not the table, and this function should
// be the only one responsible for knowing about these kinds of special cases.
Element *EnclosingBlock(const Node *node, EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingBlock",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 830);
  if (!node)
    return nullptr;
  xtrace->LogLineRun(xtrace_mrid, 832);
  xtrace->FlushAllEventsToJSONFile();
  return EnclosingBlock(FirstPositionInOrBeforeNode(*node), rule);
}

template <typename Strategy>
Element *EnclosingBlockAlgorithm(const PositionTemplate<Strategy> &position,
                                 EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingBlockAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 838);
  Node *enclosing_node = EnclosingNodeOfType(position, IsEnclosingBlock, rule);
  xtrace->LocalVarUpdate(xtrace_mrid, "enclosing_node",
                         enclosing_node ? base::ToString(*enclosing_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 839);
  xtrace->FlushAllEventsToJSONFile();
  return DynamicTo<Element>(enclosing_node);
}

Element *EnclosingBlock(const Position &position,
                        EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingBlock",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 844);
  xtrace->FlushAllEventsToJSONFile();
  return EnclosingBlockAlgorithm<EditingStrategy>(position, rule);
}

Element *EnclosingBlock(const PositionInFlatTree &position,
                        EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingBlock",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 849);
  xtrace->FlushAllEventsToJSONFile();
  return EnclosingBlockAlgorithm<EditingInFlatTreeStrategy>(position, rule);
}

Element *EnclosingBlockFlowElement(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingBlockFlowElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 853);
  if (IsBlockFlowElement(node))
    return const_cast<Element *>(To<Element>(&node));

  xtrace->LogLineRun(xtrace_mrid, 856);
  for (Node &runner : NodeTraversal::AncestorsOf(node)) {
    xtrace->LogLineRun(xtrace_mrid, 857);
    if (IsBlockFlowElement(runner) || IsA<HTMLBodyElement>(runner))
      return To<Element>(&runner);
  }
  xtrace->LogLineRun(xtrace_mrid, 860);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

template <typename Strategy>
TextDirection DirectionOfEnclosingBlockOfAlgorithm(
    const PositionTemplate<Strategy> &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DirectionOfEnclosingBlockOfAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 866);
  DCHECK(position.IsNotNull());
  xtrace->LogLineRun(xtrace_mrid, 867);
  Element *enclosing_block_element =
      EnclosingBlock(PositionTemplate<Strategy>::FirstPositionInOrBeforeNode(
                         *position.ComputeContainerNode()),
                     kCannotCrossEditingBoundary);
  xtrace->LocalVarUpdate(
      xtrace_mrid, "enclosing_block_element",
      enclosing_block_element ? base::ToString(*enclosing_block_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 871);
  if (!enclosing_block_element)
    return TextDirection::kLtr;
  xtrace->LogLineRun(xtrace_mrid, 873);
  LayoutObject *layout_object = enclosing_block_element->GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_object",
                         layout_object ? base::ToString(*layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 874);
  xtrace->FlushAllEventsToJSONFile();
  return layout_object ? layout_object->Style()->Direction()
                       : TextDirection::kLtr;
}

TextDirection DirectionOfEnclosingBlockOf(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DirectionOfEnclosingBlockOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 879);
  xtrace->FlushAllEventsToJSONFile();
  return DirectionOfEnclosingBlockOfAlgorithm<EditingStrategy>(position);
}

TextDirection DirectionOfEnclosingBlockOf(const PositionInFlatTree &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DirectionOfEnclosingBlockOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 883);
  xtrace->FlushAllEventsToJSONFile();
  return DirectionOfEnclosingBlockOfAlgorithm<EditingInFlatTreeStrategy>(
      position);
}

TextDirection PrimaryDirectionOf(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PrimaryDirectionOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 888);
  TextDirection primary_direction = TextDirection::kLtr;
  xtrace->LocalVarUpdate(xtrace_mrid, "primary_direction",
                         base::ToString(primary_direction));

  xtrace->LogLineRun(xtrace_mrid, 889);
  for (const LayoutObject *r = node.GetLayoutObject(); r; r = r->Parent()) {
    xtrace->LogLineRun(xtrace_mrid, 890);
    if (r->IsLayoutBlockFlow()) {
      xtrace->LogLineRun(xtrace_mrid, 891);
      primary_direction = r->Style()->Direction();
      xtrace->LogLineRun(xtrace_mrid, 892);
      break;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 896);
  xtrace->FlushAllEventsToJSONFile();
  return primary_direction;
}

const ComputedStyle *
GetComputedStyleForElementOrLayoutObject(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "GetComputedStyleForElementOrLayoutObject",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 901);
  if (const auto *element = DynamicTo<Element>(node)) {
    xtrace->LogLineRun(xtrace_mrid, 902);
    return element->GetComputedStyle();
  }
  // Text nodes and Document.
  xtrace->LogLineRun(xtrace_mrid, 905);
  if (LayoutObject *layout_object = node.GetLayoutObject()) {
    xtrace->LogLineRun(xtrace_mrid, 906);
    return layout_object->Style();
  }
  xtrace->LogLineRun(xtrace_mrid, 908);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

String StringWithRebalancedWhitespace(const String &string,
                                      bool start_is_start_of_paragraph,
                                      bool should_emit_nbs_pbefore_end) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "StringWithRebalancedWhitespace",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "string", base::ToString(string));
  xtrace->LocalVarUpdate(xtrace_mrid, "start_is_start_of_paragraph",
                         base::ToString(start_is_start_of_paragraph));
  xtrace->LocalVarUpdate(xtrace_mrid, "should_emit_nbs_pbefore_end",
                         base::ToString(should_emit_nbs_pbefore_end));
  xtrace->LogLineRun(xtrace_mrid, 914);
  unsigned length = string.length();
  xtrace->LocalVarUpdate(xtrace_mrid, "length", base::ToString(length));

  xtrace->LogLineRun(xtrace_mrid, 916);
  StringBuilder rebalanced_string;
  xtrace->LogLineRun(xtrace_mrid, 917);
  rebalanced_string.ReserveCapacity(length);

  xtrace->LogLineRun(xtrace_mrid, 919);
  UChar char_to_append = 0;
  xtrace->LocalVarUpdate(xtrace_mrid, "char_to_append",
                         base::ToString(char_to_append));

  xtrace->LogLineRun(xtrace_mrid, 920);
  for (wtf_size_t index = 0; index < length; index++) {
    xtrace->LogLineRun(xtrace_mrid, 921);
    char_to_append = WhitespaceRebalancingCharToAppend(
        string, start_is_start_of_paragraph, should_emit_nbs_pbefore_end, index,
        char_to_append);
    xtrace->LogLineRun(xtrace_mrid, 924);
    rebalanced_string.Append(char_to_append);
  }

  xtrace->LogLineRun(xtrace_mrid, 927);
  DCHECK_EQ(rebalanced_string.length(), length);

  xtrace->LogLineRun(xtrace_mrid, 929);
  xtrace->FlushAllEventsToJSONFile();
  return rebalanced_string.ToString();
}

String RepeatString(const String &string, unsigned count) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "RepeatString",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "string", base::ToString(string));
  xtrace->LocalVarUpdate(xtrace_mrid, "count", base::ToString(count));
  xtrace->LogLineRun(xtrace_mrid, 933);
  StringBuilder builder;
  xtrace->LogLineRun(xtrace_mrid, 934);
  builder.ReserveCapacity(string.length() * count);
  xtrace->LogLineRun(xtrace_mrid, 935);
  for (unsigned counter = 0; counter < count; ++counter)
    builder.Append(string);
  xtrace->LogLineRun(xtrace_mrid, 937);
  xtrace->FlushAllEventsToJSONFile();
  return builder.ToString();
}

template <typename Strategy>
static Element *TableElementJustBeforeAlgorithm(
    const VisiblePositionTemplate<Strategy> &visible_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "TableElementJustBeforeAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_position",
                         base::ToString(visible_position));
  xtrace->LogLineRun(xtrace_mrid, 943);
  const PositionTemplate<Strategy> upstream(
      MostBackwardCaretPosition(visible_position.DeepEquivalent()));
  xtrace->LocalVarUpdate(xtrace_mrid, "upstream", base::ToString(upstream));

  xtrace->LogLineRun(xtrace_mrid, 945);
  if (IsDisplayInsideTable(upstream.AnchorNode()) &&
      upstream.AtLastEditingPositionForNode())
    return To<Element>(upstream.AnchorNode());

  xtrace->LogLineRun(xtrace_mrid, 949);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

Element *TableElementJustBefore(const VisiblePosition &visible_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "TableElementJustBefore",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_position",
                         base::ToString(visible_position));
  xtrace->LogLineRun(xtrace_mrid, 953);
  xtrace->FlushAllEventsToJSONFile();
  return TableElementJustBeforeAlgorithm<EditingStrategy>(visible_position);
}

Element *
TableElementJustBefore(const VisiblePositionInFlatTree &visible_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "TableElementJustBefore",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_position",
                         base::ToString(visible_position));
  xtrace->LogLineRun(xtrace_mrid, 958);
  xtrace->FlushAllEventsToJSONFile();
  return TableElementJustBeforeAlgorithm<EditingInFlatTreeStrategy>(
      visible_position);
}

Element *EnclosingTableCell(const Position &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingTableCell",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 963);
  xtrace->FlushAllEventsToJSONFile();
  return To<Element>(EnclosingNodeOfType(p, IsTableCell));
}
Element *EnclosingTableCell(const PositionInFlatTree &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingTableCell",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 966);
  xtrace->FlushAllEventsToJSONFile();
  return To<Element>(EnclosingNodeOfType(p, IsTableCell));
}

Element *TableElementJustAfter(const VisiblePosition &visible_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "TableElementJustAfter",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_position",
                         base::ToString(visible_position));
  xtrace->LogLineRun(xtrace_mrid, 970);
  Position downstream(
      MostForwardCaretPosition(visible_position.DeepEquivalent()));
  xtrace->LocalVarUpdate(xtrace_mrid, "downstream", base::ToString(downstream));

  xtrace->LogLineRun(xtrace_mrid, 972);
  if (IsDisplayInsideTable(downstream.AnchorNode()) &&
      downstream.AtFirstEditingPositionForNode())
    return To<Element>(downstream.AnchorNode());

  xtrace->LogLineRun(xtrace_mrid, 976);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

// Returns the position at the beginning of a node
Position PositionBeforeNode(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PositionBeforeNode",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 981);
  DCHECK(!NeedsLayoutTreeUpdate(node));
  xtrace->LogLineRun(xtrace_mrid, 982);
  if (node.hasChildren())
    return FirstPositionInOrBeforeNode(node);
  xtrace->LogLineRun(xtrace_mrid, 984);
  DCHECK(node.parentNode()) << node;
  xtrace->LogLineRun(xtrace_mrid, 985);
  DCHECK(!node.parentNode()->IsShadowRoot()) << node.parentNode();
  xtrace->LogLineRun(xtrace_mrid, 986);
  xtrace->FlushAllEventsToJSONFile();
  return Position::InParentBeforeNode(node);
}

// Returns the position at the ending of a node
Position PositionAfterNode(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "PositionAfterNode",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 991);
  DCHECK(!NeedsLayoutTreeUpdate(node));
  xtrace->LogLineRun(xtrace_mrid, 992);
  if (node.hasChildren())
    return LastPositionInOrAfterNode(node);
  xtrace->LogLineRun(xtrace_mrid, 994);
  DCHECK(node.parentNode()) << node.parentNode();
  xtrace->LogLineRun(xtrace_mrid, 995);
  DCHECK(!node.parentNode()->IsShadowRoot()) << node.parentNode();
  xtrace->LogLineRun(xtrace_mrid, 996);
  xtrace->FlushAllEventsToJSONFile();
  return Position::InParentAfterNode(node);
}

bool IsHTMLListElement(const Node *n) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsHTMLListElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "n", n ? base::ToString(*n) : "");
  xtrace->LogLineRun(xtrace_mrid, 1000);
  xtrace->FlushAllEventsToJSONFile();
  return (n && (IsA<HTMLUListElement>(*n) || IsA<HTMLOListElement>(*n) ||
                IsA<HTMLDListElement>(*n)));
}

bool IsListItem(const Node *n) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsListItem",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "n", n ? base::ToString(*n) : "");
  xtrace->LogLineRun(xtrace_mrid, 1005);
  xtrace->FlushAllEventsToJSONFile();
  return n && n->GetLayoutObject() && n->GetLayoutObject()->IsListItem();
}

bool IsListItemTag(const Node *n) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsListItemTag",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "n", n ? base::ToString(*n) : "");
  xtrace->LogLineRun(xtrace_mrid, 1009);
  xtrace->FlushAllEventsToJSONFile();
  return n && (n->HasTagName(html_names::kLiTag) ||
               n->HasTagName(html_names::kDdTag) ||
               n->HasTagName(html_names::kDtTag));
}

bool IsListElementTag(const Node *n) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsListElementTag",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "n", n ? base::ToString(*n) : "");
  xtrace->LogLineRun(xtrace_mrid, 1015);
  xtrace->FlushAllEventsToJSONFile();
  return n && (n->HasTagName(html_names::kUlTag) ||
               n->HasTagName(html_names::kOlTag) ||
               n->HasTagName(html_names::kDlTag));
}

bool IsPresentationalHTMLElement(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsPresentationalHTMLElement",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1021);
  const auto *element = DynamicTo<HTMLElement>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "element",
                         element ? base::ToString(*element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1022);
  if (!element)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1025);
  xtrace->FlushAllEventsToJSONFile();
  return element->HasTagName(html_names::kUTag) ||
         element->HasTagName(html_names::kSTag) ||
         element->HasTagName(html_names::kStrikeTag) ||
         element->HasTagName(html_names::kITag) ||
         element->HasTagName(html_names::kEmTag) ||
         element->HasTagName(html_names::kBTag) ||
         element->HasTagName(html_names::kStrongTag);
}

Element *AssociatedElementOf(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AssociatedElementOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1035);
  Node *node = position.AnchorNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1036);
  if (!node)
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1039);
  if (auto *element = DynamicTo<Element>(node))
    return element;

  xtrace->LogLineRun(xtrace_mrid, 1042);
  ContainerNode *parent = NodeTraversal::Parent(*node);
  xtrace->LocalVarUpdate(xtrace_mrid, "parent",
                         parent ? base::ToString(*parent) : "");

  xtrace->LogLineRun(xtrace_mrid, 1043);
  xtrace->FlushAllEventsToJSONFile();
  return DynamicTo<Element>(parent);
}

Element *EnclosingElementWithTag(const Position &p,
                                 const QualifiedName &tag_name) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingElementWithTag",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LocalVarUpdate(xtrace_mrid, "tag_name", base::ToString(tag_name));
  xtrace->LogLineRun(xtrace_mrid, 1048);
  if (p.IsNull())
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1051);
  ContainerNode *root = HighestEditableRoot(p);
  xtrace->LocalVarUpdate(xtrace_mrid, "root",
                         root ? base::ToString(*root) : "");

  xtrace->LogLineRun(xtrace_mrid, 1052);
  for (Node &runner : NodeTraversal::InclusiveAncestorsOf(*p.AnchorNode())) {
    xtrace->LogLineRun(xtrace_mrid, 1053);
    auto *ancestor = DynamicTo<Element>(runner);
    xtrace->LogLineRun(xtrace_mrid, 1054);
    if (!ancestor)
      continue;
    xtrace->LogLineRun(xtrace_mrid, 1056);
    if (root && !IsEditable(*ancestor))
      continue;
    xtrace->LogLineRun(xtrace_mrid, 1058);
    if (ancestor->HasTagName(tag_name))
      return ancestor;
    xtrace->LogLineRun(xtrace_mrid, 1060);
    if (ancestor == root)
      return nullptr;
  }

  xtrace->LogLineRun(xtrace_mrid, 1064);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

template <typename Strategy>
static Node *EnclosingNodeOfTypeAlgorithm(const PositionTemplate<Strategy> &p,
                                          bool (*node_is_of_type)(const Node *),
                                          EditingBoundaryCrossingRule rule) {
  // TODO(yosin) support CanSkipCrossEditingBoundary
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "EnclosingNodeOfTypeAlgorithm",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 1072);
  DCHECK(rule == kCanCrossEditingBoundary ||
         rule == kCannotCrossEditingBoundary)
      << rule;
  xtrace->LogLineRun(xtrace_mrid, 1075);
  if (p.IsNull())
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1078);
  ContainerNode *const root =
      rule == kCannotCrossEditingBoundary ? RootEditableElementOf(p) : nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "root",
                         root ? base::ToString(*root) : "");

  xtrace->LogLineRun(xtrace_mrid, 1080);
  for (Node *n = p.AnchorNode(); n; n = Strategy::Parent(*n)) {
    // Don't return a non-editable node if the input position was editable,
    // since the callers from editing will no doubt want to perform editing
    // inside the returned node.
    xtrace->LogLineRun(xtrace_mrid, 1084);
    if (root && !IsEditable(*n))
      continue;
    xtrace->LogLineRun(xtrace_mrid, 1086);
    if (node_is_of_type(n))
      return n;
    xtrace->LogLineRun(xtrace_mrid, 1088);
    if (n == root)
      return nullptr;
  }

  xtrace->LogLineRun(xtrace_mrid, 1092);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

Node *EnclosingNodeOfType(const Position &p,
                          bool (*node_is_of_type)(const Node *),
                          EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingNodeOfType",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 1098);
  xtrace->FlushAllEventsToJSONFile();
  return EnclosingNodeOfTypeAlgorithm<EditingStrategy>(p, node_is_of_type,
                                                       rule);
}

Node *EnclosingNodeOfType(const PositionInFlatTree &p,
                          bool (*node_is_of_type)(const Node *),
                          EditingBoundaryCrossingRule rule) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingNodeOfType",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LogLineRun(xtrace_mrid, 1105);
  xtrace->FlushAllEventsToJSONFile();
  return EnclosingNodeOfTypeAlgorithm<EditingInFlatTreeStrategy>(
      p, node_is_of_type, rule);
}

Node *HighestEnclosingNodeOfType(const Position &p,
                                 bool (*node_is_of_type)(const Node *),
                                 EditingBoundaryCrossingRule rule,
                                 Node *stay_within) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "HighestEnclosingNodeOfType",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LocalVarUpdate(xtrace_mrid, "rule", base::ToString(rule));
  xtrace->LocalVarUpdate(xtrace_mrid, "stay_within",
                         stay_within ? base::ToString(*stay_within) : "");
  xtrace->LogLineRun(xtrace_mrid, 1113);
  Node *highest = nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "highest",
                         highest ? base::ToString(*highest) : "");

  xtrace->LogLineRun(xtrace_mrid, 1114);
  ContainerNode *root =
      rule == kCannotCrossEditingBoundary ? HighestEditableRoot(p) : nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "root",
                         root ? base::ToString(*root) : "");

  xtrace->LogLineRun(xtrace_mrid, 1116);
  for (Node *n = p.ComputeContainerNode(); n && n != stay_within;
       n = n->parentNode()) {
    xtrace->LogLineRun(xtrace_mrid, 1118);
    if (root && !IsEditable(*n))
      continue;
    xtrace->LogLineRun(xtrace_mrid, 1120);
    if (node_is_of_type(n))
      highest = n;
    xtrace->LogLineRun(xtrace_mrid, 1122);
    if (n == root)
      break;
  }

  xtrace->LogLineRun(xtrace_mrid, 1126);
  xtrace->FlushAllEventsToJSONFile();
  return highest;
}

Element *EnclosingAnchorElement(const Position &p) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EnclosingAnchorElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));
  xtrace->LogLineRun(xtrace_mrid, 1130);
  if (p.IsNull())
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1133);
  for (Element *ancestor =
           ElementTraversal::FirstAncestorOrSelf(*p.AnchorNode());
       ancestor; ancestor = ElementTraversal::FirstAncestor(*ancestor)) {
    xtrace->LogLineRun(xtrace_mrid, 1136);
    if (ancestor->IsLink())
      return ancestor;
  }
  xtrace->LogLineRun(xtrace_mrid, 1139);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

bool IsDisplayInsideTable(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsDisplayInsideTable",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1143);
  xtrace->FlushAllEventsToJSONFile();
  return node && node->GetLayoutObject() && IsA<HTMLTableElement>(node);
}

bool IsTableCell(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsTableCell",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1147);
  DCHECK(node);
  xtrace->LogLineRun(xtrace_mrid, 1148);
  LayoutObject *r = node->GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "r", r ? base::ToString(*r) : "");

  xtrace->LogLineRun(xtrace_mrid, 1149);
  xtrace->FlushAllEventsToJSONFile();
  return r ? r->IsTableCell() : IsA<HTMLTableCellElement>(*node);
}

bool IsTablePartElement(const Node *n) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsTablePartElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "n", n ? base::ToString(*n) : "");
  xtrace->LogLineRun(xtrace_mrid, 1153);
  xtrace->FlushAllEventsToJSONFile();
  return n &&
         (IsA<HTMLTableCellElement>(*n) || IsA<HTMLTableCaptionElement>(*n) ||
          IsA<HTMLTableColElement>(*n) || IsA<HTMLTableSectionElement>(*n) ||
          IsA<HTMLTableRowElement>(*n));
}

HTMLElement *CreateDefaultParagraphElement(Document &document) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "CreateDefaultParagraphElement",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "document", base::ToString(document));
  xtrace->LogLineRun(xtrace_mrid, 1160);
  switch (document.GetFrame()->GetEditor().DefaultParagraphSeparator()) {
  case EditorParagraphSeparator::kIsDiv:
    xtrace->LogLineRun(xtrace_mrid, 1162);
    return MakeGarbageCollected<HTMLDivElement>(document);
  case EditorParagraphSeparator::kIsP:
    xtrace->LogLineRun(xtrace_mrid, 1164);
    return MakeGarbageCollected<HTMLParagraphElement>(document);
  }

  xtrace->LogLineRun(xtrace_mrid, 1167);
  xtrace->FlushAllEventsToJSONFile();
  NOTREACHED();
}

bool IsTabHTMLSpanElement(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsTabHTMLSpanElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1171);
  const auto *span = DynamicTo<HTMLSpanElement>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "span",
                         span ? base::ToString(*span) : "");

  xtrace->LogLineRun(xtrace_mrid, 1172);
  if (!span) {
    xtrace->LogLineRun(xtrace_mrid, 1173);
    return false;
  }
  xtrace->LogLineRun(xtrace_mrid, 1175);
  const Node *const first_child = NodeTraversal::FirstChild(*span);
  xtrace->LocalVarUpdate(xtrace_mrid, "first_child",
                         first_child ? base::ToString(*first_child) : "");

  xtrace->LogLineRun(xtrace_mrid, 1176);
  auto *first_child_text_node = DynamicTo<Text>(first_child);
  xtrace->LocalVarUpdate(
      xtrace_mrid, "first_child_text_node",
      first_child_text_node ? base::ToString(*first_child_text_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1177);
  if (!first_child_text_node) {
    xtrace->LogLineRun(xtrace_mrid, 1178);
    return false;
  }
  xtrace->LogLineRun(xtrace_mrid, 1180);
  if (!first_child_text_node->data().Contains('\t')) {
    xtrace->LogLineRun(xtrace_mrid, 1181);
    return false;
  }
  // TODO(editing-dev): Hoist the call of UpdateStyleAndLayoutTree to callers.
  // See crbug.com/590369 for details.
  xtrace->LogLineRun(xtrace_mrid, 1185);
  span->GetDocument().UpdateStyleAndLayoutTree();
  xtrace->LogLineRun(xtrace_mrid, 1186);
  const ComputedStyle *style = span->GetComputedStyle();
  xtrace->LocalVarUpdate(xtrace_mrid, "style",
                         style ? base::ToString(*style) : "");

  xtrace->LogLineRun(xtrace_mrid, 1187);
  xtrace->FlushAllEventsToJSONFile();
  return style && style->WhiteSpace() == EWhiteSpace::kPre;
}

bool IsTabHTMLSpanElementTextNode(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsTabHTMLSpanElementTextNode",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1191);
  xtrace->FlushAllEventsToJSONFile();
  return node && node->IsTextNode() && node->parentNode() &&
         IsTabHTMLSpanElement(node->parentNode());
}

HTMLSpanElement *TabSpanElement(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "TabSpanElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1196);
  xtrace->FlushAllEventsToJSONFile();
  return IsTabHTMLSpanElementTextNode(node)
             ? To<HTMLSpanElement>(node->parentNode())
             : nullptr;
}

static HTMLSpanElement *CreateTabSpanElement(Document &document,
                                             Text *tab_text_node) {
  // Make the span to hold the tab.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "CreateTabSpanElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "document", base::ToString(document));
  xtrace->LocalVarUpdate(xtrace_mrid, "tab_text_node",
                         tab_text_node ? base::ToString(*tab_text_node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1204);
  auto *span_element = MakeGarbageCollected<HTMLSpanElement>(document);
  xtrace->LocalVarUpdate(xtrace_mrid, "span_element",
                         span_element ? base::ToString(*span_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1205);
  span_element->setAttribute(html_names::kStyleAttr,
                             AtomicString("white-space:pre"));

  // Add tab text to that span.
  xtrace->LogLineRun(xtrace_mrid, 1209);
  if (!tab_text_node)
    tab_text_node = document.CreateEditingTextNode("\t");

  xtrace->LogLineRun(xtrace_mrid, 1212);
  span_element->AppendChild(tab_text_node);

  xtrace->LogLineRun(xtrace_mrid, 1214);
  xtrace->FlushAllEventsToJSONFile();
  return span_element;
}

HTMLSpanElement *CreateTabSpanElement(Document &document,
                                      const String &tab_text) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "CreateTabSpanElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "document", base::ToString(document));
  xtrace->LocalVarUpdate(xtrace_mrid, "tab_text", base::ToString(tab_text));
  xtrace->LogLineRun(xtrace_mrid, 1219);
  xtrace->FlushAllEventsToJSONFile();
  return CreateTabSpanElement(document, document.createTextNode(tab_text));
}

HTMLSpanElement *CreateTabSpanElement(Document &document) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "CreateTabSpanElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "document", base::ToString(document));
  xtrace->LogLineRun(xtrace_mrid, 1223);
  xtrace->FlushAllEventsToJSONFile();
  return CreateTabSpanElement(document, nullptr);
}

static bool IsInPlaceholder(const TextControlElement &text_control,
                            const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsInPlaceholder",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "text_control",
                         base::ToString(text_control));
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1228);
  const auto *const placeholder_element = text_control.PlaceholderElement();
  xtrace->LocalVarUpdate(
      xtrace_mrid, "placeholder_element",
      placeholder_element ? base::ToString(*placeholder_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1229);
  if (!placeholder_element)
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1231);
  xtrace->FlushAllEventsToJSONFile();
  return placeholder_element->contains(position.ComputeContainerNode());
}

// Returns user-select:contain boundary element of specified position.
// Because of we've not yet implemented "user-select:contain", we consider
// following elements having "user-select:contain"
//  - root editable
//  - inner editor of text control (<input> and <textarea>)
// Note: inner editor of readonly text control isn't content editable.
// TODO(yosin): We should handle elements with "user-select:contain".
// See http:/crbug.com/658129
static Element *UserSelectContainBoundaryOf(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "UserSelectContainBoundaryOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1243);
  if (auto *text_control = EnclosingTextControl(position)) {
    xtrace->LogLineRun(xtrace_mrid, 1244);
    if (IsInPlaceholder(*text_control, position))
      return nullptr;
    // for <input readonly>. See http://crbug.com/185089
    xtrace->LogLineRun(xtrace_mrid, 1247);
    return text_control->InnerEditorElement();
  }
  // Note: Until we implement "user-select:contain", we treat root editable
  // element and text control as having "user-select:contain".
  xtrace->LogLineRun(xtrace_mrid, 1251);
  if (Element *editable = RootEditableElementOf(position))
    return editable;
  xtrace->LogLineRun(xtrace_mrid, 1253);
  xtrace->FlushAllEventsToJSONFile();
  return nullptr;
}

PositionWithAffinity
PositionRespectingEditingBoundary(const Position &position,
                                  const HitTestResult &hit_test_result) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "PositionRespectingEditingBoundary",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 1259);
  Node *target_node = hit_test_result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "target_node",
                         target_node ? base::ToString(*target_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1260);
  DCHECK(target_node);
  xtrace->LogLineRun(xtrace_mrid, 1261);
  const LayoutObject *target_object = target_node->GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "target_object",
                         target_object ? base::ToString(*target_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 1262);
  if (!target_object)
    return PositionWithAffinity();

  xtrace->LogLineRun(xtrace_mrid, 1265);
  Element *editable_element = UserSelectContainBoundaryOf(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "editable_element",
                         editable_element ? base::ToString(*editable_element)
                                          : "");

  xtrace->LogLineRun(xtrace_mrid, 1266);
  if (!editable_element || editable_element->contains(target_node))
    return hit_test_result.GetPosition();

  xtrace->LogLineRun(xtrace_mrid, 1269);
  const LayoutObject *editable_object = editable_element->GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "editable_object",
                         editable_object ? base::ToString(*editable_object)
                                         : "");

  xtrace->LogLineRun(xtrace_mrid, 1270);
  if (!editable_object || !editable_object->VisibleToHitTesting())
    return PositionWithAffinity();

  // TODO(yosin): Is this kIgnoreTransforms correct here?
  xtrace->LogLineRun(xtrace_mrid, 1274);
  PhysicalOffset selection_end_point = hit_test_result.LocalPoint();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_end_point",
                         base::ToString(selection_end_point));

  xtrace->LogLineRun(xtrace_mrid, 1275);
  PhysicalOffset absolute_point = target_object->LocalToAbsolutePoint(
      selection_end_point, kIgnoreTransforms);
  xtrace->LocalVarUpdate(xtrace_mrid, "absolute_point",
                         base::ToString(absolute_point));

  xtrace->LogLineRun(xtrace_mrid, 1277);
  selection_end_point =
      editable_object->AbsoluteToLocalPoint(absolute_point, kIgnoreTransforms);
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_end_point",
                         base::ToString(selection_end_point));

  xtrace->LogLineRun(xtrace_mrid, 1279);
  target_object = editable_object;
  xtrace->LocalVarUpdate(xtrace_mrid, "target_object",
                         base::ToString(target_object));

  // TODO(kojii): Support fragment-based |PositionForPoint|. LayoutObject-based
  // |PositionForPoint| may not work if NG block fragmented.
  xtrace->LogLineRun(xtrace_mrid, 1282);
  xtrace->FlushAllEventsToJSONFile();
  return target_object->PositionForPoint(selection_end_point);
}

PositionWithAffinity
AdjustForEditingBoundary(const PositionWithAffinity &position_with_affinity) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AdjustForEditingBoundary",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position_with_affinity",
                         base::ToString(position_with_affinity));
  xtrace->LogLineRun(xtrace_mrid, 1287);
  if (position_with_affinity.IsNull())
    return position_with_affinity;
  xtrace->LogLineRun(xtrace_mrid, 1289);
  const Position &position = position_with_affinity.GetPosition();
  xtrace->LogLineRun(xtrace_mrid, 1290);
  const Node &node = *position.ComputeContainerNode();
  xtrace->LogLineRun(xtrace_mrid, 1291);
  if (IsEditable(node))
    return position_with_affinity;
  // TODO(yosin): Once we fix |MostBackwardCaretPosition()| to handle
  // positions other than |kOffsetInAnchor|, we don't need to use
  // |adjusted_position|, e.g. <outer><inner contenteditable> with position
  // before <inner> vs. outer@0[1].
  // [1] editing/selection/click-outside-editable-div.html
  xtrace->LogLineRun(xtrace_mrid, 1298);
  const Position &adjusted_position = IsEditable(*position.AnchorNode())
                                          ? position.ToOffsetInAnchor()
                                          : position;
  xtrace->LogLineRun(xtrace_mrid, 1301);
  const Position &forward =
      MostForwardCaretPosition(adjusted_position, kCanCrossEditingBoundary);
  xtrace->LogLineRun(xtrace_mrid, 1303);
  if (IsEditable(*forward.ComputeContainerNode()))
    return PositionWithAffinity(forward);
  xtrace->LogLineRun(xtrace_mrid, 1305);
  const Position &backward =
      MostBackwardCaretPosition(adjusted_position, kCanCrossEditingBoundary);
  xtrace->LogLineRun(xtrace_mrid, 1307);
  if (IsEditable(*backward.ComputeContainerNode()))
    return PositionWithAffinity(backward);
  xtrace->LogLineRun(xtrace_mrid, 1309);
  xtrace->FlushAllEventsToJSONFile();
  return PositionWithAffinity(adjusted_position,
                              position_with_affinity.Affinity());
}

PositionWithAffinity AdjustForEditingBoundary(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AdjustForEditingBoundary",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1314);
  xtrace->FlushAllEventsToJSONFile();
  return AdjustForEditingBoundary(PositionWithAffinity(position));
}

Position ComputePlaceholderToCollapseAt(const Position &insertion_pos) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "ComputePlaceholderToCollapseAt",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "insertion_pos",
                         base::ToString(insertion_pos));
  xtrace->LogLineRun(xtrace_mrid, 1318);
  Position placeholder;
  // We want to remove preserved newlines and brs that will collapse (and thus
  // become unnecessary) when content is inserted just before them.
  // FIXME: We shouldn't really have to do this, but removing placeholders is a
  // workaround for 9661.
  // If the caret is just before a placeholder, downstream will normalize the
  // caret to it.
  xtrace->LogLineRun(xtrace_mrid, 1325);
  Position downstream(MostForwardCaretPosition(insertion_pos));
  xtrace->LogLineRun(xtrace_mrid, 1326);
  if (LineBreakExistsAtPosition(downstream)) {
    // FIXME: This doesn't handle placeholders at the end of anonymous blocks.
    xtrace->LogLineRun(xtrace_mrid, 1328);
    VisiblePosition caret = CreateVisiblePosition(insertion_pos);
    xtrace->LogLineRun(xtrace_mrid, 1329);
    if (IsEndOfBlock(caret) && IsStartOfParagraph(caret)) {
      xtrace->LogLineRun(xtrace_mrid, 1330);
      placeholder = downstream;
    }
    // Don't remove the placeholder yet, otherwise the block we're inserting
    // into would collapse before we get a chance to insert into it.  We check
    // for a placeholder now, though, because doing so requires the creation of
    // a VisiblePosition, and if we did that post-insertion it would force a
    // layout.
  }
  xtrace->LogLineRun(xtrace_mrid, 1338);
  xtrace->FlushAllEventsToJSONFile();
  return placeholder;
}

Position ComputePositionForNodeRemoval(const Position &position,
                                       const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "ComputePositionForNodeRemoval",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1343);
  if (position.IsNull())
    return position;
  xtrace->LogLineRun(xtrace_mrid, 1345);
  Node *container_node;
  xtrace->LogLineRun(xtrace_mrid, 1346);
  Node *anchor_node;
  xtrace->LogLineRun(xtrace_mrid, 1347);
  switch (position.AnchorType()) {
  case PositionAnchorType::kAfterChildren:
    xtrace->LogLineRun(xtrace_mrid, 1349);
    container_node = position.ComputeContainerNode();
    xtrace->LogLineRun(xtrace_mrid, 1350);
    if (!container_node ||
        !node.IsShadowIncludingInclusiveAncestorOf(*container_node)) {
      xtrace->LogLineRun(xtrace_mrid, 1352);
      return position;
    }
    xtrace->LogLineRun(xtrace_mrid, 1354);
    return Position::InParentBeforeNode(node);
  case PositionAnchorType::kOffsetInAnchor:
    xtrace->LogLineRun(xtrace_mrid, 1356);
    container_node = position.ComputeContainerNode();
    xtrace->LogLineRun(xtrace_mrid, 1357);
    if (container_node == node.parentNode() &&
        static_cast<unsigned>(position.OffsetInContainerNode()) >
            node.NodeIndex()) {
      xtrace->LogLineRun(xtrace_mrid, 1360);
      return Position(container_node, position.OffsetInContainerNode() - 1);
    }
    xtrace->LogLineRun(xtrace_mrid, 1362);
    if (!container_node ||
        !node.IsShadowIncludingInclusiveAncestorOf(*container_node)) {
      xtrace->LogLineRun(xtrace_mrid, 1364);
      return position;
    }
    xtrace->LogLineRun(xtrace_mrid, 1366);
    return Position::InParentBeforeNode(node);
  case PositionAnchorType::kAfterAnchor:
    xtrace->LogLineRun(xtrace_mrid, 1368);
    anchor_node = position.AnchorNode();
    xtrace->LogLineRun(xtrace_mrid, 1369);
    if (!anchor_node ||
        !node.IsShadowIncludingInclusiveAncestorOf(*anchor_node))
      return position;
    xtrace->LogLineRun(xtrace_mrid, 1372);
    return Position::InParentBeforeNode(node);
  case PositionAnchorType::kBeforeAnchor:
    xtrace->LogLineRun(xtrace_mrid, 1374);
    anchor_node = position.AnchorNode();
    xtrace->LogLineRun(xtrace_mrid, 1375);
    if (!anchor_node ||
        !node.IsShadowIncludingInclusiveAncestorOf(*anchor_node))
      return position;
    xtrace->LogLineRun(xtrace_mrid, 1378);
    return Position::InParentBeforeNode(node);
  }
  xtrace->LogLineRun(xtrace_mrid, 1380);
  xtrace->FlushAllEventsToJSONFile();
  NOTREACHED() << "We should handle all PositionAnchorType";
}

bool IsMailHTMLBlockquoteElement(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsMailHTMLBlockquoteElement",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1384);
  const auto *element = DynamicTo<HTMLElement>(*node);
  xtrace->LocalVarUpdate(xtrace_mrid, "element",
                         element ? base::ToString(*element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1385);
  if (!element)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1388);
  xtrace->FlushAllEventsToJSONFile();
  return element->HasTagName(html_names::kBlockquoteTag) &&
         element->getAttribute(html_names::kTypeAttr) == "cite";
}

bool ElementCannotHaveEndTag(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "ElementCannotHaveEndTag",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1393);
  auto *html_element = DynamicTo<HTMLElement>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "html_element",
                         html_element ? base::ToString(*html_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1394);
  if (!html_element)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1397);
  xtrace->FlushAllEventsToJSONFile();
  return !html_element->ShouldSerializeEndTag();
}

// FIXME: indexForVisiblePosition and visiblePositionForIndex use TextIterators
// to convert between VisiblePositions and indices. But TextIterator iteration
// using TextIteratorEmitsCharactersBetweenAllVisiblePositions does not exactly
// match VisiblePosition iteration, so using them to preserve a selection during
// an editing opertion is unreliable. TextIterator's
// TextIteratorEmitsCharactersBetweenAllVisiblePositions mode needs to be fixed,
// or these functions need to be changed to iterate using actual
// VisiblePositions.
// FIXME: Deploy these functions everywhere that TextIterators are used to
// convert between VisiblePositions and indices.
int IndexForVisiblePosition(const VisiblePosition &visible_position,
                            ContainerNode *&scope) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IndexForVisiblePosition",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_position",
                         base::ToString(visible_position));
  xtrace->LogLineRun(xtrace_mrid, 1412);
  if (visible_position.IsNull())
    return 0;

  xtrace->LogLineRun(xtrace_mrid, 1415);
  Position p(visible_position.DeepEquivalent());
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));

  xtrace->LogLineRun(xtrace_mrid, 1416);
  Document &document = *p.GetDocument();
  xtrace->LogLineRun(xtrace_mrid, 1417);
  DCHECK(!document.NeedsLayoutTreeUpdate());

  xtrace->LogLineRun(xtrace_mrid, 1419);
  ShadowRoot *shadow_root = p.AnchorNode()->ContainingShadowRoot();
  xtrace->LocalVarUpdate(xtrace_mrid, "shadow_root",
                         shadow_root ? base::ToString(*shadow_root) : "");

  xtrace->LogLineRun(xtrace_mrid, 1421);
  if (shadow_root)
    scope = shadow_root;
  else
    xtrace->LogLineRun(xtrace_mrid, 1424);
  scope = document.documentElement();

  xtrace->LogLineRun(xtrace_mrid, 1426);
  EphemeralRange range(Position::FirstPositionInNode(*scope),
                       p.ParentAnchoredEquivalent());
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));

  xtrace->LogLineRun(xtrace_mrid, 1429);
  const TextIteratorBehavior &behavior =
      TextIteratorBehavior::Builder(
          TextIteratorBehavior::AllVisiblePositionsRangeLengthBehavior())
          .SetSuppressesExtraNewlineEmission(true)
          .Build();
  xtrace->LogLineRun(xtrace_mrid, 1434);
  xtrace->FlushAllEventsToJSONFile();
  return TextIterator::RangeLength(range.StartPosition(), range.EndPosition(),
                                   behavior);
}

EphemeralRange MakeRange(const VisiblePosition &start,
                         const VisiblePosition &end) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "MakeRange",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "start", base::ToString(start));
  xtrace->LocalVarUpdate(xtrace_mrid, "end", base::ToString(end));
  xtrace->LogLineRun(xtrace_mrid, 1440);
  if (start.IsNull() || end.IsNull())
    return EphemeralRange();

  xtrace->LogLineRun(xtrace_mrid, 1443);
  Position s = start.DeepEquivalent().ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "s", base::ToString(s));

  xtrace->LogLineRun(xtrace_mrid, 1444);
  Position e = end.DeepEquivalent().ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "e", base::ToString(e));

  xtrace->LogLineRun(xtrace_mrid, 1445);
  if (s.IsNull() || e.IsNull())
    return EphemeralRange();

  xtrace->LogLineRun(xtrace_mrid, 1448);
  xtrace->FlushAllEventsToJSONFile();
  return EphemeralRange(s, e);
}

template <typename Strategy>
static EphemeralRangeTemplate<Strategy>
NormalizeRangeAlgorithm(const EphemeralRangeTemplate<Strategy> &range) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NormalizeRangeAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));
  xtrace->LogLineRun(xtrace_mrid, 1454);
  DCHECK(range.IsNotNull());
  xtrace->LogLineRun(xtrace_mrid, 1455);
  DCHECK(!range.GetDocument().NeedsLayoutTreeUpdate());
  xtrace->LogLineRun(xtrace_mrid, 1456);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      range.GetDocument().Lifecycle());
  xtrace->LocalVarUpdate(xtrace_mrid, "disallow_transition",
                         base::ToString(disallow_transition));

  // TODO(yosin) We should not call |parentAnchoredEquivalent()|, it is
  // redundant.
  xtrace->LogLineRun(xtrace_mrid, 1461);
  const PositionTemplate<Strategy> normalized_start =
      MostForwardCaretPosition(range.StartPosition())
          .ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "normalized_start",
                         base::ToString(normalized_start));

  xtrace->LogLineRun(xtrace_mrid, 1464);
  const PositionTemplate<Strategy> normalized_end =
      MostBackwardCaretPosition(range.EndPosition()).ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "normalized_end",
                         base::ToString(normalized_end));

  // The order of the positions of |start| and |end| can be swapped after
  // upstream/downstream. e.g. editing/pasteboard/copy-display-none.html
  xtrace->LogLineRun(xtrace_mrid, 1468);
  if (normalized_start.CompareTo(normalized_end) > 0)
    return EphemeralRangeTemplate<Strategy>(normalized_end, normalized_start);
  xtrace->LogLineRun(xtrace_mrid, 1470);
  xtrace->FlushAllEventsToJSONFile();
  return EphemeralRangeTemplate<Strategy>(normalized_start, normalized_end);
}

EphemeralRange NormalizeRange(const EphemeralRange &range) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NormalizeRange",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));
  xtrace->LogLineRun(xtrace_mrid, 1474);
  xtrace->FlushAllEventsToJSONFile();
  return NormalizeRangeAlgorithm<EditingStrategy>(range);
}

EphemeralRangeInFlatTree NormalizeRange(const EphemeralRangeInFlatTree &range) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "NormalizeRange",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));
  xtrace->LogLineRun(xtrace_mrid, 1478);
  xtrace->FlushAllEventsToJSONFile();
  return NormalizeRangeAlgorithm<EditingInFlatTreeStrategy>(range);
}

VisiblePosition VisiblePositionForIndex(int index, ContainerNode *scope) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "VisiblePositionForIndex",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "index", base::ToString(index));
  xtrace->LocalVarUpdate(xtrace_mrid, "scope",
                         scope ? base::ToString(*scope) : "");
  xtrace->LogLineRun(xtrace_mrid, 1482);
  if (!scope)
    return VisiblePosition();
  xtrace->LogLineRun(xtrace_mrid, 1484);
  DCHECK(!scope->GetDocument().NeedsLayoutTreeUpdate());
  xtrace->LogLineRun(xtrace_mrid, 1485);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      scope->GetDocument().Lifecycle());
  xtrace->LocalVarUpdate(xtrace_mrid, "disallow_transition",
                         base::ToString(disallow_transition));

  xtrace->LogLineRun(xtrace_mrid, 1488);
  EphemeralRange range =
      PlainTextRange(index).CreateRangeForSelectionIndexing(*scope);
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));

  // Check for an invalid index. Certain editing operations invalidate indices
  // because of problems with
  // TextIteratorEmitsCharactersBetweenAllVisiblePositions.
  xtrace->LogLineRun(xtrace_mrid, 1493);
  if (range.IsNull())
    return VisiblePosition();
  xtrace->LogLineRun(xtrace_mrid, 1495);
  xtrace->FlushAllEventsToJSONFile();
  return CreateVisiblePosition(range.StartPosition());
}

template <typename Strategy>
bool AreSameRangesAlgorithm(Node *node,
                            const PositionTemplate<Strategy> &start_position,
                            const PositionTemplate<Strategy> &end_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AreSameRangesAlgorithm",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "start_position",
                         base::ToString(start_position));
  xtrace->LocalVarUpdate(xtrace_mrid, "end_position",
                         base::ToString(end_position));
  xtrace->LogLineRun(xtrace_mrid, 1502);
  DCHECK(node);
  xtrace->LogLineRun(xtrace_mrid, 1503);
  const EphemeralRange range =
      CreateVisibleSelection(
          SelectionInDOMTree::Builder().SelectAllChildren(*node).Build())
          .ToNormalizedEphemeralRange();
  xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));

  xtrace->LogLineRun(xtrace_mrid, 1507);
  xtrace->FlushAllEventsToJSONFile();
  return ToPositionInDOMTree(start_position) == range.StartPosition() &&
         ToPositionInDOMTree(end_position) == range.EndPosition();
}

bool AreSameRanges(Node *node, const Position &start_position,
                   const Position &end_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AreSameRanges",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "start_position",
                         base::ToString(start_position));
  xtrace->LocalVarUpdate(xtrace_mrid, "end_position",
                         base::ToString(end_position));
  xtrace->LogLineRun(xtrace_mrid, 1514);
  xtrace->FlushAllEventsToJSONFile();
  return AreSameRangesAlgorithm<EditingStrategy>(node, start_position,
                                                 end_position);
}

bool AreSameRanges(Node *node, const PositionInFlatTree &start_position,
                   const PositionInFlatTree &end_position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "AreSameRanges",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "start_position",
                         base::ToString(start_position));
  xtrace->LocalVarUpdate(xtrace_mrid, "end_position",
                         base::ToString(end_position));
  xtrace->LogLineRun(xtrace_mrid, 1521);
  xtrace->FlushAllEventsToJSONFile();
  return AreSameRangesAlgorithm<EditingInFlatTreeStrategy>(node, start_position,
                                                           end_position);
}

bool IsRenderedAsNonInlineTableImageOrHR(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsRenderedAsNonInlineTableImageOrHR",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1526);
  if (!node)
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1528);
  LayoutObject *layout_object = node->GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_object",
                         layout_object ? base::ToString(*layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 1529);
  if (!layout_object || layout_object->IsInline()) {
    xtrace->LogLineRun(xtrace_mrid, 1530);
    return false;
  }
  xtrace->LogLineRun(xtrace_mrid, 1532);
  xtrace->FlushAllEventsToJSONFile();
  return layout_object->IsTable() || layout_object->IsImage() ||
         layout_object->IsHR();
}

bool IsNonTableCellHTMLBlockElement(const Node *node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsNonTableCellHTMLBlockElement",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 1537);
  const auto *element = DynamicTo<HTMLElement>(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "element",
                         element ? base::ToString(*element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1538);
  if (!element)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1541);
  xtrace->FlushAllEventsToJSONFile();
  return element->HasTagName(html_names::kListingTag) ||
         element->HasTagName(html_names::kOlTag) ||
         element->HasTagName(html_names::kPreTag) ||
         element->HasTagName(html_names::kTableTag) ||
         element->HasTagName(html_names::kUlTag) ||
         element->HasTagName(html_names::kXmpTag) ||
         element->HasTagName(html_names::kH1Tag) ||
         element->HasTagName(html_names::kH2Tag) ||
         element->HasTagName(html_names::kH3Tag) ||
         element->HasTagName(html_names::kH4Tag) ||
         element->HasTagName(html_names::kH5Tag);
}

bool IsBlockFlowElement(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsBlockFlowElement",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1555);
  LayoutObject *layout_object = node.GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_object",
                         layout_object ? base::ToString(*layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 1556);
  xtrace->FlushAllEventsToJSONFile();
  return node.IsElementNode() && layout_object &&
         layout_object->IsLayoutBlockFlow();
}

bool IsInPasswordField(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "IsInPasswordField",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1561);
  TextControlElement *text_control = EnclosingTextControl(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "text_control",
                         text_control ? base::ToString(*text_control) : "");

  xtrace->LogLineRun(xtrace_mrid, 1562);
  auto *html_input_element = DynamicTo<HTMLInputElement>(text_control);
  xtrace->LocalVarUpdate(
      xtrace_mrid, "html_input_element",
      html_input_element ? base::ToString(*html_input_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 1563);
  xtrace->FlushAllEventsToJSONFile();
  return html_input_element && html_input_element->FormControlType() ==
                                   FormControlType::kInputPassword;
}

// If current position is at grapheme boundary, return 0; otherwise, return the
// distance to its nearest left grapheme boundary.
wtf_size_t ComputeDistanceToLeftGraphemeBoundary(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "ComputeDistanceToLeftGraphemeBoundary",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1570);
  const Position &adjusted_position = PreviousPositionOf(
      NextPositionOf(position, PositionMoveType::kGraphemeCluster),
      PositionMoveType::kGraphemeCluster);
  xtrace->LogLineRun(xtrace_mrid, 1573);
  DCHECK_EQ(position.AnchorNode(), adjusted_position.AnchorNode());
  xtrace->LogLineRun(xtrace_mrid, 1574);
  DCHECK_GE(position.ComputeOffsetInContainerNode(),
            adjusted_position.ComputeOffsetInContainerNode());
  xtrace->LogLineRun(xtrace_mrid, 1576);
  xtrace->FlushAllEventsToJSONFile();
  return static_cast<wtf_size_t>(
      position.ComputeOffsetInContainerNode() -
      adjusted_position.ComputeOffsetInContainerNode());
}

// If current position is at grapheme boundary, return 0; otherwise, return the
// distance to its nearest right grapheme boundary.
wtf_size_t ComputeDistanceToRightGraphemeBoundary(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "ComputeDistanceToRightGraphemeBoundary",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1584);
  const Position &adjusted_position = NextPositionOf(
      PreviousPositionOf(position, PositionMoveType::kGraphemeCluster),
      PositionMoveType::kGraphemeCluster);
  xtrace->LogLineRun(xtrace_mrid, 1587);
  DCHECK_EQ(position.AnchorNode(), adjusted_position.AnchorNode());
  xtrace->LogLineRun(xtrace_mrid, 1588);
  DCHECK_GE(adjusted_position.ComputeOffsetInContainerNode(),
            position.ComputeOffsetInContainerNode());
  xtrace->LogLineRun(xtrace_mrid, 1590);
  xtrace->FlushAllEventsToJSONFile();
  return static_cast<wtf_size_t>(
      adjusted_position.ComputeOffsetInContainerNode() -
      position.ComputeOffsetInContainerNode());
}

gfx::QuadF LocalToAbsoluteQuadOf(const LocalCaretRect &caret_rect) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "LocalToAbsoluteQuadOf",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "caret_rect", base::ToString(caret_rect));
  xtrace->LogLineRun(xtrace_mrid, 1596);
  xtrace->FlushAllEventsToJSONFile();
  return caret_rect.layout_object->LocalRectToAbsoluteQuad(caret_rect.rect);
}

const StaticRangeVector *TargetRangesForInputEvent(const Node &node) {
  // TODO(editing-dev): The use of UpdateStyleAndLayout
  // needs to be audited. see http://crbug.com/590369 for more details.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "TargetRangesForInputEvent",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1602);
  node.GetDocument().UpdateStyleAndLayout(DocumentUpdateReason::kEditing);
  xtrace->LogLineRun(xtrace_mrid, 1603);
  if (!IsRichlyEditable(node))
    return nullptr;
  xtrace->LogLineRun(xtrace_mrid, 1605);
  const EphemeralRange &range =
      FirstEphemeralRangeOf(node.GetDocument()
                                .GetFrame()
                                ->Selection()
                                .ComputeVisibleSelectionInDOMTree());
  xtrace->LogLineRun(xtrace_mrid, 1610);
  if (range.IsNull())
    return nullptr;
  xtrace->LogLineRun(xtrace_mrid, 1612);
  xtrace->FlushAllEventsToJSONFile();
  return MakeGarbageCollected<StaticRangeVector>(1, StaticRange::Create(range));
}

DispatchEventResult
DispatchBeforeInputInsertText(Node *target, const String &data,
                              InputEvent::InputType input_type,
                              const StaticRangeVector *ranges) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DispatchBeforeInputInsertText",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "data", base::ToString(data));
  xtrace->LocalVarUpdate(xtrace_mrid, "input_type", base::ToString(input_type));
  xtrace->LocalVarUpdate(xtrace_mrid, "ranges",
                         ranges ? base::ToString(*ranges) : "");
  xtrace->LogLineRun(xtrace_mrid, 1620);
  if (!target)
    return DispatchEventResult::kNotCanceled;
  // TODO(editing-dev): Pass appropriate |ranges| after it's defined on spec.
  // http://w3c.github.io/editing/input-events.html#dom-inputevent-inputtype
  xtrace->LogLineRun(xtrace_mrid, 1624);
  InputEvent *before_input_event = InputEvent::CreateBeforeInput(
      input_type, data, InputEvent::EventIsComposing::kNotComposing,
      ranges ? ranges : TargetRangesForInputEvent(*target));
  xtrace->LocalVarUpdate(
      xtrace_mrid, "before_input_event",
      before_input_event ? base::ToString(*before_input_event) : "");

  xtrace->LogLineRun(xtrace_mrid, 1627);
  xtrace->FlushAllEventsToJSONFile();
  return target->DispatchEvent(*before_input_event);
}

DispatchEventResult
DispatchBeforeInputEditorCommand(Node *target, InputEvent::InputType input_type,
                                 const StaticRangeVector *ranges) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DispatchBeforeInputEditorCommand",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "input_type", base::ToString(input_type));
  xtrace->LocalVarUpdate(xtrace_mrid, "ranges",
                         ranges ? base::ToString(*ranges) : "");
  xtrace->LogLineRun(xtrace_mrid, 1634);
  if (!target)
    return DispatchEventResult::kNotCanceled;
  xtrace->LogLineRun(xtrace_mrid, 1636);
  InputEvent *before_input_event = InputEvent::CreateBeforeInput(
      input_type, g_null_atom, InputEvent::EventIsComposing::kNotComposing,
      ranges);
  xtrace->LocalVarUpdate(
      xtrace_mrid, "before_input_event",
      before_input_event ? base::ToString(*before_input_event) : "");

  xtrace->LogLineRun(xtrace_mrid, 1639);
  xtrace->FlushAllEventsToJSONFile();
  return target->DispatchEvent(*before_input_event);
}

DispatchEventResult
DispatchBeforeInputDataTransfer(Node *target, InputEvent::InputType input_type,
                                DataTransfer *data_transfer) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "DispatchBeforeInputDataTransfer",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "input_type", base::ToString(input_type));
  xtrace->LocalVarUpdate(xtrace_mrid, "data_transfer",
                         data_transfer ? base::ToString(*data_transfer) : "");
  xtrace->LogLineRun(xtrace_mrid, 1646);
  if (!target)
    return DispatchEventResult::kNotCanceled;

  xtrace->LogLineRun(xtrace_mrid, 1649);
  DCHECK(input_type == InputEvent::InputType::kInsertFromPaste ||
         input_type == InputEvent::InputType::kInsertReplacementText ||
         input_type == InputEvent::InputType::kInsertFromDrop ||
         input_type == InputEvent::InputType::kDeleteByCut)
      << "Unsupported inputType: " << (int)input_type;

  xtrace->LogLineRun(xtrace_mrid, 1655);
  InputEvent *before_input_event;

  xtrace->LogLineRun(xtrace_mrid, 1657);
  if (IsRichlyEditable(*target) || !data_transfer) {
    xtrace->LogLineRun(xtrace_mrid, 1658);
    before_input_event = InputEvent::CreateBeforeInput(
        input_type, data_transfer, InputEvent::EventIsComposing::kNotComposing,
        TargetRangesForInputEvent(*target));
  } else {
    xtrace->LogLineRun(xtrace_mrid, 1662);
    const String &data = data_transfer->getData(kMimeTypeTextPlain);
    // TODO(editing-dev): Pass appropriate |ranges| after it's defined on spec.
    // http://w3c.github.io/editing/input-events.html#dom-inputevent-inputtype
    xtrace->LogLineRun(xtrace_mrid, 1665);
    before_input_event = InputEvent::CreateBeforeInput(
        input_type, data, InputEvent::EventIsComposing::kNotComposing,
        TargetRangesForInputEvent(*target));
  }
  xtrace->LogLineRun(xtrace_mrid, 1669);
  xtrace->FlushAllEventsToJSONFile();
  return target->DispatchEvent(*before_input_event);
}

void InsertTextAndSendInputEventsOfTypeInsertReplacementText(
    LocalFrame &frame, const String &replacement, bool allow_edit_context) {
  // TODO(editing-dev): The use of UpdateStyleAndLayout
  // needs to be audited.  See http://crbug.com/590369 for more details.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc",
      "InsertTextAndSendInputEventsOfTypeInsertReplacementText",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "frame", base::ToString(frame));
  xtrace->LocalVarUpdate(xtrace_mrid, "replacement",
                         base::ToString(replacement));
  xtrace->LocalVarUpdate(xtrace_mrid, "allow_edit_context",
                         base::ToString(allow_edit_context));
  xtrace->LogLineRun(xtrace_mrid, 1678);
  frame.GetDocument()->UpdateStyleAndLayout(DocumentUpdateReason::kSpellCheck);

  xtrace->LogLineRun(xtrace_mrid, 1680);
  Document &current_document = *frame.GetDocument();

  // Dispatch 'beforeinput'.
  xtrace->LogLineRun(xtrace_mrid, 1683);
  Element *const target = FindEventTargetFrom(
      frame, frame.Selection().ComputeVisibleSelectionInDOMTree());
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");

  // Copy the original target text into a string, in case the 'beforeinput'
  // event handler modifies the text.
  xtrace->LogLineRun(xtrace_mrid, 1688);
  const String before_input_target_string = target->GetInnerTextWithoutUpdate();
  xtrace->LocalVarUpdate(xtrace_mrid, "before_input_target_string",
                         base::ToString(before_input_target_string));

  xtrace->LogLineRun(xtrace_mrid, 1690);
  DataTransfer *const data_transfer = DataTransfer::Create(
      DataTransfer::DataTransferType::kInsertReplacementText,
      DataTransferAccessPolicy::kReadable,
      DataObject::CreateFromString(replacement));
  xtrace->LocalVarUpdate(xtrace_mrid, "data_transfer",
                         data_transfer ? base::ToString(*data_transfer) : "");

  xtrace->LogLineRun(xtrace_mrid, 1695);
  const bool is_canceled =
      DispatchBeforeInputDataTransfer(
          target, InputEvent::InputType::kInsertReplacementText,
          data_transfer) != DispatchEventResult::kNotCanceled;
  xtrace->LocalVarUpdate(xtrace_mrid, "is_canceled",
                         base::ToString(is_canceled));

  // 'beforeinput' event handler may destroy target frame.
  xtrace->LogLineRun(xtrace_mrid, 1701);
  if (current_document != frame.GetDocument()) {
    xtrace->LogLineRun(xtrace_mrid, 1702);
    return;
  }

  // If the 'beforeinput' event handler has modified the input text, then the
  // replacement text shouldn't be inserted.
  xtrace->LogLineRun(xtrace_mrid, 1707);
  if (target->innerText() != before_input_target_string) {
    xtrace->LogLineRun(xtrace_mrid, 1708);
    return;
  }

  // When allowed, insert the text into the active edit context if it exists.
  xtrace->LogLineRun(xtrace_mrid, 1712);
  if (auto *edit_context =
          frame.GetInputMethodController().GetActiveEditContext()) {
    xtrace->LogLineRun(xtrace_mrid, 1714);
    if (allow_edit_context) {
      xtrace->LogLineRun(xtrace_mrid, 1715);
      edit_context->InsertText(replacement);
    }
    xtrace->LogLineRun(xtrace_mrid, 1717);
    return;
  }

  // TODO(editing-dev): The use of UpdateStyleAndLayout
  // needs to be audited.  See http://crbug.com/590369 for more details.
  xtrace->LogLineRun(xtrace_mrid, 1722);
  frame.GetDocument()->UpdateStyleAndLayout(DocumentUpdateReason::kSpellCheck);

  xtrace->LogLineRun(xtrace_mrid, 1724);
  if (is_canceled) {
    xtrace->LogLineRun(xtrace_mrid, 1725);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 1728);
  xtrace->FlushAllEventsToJSONFile();
  frame.GetEditor().InsertTextWithoutSendingTextEvent(
      replacement, false, nullptr,
      InputEvent::InputType::kInsertReplacementText);
}

// |IsEmptyNonEditableNodeInEditable()| is introduced for fixing
// http://crbug.com/428986.
static bool IsEmptyNonEditableNodeInEditable(const Node &node) {
  // Editability is defined the DOM tree rather than the flat tree. For example:
  // DOM:
  //   <host>
  //     <span>unedittable</span>
  //     <shadowroot><div ce><content /></div></shadowroot>
  //   </host>
  //
  // Flat Tree:
  //   <host><div ce><span1>unedittable</span></div></host>
  // e.g. editing/shadow/breaking-editing-boundaries.html
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "IsEmptyNonEditableNodeInEditable",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1746);
  xtrace->FlushAllEventsToJSONFile();
  return !NodeTraversal::HasChildren(node) && !IsEditable(node) &&
         node.parentNode() && IsEditable(*node.parentNode());
}

// TODO(yosin): We should not use |IsEmptyNonEditableNodeInEditable()| in
// |EditingIgnoresContent()| since |IsEmptyNonEditableNodeInEditable()|
// requires clean layout tree.
bool EditingIgnoresContent(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "EditingIgnoresContent",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1754);
  xtrace->FlushAllEventsToJSONFile();
  return !node.CanContainRangeEndPoint() ||
         IsEmptyNonEditableNodeInEditable(node);
}

ContainerNode *
RootEditableElementOrTreeScopeRootNodeOf(const Position &position) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "RootEditableElementOrTreeScopeRootNodeOf",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1760);
  Element *const selection_root = RootEditableElementOf(position);
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_root",
                         selection_root ? base::ToString(*selection_root) : "");

  xtrace->LogLineRun(xtrace_mrid, 1761);
  if (selection_root)
    return selection_root;

  xtrace->LogLineRun(xtrace_mrid, 1764);
  Node *const node = position.ComputeContainerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1765);
  xtrace->FlushAllEventsToJSONFile();
  return node ? &node->GetTreeScope().RootNode() : nullptr;
}

static scoped_refptr<Image> ImageFromNode(const Node &node) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "ImageFromNode",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1769);
  DCHECK(!node.GetDocument().NeedsLayoutTreeUpdate());
  xtrace->LogLineRun(xtrace_mrid, 1770);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      node.GetDocument().Lifecycle());
  xtrace->LocalVarUpdate(xtrace_mrid, "disallow_transition",
                         base::ToString(disallow_transition));

  xtrace->LogLineRun(xtrace_mrid, 1773);
  const LayoutObject *const layout_object = node.GetLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_object",
                         layout_object ? base::ToString(*layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 1774);
  if (!layout_object)
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1777);
  if (layout_object->IsCanvas()) {
    xtrace->LogLineRun(xtrace_mrid, 1778);
    return To<HTMLCanvasElement>(const_cast<Node &>(node))
        .Snapshot(FlushReason::kClipboard, kFrontBuffer);
  }

  xtrace->LogLineRun(xtrace_mrid, 1782);
  if (!layout_object->IsImage())
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1785);
  const auto &layout_image = To<LayoutImage>(*layout_object);
  xtrace->LogLineRun(xtrace_mrid, 1786);
  const ImageResourceContent *const cached_image = layout_image.CachedImage();
  xtrace->LocalVarUpdate(xtrace_mrid, "cached_image",
                         cached_image ? base::ToString(*cached_image) : "");

  xtrace->LogLineRun(xtrace_mrid, 1787);
  if (!cached_image || cached_image->ErrorOccurred())
    return nullptr;
  xtrace->LogLineRun(xtrace_mrid, 1789);
  xtrace->FlushAllEventsToJSONFile();
  return cached_image->GetImage();
}

AtomicString GetUrlStringFromNode(const Node &node) {
  // TODO(editing-dev): This should probably be reconciled with
  // HitTestResult::absoluteImageURL.
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "GetUrlStringFromNode",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LogLineRun(xtrace_mrid, 1795);
  if (IsA<HTMLImageElement>(node) || IsA<HTMLInputElement>(node))
    return To<HTMLElement>(node).FastGetAttribute(html_names::kSrcAttr);
  xtrace->LogLineRun(xtrace_mrid, 1797);
  if (IsA<SVGImageElement>(node))
    return To<SVGElement>(node).ImageSourceURL();
  xtrace->LogLineRun(xtrace_mrid, 1799);
  if (IsA<HTMLEmbedElement>(node) || IsA<HTMLObjectElement>(node) ||
      IsA<HTMLCanvasElement>(node))
    return To<HTMLElement>(node).ImageSourceURL();
  xtrace->LogLineRun(xtrace_mrid, 1802);
  xtrace->FlushAllEventsToJSONFile();
  return AtomicString();
}

void WriteImageToClipboard(SystemClipboard &system_clipboard,
                           const scoped_refptr<Image> &image,
                           const KURL &url_string, const String &title) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "WriteImageToClipboard",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "system_clipboard",
                         base::ToString(system_clipboard));
  xtrace->LocalVarUpdate(xtrace_mrid, "image", base::ToString(image));
  xtrace->LocalVarUpdate(xtrace_mrid, "url_string", base::ToString(url_string));
  xtrace->LocalVarUpdate(xtrace_mrid, "title", base::ToString(title));
  xtrace->LogLineRun(xtrace_mrid, 1809);
  system_clipboard.WriteImageWithTag(image.get(), url_string, title);
  xtrace->LogLineRun(xtrace_mrid, 1810);
  xtrace->FlushAllEventsToJSONFile();
  system_clipboard.CommitWrite();
}

void WriteImageNodeToClipboard(SystemClipboard &system_clipboard,
                               const Node &node, const String &title) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "WriteImageNodeToClipboard",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "system_clipboard",
                         base::ToString(system_clipboard));
  xtrace->LocalVarUpdate(xtrace_mrid, "node", base::ToString(node));
  xtrace->LocalVarUpdate(xtrace_mrid, "title", base::ToString(title));
  xtrace->LogLineRun(xtrace_mrid, 1816);
  const scoped_refptr<Image> image = ImageFromNode(node);
  xtrace->LocalVarUpdate(xtrace_mrid, "image", base::ToString(image));

  xtrace->LogLineRun(xtrace_mrid, 1817);
  if (!image.get())
    return;
  xtrace->LogLineRun(xtrace_mrid, 1819);
  const KURL url_string = node.GetDocument().CompleteURL(
      StripLeadingAndTrailingHTMLSpaces(GetUrlStringFromNode(node)));
  xtrace->LocalVarUpdate(xtrace_mrid, "url_string", base::ToString(url_string));

  xtrace->LogLineRun(xtrace_mrid, 1821);
  xtrace->FlushAllEventsToJSONFile();
  WriteImageToClipboard(system_clipboard, image, url_string, title);
}

Element *FindEventTargetFrom(LocalFrame &frame,
                             const VisibleSelection &selection) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("editing_utilities.cc", "FindEventTargetFrom",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "frame", base::ToString(frame));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 1826);
  Element *const target = AssociatedElementOf(selection.Start());
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");

  xtrace->LogLineRun(xtrace_mrid, 1827);
  if (!target)
    return frame.GetDocument()->body();
  xtrace->LogLineRun(xtrace_mrid, 1829);
  if (target->IsInUserAgentShadowRoot())
    return target->OwnerShadowHost();
  xtrace->LogLineRun(xtrace_mrid, 1831);
  xtrace->FlushAllEventsToJSONFile();
  return target;
}

HTMLImageElement *ImageElementFromImageDocument(const Document *document) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "editing_utilities.cc", "ImageElementFromImageDocument",
      "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "document",
                         document ? base::ToString(*document) : "");
  xtrace->LogLineRun(xtrace_mrid, 1835);
  if (!document)
    return nullptr;
  xtrace->LogLineRun(xtrace_mrid, 1837);
  if (!IsA<ImageDocument>(document))
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1840);
  const HTMLElement *const body = document->body();
  xtrace->LocalVarUpdate(xtrace_mrid, "body",
                         body ? base::ToString(*body) : "");

  xtrace->LogLineRun(xtrace_mrid, 1841);
  if (!body)
    return nullptr;

  xtrace->LogLineRun(xtrace_mrid, 1844);
  xtrace->FlushAllEventsToJSONFile();
  return DynamicTo<HTMLImageElement>(body->firstChild());
}

} // namespace blink

