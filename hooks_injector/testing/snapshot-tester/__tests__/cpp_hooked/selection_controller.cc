#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2012 Digia Plc. and/or its subsidiary(-ies)
 * Copyright (C) 2015 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/editing/selection_controller.h"

#include "base/auto_reset.h"
#include "third_party/blink/public/common/input/web_menu_source_type.h"
#include "third_party/blink/public/platform/web_input_event_result.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/editing/bidi_adjustment.h"
#include "third_party/blink/renderer/core/editing/editing_behavior.h"
#include "third_party/blink/renderer/core/editing/editing_boundary.h"
#include "third_party/blink/renderer/core/editing/editing_utilities.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/core/editing/markers/document_marker_controller.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/set_selection_options.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/editing/suggestion/text_suggestion_controller.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"
#include "third_party/blink/renderer/core/fragment_directive/text_fragment_handler.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "ui/gfx/geometry/point_conversions.h"

namespace blink {

SelectionController::SelectionController(LocalFrame &frame)
    : ExecutionContextLifecycleObserver(frame.DomWindow()), frame_(&frame),
      mouse_down_may_start_select_(false),
      mouse_down_was_single_click_in_selection_(false),
      mouse_down_allows_multi_click_(false),
      selection_state_(SelectionState::kHaveNotStartedSelection) {}

void SelectionController::Trace(Visitor *visitor) const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("selection_controller.cc",
                            "SelectionController::Trace", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "visitor",
                         visitor ? base::ToString(*visitor) : "");
  xtrace->LogLineRun(xtrace_mrid, 76);
  visitor->Trace(frame_);
  xtrace->LogLineRun(xtrace_mrid, 77);
  visitor->Trace(original_anchor_in_flat_tree_);
  xtrace->LogLineRun(xtrace_mrid, 78);
  ExecutionContextLifecycleObserver::Trace(visitor);
}

namespace {

DispatchEventResult DispatchSelectStart(Node *node) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "DispatchSelectStart", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 84);
  if (!node || !node->GetLayoutObject())
    return DispatchEventResult::kNotCanceled;

  xtrace->LogLineRun(xtrace_mrid, 87);
  return node->DispatchEvent(
      *Event::CreateCancelableBubble(event_type_names::kSelectstart));
}

SelectionInFlatTree
ExpandSelectionToRespectUserSelectAll(Node *target_node,
                                      const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "ExpandSelectionToRespectUserSelectAll",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "target_node",
                         target_node ? base::ToString(*target_node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 94);
  if (selection.IsNone())
    return SelectionInFlatTree();
  xtrace->LogLineRun(xtrace_mrid, 96);
  Node *const root_user_select_all =
      EditingInFlatTreeStrategy::RootUserSelectAllForNode(target_node);
  xtrace->LocalVarUpdate(
      xtrace_mrid, "root_user_select_all",
      root_user_select_all ? base::ToString(*root_user_select_all) : "");

  xtrace->LogLineRun(xtrace_mrid, 98);
  if (!root_user_select_all)
    return selection;
  xtrace->LogLineRun(xtrace_mrid, 100);
  return SelectionInFlatTree::Builder(selection)
      .Collapse(MostBackwardCaretPosition(
          PositionInFlatTree::BeforeNode(*root_user_select_all),
          kCanCrossEditingBoundary))
      .Extend(MostForwardCaretPosition(
          PositionInFlatTree::AfterNode(*root_user_select_all),
          kCanCrossEditingBoundary))
      .Build();
}

static int TextDistance(const PositionInFlatTree &start,
                        const PositionInFlatTree &end) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "TextDistance", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "start", base::ToString(start));
  xtrace->LocalVarUpdate(xtrace_mrid, "end", base::ToString(end));
  xtrace->LogLineRun(xtrace_mrid, 112);
  return TextIteratorInFlatTree::RangeLength(
      start, end,
      TextIteratorBehavior::AllVisiblePositionsRangeLengthBehavior());
}

bool CanMouseDownStartSelect(Node *node) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "CanMouseDownStartSelect", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 118);
  if (!node || !node->GetLayoutObject())
    return true;

  xtrace->LogLineRun(xtrace_mrid, 121);
  if (!node->CanStartSelection())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 124);
  return true;
}

PositionInFlatTreeWithAffinity
PositionWithAffinityOfHitTestResult(const HitTestResult &hit_test_result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "PositionWithAffinityOfHitTestResult",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 129);
  return FromPositionInDOMTree<EditingInFlatTreeStrategy>(
      hit_test_result.GetPosition());
}

DocumentMarkerGroup *SpellCheckMarkerGroupAtPosition(
    DocumentMarkerController &document_marker_controller,
    const PositionInFlatTree &position) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SpellCheckMarkerGroupAtPosition",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "document_marker_controller",
                         base::ToString(document_marker_controller));
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 136);
  return document_marker_controller.FirstMarkerGroupAroundPosition(
      position, DocumentMarker::MarkerTypes::Misspelling());
}

void MarkSelectionEndpointsForRepaint(const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "MarkSelectionEndpointsForRepaint",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 141);
  LayoutObject *anchor_layout_object =
      selection.Anchor().AnchorNode()->GetLayoutObject();
  xtrace->LocalVarUpdate(
      xtrace_mrid, "anchor_layout_object",
      anchor_layout_object ? base::ToString(*anchor_layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 143);
  if (anchor_layout_object) {
    xtrace->LogLineRun(xtrace_mrid, 144);
    if (auto *layer = anchor_layout_object->PaintingLayer())
      layer->SetNeedsRepaint();
  }

  xtrace->LogLineRun(xtrace_mrid, 148);
  LayoutObject *focus_layout_object =
      selection.Focus().AnchorNode()->GetLayoutObject();
  xtrace->LocalVarUpdate(
      xtrace_mrid, "focus_layout_object",
      focus_layout_object ? base::ToString(*focus_layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 150);
  if (focus_layout_object) {
    xtrace->LogLineRun(xtrace_mrid, 151);
    if (auto *layer = focus_layout_object->PaintingLayer()) {
      xtrace->LogLineRun(xtrace_mrid, 152);
      layer->SetNeedsRepaint();
    }
  }
}

bool IsNonSelectable(const Node *node) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "IsNonSelectable", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 158);
  LayoutObject *layout_object = node ? node->GetLayoutObject() : nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_object",
                         layout_object ? base::ToString(*layout_object) : "");

  xtrace->LogLineRun(xtrace_mrid, 159);
  return layout_object && !layout_object->IsSelectable();
}

inline bool ShouldIgnoreNodeForCheckSelectable(const Node *enclosing_block,
                                               const Node *node) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "ShouldIgnoreNodeForCheckSelectable",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "enclosing_block",
                         enclosing_block ? base::ToString(*enclosing_block)
                                         : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "node",
                         node ? base::ToString(*node) : "");
  xtrace->LogLineRun(xtrace_mrid, 164);
  return node == enclosing_block || (node && node->IsTextNode());
}

} // namespace

SelectionInFlatTree
AdjustSelectionWithTrailingWhitespace(const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "AdjustSelectionWithTrailingWhitespace",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 171);
  if (selection.IsNone())
    return selection;
  xtrace->LogLineRun(xtrace_mrid, 173);
  if (!selection.IsRange())
    return selection;
  xtrace->LogLineRun(xtrace_mrid, 175);
  const PositionInFlatTree &end = selection.ComputeEndPosition();
  xtrace->LocalVarUpdate(xtrace_mrid, "end", base::ToString(end));

  xtrace->LogLineRun(xtrace_mrid, 176);
  const PositionInFlatTree &new_end = SkipWhitespace(end);
  xtrace->LocalVarUpdate(xtrace_mrid, "new_end", base::ToString(new_end));

  xtrace->LogLineRun(xtrace_mrid, 177);
  if (end == new_end)
    return selection;
  xtrace->LogLineRun(xtrace_mrid, 179);
  if (selection.IsAnchorFirst()) {
    xtrace->LogLineRun(xtrace_mrid, 180);
    return SelectionInFlatTree::Builder(selection)
        .SetBaseAndExtent(selection.Anchor(), new_end)
        .Build();
  }
  xtrace->LogLineRun(xtrace_mrid, 184);
  return SelectionInFlatTree::Builder(selection)
      .SetBaseAndExtent(new_end, selection.Focus())
      .Build();
}

SelectionInFlatTree
AdjustSelectionByUserSelect(Node *anchor_node,
                            const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("selection_controller.cc",
                            "AdjustSelectionByUserSelect", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "anchor_node",
                         anchor_node ? base::ToString(*anchor_node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 192);
  DCHECK(anchor_node);

  xtrace->LogLineRun(xtrace_mrid, 194);
  if (selection.IsNone())
    return SelectionInFlatTree();

  xtrace->LogLineRun(xtrace_mrid, 197);
  SelectionInFlatTree expanded_selection =
      ExpandSelectionToRespectUserSelectAll(anchor_node, selection);
  xtrace->LocalVarUpdate(xtrace_mrid, "expanded_selection",
                         base::ToString(expanded_selection));

  xtrace->LogLineRun(xtrace_mrid, 199);
  Element *enclosing_block = EnclosingBlock(anchor_node);
  xtrace->LocalVarUpdate(xtrace_mrid, "enclosing_block",
                         enclosing_block ? base::ToString(*enclosing_block)
                                         : "");

  xtrace->LogLineRun(xtrace_mrid, 201);
  PositionInFlatTree anchor = expanded_selection.Anchor();
  xtrace->LocalVarUpdate(xtrace_mrid, "anchor", base::ToString(anchor));

  xtrace->LogLineRun(xtrace_mrid, 202);
  PositionInFlatTree new_start_pos =
      PositionInFlatTree::FirstPositionInNode(*anchor_node);
  xtrace->LocalVarUpdate(xtrace_mrid, "new_start_pos",
                         base::ToString(new_start_pos));

  xtrace->LogLineRun(xtrace_mrid, 204);
  for (PositionIteratorInFlatTree iter =
           PositionIteratorInFlatTree(new_start_pos);
       !iter.AtStart(); iter.Decrement()) {
    xtrace->LogLineRun(xtrace_mrid, 207);
    PositionInFlatTree current_pos = iter.ComputePosition();
    xtrace->LocalVarUpdate(xtrace_mrid, "current_pos",
                           base::ToString(current_pos));

    xtrace->LogLineRun(xtrace_mrid, 208);
    if (current_pos <= anchor) {
      xtrace->LogLineRun(xtrace_mrid, 209);
      new_start_pos = anchor;
      xtrace->LocalVarUpdate(xtrace_mrid, "new_start_pos",
                             base::ToString(new_start_pos));

      xtrace->LogLineRun(xtrace_mrid, 210);
      break;
    }

    xtrace->LogLineRun(xtrace_mrid, 213);
    if (!ShouldIgnoreNodeForCheckSelectable(enclosing_block, iter.GetNode()) &&
        IsNonSelectable(iter.GetNode())) {
      xtrace->LogLineRun(xtrace_mrid, 215);
      new_start_pos = current_pos;
      xtrace->LocalVarUpdate(xtrace_mrid, "new_start_pos",
                             base::ToString(new_start_pos));

      xtrace->LogLineRun(xtrace_mrid, 216);
      break;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 220);
  PositionInFlatTree focus = expanded_selection.Focus();
  xtrace->LocalVarUpdate(xtrace_mrid, "focus", base::ToString(focus));

  xtrace->LogLineRun(xtrace_mrid, 221);
  PositionInFlatTree new_end_pos =
      PositionInFlatTree::LastPositionInNode(*anchor_node);
  xtrace->LocalVarUpdate(xtrace_mrid, "new_end_pos",
                         base::ToString(new_end_pos));

  xtrace->LogLineRun(xtrace_mrid, 223);
  for (PositionIteratorInFlatTree iter =
           PositionIteratorInFlatTree(new_end_pos);
       !iter.AtEnd(); iter.Increment()) {
    xtrace->LogLineRun(xtrace_mrid, 226);
    PositionInFlatTree current_pos = iter.ComputePosition();
    xtrace->LocalVarUpdate(xtrace_mrid, "current_pos",
                           base::ToString(current_pos));

    xtrace->LogLineRun(xtrace_mrid, 227);
    if (current_pos >= focus) {
      xtrace->LogLineRun(xtrace_mrid, 228);
      new_end_pos = focus;
      xtrace->LocalVarUpdate(xtrace_mrid, "new_end_pos",
                             base::ToString(new_end_pos));

      xtrace->LogLineRun(xtrace_mrid, 229);
      break;
    }

    xtrace->LogLineRun(xtrace_mrid, 232);
    if (!ShouldIgnoreNodeForCheckSelectable(enclosing_block, iter.GetNode()) &&
        IsNonSelectable(iter.GetNode())) {
      xtrace->LogLineRun(xtrace_mrid, 234);
      new_end_pos = current_pos;
      xtrace->LocalVarUpdate(xtrace_mrid, "new_end_pos",
                             base::ToString(new_end_pos));

      xtrace->LogLineRun(xtrace_mrid, 235);
      break;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 239);
  return SelectionInFlatTree::Builder()
      .SetBaseAndExtent(new_start_pos, new_end_pos)
      .Build();
}

SelectionController::~SelectionController() = default;

Document &SelectionController::GetDocument() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::GetDocument",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 247);
  DCHECK(frame_->GetDocument());
  xtrace->LogLineRun(xtrace_mrid, 248);
  return *frame_->GetDocument();
}

void SelectionController::ContextDestroyed() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::ContextDestroyed",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 252);
  original_anchor_in_flat_tree_ = PositionInFlatTreeWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "original_anchor_in_flat_tree_",
                         base::ToString(original_anchor_in_flat_tree_));
}

static PositionInFlatTreeWithAffinity AdjustPositionRespectUserSelectAll(
    Node *inner_node, const PositionInFlatTree &selection_start,
    const PositionInFlatTree &selection_end,
    const PositionInFlatTreeWithAffinity &position) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "AdjustPositionRespectUserSelectAll",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_start",
                         base::ToString(selection_start));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_end",
                         base::ToString(selection_end));
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 260);
  const SelectionInFlatTree selection_in_user_select_all =
      CreateVisibleSelection(
          ExpandSelectionToRespectUserSelectAll(
              inner_node,
              position.IsNull()
                  ? SelectionInFlatTree()
                  : SelectionInFlatTree::Builder().Collapse(position).Build()))
          .AsSelection();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_in_user_select_all",
                         base::ToString(selection_in_user_select_all));

  xtrace->LogLineRun(xtrace_mrid, 268);
  if (!selection_in_user_select_all.IsRange())
    return position;
  xtrace->LogLineRun(xtrace_mrid, 270);
  if (selection_in_user_select_all.ComputeStartPosition().CompareTo(
          selection_start) < 0) {
    xtrace->LogLineRun(xtrace_mrid, 272);
    return PositionInFlatTreeWithAffinity(
        selection_in_user_select_all.ComputeStartPosition());
  }
  // TODO(xiaochengh): Do we need to use upstream affinity for end?
  xtrace->LogLineRun(xtrace_mrid, 276);
  if (selection_end.CompareTo(
          selection_in_user_select_all.ComputeEndPosition()) < 0) {
    xtrace->LogLineRun(xtrace_mrid, 278);
    return PositionInFlatTreeWithAffinity(
        selection_in_user_select_all.ComputeEndPosition());
  }
  xtrace->LogLineRun(xtrace_mrid, 281);
  return position;
}

static PositionInFlatTree
ComputeStartFromEndForExtendForward(const PositionInFlatTree &end,
                                    TextGranularity granularity) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "ComputeStartFromEndForExtendForward",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "end", base::ToString(end));
  xtrace->LocalVarUpdate(xtrace_mrid, "granularity",
                         base::ToString(granularity));
  xtrace->LogLineRun(xtrace_mrid, 287);
  if (granularity == TextGranularity::kCharacter)
    return end;
  // |ComputeStartRespectingGranularity()| returns next word/paragraph for
  // end of word/paragraph position. To get start of word/paragraph at |end|,
  // we pass previous position of |end|.
  xtrace->LogLineRun(xtrace_mrid, 292);
  return ComputeStartRespectingGranularity(
      PositionInFlatTreeWithAffinity(
          PreviousPositionOf(CreateVisiblePosition(end),
                             kCannotCrossEditingBoundary)
              .DeepEquivalent()),
      granularity);
}

static SelectionInFlatTree
ExtendSelectionAsDirectional(const PositionInFlatTreeWithAffinity &position,
                             const SelectionInFlatTree &selection,
                             TextGranularity granularity) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("selection_controller.cc",
                            "ExtendSelectionAsDirectional", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LocalVarUpdate(xtrace_mrid, "granularity",
                         base::ToString(granularity));
  xtrace->LogLineRun(xtrace_mrid, 304);
  DCHECK(!selection.IsNone());
  xtrace->LogLineRun(xtrace_mrid, 305);
  DCHECK(position.IsNotNull());
  xtrace->LogLineRun(xtrace_mrid, 306);
  const PositionInFlatTree &anchor = selection.Anchor();
  xtrace->LocalVarUpdate(xtrace_mrid, "anchor", base::ToString(anchor));

  xtrace->LogLineRun(xtrace_mrid, 307);
  if (position.GetPosition() < anchor) {
    // Extend backward yields backward selection
    //  - forward selection:  *abc ^def ghi| => |abc def^ ghi
    //  - backward selection: *abc |def ghi^ => |abc def ghi^
    xtrace->LogLineRun(xtrace_mrid, 311);
    const PositionInFlatTree &new_start = ComputeStartRespectingGranularity(
        PositionInFlatTreeWithAffinity(position), granularity);
    xtrace->LocalVarUpdate(xtrace_mrid, "new_start", base::ToString(new_start));

    xtrace->LogLineRun(xtrace_mrid, 313);
    const PositionInFlatTree &new_end =
        selection.IsAnchorFirst()
            ? ComputeEndRespectingGranularity(
                  new_start, PositionInFlatTreeWithAffinity(anchor),
                  granularity)
            : anchor;
    xtrace->LocalVarUpdate(xtrace_mrid, "new_end", base::ToString(new_end));

    xtrace->LogLineRun(xtrace_mrid, 319);
    if (new_start.IsNull() || new_end.IsNull()) {
      // By some reasons, we fail to extend `selection`.
      // TODO(crbug.com/1386012) We want to have a test case of this.
      xtrace->LogLineRun(xtrace_mrid, 322);
      return selection;
    }
    xtrace->LogLineRun(xtrace_mrid, 324);
    SelectionInFlatTree::Builder builder;
    xtrace->LogLineRun(xtrace_mrid, 325);
    builder.SetBaseAndExtent(new_end, new_start);
    xtrace->LogLineRun(xtrace_mrid, 326);
    if (new_start == new_end)
      builder.SetAffinity(position.Affinity());
    xtrace->LogLineRun(xtrace_mrid, 328);
    return builder.Build();
  }

  // Extend forward yields forward selection
  //  - forward selection:  ^abc def| ghi* => ^abc def ghi|
  //  - backward selection: |abc def^ ghi* => abc ^def ghi|
  xtrace->LogLineRun(xtrace_mrid, 334);
  const PositionInFlatTree &new_start =
      selection.IsAnchorFirst()
          ? anchor
          : ComputeStartFromEndForExtendForward(anchor, granularity);
  xtrace->LocalVarUpdate(xtrace_mrid, "new_start", base::ToString(new_start));

  xtrace->LogLineRun(xtrace_mrid, 338);
  const PositionInFlatTree &new_end = ComputeEndRespectingGranularity(
      new_start, PositionInFlatTreeWithAffinity(position), granularity);
  xtrace->LocalVarUpdate(xtrace_mrid, "new_end", base::ToString(new_end));

  xtrace->LogLineRun(xtrace_mrid, 340);
  if (new_start.IsNull() || new_end.IsNull()) {
    // By some reasons, we fail to extend `selection`.
    // TODO(crbug.com/1386012) We want to have a test case of this.
    xtrace->LogLineRun(xtrace_mrid, 343);
    return selection;
  }
  xtrace->LogLineRun(xtrace_mrid, 345);
  SelectionInFlatTree::Builder builder;
  xtrace->LogLineRun(xtrace_mrid, 346);
  builder.SetBaseAndExtent(new_start, new_end);
  xtrace->LogLineRun(xtrace_mrid, 347);
  if (new_start == new_end)
    builder.SetAffinity(position.Affinity());
  xtrace->LogLineRun(xtrace_mrid, 349);
  return builder.Build();
}

static SelectionInFlatTree
ExtendSelectionAsNonDirectional(const PositionInFlatTree &position,
                                const SelectionInFlatTree &selection,
                                TextGranularity granularity) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "ExtendSelectionAsNonDirectional",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LocalVarUpdate(xtrace_mrid, "granularity",
                         base::ToString(granularity));
  xtrace->LogLineRun(xtrace_mrid, 356);
  DCHECK(!selection.IsNone());
  xtrace->LogLineRun(xtrace_mrid, 357);
  DCHECK(position.IsNotNull());
  // Shift+Click deselects when selection was created right-to-left
  xtrace->LogLineRun(xtrace_mrid, 359);
  const PositionInFlatTree &start = selection.ComputeStartPosition();
  xtrace->LocalVarUpdate(xtrace_mrid, "start", base::ToString(start));

  xtrace->LogLineRun(xtrace_mrid, 360);
  const PositionInFlatTree &end = selection.ComputeEndPosition();
  xtrace->LocalVarUpdate(xtrace_mrid, "end", base::ToString(end));

  xtrace->LogLineRun(xtrace_mrid, 361);
  if (start == end && position == start)
    return selection;
  xtrace->LogLineRun(xtrace_mrid, 363);
  if (position < start) {
    xtrace->LogLineRun(xtrace_mrid, 364);
    return SelectionInFlatTree::Builder()
        .SetBaseAndExtent(
            end, ComputeStartRespectingGranularity(
                     PositionInFlatTreeWithAffinity(position), granularity))
        .Build();
  }
  xtrace->LogLineRun(xtrace_mrid, 370);
  if (end < position) {
    xtrace->LogLineRun(xtrace_mrid, 371);
    return SelectionInFlatTree::Builder()
        .SetBaseAndExtent(
            start,
            ComputeEndRespectingGranularity(
                start, PositionInFlatTreeWithAffinity(position), granularity))
        .Build();
  }
  xtrace->LogLineRun(xtrace_mrid, 378);
  const int distance_to_start = TextDistance(start, position);
  xtrace->LocalVarUpdate(xtrace_mrid, "distance_to_start",
                         base::ToString(distance_to_start));

  xtrace->LogLineRun(xtrace_mrid, 379);
  const int distance_to_end = TextDistance(position, end);
  xtrace->LocalVarUpdate(xtrace_mrid, "distance_to_end",
                         base::ToString(distance_to_end));

  xtrace->LogLineRun(xtrace_mrid, 380);
  if (distance_to_start <= distance_to_end) {
    xtrace->LogLineRun(xtrace_mrid, 381);
    return SelectionInFlatTree::Builder()
        .SetBaseAndExtent(
            end, ComputeStartRespectingGranularity(
                     PositionInFlatTreeWithAffinity(position), granularity))
        .Build();
  }
  xtrace->LogLineRun(xtrace_mrid, 387);
  return SelectionInFlatTree::Builder()
      .SetBaseAndExtent(
          start,
          ComputeEndRespectingGranularity(
              start, PositionInFlatTreeWithAffinity(position), granularity))
      .Build();
}

// Updating the selection is considered side-effect of the event and so it
// doesn't impact the handled state.
bool SelectionController::HandleSingleClick(
    const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleSingleClick",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 399);
  TRACE_EVENT0("blink",
               "SelectionController::handleMousePressEventSingleClick");

  xtrace->LogLineRun(xtrace_mrid, 402);
  DCHECK(!frame_->GetDocument()->NeedsLayoutTreeUpdate());
  xtrace->LogLineRun(xtrace_mrid, 403);
  Node *inner_node = event.InnerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 404);
  Node *inner_pseudo = event.GetHitTestResult().InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_pseudo",
                         inner_pseudo ? base::ToString(*inner_pseudo) : "");

  xtrace->LogLineRun(xtrace_mrid, 405);
  if (!(inner_node && inner_node->GetLayoutObject() && inner_pseudo &&
        inner_pseudo->GetLayoutObject() && mouse_down_may_start_select_))
    return false;

  // Extend the selection if the Shift key is down, unless the click is in a
  // link or image.
  xtrace->LogLineRun(xtrace_mrid, 411);
  bool extend_selection = IsExtendingSelection(event);
  xtrace->LocalVarUpdate(xtrace_mrid, "extend_selection",
                         base::ToString(extend_selection));

  xtrace->LogLineRun(xtrace_mrid, 413);
  const PositionInFlatTreeWithAffinity visible_hit_position =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(event.GetHitTestResult()))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_hit_position",
                         base::ToString(visible_hit_position));

  xtrace->LogLineRun(xtrace_mrid, 417);
  const PositionInFlatTreeWithAffinity &position_to_use =
      visible_hit_position.IsNull()
          ? CreateVisiblePosition(
                PositionInFlatTree::FirstPositionInOrBeforeNode(*inner_node))
                .ToPositionWithAffinity()
          : visible_hit_position;
  xtrace->LocalVarUpdate(xtrace_mrid, "position_to_use",
                         base::ToString(position_to_use));

  xtrace->LogLineRun(xtrace_mrid, 423);
  const VisibleSelectionInFlatTree &selection =
      Selection().ComputeVisibleSelectionInFlatTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));

  xtrace->LogLineRun(xtrace_mrid, 425);
  const bool is_editable = IsEditable(*inner_node);
  xtrace->LocalVarUpdate(xtrace_mrid, "is_editable",
                         base::ToString(is_editable));

  xtrace->LogLineRun(xtrace_mrid, 427);
  if (frame_->GetEditor().Behavior().ShouldToggleMenuWhenCaretTapped() &&
      is_editable && event.Event().FromTouch() && selection.IsCaret() &&
      selection.Anchor() == position_to_use.GetPosition()) {
    xtrace->LogLineRun(xtrace_mrid, 430);
    mouse_down_was_single_click_on_caret_ = true;
    xtrace->LocalVarUpdate(
        xtrace_mrid, "mouse_down_was_single_click_on_caret_",
        base::ToString(mouse_down_was_single_click_on_caret_));

    xtrace->LogLineRun(xtrace_mrid, 431);
    HandleTapOnCaret(event, selection.AsSelection());
    xtrace->LogLineRun(xtrace_mrid, 432);
    return false;
  }

  // Don't restart the selection when the mouse is pressed on an
  // existing selection so we can allow for text dragging.
  xtrace->LogLineRun(xtrace_mrid, 437);
  if (LocalFrameView *view = frame_->View()) {
    xtrace->LogLineRun(xtrace_mrid, 438);
    const PhysicalOffset v_point(view->ConvertFromRootFrame(
        gfx::ToFlooredPoint(event.Event().PositionInRootFrame())));
    xtrace->LocalVarUpdate(xtrace_mrid, "v_point", base::ToString(v_point));

    xtrace->LogLineRun(xtrace_mrid, 440);
    if (!extend_selection && Selection().Contains(v_point)) {
      xtrace->LogLineRun(xtrace_mrid, 441);
      mouse_down_was_single_click_in_selection_ = true;
      xtrace->LocalVarUpdate(
          xtrace_mrid, "mouse_down_was_single_click_in_selection_",
          base::ToString(mouse_down_was_single_click_in_selection_));

      xtrace->LogLineRun(xtrace_mrid, 442);
      if (!event.Event().FromTouch())
        return false;

      xtrace->LogLineRun(xtrace_mrid, 445);
      if (HandleTapInsideSelection(event, selection.AsSelection()))
        return false;
    }
  }

  xtrace->LogLineRun(xtrace_mrid, 450);
  if (extend_selection && !selection.IsNone()) {
    // Note: "fast/events/shift-click-user-select-none.html" makes
    // |pos.isNull()| true.
    xtrace->LogLineRun(xtrace_mrid, 453);
    const PositionInFlatTreeWithAffinity adjusted_position =
        AdjustPositionRespectUserSelectAll(inner_node, selection.Start(),
                                           selection.End(), position_to_use);
    xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_position",
                           base::ToString(adjusted_position));

    xtrace->LogLineRun(xtrace_mrid, 456);
    const TextGranularity granularity = Selection().Granularity();
    xtrace->LocalVarUpdate(xtrace_mrid, "granularity",
                           base::ToString(granularity));

    xtrace->LogLineRun(xtrace_mrid, 457);
    if (adjusted_position.IsNull()) {
      xtrace->LogLineRun(xtrace_mrid, 458);
      UpdateSelectionForMouseDownDispatchingSelectStart(
          inner_node, selection.AsSelection(),
          SetSelectionOptions::Builder().SetGranularity(granularity).Build());
      xtrace->LogLineRun(xtrace_mrid, 461);
      return false;
    }
    xtrace->LogLineRun(xtrace_mrid, 463);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node,
        frame_->GetEditor().Behavior().ShouldConsiderSelectionAsDirectional()
            ? ExtendSelectionAsDirectional(adjusted_position,
                                           selection.AsSelection(), granularity)
            : ExtendSelectionAsNonDirectional(adjusted_position.GetPosition(),
                                              selection.AsSelection(),
                                              granularity),
        SetSelectionOptions::Builder().SetGranularity(granularity).Build());
    xtrace->LogLineRun(xtrace_mrid, 472);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 475);
  if (selection_state_ == SelectionState::kExtendedSelection) {
    xtrace->LogLineRun(xtrace_mrid, 476);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node, selection.AsSelection(), SetSelectionOptions());
    xtrace->LogLineRun(xtrace_mrid, 478);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 481);
  if (position_to_use.IsNull()) {
    xtrace->LogLineRun(xtrace_mrid, 482);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node, SelectionInFlatTree(), SetSelectionOptions());
    xtrace->LogLineRun(xtrace_mrid, 484);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 487);
  bool is_handle_visible = false;
  xtrace->LocalVarUpdate(xtrace_mrid, "is_handle_visible",
                         base::ToString(is_handle_visible));

  xtrace->LogLineRun(xtrace_mrid, 488);
  if (is_editable) {
    xtrace->LogLineRun(xtrace_mrid, 489);
    const bool is_text_box_empty =
        !RootEditableElement(*inner_node)->HasChildren();
    xtrace->LocalVarUpdate(xtrace_mrid, "is_text_box_empty",
                           base::ToString(is_text_box_empty));

    xtrace->LogLineRun(xtrace_mrid, 491);
    const bool not_left_click =
        event.Event().button != WebPointerProperties::Button::kLeft;
    xtrace->LocalVarUpdate(xtrace_mrid, "not_left_click",
                           base::ToString(not_left_click));

    xtrace->LogLineRun(xtrace_mrid, 493);
    if (!is_text_box_empty || not_left_click)
      is_handle_visible = event.Event().FromTouch();
  }

  // This applies the JavaScript selectstart handler, which can change the DOM.
  // SelectionControllerTest_SelectStartHandlerRemovesElement makes this return
  // false.
  xtrace->LogLineRun(xtrace_mrid, 500);
  if (!UpdateSelectionForMouseDownDispatchingSelectStart(
          inner_node,
          ExpandSelectionToRespectUserSelectAll(
              inner_node,
              SelectionInFlatTree::Builder().Collapse(position_to_use).Build()),
          SetSelectionOptions::Builder()
              .SetShouldShowHandle(is_handle_visible)
              .Build())) {
    // UpdateSelectionForMouseDownDispatchingSelectStart() returns false when
    // the selectstart handler has prevented the default selection behavior from
    // occurring.
    xtrace->LogLineRun(xtrace_mrid, 511);
    return false;
  }

  // SelectionControllerTest_SetCaretAtHitTestResultWithDisconnectedPosition
  // makes the IsValidFor() check fail.
  xtrace->LogLineRun(xtrace_mrid, 516);
  if (is_editable && event.Event().FromTouch() &&
      position_to_use.IsValidFor(*frame_->GetDocument())) {
    xtrace->LogLineRun(xtrace_mrid, 518);
    frame_->GetTextSuggestionController().HandlePotentialSuggestionTap(
        position_to_use.GetPosition());
  }

  xtrace->LogLineRun(xtrace_mrid, 522);
  return false;
}

// Returns true if the tap is processed.
void SelectionController::HandleTapOnCaret(
    const MouseEventWithHitTestResults &event,
    const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleTapOnCaret",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 529);
  Node *inner_node = event.InnerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 530);
  const bool is_text_box_empty =
      !RootEditableElement(*inner_node)->HasChildren();
  xtrace->LocalVarUpdate(xtrace_mrid, "is_text_box_empty",
                         base::ToString(is_text_box_empty));

  // If the textbox is empty, tapping the caret should toggle showing/hiding the
  // handle. Otherwise, always show the handle.
  xtrace->LogLineRun(xtrace_mrid, 535);
  const bool should_show_handle =
      !is_text_box_empty || !Selection().IsHandleVisible();
  xtrace->LocalVarUpdate(xtrace_mrid, "should_show_handle",
                         base::ToString(should_show_handle));

  // Repaint the caret to ensure that the handle is shown if needed.
  xtrace->LogLineRun(xtrace_mrid, 539);
  MarkSelectionEndpointsForRepaint(selection);
  xtrace->LogLineRun(xtrace_mrid, 540);
  const bool did_select = UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node, selection,
      SetSelectionOptions::Builder()
          .SetShouldShowHandle(should_show_handle)
          .Build());
  xtrace->LocalVarUpdate(xtrace_mrid, "did_select", base::ToString(did_select));

  xtrace->LogLineRun(xtrace_mrid, 545);
  if (did_select) {
    xtrace->LogLineRun(xtrace_mrid, 546);
    frame_->GetEventHandler().ShowNonLocatedContextMenu(nullptr,
                                                        kMenuSourceTouch);
  }
}

// Returns true if the tap is processed.
bool SelectionController::HandleTapInsideSelection(
    const MouseEventWithHitTestResults &event,
    const SelectionInFlatTree &selection) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::HandleTapInsideSelection", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LogLineRun(xtrace_mrid, 555);
  if (Selection().ShouldShrinkNextTap()) {
    xtrace->LogLineRun(xtrace_mrid, 556);
    const bool did_select = SelectClosestWordFromHitTestResult(
        event.GetHitTestResult(), AppendTrailingWhitespace::kDontAppend,
        SelectInputEventType::kTouch);
    xtrace->LocalVarUpdate(xtrace_mrid, "did_select",
                           base::ToString(did_select));

    xtrace->LogLineRun(xtrace_mrid, 559);
    if (did_select) {
      xtrace->LogLineRun(xtrace_mrid, 560);
      frame_->GetEventHandler().ShowNonLocatedContextMenu(
          nullptr, kMenuSourceAdjustSelectionReset);
    }
    xtrace->LogLineRun(xtrace_mrid, 563);
    return true;
  }

  xtrace->LogLineRun(xtrace_mrid, 566);
  if (Selection().IsHandleVisible())
    return false;

  // We need to trigger a repaint on the selection endpoints if the selection is
  // tapped when the selection handle was previously not visible. Repainting
  // will record the painted selection bounds and send it through the pipeline
  // so the handles show up in the next frame after the tap.
  xtrace->LogLineRun(xtrace_mrid, 573);
  MarkSelectionEndpointsForRepaint(selection);

  xtrace->LogLineRun(xtrace_mrid, 575);
  const bool did_select = UpdateSelectionForMouseDownDispatchingSelectStart(
      event.InnerNode(), selection,
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
  xtrace->LocalVarUpdate(xtrace_mrid, "did_select", base::ToString(did_select));

  xtrace->LogLineRun(xtrace_mrid, 578);
  if (did_select) {
    xtrace->LogLineRun(xtrace_mrid, 579);
    frame_->GetEventHandler().ShowNonLocatedContextMenu(nullptr,
                                                        kMenuSourceTouch);
  }
  xtrace->LogLineRun(xtrace_mrid, 582);
  return true;
}

WebInputEventResult SelectionController::UpdateSelectionForMouseDrag(
    const HitTestResult &hit_test_result,
    const PhysicalOffset &last_known_mouse_position) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::UpdateSelectionForMouseDrag", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LocalVarUpdate(xtrace_mrid, "last_known_mouse_position",
                         base::ToString(last_known_mouse_position));
  xtrace->LogLineRun(xtrace_mrid, 588);
  if (!mouse_down_may_start_select_)
    return WebInputEventResult::kNotHandled;

  xtrace->LogLineRun(xtrace_mrid, 591);
  Node *target = hit_test_result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "target",
                         target ? base::ToString(*target) : "");

  xtrace->LogLineRun(xtrace_mrid, 592);
  if (!target)
    return WebInputEventResult::kNotHandled;

  // TODO(editing-dev): Use of UpdateStyleAndLayout
  // needs to be audited.  See http://crbug.com/590369 for more details.
  xtrace->LogLineRun(xtrace_mrid, 597);
  frame_->GetDocument()->UpdateStyleAndLayout(DocumentUpdateReason::kSelection);

  xtrace->LogLineRun(xtrace_mrid, 599);
  const PositionWithAffinity &raw_target_position =
      Selection().SelectionHasFocus()
          ? PositionRespectingEditingBoundary(
                Selection().ComputeVisibleSelectionInDOMTree().Start(),
                hit_test_result)
          : PositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "raw_target_position",
                         base::ToString(raw_target_position));

  xtrace->LogLineRun(xtrace_mrid, 605);
  const PositionInFlatTreeWithAffinity target_position =
      CreateVisiblePosition(
          FromPositionInDOMTree<EditingInFlatTreeStrategy>(raw_target_position))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "target_position",
                         base::ToString(target_position));

  // Don't modify the selection if we're not on a node.
  xtrace->LogLineRun(xtrace_mrid, 611);
  if (target_position.IsNull())
    return WebInputEventResult::kNotHandled;

  // Restart the selection if this is the first mouse move. This work is usually
  // done in handleMousePressEvent, but not if the mouse press was on an
  // existing selection.

  xtrace->LogLineRun(xtrace_mrid, 618);
  if (selection_state_ == SelectionState::kHaveNotStartedSelection &&
      DispatchSelectStart(target) != DispatchEventResult::kNotCanceled) {
    xtrace->LogLineRun(xtrace_mrid, 620);
    return WebInputEventResult::kHandledApplication;
  }

  // |DispatchSelectStart()| can change |GetDocument()| or invalidate
  // target_position by 'selectstart' event handler.
  // TODO(editing-dev): We should also add a regression test when above
  // behaviour happens. See crbug.com/775149.
  xtrace->LogLineRun(xtrace_mrid, 627);
  if (!Selection().IsAvailable() || !target_position.IsValidFor(GetDocument()))
    return WebInputEventResult::kNotHandled;

  xtrace->LogLineRun(xtrace_mrid, 630);
  const bool should_extend_selection =
      selection_state_ == SelectionState::kExtendedSelection;
  xtrace->LocalVarUpdate(xtrace_mrid, "should_extend_selection",
                         base::ToString(should_extend_selection));

  // Always extend selection here because it's caused by a mouse drag
  xtrace->LogLineRun(xtrace_mrid, 633);
  selection_state_ = SelectionState::kExtendedSelection;
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                         base::ToString(selection_state_));

  xtrace->LogLineRun(xtrace_mrid, 635);
  const VisibleSelectionInFlatTree &visible_selection =
      Selection().ComputeVisibleSelectionInFlatTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_selection",
                         base::ToString(visible_selection));

  xtrace->LogLineRun(xtrace_mrid, 637);
  if (visible_selection.IsNone()) {
    // TODO(editing-dev): This is an urgent fix to crbug.com/745501. We should
    // find the root cause and replace this by a proper fix.
    xtrace->LogLineRun(xtrace_mrid, 640);
    return WebInputEventResult::kNotHandled;
  }

  xtrace->LogLineRun(xtrace_mrid, 643);
  const PositionInFlatTreeWithAffinity adjusted_position =
      AdjustPositionRespectUserSelectAll(target, visible_selection.Start(),
                                         visible_selection.End(),
                                         target_position);
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_position",
                         base::ToString(adjusted_position));

  xtrace->LogLineRun(xtrace_mrid, 647);
  const SelectionInFlatTree &adjusted_selection =
      should_extend_selection
          ? ExtendSelectionAsDirectional(adjusted_position,
                                         visible_selection.AsSelection(),
                                         Selection().Granularity())
          : SelectionInFlatTree::Builder().Collapse(adjusted_position).Build();
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_selection",
                         base::ToString(adjusted_selection));

  // When |adjusted_selection| is caret, it's already canonical. No need to re-
  // canonicalize it.
  xtrace->LogLineRun(xtrace_mrid, 656);
  const SelectionInFlatTree new_visible_selection =
      adjusted_selection.IsRange()
          ? CreateVisibleSelection(adjusted_selection).AsSelection()
          : adjusted_selection;
  xtrace->LocalVarUpdate(xtrace_mrid, "new_visible_selection",
                         base::ToString(new_visible_selection));

  xtrace->LogLineRun(xtrace_mrid, 660);
  if (new_visible_selection.IsNone()) {
    // See http://crbug.com/1412880
    xtrace->LogLineRun(xtrace_mrid, 662);
    return WebInputEventResult::kNotHandled;
  }
  xtrace->LogLineRun(xtrace_mrid, 664);
  const bool selection_is_directional =
      should_extend_selection && Selection().IsDirectional();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_is_directional",
                         base::ToString(selection_is_directional));

  xtrace->LogLineRun(xtrace_mrid, 666);
  SetNonDirectionalSelectionIfNeeded(
      new_visible_selection,
      SetSelectionOptions::Builder()
          .SetGranularity(Selection().Granularity())
          .SetIsDirectional(selection_is_directional)
          .Build(),
      kAdjustEndpointsAtBidiBoundary);

  xtrace->LogLineRun(xtrace_mrid, 674);
  return WebInputEventResult::kHandledSystem;
}

bool SelectionController::UpdateSelectionForMouseDownDispatchingSelectStart(
    Node *target_node, const SelectionInFlatTree &selection,
    const SetSelectionOptions &set_selection_options) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::UpdateSelectionForMouseDownDispatchingSelectStart",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "target_node",
                         target_node ? base::ToString(*target_node) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));
  xtrace->LocalVarUpdate(xtrace_mrid, "set_selection_options",
                         base::ToString(set_selection_options));
  xtrace->LogLineRun(xtrace_mrid, 681);
  if (target_node && target_node->GetLayoutObject() &&
      !target_node->GetLayoutObject()->IsSelectable())
    return false;

  {
    SelectionInFlatTree::InvalidSelectionResetter resetter(selection);
    if (DispatchSelectStart(target_node) != DispatchEventResult::kNotCanceled)
      return false;
  }

  // |DispatchSelectStart()| can change document hosted by |frame_|.
  xtrace->LogLineRun(xtrace_mrid, 692);
  if (!Selection().IsAvailable())
    return false;

  // TODO(editing-dev): Use of UpdateStyleAndLayout
  // needs to be audited.  See http://crbug.com/590369 for more details.
  xtrace->LogLineRun(xtrace_mrid, 697);
  GetDocument().UpdateStyleAndLayout(DocumentUpdateReason::kSelection);
  xtrace->LogLineRun(xtrace_mrid, 698);
  const SelectionInFlatTree visible_selection =
      CreateVisibleSelection(selection).AsSelection();
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_selection",
                         base::ToString(visible_selection));

  xtrace->LogLineRun(xtrace_mrid, 701);
  if (visible_selection.IsRange()) {
    xtrace->LogLineRun(xtrace_mrid, 702);
    selection_state_ = SelectionState::kExtendedSelection;
    xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                           base::ToString(selection_state_));

    xtrace->LogLineRun(xtrace_mrid, 703);
    SetNonDirectionalSelectionIfNeeded(visible_selection, set_selection_options,
                                       kDoNotAdjustEndpoints);
    xtrace->LogLineRun(xtrace_mrid, 705);
    return true;
  }

  xtrace->LogLineRun(xtrace_mrid, 708);
  selection_state_ = SelectionState::kPlacedCaret;
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                         base::ToString(selection_state_));

  xtrace->LogLineRun(xtrace_mrid, 709);
  SetNonDirectionalSelectionIfNeeded(visible_selection, set_selection_options,
                                     kDoNotAdjustEndpoints);
  xtrace->LogLineRun(xtrace_mrid, 711);
  return true;
}

bool SelectionController::SelectClosestWordFromHitTestResult(
    const HitTestResult &result,
    AppendTrailingWhitespace append_trailing_whitespace,
    SelectInputEventType select_input_event_type) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SelectClosestWordFromHitTestResult",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));
  xtrace->LocalVarUpdate(xtrace_mrid, "append_trailing_whitespace",
                         base::ToString(append_trailing_whitespace));
  xtrace->LocalVarUpdate(xtrace_mrid, "select_input_event_type",
                         base::ToString(select_input_event_type));
  xtrace->LogLineRun(xtrace_mrid, 718);
  Node *const inner_node = result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 720);
  if (!inner_node || !inner_node->GetLayoutObject() ||
      !inner_node->GetLayoutObject()->IsSelectable())
    return false;

  // Special-case image local offset to always be zero, to avoid triggering
  // LayoutReplaced::positionFromPoint's advancement of the position at the
  // mid-point of the the image (which was intended for mouse-drag selection
  // and isn't desirable for touch).
  xtrace->LogLineRun(xtrace_mrid, 728);
  HitTestResult adjusted_hit_test_result = result;
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_hit_test_result",
                         base::ToString(adjusted_hit_test_result));

  xtrace->LogLineRun(xtrace_mrid, 729);
  if (select_input_event_type == SelectInputEventType::kTouch &&
      result.GetImage()) {
    xtrace->LogLineRun(xtrace_mrid, 731);
    adjusted_hit_test_result.SetNodeAndPosition(
        result.InnerPossiblyPseudoNode(), PhysicalOffset());
  }

  xtrace->LogLineRun(xtrace_mrid, 735);
  const PositionInFlatTreeWithAffinity pos =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(adjusted_hit_test_result))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "pos", base::ToString(pos));

  xtrace->LogLineRun(xtrace_mrid, 739);
  const SelectionInFlatTree new_selection =
      pos.IsNotNull()
          ? ExpandWithGranularity(
                SelectionInFlatTree::Builder().Collapse(pos).Build(),
                TextGranularity::kWord)
          : SelectionInFlatTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                         base::ToString(new_selection));

  // TODO(editing-dev): Fix CreateVisibleSelectionWithGranularity() to not
  // return invalid ranges. Until we do that, we need this check here to avoid a
  // renderer crash when we call PlainText() below (see crbug.com/735774).
  xtrace->LogLineRun(xtrace_mrid, 749);
  if (new_selection.IsNone() ||
      new_selection.ComputeStartPosition() > new_selection.ComputeEndPosition())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 753);
  if (select_input_event_type == SelectInputEventType::kTouch) {
    // If node doesn't have text except space, tab or line break, do not
    // select that 'empty' area.
    xtrace->LogLineRun(xtrace_mrid, 756);
    EphemeralRangeInFlatTree range = new_selection.ComputeRange();
    xtrace->LocalVarUpdate(xtrace_mrid, "range", base::ToString(range));

    xtrace->LogLineRun(xtrace_mrid, 757);
    const String word =
        PlainText(range, TextIteratorBehavior::Builder()
                             .SetEmitsObjectReplacementCharacter(IsEditable(
                                 *range.StartPosition().AnchorNode()))
                             .Build());
    xtrace->LocalVarUpdate(xtrace_mrid, "word", base::ToString(word));

    xtrace->LogLineRun(xtrace_mrid, 762);
    if (word.length() >= 1 && word[0] == '\n') {
      // We should not select word from end of line, e.g.
      // "(1)|\n(2)" => "(1)^\n(|2)". See http://crbug.com/974569
      xtrace->LogLineRun(xtrace_mrid, 765);
      return false;
    }
    xtrace->LogLineRun(xtrace_mrid, 767);
    if (word.SimplifyWhiteSpace().ContainsOnlyWhitespaceOrEmpty())
      return false;

    xtrace->LogLineRun(xtrace_mrid, 770);
    Element *const editable =
        RootEditableElementOf(new_selection.ComputeStartPosition());
    xtrace->LocalVarUpdate(xtrace_mrid, "editable",
                           editable ? base::ToString(*editable) : "");

    xtrace->LogLineRun(xtrace_mrid, 772);
    if (editable && pos.GetPosition() ==
                        VisiblePositionInFlatTree::LastPositionInNode(*editable)
                            .DeepEquivalent())
      return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 778);
  const SelectionInFlatTree &adjusted_selection =
      append_trailing_whitespace == AppendTrailingWhitespace::kShouldAppend
          ? AdjustSelectionWithTrailingWhitespace(new_selection)
          : new_selection;
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_selection",
                         base::ToString(adjusted_selection));

  xtrace->LogLineRun(xtrace_mrid, 783);
  return UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node,
      ExpandSelectionToRespectUserSelectAll(inner_node, adjusted_selection),
      SetSelectionOptions::Builder()
          .SetGranularity(TextGranularity::kWord)
          .SetShouldShowHandle(select_input_event_type ==
                               SelectInputEventType::kTouch)
          .Build());
}

void SelectionController::SelectClosestMisspellingFromHitTestResult(
    const HitTestResult &result,
    AppendTrailingWhitespace append_trailing_whitespace) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SelectClosestMisspellingFromHitTestResult",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));
  xtrace->LocalVarUpdate(xtrace_mrid, "append_trailing_whitespace",
                         base::ToString(append_trailing_whitespace));
  xtrace->LogLineRun(xtrace_mrid, 796);
  Node *inner_node = result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 798);
  if (!inner_node || !inner_node->GetLayoutObject())
    return;

  xtrace->LogLineRun(xtrace_mrid, 801);
  const PositionInFlatTreeWithAffinity pos =
      CreateVisiblePosition(PositionWithAffinityOfHitTestResult(result))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "pos", base::ToString(pos));

  xtrace->LogLineRun(xtrace_mrid, 804);
  if (pos.IsNull()) {
    xtrace->LogLineRun(xtrace_mrid, 805);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node, SelectionInFlatTree(),
        SetSelectionOptions::Builder()
            .SetGranularity(TextGranularity::kWord)
            .Build());
    xtrace->LogLineRun(xtrace_mrid, 810);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 813);
  const PositionInFlatTree &marker_position =
      pos.GetPosition().ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "marker_position",
                         base::ToString(marker_position));

  xtrace->LogLineRun(xtrace_mrid, 815);
  const DocumentMarkerGroup *const marker_group =
      SpellCheckMarkerGroupAtPosition(inner_node->GetDocument().Markers(),
                                      marker_position);
  xtrace->LocalVarUpdate(xtrace_mrid, "marker_group",
                         marker_group ? base::ToString(*marker_group) : "");

  xtrace->LogLineRun(xtrace_mrid, 818);
  if (!marker_group) {
    xtrace->LogLineRun(xtrace_mrid, 819);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node, SelectionInFlatTree(),
        SetSelectionOptions::Builder()
            .SetGranularity(TextGranularity::kWord)
            .Build());
    xtrace->LogLineRun(xtrace_mrid, 824);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 827);
  const SelectionInFlatTree new_selection =
      CreateVisibleSelection(
          SelectionInFlatTree::Builder()
              .Collapse(marker_group->StartPositionInFlatTree())
              .Extend(marker_group->EndPositionInFlatTree())
              .Build())
          .AsSelection();
  xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                         base::ToString(new_selection));

  xtrace->LogLineRun(xtrace_mrid, 834);
  const SelectionInFlatTree &adjusted_selection =
      append_trailing_whitespace == AppendTrailingWhitespace::kShouldAppend
          ? AdjustSelectionWithTrailingWhitespace(new_selection)
          : new_selection;
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_selection",
                         base::ToString(adjusted_selection));

  xtrace->LogLineRun(xtrace_mrid, 838);
  UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node,
      ExpandSelectionToRespectUserSelectAll(inner_node, adjusted_selection),
      SetSelectionOptions::Builder()
          .SetGranularity(TextGranularity::kWord)
          .Build());
}

template <typename MouseEventObject>
bool SelectionController::SelectClosestWordFromMouseEvent(
    const MouseEventObject *mouse_event, const HitTestResult &result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SelectClosestWordFromMouseEvent", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_event",
                         mouse_event ? base::ToString(*mouse_event) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));
  xtrace->LogLineRun(xtrace_mrid, 850);
  if (!mouse_down_may_start_select_)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 853);
  AppendTrailingWhitespace append_trailing_whitespace =
      (mouse_event->ClickCount() == 2 &&
       frame_->GetEditor().IsSelectTrailingWhitespaceEnabled())
          ? AppendTrailingWhitespace::kShouldAppend
          : AppendTrailingWhitespace::kDontAppend;
  xtrace->LocalVarUpdate(xtrace_mrid, "append_trailing_whitespace",
                         base::ToString(append_trailing_whitespace));

  xtrace->LogLineRun(xtrace_mrid, 859);
  DCHECK(!frame_->GetDocument()->NeedsLayoutTreeUpdate());

  xtrace->LogLineRun(xtrace_mrid, 861);
  return SelectClosestWordFromHitTestResult(result, append_trailing_whitespace,
                                            mouse_event->FromTouch()
                                                ? SelectInputEventType::kTouch
                                                : SelectInputEventType::kMouse);
}

template <typename MouseEventObject>
void SelectionController::SelectClosestMisspellingFromMouseEvent(
    const MouseEventObject *mouse_event, const HitTestResult &hit_test_result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SelectClosestMisspellingFromMouseEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_event",
                         mouse_event ? base::ToString(*mouse_event) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 871);
  if (!mouse_down_may_start_select_)
    return;

  xtrace->LogLineRun(xtrace_mrid, 874);
  SelectClosestMisspellingFromHitTestResult(
      hit_test_result, (mouse_event->ClickCount() == 2 &&
                        frame_->GetEditor().IsSelectTrailingWhitespaceEnabled())
                           ? AppendTrailingWhitespace::kShouldAppend
                           : AppendTrailingWhitespace::kDontAppend);
}

template <typename MouseEventObject>
void SelectionController::SelectClosestWordOrLinkFromMouseEvent(
    const MouseEventObject *mouse_event, const HitTestResult &hit_test_result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SelectClosestWordOrLinkFromMouseEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_event",
                         mouse_event ? base::ToString(*mouse_event) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 885);
  if (!hit_test_result.IsLiveLink()) {
    xtrace->LogLineRun(xtrace_mrid, 886);
    SelectClosestWordFromMouseEvent(mouse_event, hit_test_result);
    xtrace->LogLineRun(xtrace_mrid, 887);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 890);
  Node *const inner_node = hit_test_result.InnerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 892);
  if (!inner_node || !inner_node->GetLayoutObject() ||
      !mouse_down_may_start_select_)
    return;

  xtrace->LogLineRun(xtrace_mrid, 896);
  Element *url_element = hit_test_result.URLElement();
  xtrace->LocalVarUpdate(xtrace_mrid, "url_element",
                         url_element ? base::ToString(*url_element) : "");

  xtrace->LogLineRun(xtrace_mrid, 897);
  const PositionInFlatTreeWithAffinity pos =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(hit_test_result))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "pos", base::ToString(pos));

  xtrace->LogLineRun(xtrace_mrid, 901);
  const SelectionInFlatTree &new_selection =
      pos.IsNotNull() && pos.AnchorNode()->IsDescendantOf(url_element)
          ? SelectionInFlatTree::Builder()
                .SelectAllChildren(*url_element)
                .Build()
          : SelectionInFlatTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                         base::ToString(new_selection));

  xtrace->LogLineRun(xtrace_mrid, 908);
  UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node,
      ExpandSelectionToRespectUserSelectAll(inner_node, new_selection),
      SetSelectionOptions::Builder()
          .SetGranularity(TextGranularity::kWord)
          .Build());
}

// TODO(yosin): We should take |granularity| and |handleVisibility| from
// |newSelection|.
// We should rename this function to appropriate name because
// set_selection_options has selection directional value in few cases.
void SelectionController::SetNonDirectionalSelectionIfNeeded(
    const SelectionInFlatTree &new_selection,
    const SetSelectionOptions &set_selection_options,
    EndPointsAdjustmentMode endpoints_adjustment_mode) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SetNonDirectionalSelectionIfNeeded",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                         base::ToString(new_selection));
  xtrace->LocalVarUpdate(xtrace_mrid, "set_selection_options",
                         base::ToString(set_selection_options));
  xtrace->LocalVarUpdate(xtrace_mrid, "endpoints_adjustment_mode",
                         base::ToString(endpoints_adjustment_mode));
  xtrace->LogLineRun(xtrace_mrid, 924);
  DCHECK(!GetDocument().NeedsLayoutTreeUpdate());

  // TODO(editing-dev): We should use |PositionWithAffinity| to pass affinity
  // to |CreateVisiblePosition()| for |original_anchor|.
  xtrace->LogLineRun(xtrace_mrid, 928);
  const PositionInFlatTree &anchor_position =
      original_anchor_in_flat_tree_.GetPosition();
  xtrace->LocalVarUpdate(xtrace_mrid, "anchor_position",
                         base::ToString(anchor_position));

  xtrace->LogLineRun(xtrace_mrid, 930);
  const PositionInFlatTreeWithAffinity original_anchor =
      anchor_position.IsConnected()
          ? CreateVisiblePosition(anchor_position).ToPositionWithAffinity()
          : PositionInFlatTreeWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "original_anchor",
                         base::ToString(original_anchor));

  xtrace->LogLineRun(xtrace_mrid, 934);
  const PositionInFlatTreeWithAffinity anchor =
      original_anchor.IsNotNull()
          ? original_anchor
          : CreateVisiblePosition(new_selection.Anchor())
                .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "anchor", base::ToString(anchor));

  xtrace->LogLineRun(xtrace_mrid, 939);
  const PositionInFlatTreeWithAffinity focus =
      CreateVisiblePosition(new_selection.Focus()).ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "focus", base::ToString(focus));

  xtrace->LogLineRun(xtrace_mrid, 941);
  const SelectionInFlatTree &adjusted_selection =
      endpoints_adjustment_mode == kAdjustEndpointsAtBidiBoundary
          ? BidiAdjustment::AdjustForRangeSelection(anchor, focus)
          : SelectionInFlatTree::Builder()
                .SetBaseAndExtent(anchor.GetPosition(), focus.GetPosition())
                .Build();
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_selection",
                         base::ToString(adjusted_selection));

  xtrace->LogLineRun(xtrace_mrid, 948);
  SelectionInFlatTree::Builder builder(new_selection);
  xtrace->LocalVarUpdate(xtrace_mrid, "builder", base::ToString(builder));

  xtrace->LogLineRun(xtrace_mrid, 949);
  if (adjusted_selection.Anchor() != anchor.GetPosition() ||
      adjusted_selection.Focus() != focus.GetPosition()) {
    xtrace->LogLineRun(xtrace_mrid, 951);
    original_anchor_in_flat_tree_ = anchor;
    xtrace->LocalVarUpdate(xtrace_mrid, "original_anchor_in_flat_tree_",
                           base::ToString(original_anchor_in_flat_tree_));

    xtrace->LogLineRun(xtrace_mrid, 952);
    SetExecutionContext(frame_->DomWindow());
    xtrace->LogLineRun(xtrace_mrid, 953);
    builder.SetBaseAndExtent(adjusted_selection.Anchor(),
                             adjusted_selection.Focus());
  } else if (original_anchor.IsNotNull()) {
    xtrace->LogLineRun(xtrace_mrid, 956);
    if (CreateVisiblePosition(
            Selection().ComputeVisibleSelectionInFlatTree().Anchor())
            .DeepEquivalent() ==
        CreateVisiblePosition(new_selection.Anchor()).DeepEquivalent()) {
      xtrace->LogLineRun(xtrace_mrid, 960);
      builder.SetBaseAndExtent(original_anchor.GetPosition(),
                               new_selection.Focus());
    }
    xtrace->LogLineRun(xtrace_mrid, 963);
    original_anchor_in_flat_tree_ = PositionInFlatTreeWithAffinity();
    xtrace->LocalVarUpdate(xtrace_mrid, "original_anchor_in_flat_tree_",
                           base::ToString(original_anchor_in_flat_tree_));
  }

  xtrace->LogLineRun(xtrace_mrid, 966);
  const bool selection_is_directional =
      frame_->GetEditor().Behavior().ShouldConsiderSelectionAsDirectional() ||
      set_selection_options.IsDirectional();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_is_directional",
                         base::ToString(selection_is_directional));

  xtrace->LogLineRun(xtrace_mrid, 969);
  const SelectionInFlatTree &selection_in_flat_tree = builder.Build();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_in_flat_tree",
                         base::ToString(selection_in_flat_tree));

  xtrace->LogLineRun(xtrace_mrid, 971);
  const bool selection_remains_the_same =
      Selection().ComputeVisibleSelectionInFlatTree() ==
          CreateVisibleSelection(selection_in_flat_tree) &&
      Selection().IsHandleVisible() ==
          set_selection_options.ShouldShowHandle() &&
      selection_is_directional == Selection().IsDirectional();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_remains_the_same",
                         base::ToString(selection_remains_the_same));

  // If selection has not changed we do not clear editing style.
  xtrace->LogLineRun(xtrace_mrid, 979);
  if (selection_remains_the_same)
    return;
  xtrace->LogLineRun(xtrace_mrid, 981);
  Selection().SetSelection(
      ConvertToSelectionInDOMTree(selection_in_flat_tree),
      SetSelectionOptions::Builder(set_selection_options)
          .SetShouldCloseTyping(true)
          .SetShouldClearTypingStyle(true)
          .SetIsDirectional(selection_is_directional)
          .SetCursorAlignOnScroll(CursorAlignOnScroll::kIfNeeded)
          .Build());
}

void SelectionController::SetCaretAtHitTestResult(
    const HitTestResult &hit_test_result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::SetCaretAtHitTestResult",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 993);
  Node *inner_node = hit_test_result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 994);
  DCHECK(inner_node);
  xtrace->LogLineRun(xtrace_mrid, 995);
  const PositionInFlatTreeWithAffinity visible_hit_pos =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(hit_test_result))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_hit_pos",
                         base::ToString(visible_hit_pos));

  xtrace->LogLineRun(xtrace_mrid, 999);
  const PositionInFlatTreeWithAffinity visible_pos =
      visible_hit_pos.IsNull()
          ? CreateVisiblePosition(
                PositionInFlatTree::FirstPositionInOrBeforeNode(*inner_node))
                .ToPositionWithAffinity()
          : visible_hit_pos;
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_pos",
                         base::ToString(visible_pos));

  xtrace->LogLineRun(xtrace_mrid, 1006);
  if (visible_pos.IsNull()) {
    xtrace->LogLineRun(xtrace_mrid, 1007);
    UpdateSelectionForMouseDownDispatchingSelectStart(
        inner_node, SelectionInFlatTree(),
        SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
    xtrace->LogLineRun(xtrace_mrid, 1010);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 1012);
  UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node,
      ExpandSelectionToRespectUserSelectAll(
          inner_node,
          SelectionInFlatTree::Builder().Collapse(visible_pos).Build()),
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
}

bool SelectionController::HandleDoubleClick(
    const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleDoubleClick",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1022);
  TRACE_EVENT0("blink",
               "SelectionController::handleMousePressEventDoubleClick");

  xtrace->LogLineRun(xtrace_mrid, 1025);
  if (!Selection().IsAvailable())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1028);
  if (!mouse_down_allows_multi_click_)
    return HandleSingleClick(event);

  xtrace->LogLineRun(xtrace_mrid, 1031);
  if (event.Event().button != WebPointerProperties::Button::kLeft)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1034);
  if (Selection().ComputeVisibleSelectionInDOMTreeDeprecated().IsRange()) {
    // A double-click when range is already selected
    // should not change the selection.  So, do not call
    // SelectClosestWordFromMouseEvent, but do set
    // began_selecting_text_ to prevent HandleMouseReleaseEvent
    // from setting caret selection.
    xtrace->LogLineRun(xtrace_mrid, 1040);
    selection_state_ = SelectionState::kExtendedSelection;
    xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                           base::ToString(selection_state_));

    xtrace->LogLineRun(xtrace_mrid, 1041);
    return true;
  }
  xtrace->LogLineRun(xtrace_mrid, 1043);
  if (!SelectClosestWordFromMouseEvent(&event.Event(),
                                       event.GetHitTestResult()))
    return true;
  xtrace->LogLineRun(xtrace_mrid, 1046);
  if (!Selection().IsHandleVisible())
    return true;
  xtrace->LogLineRun(xtrace_mrid, 1048);
  frame_->GetEventHandler().ShowNonLocatedContextMenu(nullptr,
                                                      kMenuSourceTouch);
  xtrace->LogLineRun(xtrace_mrid, 1050);
  return true;
}

bool SelectionController::HandleTripleClick(
    const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleTripleClick",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1055);
  TRACE_EVENT0("blink",
               "SelectionController::handleMousePressEventTripleClick");

  xtrace->LogLineRun(xtrace_mrid, 1058);
  if (!Selection().IsAvailable()) {
    // editing/shadow/doubleclick-on-meter-in-shadow-crash.html reach here.
    xtrace->LogLineRun(xtrace_mrid, 1060);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 1063);
  if (!mouse_down_allows_multi_click_)
    return HandleSingleClick(event);

  xtrace->LogLineRun(xtrace_mrid, 1066);
  if (event.Event().button != WebPointerProperties::Button::kLeft)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1069);
  Node *const inner_node = event.InnerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1070);
  Node *inner_pseudo = event.GetHitTestResult().InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_pseudo",
                         inner_pseudo ? base::ToString(*inner_pseudo) : "");

  xtrace->LogLineRun(xtrace_mrid, 1071);
  if (!(inner_node && inner_node->GetLayoutObject() && inner_pseudo &&
        inner_pseudo->GetLayoutObject() && mouse_down_may_start_select_))
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1075);
  const PositionInFlatTreeWithAffinity pos =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(event.GetHitTestResult()))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "pos", base::ToString(pos));

  xtrace->LogLineRun(xtrace_mrid, 1079);
  const SelectionInFlatTree new_selection =
      pos.IsNotNull()
          ? ExpandWithGranularity(
                SelectionInFlatTree::Builder().Collapse(pos).Build(),
                TextGranularity::kParagraph)
          : SelectionInFlatTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                         base::ToString(new_selection));

  xtrace->LogLineRun(xtrace_mrid, 1085);
  const SelectionInFlatTree adjusted_selection =
      AdjustSelectionByUserSelect(inner_node, new_selection);
  xtrace->LocalVarUpdate(xtrace_mrid, "adjusted_selection",
                         base::ToString(adjusted_selection));

  xtrace->LogLineRun(xtrace_mrid, 1088);
  const bool is_handle_visible =
      event.Event().FromTouch() && new_selection.IsRange();
  xtrace->LocalVarUpdate(xtrace_mrid, "is_handle_visible",
                         base::ToString(is_handle_visible));

  xtrace->LogLineRun(xtrace_mrid, 1091);
  const bool did_select = UpdateSelectionForMouseDownDispatchingSelectStart(
      inner_node, adjusted_selection,
      SetSelectionOptions::Builder()
          .SetGranularity(TextGranularity::kParagraph)
          .SetShouldShowHandle(is_handle_visible)
          .Build());
  xtrace->LocalVarUpdate(xtrace_mrid, "did_select", base::ToString(did_select));

  xtrace->LogLineRun(xtrace_mrid, 1097);
  if (!did_select)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1100);
  if (!Selection().IsHandleVisible())
    return true;
  xtrace->LogLineRun(xtrace_mrid, 1102);
  frame_->GetEventHandler().ShowNonLocatedContextMenu(nullptr,
                                                      kMenuSourceTouch);
  xtrace->LogLineRun(xtrace_mrid, 1104);
  return true;
}

bool SelectionController::HandleMousePressEvent(
    const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleMousePressEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1109);
  TRACE_EVENT0("blink", "SelectionController::handleMousePressEvent");

  // If we got the event back, that must mean it wasn't prevented,
  // so it's allowed to start a drag or selection if it wasn't in a scrollbar.
  xtrace->LogLineRun(xtrace_mrid, 1113);
  mouse_down_may_start_select_ = (CanMouseDownStartSelect(event.InnerNode()) ||
                                  IsSelectionOverLink(event)) &&
                                 !event.GetScrollbar();
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_may_start_select_",
                         base::ToString(mouse_down_may_start_select_));

  xtrace->LogLineRun(xtrace_mrid, 1116);
  mouse_down_was_single_click_on_caret_ = false;
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_was_single_click_on_caret_",
                         base::ToString(mouse_down_was_single_click_on_caret_));

  xtrace->LogLineRun(xtrace_mrid, 1117);
  mouse_down_was_single_click_in_selection_ = false;
  xtrace->LocalVarUpdate(
      xtrace_mrid, "mouse_down_was_single_click_in_selection_",
      base::ToString(mouse_down_was_single_click_in_selection_));

  xtrace->LogLineRun(xtrace_mrid, 1118);
  if (!Selection().IsAvailable()) {
    // "gesture-tap-frame-removed.html" reaches here.
    xtrace->LogLineRun(xtrace_mrid, 1120);
    mouse_down_allows_multi_click_ = !event.Event().FromTouch();
    xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_allows_multi_click_",
                           base::ToString(mouse_down_allows_multi_click_));

  } else {
    // Avoid double-tap touch gesture confusion by restricting multi-click side
    // effects, e.g., word selection, to editable regions.
    xtrace->LogLineRun(xtrace_mrid, 1124);
    mouse_down_allows_multi_click_ =
        !event.Event().FromTouch() ||
        IsEditablePosition(
            Selection().ComputeVisibleSelectionInDOMTreeDeprecated().Start());
    xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_allows_multi_click_",
                           base::ToString(mouse_down_allows_multi_click_));
  }

  xtrace->LogLineRun(xtrace_mrid, 1130);
  if (event.Event().click_count >= 3)
    return HandleTripleClick(event);
  xtrace->LogLineRun(xtrace_mrid, 1132);
  if (event.Event().click_count == 2)
    return HandleDoubleClick(event);
  xtrace->LogLineRun(xtrace_mrid, 1134);
  return HandleSingleClick(event);
}

WebInputEventResult SelectionController::HandleMouseDraggedEvent(
    const MouseEventWithHitTestResults &event, const gfx::Point &mouse_down_pos,
    const PhysicalOffset &last_known_mouse_position) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleMouseDraggedEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_pos",
                         base::ToString(mouse_down_pos));
  xtrace->LocalVarUpdate(xtrace_mrid, "last_known_mouse_position",
                         base::ToString(last_known_mouse_position));
  xtrace->LogLineRun(xtrace_mrid, 1141);
  TRACE_EVENT0("blink", "SelectionController::handleMouseDraggedEvent");

  xtrace->LogLineRun(xtrace_mrid, 1143);
  if (!Selection().IsAvailable())
    return WebInputEventResult::kNotHandled;
  xtrace->LogLineRun(xtrace_mrid, 1145);
  if (selection_state_ != SelectionState::kExtendedSelection) {
    xtrace->LogLineRun(xtrace_mrid, 1146);
    HitTestRequest request(HitTestRequest::kReadOnly | HitTestRequest::kActive);
    xtrace->LocalVarUpdate(xtrace_mrid, "request", base::ToString(request));

    xtrace->LogLineRun(xtrace_mrid, 1147);
    HitTestLocation location(mouse_down_pos);
    xtrace->LocalVarUpdate(xtrace_mrid, "location", base::ToString(location));

    xtrace->LogLineRun(xtrace_mrid, 1148);
    HitTestResult result(request, location);
    xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));

    xtrace->LogLineRun(xtrace_mrid, 1149);
    frame_->GetDocument()->GetLayoutView()->HitTest(location, result);

    xtrace->LogLineRun(xtrace_mrid, 1151);
    UpdateSelectionForMouseDrag(result, last_known_mouse_position);
  }
  xtrace->LogLineRun(xtrace_mrid, 1153);
  return UpdateSelectionForMouseDrag(event.GetHitTestResult(),
                                     last_known_mouse_position);
}

void SelectionController::UpdateSelectionForMouseDrag(
    const PhysicalOffset &drag_start_pos_in_root_frame,
    const PhysicalOffset &last_known_mouse_position_in_root_frame) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::UpdateSelectionForMouseDrag", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "drag_start_pos_in_root_frame",
                         base::ToString(drag_start_pos_in_root_frame));
  xtrace->LocalVarUpdate(
      xtrace_mrid, "last_known_mouse_position_in_root_frame",
      base::ToString(last_known_mouse_position_in_root_frame));
  xtrace->LogLineRun(xtrace_mrid, 1160);
  LocalFrameView *view = frame_->View();
  xtrace->LocalVarUpdate(xtrace_mrid, "view",
                         view ? base::ToString(*view) : "");

  xtrace->LogLineRun(xtrace_mrid, 1161);
  if (!view)
    return;
  xtrace->LogLineRun(xtrace_mrid, 1163);
  LayoutView *layout_view = frame_->ContentLayoutObject();
  xtrace->LocalVarUpdate(xtrace_mrid, "layout_view",
                         layout_view ? base::ToString(*layout_view) : "");

  xtrace->LogLineRun(xtrace_mrid, 1164);
  if (!layout_view)
    return;

  xtrace->LogLineRun(xtrace_mrid, 1167);
  HitTestRequest request(HitTestRequest::kReadOnly | HitTestRequest::kActive |
                         HitTestRequest::kMove);
  xtrace->LocalVarUpdate(xtrace_mrid, "request", base::ToString(request));

  xtrace->LogLineRun(xtrace_mrid, 1169);
  HitTestLocation location(
      view->ConvertFromRootFrame(last_known_mouse_position_in_root_frame));
  xtrace->LocalVarUpdate(xtrace_mrid, "location", base::ToString(location));

  xtrace->LogLineRun(xtrace_mrid, 1171);
  HitTestResult result(request, location);
  xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));

  xtrace->LogLineRun(xtrace_mrid, 1172);
  layout_view->HitTest(location, result);
  xtrace->LogLineRun(xtrace_mrid, 1173);
  UpdateSelectionForMouseDrag(result, last_known_mouse_position_in_root_frame);
}

bool SelectionController::HandleMouseReleaseEvent(
    const MouseEventWithHitTestResults &event,
    const PhysicalOffset &drag_start_pos) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleMouseReleaseEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LocalVarUpdate(xtrace_mrid, "drag_start_pos",
                         base::ToString(drag_start_pos));
  xtrace->LogLineRun(xtrace_mrid, 1179);
  TRACE_EVENT0("blink", "SelectionController::handleMouseReleaseEvent");

  xtrace->LogLineRun(xtrace_mrid, 1181);
  if (!Selection().IsAvailable())
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1184);
  bool handled = false;
  xtrace->LocalVarUpdate(xtrace_mrid, "handled", base::ToString(handled));

  xtrace->LogLineRun(xtrace_mrid, 1185);
  mouse_down_may_start_select_ = false;
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_may_start_select_",
                         base::ToString(mouse_down_may_start_select_));

  // Clear the selection if the mouse didn't move after the last mouse
  // press and it's not a context menu click.  We do this so when clicking
  // on the selection, the selection goes away.  However, if we are
  // editing, place the caret.
  xtrace->LogLineRun(xtrace_mrid, 1190);
  if (mouse_down_was_single_click_in_selection_ &&
      selection_state_ != SelectionState::kExtendedSelection &&
      drag_start_pos == PhysicalOffset(gfx::ToFlooredPoint(
                            event.Event().PositionInRootFrame())) &&
      Selection().ComputeVisibleSelectionInDOMTreeDeprecated().IsRange() &&
      event.Event().button != WebPointerProperties::Button::kRight) {
    // TODO(editing-dev): Use of UpdateStyleAndLayout
    // needs to be audited.  See http://crbug.com/590369 for more details.
    xtrace->LogLineRun(xtrace_mrid, 1198);
    frame_->GetDocument()->UpdateStyleAndLayout(
        DocumentUpdateReason::kSelection);

    xtrace->LogLineRun(xtrace_mrid, 1201);
    SelectionInFlatTree::Builder builder;
    xtrace->LogLineRun(xtrace_mrid, 1202);
    Node *node = event.InnerNode();
    xtrace->LocalVarUpdate(xtrace_mrid, "node",
                           node ? base::ToString(*node) : "");

    xtrace->LogLineRun(xtrace_mrid, 1203);
    if (node && node->GetLayoutObject() && IsEditable(*node)) {
      xtrace->LogLineRun(xtrace_mrid, 1204);
      const PositionInFlatTreeWithAffinity pos =
          CreateVisiblePosition(
              PositionWithAffinityOfHitTestResult(event.GetHitTestResult()))
              .ToPositionWithAffinity();
      xtrace->LocalVarUpdate(xtrace_mrid, "pos", base::ToString(pos));

      xtrace->LogLineRun(xtrace_mrid, 1208);
      if (pos.IsNotNull())
        builder.Collapse(pos);
    }

    xtrace->LogLineRun(xtrace_mrid, 1212);
    const SelectionInFlatTree new_selection = builder.Build();
    xtrace->LocalVarUpdate(xtrace_mrid, "new_selection",
                           base::ToString(new_selection));

    xtrace->LogLineRun(xtrace_mrid, 1213);
    if (Selection().ComputeVisibleSelectionInFlatTree() !=
        CreateVisibleSelection(new_selection)) {
      xtrace->LogLineRun(xtrace_mrid, 1215);
      Selection().SetSelectionAndEndTyping(
          ConvertToSelectionInDOMTree(new_selection));
    }

    xtrace->LogLineRun(xtrace_mrid, 1219);
    handled = true;
    xtrace->LocalVarUpdate(xtrace_mrid, "handled", base::ToString(handled));
  }

  xtrace->LogLineRun(xtrace_mrid, 1222);
  Selection().NotifyTextControlOfSelectionChange(SetSelectionBy::kUser);

  xtrace->LogLineRun(xtrace_mrid, 1224);
  Selection().SelectFrameElementInParentIfFullySelected();

  xtrace->LogLineRun(xtrace_mrid, 1226);
  if (event.Event().button == WebPointerProperties::Button::kMiddle &&
      !event.IsOverLink()) {
    // Ignore handled, since we want to paste to where the caret was placed
    // anyway.
    xtrace->LogLineRun(xtrace_mrid, 1230);
    handled = HandlePasteGlobalSelection(event.Event()) || handled;
    xtrace->LocalVarUpdate(xtrace_mrid, "handled", base::ToString(handled));
  }

  xtrace->LogLineRun(xtrace_mrid, 1233);
  return handled;
}

bool SelectionController::HandlePasteGlobalSelection(
    const WebMouseEvent &mouse_event) {
  // If the event was a middle click, attempt to copy global selection in after
  // the newly set caret position.
  //
  // This code is called from either the mouse up or mouse down handling. There
  // is some debate about when the global selection is pasted:
  //   xterm: pastes on up.
  //   GTK: pastes on down.
  //   Qt: pastes on up.
  //   Firefox: pastes on up.
  //   Chromium: pastes on up.
  //
  // There is something of a webcompat angle to this well, as highlighted by
  // crbug.com/14608. Pages can clear text boxes 'onclick' and, if we paste on
  // down then the text is pasted just before the onclick handler runs and
  // clears the text box. So it's important this happens after the event
  // handlers have been fired.
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::HandlePasteGlobalSelection", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_event",
                         base::ToString(mouse_event));
  xtrace->LogLineRun(xtrace_mrid, 1254);
  if (mouse_event.GetType() != WebInputEvent::Type::kMouseUp)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1257);
  if (!frame_->GetPage())
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1259);
  Frame *focus_frame =
      frame_->GetPage()->GetFocusController().FocusedOrMainFrame();
  xtrace->LocalVarUpdate(xtrace_mrid, "focus_frame",
                         focus_frame ? base::ToString(*focus_frame) : "");

  // Do not paste here if the focus was moved somewhere else.
  xtrace->LogLineRun(xtrace_mrid, 1262);
  if (frame_ == focus_frame)
    return frame_->GetEditor().ExecuteCommand("PasteGlobalSelection");

  xtrace->LogLineRun(xtrace_mrid, 1265);
  return false;
}

bool SelectionController::HandleGestureLongPress(
    const HitTestResult &hit_test_result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::HandleGestureLongPress",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LogLineRun(xtrace_mrid, 1270);
  TRACE_EVENT0("blink", "SelectionController::handleGestureLongPress");

  xtrace->LogLineRun(xtrace_mrid, 1272);
  if (!Selection().IsAvailable())
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1274);
  if (!RuntimeEnabledFeatures::LongPressLinkSelectTextEnabled() &&
      hit_test_result.IsLiveLink()) {
    xtrace->LogLineRun(xtrace_mrid, 1276);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 1279);
  Node *inner_node = hit_test_result.InnerPossiblyPseudoNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  xtrace->LogLineRun(xtrace_mrid, 1280);
  inner_node->GetDocument().UpdateStyleAndLayoutTree();
  xtrace->LogLineRun(xtrace_mrid, 1281);
  bool inner_node_is_selectable = IsEditable(*inner_node) ||
                                  inner_node->IsTextNode() ||
                                  inner_node->CanStartSelection();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node_is_selectable",
                         base::ToString(inner_node_is_selectable));

  xtrace->LogLineRun(xtrace_mrid, 1284);
  if (!inner_node_is_selectable)
    return false;

  xtrace->LogLineRun(xtrace_mrid, 1287);
  if (SelectClosestWordFromHitTestResult(hit_test_result,
                                         AppendTrailingWhitespace::kDontAppend,
                                         SelectInputEventType::kTouch))
    return Selection().IsAvailable();

  xtrace->LogLineRun(xtrace_mrid, 1292);
  if (!inner_node->isConnected() || !inner_node->GetLayoutObject())
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1294);
  SetCaretAtHitTestResult(hit_test_result);
  xtrace->LogLineRun(xtrace_mrid, 1295);
  return false;
}

void SelectionController::HandleGestureTwoFingerTap(
    const GestureEventWithHitTestResults &targeted_event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::HandleGestureTwoFingerTap", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "targeted_event",
                         base::ToString(targeted_event));
  xtrace->LogLineRun(xtrace_mrid, 1300);
  TRACE_EVENT0("blink", "SelectionController::handleGestureTwoFingerTap");

  xtrace->LogLineRun(xtrace_mrid, 1302);
  SetCaretAtHitTestResult(targeted_event.GetHitTestResult());
}

static bool HitTestResultIsMisspelled(const HitTestResult &result) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "HitTestResultIsMisspelled", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "result", base::ToString(result));
  xtrace->LogLineRun(xtrace_mrid, 1306);
  PositionWithAffinity pos_with_affinity = result.GetPosition();
  xtrace->LocalVarUpdate(xtrace_mrid, "pos_with_affinity",
                         base::ToString(pos_with_affinity));

  xtrace->LogLineRun(xtrace_mrid, 1307);
  if (pos_with_affinity.IsNull())
    return false;
  // TODO(xiaochengh): Don't use |ParentAnchoredEquivalent()|.
  xtrace->LogLineRun(xtrace_mrid, 1310);
  const Position marker_position =
      pos_with_affinity.GetPosition().ParentAnchoredEquivalent();
  xtrace->LocalVarUpdate(xtrace_mrid, "marker_position",
                         base::ToString(marker_position));

  xtrace->LogLineRun(xtrace_mrid, 1312);
  if (!SpellChecker::IsSpellCheckingEnabledAt(marker_position))
    return false;
  xtrace->LogLineRun(xtrace_mrid, 1314);
  return SpellCheckMarkerGroupAtPosition(
      result.InnerPossiblyPseudoNode()->GetDocument().Markers(),
      ToPositionInFlatTree(marker_position));
}

template <typename MouseEventObject>
void SelectionController::UpdateSelectionForContextMenuEvent(
    const MouseEventObject *mouse_event, const HitTestResult &hit_test_result,
    const PhysicalOffset &position) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::UpdateSelectionForContextMenuEvent",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_event",
                         mouse_event ? base::ToString(*mouse_event) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "hit_test_result",
                         base::ToString(hit_test_result));
  xtrace->LocalVarUpdate(xtrace_mrid, "position", base::ToString(position));
  xtrace->LogLineRun(xtrace_mrid, 1324);
  if (!Selection().IsAvailable())
    return;
  xtrace->LogLineRun(xtrace_mrid, 1326);
  if (mouse_down_was_single_click_on_caret_ || Selection().Contains(position) ||
      hit_test_result.GetScrollbar() ||
      // FIXME: In the editable case, word selection sometimes selects content
      // that isn't underneath the mouse.
      // If the selection is non-editable, we do word selection to make it
      // easier to use the contextual menu items available for text selections.
      // But only if we're above text.
      !(Selection()
            .ComputeVisibleSelectionInDOMTreeDeprecated()
            .IsContentEditable() ||
        (hit_test_result.InnerNode() &&
         hit_test_result.InnerNode()->IsTextNode()))) {
    xtrace->LogLineRun(xtrace_mrid, 1338);
    return;
  }

  // Context menu events are always allowed to perform a selection.
  xtrace->LogLineRun(xtrace_mrid, 1342);
  base::AutoReset<bool> mouse_down_may_start_select_change(
      &mouse_down_may_start_select_, true);
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_may_start_select_change",
                         base::ToString(mouse_down_may_start_select_change));

  xtrace->LogLineRun(xtrace_mrid, 1345);
  if (mouse_event->GetMenuSourceType() != kMenuSourceTouchHandle &&
      HitTestResultIsMisspelled(hit_test_result)) {
    xtrace->LogLineRun(xtrace_mrid, 1347);
    return SelectClosestMisspellingFromMouseEvent(mouse_event, hit_test_result);
  }

  xtrace->LogLineRun(xtrace_mrid, 1350);
  if (!frame_->GetEditor().Behavior().ShouldSelectOnContextualMenuClick())
    return;

  // Opening a context menu from an existing text fragment/highlight should not
  // select additional text.
  xtrace->LogLineRun(xtrace_mrid, 1355);
  if (TextFragmentHandler::IsOverTextFragment(hit_test_result))
    return;

  // Opening the context menu, triggered by long press or keyboard, should not
  // change the selected text.
  xtrace->LogLineRun(xtrace_mrid, 1360);
  if (mouse_event->GetMenuSourceType() == kMenuSourceLongPress ||
      mouse_event->GetMenuSourceType() == kMenuSourceKeyboard) {
    xtrace->LogLineRun(xtrace_mrid, 1362);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 1365);
  SelectClosestWordOrLinkFromMouseEvent(mouse_event, hit_test_result);
}

void SelectionController::PassMousePressEventToSubframe(
    const MouseEventWithHitTestResults &mev) {
  // TODO(editing-dev): The use of UpdateStyleAndLayout
  // needs to be audited.  See http://crbug.com/590369 for more details.
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::PassMousePressEventToSubframe", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "mev", base::ToString(mev));
  xtrace->LogLineRun(xtrace_mrid, 1372);
  frame_->GetDocument()->UpdateStyleAndLayout(DocumentUpdateReason::kInput);

  // If we're clicking into a frame that is selected, the frame will appear
  // greyed out even though we're clicking on the selection.  This looks
  // really strange (having the whole frame be greyed out), so we deselect the
  // selection.
  xtrace->LogLineRun(xtrace_mrid, 1378);
  PhysicalOffset p(frame_->View()->ConvertFromRootFrame(
      gfx::ToFlooredPoint(mev.Event().PositionInRootFrame())));
  xtrace->LocalVarUpdate(xtrace_mrid, "p", base::ToString(p));

  xtrace->LogLineRun(xtrace_mrid, 1380);
  if (!Selection().Contains(p))
    return;

  xtrace->LogLineRun(xtrace_mrid, 1383);
  const PositionInFlatTreeWithAffinity visible_pos =
      CreateVisiblePosition(
          PositionWithAffinityOfHitTestResult(mev.GetHitTestResult()))
          .ToPositionWithAffinity();
  xtrace->LocalVarUpdate(xtrace_mrid, "visible_pos",
                         base::ToString(visible_pos));

  xtrace->LogLineRun(xtrace_mrid, 1387);
  if (visible_pos.IsNull()) {
    xtrace->LogLineRun(xtrace_mrid, 1388);
    Selection().SetSelectionAndEndTyping(SelectionInDOMTree());
    xtrace->LogLineRun(xtrace_mrid, 1389);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 1391);
  Selection().SetSelectionAndEndTyping(ConvertToSelectionInDOMTree(
      SelectionInFlatTree::Builder().Collapse(visible_pos).Build()));
}

void SelectionController::InitializeSelectionState() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::InitializeSelectionState", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1396);
  selection_state_ = SelectionState::kHaveNotStartedSelection;
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                         base::ToString(selection_state_));
}

void SelectionController::SetMouseDownMayStartSelect(bool may_start_select) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::SetMouseDownMayStartSelect", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "may_start_select",
                         base::ToString(may_start_select));
  xtrace->LogLineRun(xtrace_mrid, 1400);
  mouse_down_may_start_select_ = may_start_select;
  xtrace->LocalVarUpdate(xtrace_mrid, "mouse_down_may_start_select_",
                         base::ToString(mouse_down_may_start_select_));
}

bool SelectionController::MouseDownMayStartSelect() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::MouseDownMayStartSelect",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1404);
  return mouse_down_may_start_select_;
}

bool SelectionController::MouseDownWasSingleClickInSelection() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc",
      "SelectionController::MouseDownWasSingleClickInSelection",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1408);
  return mouse_down_was_single_click_in_selection_;
}

void SelectionController::NotifySelectionChanged() {
  // To avoid regression on speedometer benchmark[1] test, we should not
  // update layout tree in this code block.
  // [1] http://browserbench.org/Speedometer/
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "SelectionController::NotifySelectionChanged",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1415);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      frame_->GetDocument()->Lifecycle());
  xtrace->LocalVarUpdate(xtrace_mrid, "disallow_transition",
                         base::ToString(disallow_transition));

  xtrace->LogLineRun(xtrace_mrid, 1418);
  const SelectionInDOMTree &selection = Selection().GetSelectionInDOMTree();
  xtrace->LocalVarUpdate(xtrace_mrid, "selection", base::ToString(selection));

  xtrace->LogLineRun(xtrace_mrid, 1419);
  if (selection.IsNone()) {
    xtrace->LogLineRun(xtrace_mrid, 1420);
    selection_state_ = SelectionState::kHaveNotStartedSelection;
    xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                           base::ToString(selection_state_));

    xtrace->LogLineRun(xtrace_mrid, 1421);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 1423);
  if (selection.IsCaret()) {
    xtrace->LogLineRun(xtrace_mrid, 1424);
    selection_state_ = SelectionState::kPlacedCaret;
    xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                           base::ToString(selection_state_));

    xtrace->LogLineRun(xtrace_mrid, 1425);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 1427);
  DCHECK(selection.IsRange()) << selection;
  xtrace->LogLineRun(xtrace_mrid, 1428);
  selection_state_ = SelectionState::kExtendedSelection;
  xtrace->LocalVarUpdate(xtrace_mrid, "selection_state_",
                         base::ToString(selection_state_));
}

FrameSelection &SelectionController::Selection() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("selection_controller.cc",
                            "SelectionController::Selection", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 1432);
  return frame_->Selection();
}

bool IsSelectionOverLink(const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "IsSelectionOverLink", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1436);
  return (event.Event().GetModifiers() & WebInputEvent::Modifiers::kAltKey) !=
             0 &&
         event.IsOverLink();
}

bool IsUserNodeDraggable(const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "IsUserNodeDraggable", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1442);
  Node *inner_node = event.InnerNode();
  xtrace->LocalVarUpdate(xtrace_mrid, "inner_node",
                         inner_node ? base::ToString(*inner_node) : "");

  // TODO(huangdarwin): event.InnerNode() should never be nullptr, but unit
  // tests WebFrameTest.FrameWidgetTest and WebViewTest.ClientTapHandling fail
  // without a nullptr check, as they don't set the InnerNode() appropriately.
  // Remove the if statement nullptr check when those tests are fixed.
  xtrace->LogLineRun(xtrace_mrid, 1448);
  if (!inner_node) {
    xtrace->LogLineRun(xtrace_mrid, 1449);
    return false;
  }

  xtrace->LogLineRun(xtrace_mrid, 1452);
  const ComputedStyle *style =
      GetComputedStyleForElementOrLayoutObject(*inner_node);
  xtrace->LocalVarUpdate(xtrace_mrid, "style",
                         style ? base::ToString(*style) : "");

  xtrace->LogLineRun(xtrace_mrid, 1454);
  return style && style->UserDrag() == EUserDrag::kElement;
}

bool IsExtendingSelection(const MouseEventWithHitTestResults &event) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "selection_controller.cc", "IsExtendingSelection", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "event", base::ToString(event));
  xtrace->LogLineRun(xtrace_mrid, 1458);
  bool is_mouse_down_on_link_or_image =
      event.IsOverLink() || event.GetHitTestResult().GetImage();
  xtrace->LocalVarUpdate(xtrace_mrid, "is_mouse_down_on_link_or_image",
                         base::ToString(is_mouse_down_on_link_or_image));

  xtrace->LogLineRun(xtrace_mrid, 1461);
  return (event.Event().GetModifiers() & WebInputEvent::Modifiers::kShiftKey) !=
             0 &&
         !is_mouse_down_on_link_or_image && !IsUserNodeDraggable(event);
}

template void
SelectionController::UpdateSelectionForContextMenuEvent<MouseEvent>(
    const MouseEvent *, const HitTestResult &, const PhysicalOffset &);

} // namespace blink
