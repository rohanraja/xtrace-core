#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/clipboard/clipboard_promise.h"

#include <memory>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/single_thread_task_runner.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/permissions_policy/permissions_policy_feature.mojom-blink.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/bindings/core/v8/promise_all.h"
#include "third_party/blink/renderer/bindings/core/v8/script_function.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_clipboard_unsanitized_formats.h"
#include "third_party/blink/renderer/core/clipboard/clipboard_mime_types.h"
#include "third_party/blink/renderer/core/clipboard/system_clipboard.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/commands/clipboard_commands.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/clipboard/clipboard.h"
#include "third_party/blink/renderer/modules/clipboard/clipboard_item.h"
#include "third_party/blink/renderer/modules/clipboard/clipboard_reader.h"
#include "third_party/blink/renderer/modules/clipboard/clipboard_writer.h"
#include "third_party/blink/renderer/modules/permissions/permission_utils.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/worker_pool.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "ui/base/clipboard/clipboard_constants.h"

// There are 2 clipboard permissions defined in the spec:
// * clipboard-read
// * clipboard-write
// See https://w3c.github.io/clipboard-apis/#clipboard-permissions
//
// These permissions map to these ContentSettings:
// * CLIPBOARD_READ_WRITE, for sanitized read, and unsanitized read/write.
// * CLIPBOARD_SANITIZED_WRITE, for sanitized write only.

namespace blink {

using mojom::blink::PermissionService;

// This class deals with all the clipboard item promises and executes the write
// operation after all the promises have been resolved.
class ClipboardPromise::ClipboardItemDataPromiseFulfill final
    : public ThenCallable<IDLSequence<V8UnionBlobOrString>,
                          ClipboardItemDataPromiseFulfill> {
public:
  explicit ClipboardItemDataPromiseFulfill(ClipboardPromise *clipboard_promise)
      : clipboard_promise_(clipboard_promise) {}

  void Trace(Visitor *visitor) const final {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("clip_prom.cc", "Trace", "GUID_FROM_TEST");
    xtrace->LocalVarUpdate(xtrace_mrid, "visitor",
                           visitor ? base::ToString(*visitor) : "");
    xtrace->LogLineRun(xtrace_mrid, 66);
    ThenCallable<IDLSequence<V8UnionBlobOrString>,
                 ClipboardItemDataPromiseFulfill>::Trace(visitor);
    xtrace->LogLineRun(xtrace_mrid, 68);
    visitor->Trace(clipboard_promise_);
  }

  void React(ScriptState *script_state,
             HeapVector<Member<V8UnionBlobOrString>> clipboard_item_list) {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("clip_prom.cc", "React", "GUID_FROM_TEST");
    xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                           script_state ? base::ToString(*script_state) : "");
    xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_item_list",
                           base::ToString(clipboard_item_list));
    xtrace->LogLineRun(xtrace_mrid, 73);
    auto *list_copy =
        MakeGarbageCollected<HeapVector<Member<V8UnionBlobOrString>>>(
            std::move(clipboard_item_list));
    xtrace->LocalVarUpdate(xtrace_mrid, "list_copy",
                           list_copy ? base::ToString(*list_copy) : "");

    xtrace->LogLineRun(xtrace_mrid, 76);
    clipboard_promise_->HandlePromiseWrite(list_copy);
  }

private:
  Member<ClipboardPromise> clipboard_promise_;
};

class ClipboardPromise::ClipboardItemDataPromiseReject final
    : public ThenCallable<IDLAny, ClipboardItemDataPromiseReject> {
public:
  explicit ClipboardItemDataPromiseReject(ClipboardPromise *clipboard_promise)
      : clipboard_promise_(clipboard_promise) {}

  void Trace(Visitor *visitor) const final {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("clip_prom.cc", "Trace", "GUID_FROM_TEST");
    xtrace->LocalVarUpdate(xtrace_mrid, "visitor",
                           visitor ? base::ToString(*visitor) : "");
    xtrace->LogLineRun(xtrace_mrid, 90);
    ThenCallable<IDLAny, ClipboardItemDataPromiseReject>::Trace(visitor);
    xtrace->LogLineRun(xtrace_mrid, 91);
    visitor->Trace(clipboard_promise_);
  }

  void React(ScriptState *script_state, ScriptValue exception) {
    blink::XTrace *xtrace = blink::XTrace::getInstance();
    std::string xtrace_mrid =
        xtrace->OnMethodEnter("clip_prom.cc", "React", "GUID_FROM_TEST");
    xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                           script_state ? base::ToString(*script_state) : "");
    xtrace->LocalVarUpdate(xtrace_mrid, "exception", base::ToString(exception));
    xtrace->LogLineRun(xtrace_mrid, 95);
    clipboard_promise_->RejectClipboardItemPromise(exception);
  }

private:
  Member<ClipboardPromise> clipboard_promise_;
};

// static
ScriptPromise<IDLSequence<ClipboardItem>> ClipboardPromise::CreateForRead(
    ExecutionContext *context, ScriptState *script_state,
    ClipboardUnsanitizedFormats *formats, ExceptionState &exception_state) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::CreateForRead", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "formats",
                         formats ? base::ToString(*formats) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 108);
  if (!script_state->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 109);
    return ScriptPromise<IDLSequence<ClipboardItem>>();
  }
  xtrace->LogLineRun(xtrace_mrid, 111);
  auto *resolver =
      MakeGarbageCollected<ScriptPromiseResolver<IDLSequence<ClipboardItem>>>(
          script_state, exception_state.GetContext());
  xtrace->LocalVarUpdate(xtrace_mrid, "resolver",
                         resolver ? base::ToString(*resolver) : "");

  xtrace->LogLineRun(xtrace_mrid, 114);
  auto promise = resolver->Promise();
  xtrace->LocalVarUpdate(xtrace_mrid, "promise", base::ToString(promise));

  xtrace->LogLineRun(xtrace_mrid, 115);
  ClipboardPromise *clipboard_promise = MakeGarbageCollected<ClipboardPromise>(
      context, resolver, exception_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_promise",
                         clipboard_promise ? base::ToString(*clipboard_promise)
                                           : "");

  xtrace->LogLineRun(xtrace_mrid, 117);
  clipboard_promise->HandleRead(formats);
  xtrace->LogLineRun(xtrace_mrid, 118);
  return promise;
}

// static
ScriptPromise<IDLString>
ClipboardPromise::CreateForReadText(ExecutionContext *context,
                                    ScriptState *script_state,
                                    ExceptionState &exception_state) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::CreateForReadText", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 126);
  if (!script_state->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 127);
    return EmptyPromise();
  }
  xtrace->LogLineRun(xtrace_mrid, 129);
  auto *resolver = MakeGarbageCollected<ScriptPromiseResolver<IDLString>>(
      script_state, exception_state.GetContext());
  xtrace->LocalVarUpdate(xtrace_mrid, "resolver",
                         resolver ? base::ToString(*resolver) : "");

  xtrace->LogLineRun(xtrace_mrid, 131);
  ClipboardPromise *clipboard_promise = MakeGarbageCollected<ClipboardPromise>(
      context, resolver, exception_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_promise",
                         clipboard_promise ? base::ToString(*clipboard_promise)
                                           : "");

  xtrace->LogLineRun(xtrace_mrid, 133);
  auto promise = resolver->Promise();
  xtrace->LocalVarUpdate(xtrace_mrid, "promise", base::ToString(promise));

  xtrace->LogLineRun(xtrace_mrid, 134);
  clipboard_promise->HandleReadText();
  xtrace->LogLineRun(xtrace_mrid, 135);
  return promise;
}

// static
ScriptPromise<IDLUndefined>
ClipboardPromise::CreateForWrite(ExecutionContext *context,
                                 ScriptState *script_state,
                                 const HeapVector<Member<ClipboardItem>> &items,
                                 ExceptionState &exception_state) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::CreateForWrite", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "items", base::ToString(items));
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 144);
  if (!script_state->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 145);
    return EmptyPromise();
  }
  xtrace->LogLineRun(xtrace_mrid, 147);
  auto *resolver = MakeGarbageCollected<ScriptPromiseResolver<IDLUndefined>>(
      script_state, exception_state.GetContext());
  xtrace->LocalVarUpdate(xtrace_mrid, "resolver",
                         resolver ? base::ToString(*resolver) : "");

  xtrace->LogLineRun(xtrace_mrid, 149);
  ClipboardPromise *clipboard_promise = MakeGarbageCollected<ClipboardPromise>(
      context, resolver, exception_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_promise",
                         clipboard_promise ? base::ToString(*clipboard_promise)
                                           : "");

  xtrace->LogLineRun(xtrace_mrid, 151);
  auto promise = resolver->Promise();
  xtrace->LocalVarUpdate(xtrace_mrid, "promise", base::ToString(promise));

  xtrace->LogLineRun(xtrace_mrid, 152);
  clipboard_promise->HandleWrite(items);
  xtrace->LogLineRun(xtrace_mrid, 153);
  return promise;
}

// static
ScriptPromise<IDLUndefined> ClipboardPromise::CreateForWriteText(
    ExecutionContext *context, ScriptState *script_state, const String &data,
    ExceptionState &exception_state) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::CreateForWriteText", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "data", base::ToString(data));
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 162);
  if (!script_state->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 163);
    return EmptyPromise();
  }
  xtrace->LogLineRun(xtrace_mrid, 165);
  auto *resolver = MakeGarbageCollected<ScriptPromiseResolver<IDLUndefined>>(
      script_state, exception_state.GetContext());
  xtrace->LocalVarUpdate(xtrace_mrid, "resolver",
                         resolver ? base::ToString(*resolver) : "");

  xtrace->LogLineRun(xtrace_mrid, 167);
  ClipboardPromise *clipboard_promise = MakeGarbageCollected<ClipboardPromise>(
      context, resolver, exception_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_promise",
                         clipboard_promise ? base::ToString(*clipboard_promise)
                                           : "");

  xtrace->LogLineRun(xtrace_mrid, 169);
  auto promise = resolver->Promise();
  xtrace->LocalVarUpdate(xtrace_mrid, "promise", base::ToString(promise));

  xtrace->LogLineRun(xtrace_mrid, 170);
  clipboard_promise->HandleWriteText(data);
  xtrace->LogLineRun(xtrace_mrid, 171);
  return promise;
}

ClipboardPromise::ClipboardPromise(ExecutionContext *context,
                                   ScriptPromiseResolverBase *resolver,
                                   ExceptionState &exception_state)
    : ExecutionContextLifecycleObserver(context),
      script_promise_resolver_(resolver), permission_service_(context) {}

ClipboardPromise::~ClipboardPromise() = default;

void ClipboardPromise::CompleteWriteRepresentation() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::CompleteWriteRepresentation",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 184);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 185);
  clipboard_writer_.Clear(); // The previous write is done.
  xtrace->LogLineRun(xtrace_mrid, 186);
  ++clipboard_representation_index_;
  xtrace->LogLineRun(xtrace_mrid, 187);
  WriteNextRepresentation();
}

void ClipboardPromise::WriteNextRepresentation() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::WriteNextRepresentation",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 191);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 192);
  if (!GetExecutionContext() || !GetScriptState()->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 193);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 195);
  ScriptState::Scope scope(GetScriptState());
  xtrace->LocalVarUpdate(xtrace_mrid, "scope", base::ToString(scope));

  xtrace->LogLineRun(xtrace_mrid, 196);
  LocalFrame *local_frame = GetLocalFrame();
  xtrace->LocalVarUpdate(xtrace_mrid, "local_frame",
                         local_frame ? base::ToString(*local_frame) : "");

  // Commit to system clipboard when all representations are written.
  // This is in the start flow so that a |clipboard_item_data_| with 0 items
  // will still commit gracefully.
  xtrace->LogLineRun(xtrace_mrid, 200);
  if (clipboard_representation_index_ == clipboard_item_data_.size()) {
    xtrace->LogLineRun(xtrace_mrid, 201);
    local_frame->GetSystemClipboard()->CommitWrite();
    xtrace->LogLineRun(xtrace_mrid, 202);
    script_promise_resolver_->DowncastTo<IDLUndefined>()->Resolve();
    xtrace->LogLineRun(xtrace_mrid, 203);
    return;
  }

  // We currently write the ClipboardItem type, but don't use the blob type.
  xtrace->LogLineRun(xtrace_mrid, 207);
  const String &type =
      clipboard_item_data_[clipboard_representation_index_].first;
  xtrace->LocalVarUpdate(xtrace_mrid, "type", base::ToString(type));

  xtrace->LogLineRun(xtrace_mrid, 209);
  const Member<V8UnionBlobOrString> &clipboard_item_data =
      clipboard_item_data_[clipboard_representation_index_].second;
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_item_data",
                         base::ToString(clipboard_item_data));

  xtrace->LogLineRun(xtrace_mrid, 212);
  DCHECK(!clipboard_writer_);
  xtrace->LogLineRun(xtrace_mrid, 213);
  clipboard_writer_ =
      ClipboardWriter::Create(local_frame->GetSystemClipboard(), type, this);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_writer_",
                         base::ToString(clipboard_writer_));

  xtrace->LogLineRun(xtrace_mrid, 215);
  if (!clipboard_writer_) {
    xtrace->LogLineRun(xtrace_mrid, 216);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError,
        "Type " + type + " is not supported");
    xtrace->LogLineRun(xtrace_mrid, 219);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 221);
  clipboard_writer_->WriteToSystem(clipboard_item_data);
}

void ClipboardPromise::RejectFromReadOrDecodeFailure() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::RejectFromReadOrDecodeFailure",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 225);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 226);
  if (!GetExecutionContext() || !GetScriptState()->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 227);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 229);
  ScriptState::Scope scope(GetScriptState());
  xtrace->LocalVarUpdate(xtrace_mrid, "scope", base::ToString(scope));

  xtrace->LogLineRun(xtrace_mrid, 230);
  String exception_text =
      RuntimeEnabledFeatures::ClipboardItemWithDOMStringSupportEnabled()
          ? "Failed to read or decode ClipboardItemData for type "
          : "Failed to read or decode Blob for clipboard item type ";
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_text",
                         base::ToString(exception_text));

  xtrace->LogLineRun(xtrace_mrid, 234);
  script_promise_resolver_->RejectWithDOMException(
      DOMExceptionCode::kDataError,
      exception_text +
          clipboard_item_data_[clipboard_representation_index_].first + ".");
}

void ClipboardPromise::HandleRead(ClipboardUnsanitizedFormats *formats) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleRead", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "formats",
                         formats ? base::ToString(*formats) : "");
  xtrace->LogLineRun(xtrace_mrid, 241);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  xtrace->LogLineRun(xtrace_mrid, 243);
  if (formats && formats->hasUnsanitized() && !formats->unsanitized().empty()) {
    xtrace->LogLineRun(xtrace_mrid, 244);
    Vector<String> unsanitized_formats = formats->unsanitized();
    xtrace->LocalVarUpdate(xtrace_mrid, "unsanitized_formats",
                           base::ToString(unsanitized_formats));

    xtrace->LogLineRun(xtrace_mrid, 245);
    if (unsanitized_formats.size() > 1) {
      xtrace->LogLineRun(xtrace_mrid, 246);
      script_promise_resolver_->RejectWithDOMException(
          DOMExceptionCode::kNotAllowedError,
          "Reading multiple unsanitized formats is not supported.");
      xtrace->LogLineRun(xtrace_mrid, 249);
      return;
    }
    xtrace->LogLineRun(xtrace_mrid, 251);
    if (unsanitized_formats[0] != kMimeTypeTextHTML) {
      xtrace->LogLineRun(xtrace_mrid, 252);
      script_promise_resolver_->RejectWithDOMException(
          DOMExceptionCode::kNotAllowedError, "The unsanitized type " +
                                                  unsanitized_formats[0] +
                                                  " is not supported.");
      xtrace->LogLineRun(xtrace_mrid, 256);
      return;
    }
    // HTML is the only standard format that can be read without any processing
    // for now.
    xtrace->LogLineRun(xtrace_mrid, 260);
    will_read_unprocessed_html_ = true;
    xtrace->LocalVarUpdate(xtrace_mrid, "will_read_unprocessed_html_",
                           base::ToString(will_read_unprocessed_html_));
  }

  xtrace->LogLineRun(xtrace_mrid, 263);
  ValidatePreconditions(
      mojom::blink::PermissionName::CLIPBOARD_READ,
      /*will_be_sanitized=*/false,
      WTF::BindOnce(&ClipboardPromise::HandleReadWithPermission,
                    WrapPersistent(this)));
}

void ClipboardPromise::HandleReadText() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleReadText", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 271);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 272);
  ValidatePreconditions(
      mojom::blink::PermissionName::CLIPBOARD_READ,
      /*will_be_sanitized=*/true,
      WTF::BindOnce(&ClipboardPromise::HandleReadTextWithPermission,
                    WrapPersistent(this)));
}

void ClipboardPromise::HandleWrite(
    const HeapVector<Member<ClipboardItem>> &clipboard_items) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleWrite", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_items",
                         base::ToString(clipboard_items));
  xtrace->LogLineRun(xtrace_mrid, 281);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 282);
  DCHECK(GetExecutionContext());

  xtrace->LogLineRun(xtrace_mrid, 284);
  if (clipboard_items.size() > 1) {
    xtrace->LogLineRun(xtrace_mrid, 285);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError,
        "Support for multiple ClipboardItems is not implemented.");
    xtrace->LogLineRun(xtrace_mrid, 288);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 290);
  if (!clipboard_items.size()) {
    // Do nothing if there are no ClipboardItems.
    xtrace->LogLineRun(xtrace_mrid, 292);
    script_promise_resolver_->DowncastTo<IDLUndefined>()->Resolve();
    xtrace->LogLineRun(xtrace_mrid, 293);
    return;
  }

  // For now, we only process the first ClipboardItem.
  xtrace->LogLineRun(xtrace_mrid, 297);
  ClipboardItem *clipboard_item = clipboard_items[0];
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_item",
                         clipboard_item ? base::ToString(*clipboard_item) : "");

  xtrace->LogLineRun(xtrace_mrid, 298);
  clipboard_item_data_with_promises_ = clipboard_item->GetRepresentations();
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_item_data_with_promises_",
                         base::ToString(clipboard_item_data_with_promises_));

  xtrace->LogLineRun(xtrace_mrid, 299);
  write_custom_format_types_ = clipboard_item->CustomFormats();
  xtrace->LocalVarUpdate(xtrace_mrid, "write_custom_format_types_",
                         base::ToString(write_custom_format_types_));

  xtrace->LogLineRun(xtrace_mrid, 301);
  if (static_cast<int>(write_custom_format_types_.size()) >
      ui::kMaxRegisteredClipboardFormats) {
    xtrace->LogLineRun(xtrace_mrid, 303);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError,
        "Number of custom formats exceeds the max limit which is set to 100.");
    xtrace->LogLineRun(xtrace_mrid, 306);
    return;
  }

  // Input in standard formats is sanitized, so the write will be sanitized
  // unless there are custom formats.
  xtrace->LogLineRun(xtrace_mrid, 311);
  ValidatePreconditions(
      mojom::blink::PermissionName::CLIPBOARD_WRITE,
      /*will_be_sanitized=*/write_custom_format_types_.empty(),
      WTF::BindOnce(&ClipboardPromise::HandleWriteWithPermission,
                    WrapPersistent(this)));
}

void ClipboardPromise::HandleWriteText(const String &data) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleWriteText", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "data", base::ToString(data));
  xtrace->LogLineRun(xtrace_mrid, 319);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 320);
  plain_text_ = data;
  xtrace->LocalVarUpdate(xtrace_mrid, "plain_text_",
                         base::ToString(plain_text_));

  xtrace->LogLineRun(xtrace_mrid, 321);
  ValidatePreconditions(
      mojom::blink::PermissionName::CLIPBOARD_WRITE,
      /*will_be_sanitized=*/true,
      WTF::BindOnce(&ClipboardPromise::HandleWriteTextWithPermission,
                    WrapPersistent(this)));
}

void ClipboardPromise::HandleReadWithPermission(
    mojom::blink::PermissionStatus status) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleReadWithPermission",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "status", base::ToString(status));
  xtrace->LogLineRun(xtrace_mrid, 330);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 331);
  if (!GetExecutionContext()) {
    xtrace->LogLineRun(xtrace_mrid, 332);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 334);
  if (status != mojom::blink::PermissionStatus::GRANTED) {
    xtrace->LogLineRun(xtrace_mrid, 335);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, "Read permission denied.");
    xtrace->LogLineRun(xtrace_mrid, 337);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 340);
  SystemClipboard *system_clipboard = GetLocalFrame()->GetSystemClipboard();
  xtrace->LocalVarUpdate(xtrace_mrid, "system_clipboard",
                         system_clipboard ? base::ToString(*system_clipboard)
                                          : "");

  xtrace->LogLineRun(xtrace_mrid, 341);
  system_clipboard->ReadAvailableCustomAndStandardFormats(WTF::BindOnce(
      &ClipboardPromise::OnReadAvailableFormatNames, WrapPersistent(this)));
}

void ClipboardPromise::ResolveRead() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::ResolveRead", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 346);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 347);
  DCHECK(GetExecutionContext());

  xtrace->LogLineRun(xtrace_mrid, 349);
  base::UmaHistogramCounts100("Blink.Clipboard.Read.NumberOfFormats",
                              clipboard_item_data_.size());
  xtrace->LogLineRun(xtrace_mrid, 351);
  ScriptState *script_state = GetScriptState();
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");

  xtrace->LogLineRun(xtrace_mrid, 352);
  if (!script_state->ContextIsValid()) {
    xtrace->LogLineRun(xtrace_mrid, 353);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 355);
  ScriptState::Scope scope(script_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "scope", base::ToString(scope));

  xtrace->LogLineRun(xtrace_mrid, 356);
  HeapVector<std::pair<String, MemberScriptPromise<V8UnionBlobOrString>>> items;
  xtrace->LogLineRun(xtrace_mrid, 357);
  items.ReserveInitialCapacity(clipboard_item_data_.size());

  xtrace->LogLineRun(xtrace_mrid, 359);
  for (const auto &item : clipboard_item_data_) {
    xtrace->LogLineRun(xtrace_mrid, 360);
    if (!item.second) {
      xtrace->LogLineRun(xtrace_mrid, 361);
      continue;
    }
    xtrace->LogLineRun(xtrace_mrid, 363);
    auto promise =
        ToResolvedPromise<V8UnionBlobOrString>(script_state, item.second);
    xtrace->LocalVarUpdate(xtrace_mrid, "promise", base::ToString(promise));

    xtrace->LogLineRun(xtrace_mrid, 365);
    items.emplace_back(item.first, promise);
  }
  xtrace->LogLineRun(xtrace_mrid, 367);
  HeapVector<Member<ClipboardItem>> clipboard_items = {
      MakeGarbageCollected<ClipboardItem>(items)};
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_items",
                         base::ToString(clipboard_items));

  xtrace->LogLineRun(xtrace_mrid, 369);
  script_promise_resolver_->DowncastTo<IDLSequence<ClipboardItem>>()->Resolve(
      clipboard_items);
}

void ClipboardPromise::OnReadAvailableFormatNames(
    const Vector<String> &format_names) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::OnReadAvailableFormatNames",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "format_names",
                         base::ToString(format_names));
  xtrace->LogLineRun(xtrace_mrid, 375);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 376);
  if (!GetExecutionContext()) {
    xtrace->LogLineRun(xtrace_mrid, 377);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 380);
  clipboard_item_data_.ReserveInitialCapacity(format_names.size());
  xtrace->LogLineRun(xtrace_mrid, 381);
  for (const String &format_name : format_names) {
    xtrace->LogLineRun(xtrace_mrid, 382);
    if (ClipboardItem::supports(format_name)) {
      xtrace->LogLineRun(xtrace_mrid, 383);
      clipboard_item_data_.emplace_back(format_name,
                                        /* Placeholder value. */ nullptr);
    }
  }
  xtrace->LogLineRun(xtrace_mrid, 387);
  ReadNextRepresentation();
}

void ClipboardPromise::ReadNextRepresentation() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::ReadNextRepresentation",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 391);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 392);
  if (!GetExecutionContext())
    return;
  xtrace->LogLineRun(xtrace_mrid, 394);
  if (clipboard_representation_index_ == clipboard_item_data_.size()) {
    xtrace->LogLineRun(xtrace_mrid, 395);
    ResolveRead();
    xtrace->LogLineRun(xtrace_mrid, 396);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 399);
  ClipboardReader *clipboard_reader = ClipboardReader::Create(
      GetLocalFrame()->GetSystemClipboard(),
      clipboard_item_data_[clipboard_representation_index_].first, this,
      /*sanitize_html=*/!will_read_unprocessed_html_);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_reader",
                         clipboard_reader ? base::ToString(*clipboard_reader)
                                          : "");

  xtrace->LogLineRun(xtrace_mrid, 403);
  if (!clipboard_reader) {
    xtrace->LogLineRun(xtrace_mrid, 404);
    OnRead(nullptr);
    xtrace->LogLineRun(xtrace_mrid, 405);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 407);
  clipboard_reader->Read();
}

void ClipboardPromise::OnRead(Blob *blob) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::OnRead", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "blob",
                         blob ? base::ToString(*blob) : "");
  xtrace->LogLineRun(xtrace_mrid, 411);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 412);
  if (blob) {
    xtrace->LogLineRun(xtrace_mrid, 413);
    clipboard_item_data_[clipboard_representation_index_].second =
        MakeGarbageCollected<V8UnionBlobOrString>(blob);
  }
  xtrace->LogLineRun(xtrace_mrid, 416);
  ++clipboard_representation_index_;
  xtrace->LogLineRun(xtrace_mrid, 417);
  ReadNextRepresentation();
}

void ClipboardPromise::HandleReadTextWithPermission(
    mojom::blink::PermissionStatus status) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleReadTextWithPermission",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "status", base::ToString(status));
  xtrace->LogLineRun(xtrace_mrid, 422);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 423);
  if (!GetExecutionContext()) {
    xtrace->LogLineRun(xtrace_mrid, 424);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 426);
  if (status != mojom::blink::PermissionStatus::GRANTED) {
    xtrace->LogLineRun(xtrace_mrid, 427);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, "Read permission denied.");
    xtrace->LogLineRun(xtrace_mrid, 429);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 432);
  String text = GetLocalFrame()->GetSystemClipboard()->ReadPlainText(
      mojom::blink::ClipboardBuffer::kStandard);
  xtrace->LocalVarUpdate(xtrace_mrid, "text", base::ToString(text));

  xtrace->LogLineRun(xtrace_mrid, 434);
  script_promise_resolver_->DowncastTo<IDLString>()->Resolve(text);
}

void ClipboardPromise::HandlePromiseWrite(
    HeapVector<Member<V8UnionBlobOrString>> *clipboard_item_list) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandlePromiseWrite", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(
      xtrace_mrid, "clipboard_item_list",
      clipboard_item_list ? base::ToString(*clipboard_item_list) : "");
  xtrace->LogLineRun(xtrace_mrid, 439);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  xtrace->LogLineRun(xtrace_mrid, 441);
  GetClipboardTaskRunner()->PostTask(
      FROM_HERE,
      WTF::BindOnce(&ClipboardPromise::WriteClipboardItemData,
                    WrapPersistent(this), WrapPersistent(clipboard_item_list)));
}

void ClipboardPromise::WriteClipboardItemData(
    HeapVector<Member<V8UnionBlobOrString>> *clipboard_item_list) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::WriteClipboardItemData",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(
      xtrace_mrid, "clipboard_item_list",
      clipboard_item_list ? base::ToString(*clipboard_item_list) : "");
  xtrace->LogLineRun(xtrace_mrid, 449);
  wtf_size_t clipboard_item_index = 0;
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard_item_index",
                         base::ToString(clipboard_item_index));

  xtrace->LogLineRun(xtrace_mrid, 450);
  CHECK_EQ(write_clipboard_item_types_.size(), clipboard_item_list->size());
  xtrace->LogLineRun(xtrace_mrid, 451);
  for (const auto &clipboard_item_data : *clipboard_item_list) {
    xtrace->LogLineRun(xtrace_mrid, 452);
    if (!RuntimeEnabledFeatures::ClipboardItemWithDOMStringSupportEnabled() &&
        !clipboard_item_data->IsBlob()) {
      xtrace->LogLineRun(xtrace_mrid, 454);
      script_promise_resolver_->RejectWithDOMException(
          DOMExceptionCode::kNotAllowedError,
          "DOMString is not supported in ClipboardItem");
      xtrace->LogLineRun(xtrace_mrid, 457);
      return;
    }

    xtrace->LogLineRun(xtrace_mrid, 460);
    const String &type = write_clipboard_item_types_[clipboard_item_index];
    xtrace->LocalVarUpdate(xtrace_mrid, "type", base::ToString(type));

    xtrace->LogLineRun(xtrace_mrid, 461);
    if (clipboard_item_data->IsBlob()) {
      xtrace->LogLineRun(xtrace_mrid, 462);
      const String &type_with_args = clipboard_item_data->GetAsBlob()->type();
      xtrace->LocalVarUpdate(xtrace_mrid, "type_with_args",
                             base::ToString(type_with_args));

      // For web custom types, extract the MIME type after removing the "web "
      // prefix. For normal (not-custom) write, blobs may have a full MIME type
      // with args (ex. 'text/plain;charset=utf-8'), whereas the type must not
      // have args (ex. 'text/plain' only), so ensure that Blob->type is
      // contained in type.
      xtrace->LogLineRun(xtrace_mrid, 468);
      String web_custom_format = Clipboard::ParseWebCustomFormat(type);
      xtrace->LocalVarUpdate(xtrace_mrid, "web_custom_format",
                             base::ToString(web_custom_format));

      xtrace->LogLineRun(xtrace_mrid, 469);
      if ((!type_with_args.Contains(type.LowerASCII()) &&
           web_custom_format.empty()) ||
          (!web_custom_format.empty() &&
           !type_with_args.Contains(web_custom_format))) {
        xtrace->LogLineRun(xtrace_mrid, 473);
        script_promise_resolver_->RejectWithDOMException(
            DOMExceptionCode::kNotAllowedError,
            "Type " + type + " does not match the blob's type " +
                type_with_args);
        xtrace->LogLineRun(xtrace_mrid, 477);
        return;
      }
    }
    xtrace->LogLineRun(xtrace_mrid, 480);
    clipboard_item_data_.emplace_back(type, clipboard_item_data);
    xtrace->LogLineRun(xtrace_mrid, 481);
    clipboard_item_index++;
  }
  xtrace->LogLineRun(xtrace_mrid, 483);
  write_clipboard_item_types_.clear();

  xtrace->LogLineRun(xtrace_mrid, 485);
  DCHECK(!clipboard_representation_index_);
  xtrace->LogLineRun(xtrace_mrid, 486);
  WriteNextRepresentation();
}

void ClipboardPromise::HandleWriteWithPermission(
    mojom::blink::PermissionStatus status) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleWriteWithPermission",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "status", base::ToString(status));
  xtrace->LogLineRun(xtrace_mrid, 491);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 492);
  if (!GetExecutionContext()) {
    xtrace->LogLineRun(xtrace_mrid, 493);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 495);
  if (status != mojom::blink::PermissionStatus::GRANTED) {
    xtrace->LogLineRun(xtrace_mrid, 496);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, "Write permission denied.");
    xtrace->LogLineRun(xtrace_mrid, 498);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 501);
  HeapVector<MemberScriptPromise<V8UnionBlobOrString>> promise_list;
  xtrace->LogLineRun(xtrace_mrid, 502);
  promise_list.ReserveInitialCapacity(
      clipboard_item_data_with_promises_.size());
  xtrace->LogLineRun(xtrace_mrid, 504);
  write_clipboard_item_types_.ReserveInitialCapacity(
      clipboard_item_data_with_promises_.size());
  // Check that all types are valid.
  xtrace->LogLineRun(xtrace_mrid, 507);
  for (const auto &type_and_promise : clipboard_item_data_with_promises_) {
    xtrace->LogLineRun(xtrace_mrid, 508);
    const String &type = type_and_promise.first;
    xtrace->LocalVarUpdate(xtrace_mrid, "type", base::ToString(type));

    xtrace->LogLineRun(xtrace_mrid, 509);
    write_clipboard_item_types_.emplace_back(type);
    xtrace->LogLineRun(xtrace_mrid, 510);
    promise_list.emplace_back(type_and_promise.second);
    xtrace->LogLineRun(xtrace_mrid, 511);
    if (!ClipboardItem::supports(type)) {
      xtrace->LogLineRun(xtrace_mrid, 512);
      script_promise_resolver_->RejectWithDOMException(
          DOMExceptionCode::kNotAllowedError,
          "Type " + type + " not supported on write.");
      xtrace->LogLineRun(xtrace_mrid, 515);
      return;
    }
  }
  xtrace->LogLineRun(xtrace_mrid, 518);
  ScriptState *script_state = GetScriptState();
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");

  xtrace->LogLineRun(xtrace_mrid, 519);
  ScriptState::Scope scope(script_state);
  xtrace->LocalVarUpdate(xtrace_mrid, "scope", base::ToString(scope));

  xtrace->LogLineRun(xtrace_mrid, 520);
  PromiseAll<V8UnionBlobOrString>::Create(script_state, promise_list)
      .Then(script_state,
            MakeGarbageCollected<ClipboardItemDataPromiseFulfill>(this),
            MakeGarbageCollected<ClipboardItemDataPromiseReject>(this));
}

void ClipboardPromise::HandleWriteTextWithPermission(
    mojom::blink::PermissionStatus status) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::HandleWriteTextWithPermission",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "status", base::ToString(status));
  xtrace->LogLineRun(xtrace_mrid, 528);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 529);
  if (!GetExecutionContext()) {
    xtrace->LogLineRun(xtrace_mrid, 530);
    return;
  }
  xtrace->LogLineRun(xtrace_mrid, 532);
  if (status != mojom::blink::PermissionStatus::GRANTED) {
    xtrace->LogLineRun(xtrace_mrid, 533);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, "Write permission denied.");
    xtrace->LogLineRun(xtrace_mrid, 535);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 538);
  SystemClipboard *system_clipboard = GetLocalFrame()->GetSystemClipboard();
  xtrace->LocalVarUpdate(xtrace_mrid, "system_clipboard",
                         system_clipboard ? base::ToString(*system_clipboard)
                                          : "");

  xtrace->LogLineRun(xtrace_mrid, 539);
  system_clipboard->WritePlainText(plain_text_);
  xtrace->LogLineRun(xtrace_mrid, 540);
  system_clipboard->CommitWrite();
  xtrace->LogLineRun(xtrace_mrid, 541);
  script_promise_resolver_->DowncastTo<IDLUndefined>()->Resolve();
}

void ClipboardPromise::RejectClipboardItemPromise(ScriptValue exception) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::RejectClipboardItemPromise",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "exception", base::ToString(exception));
  xtrace->LogLineRun(xtrace_mrid, 545);
  script_promise_resolver_->Reject(exception);
}

PermissionService *ClipboardPromise::GetPermissionService() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::GetPermissionService",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 549);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 550);
  ExecutionContext *context = GetExecutionContext();
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");

  xtrace->LogLineRun(xtrace_mrid, 551);
  DCHECK(context);
  xtrace->LogLineRun(xtrace_mrid, 552);
  if (!permission_service_.is_bound()) {
    xtrace->LogLineRun(xtrace_mrid, 553);
    ConnectToPermissionService(context,
                               permission_service_.BindNewPipeAndPassReceiver(
                                   GetClipboardTaskRunner()));
  }
  xtrace->LogLineRun(xtrace_mrid, 557);
  return permission_service_.get();
}

void ClipboardPromise::ValidatePreconditions(
    mojom::blink::PermissionName permission, bool will_be_sanitized,
    base::OnceCallback<void(mojom::blink::PermissionStatus)> callback) {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::ValidatePreconditions",
      "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "permission", base::ToString(permission));
  xtrace->LocalVarUpdate(xtrace_mrid, "will_be_sanitized",
                         base::ToString(will_be_sanitized));
  xtrace->LocalVarUpdate(xtrace_mrid, "callback", base::ToString(callback));
  xtrace->LogLineRun(xtrace_mrid, 564);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 565);
  DCHECK(script_promise_resolver_);
  xtrace->LogLineRun(xtrace_mrid, 566);
  DCHECK(permission == mojom::blink::PermissionName::CLIPBOARD_READ ||
         permission == mojom::blink::PermissionName::CLIPBOARD_WRITE);

  xtrace->LogLineRun(xtrace_mrid, 569);
  ExecutionContext *context = GetExecutionContext();
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");

  xtrace->LogLineRun(xtrace_mrid, 570);
  DCHECK(context);
  xtrace->LogLineRun(xtrace_mrid, 571);
  LocalDOMWindow &window = *To<LocalDOMWindow>(context);
  xtrace->LocalVarUpdate(xtrace_mrid, "window", base::ToString(window));

  xtrace->LogLineRun(xtrace_mrid, 572);
  DCHECK(window.IsSecureContext()); // [SecureContext] in IDL

  xtrace->LogLineRun(xtrace_mrid, 574);
  if (!window.document()->hasFocus()) {
    xtrace->LogLineRun(xtrace_mrid, 575);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, "Document is not focused.");
    xtrace->LogLineRun(xtrace_mrid, 577);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 580);
  constexpr char kFeaturePolicyMessage[] =
      "The Clipboard API has been blocked because of a permissions policy "
      "applied to the current document. See https://goo.gl/EuHzyv for more "
      "details.";
  xtrace->LocalVarUpdate(xtrace_mrid, "kFeaturePolicyMessage",
                         base::ToString(kFeaturePolicyMessage));

  xtrace->LogLineRun(xtrace_mrid, 585);
  if ((permission == mojom::blink::PermissionName::CLIPBOARD_READ &&
       !window.IsFeatureEnabled(
           mojom::blink::PermissionsPolicyFeature::kClipboardRead,
           ReportOptions::kReportOnFailure, kFeaturePolicyMessage)) ||
      (permission == mojom::blink::PermissionName::CLIPBOARD_WRITE &&
       !window.IsFeatureEnabled(
           mojom::blink::PermissionsPolicyFeature::kClipboardWrite,
           ReportOptions::kReportOnFailure, kFeaturePolicyMessage))) {
    xtrace->LogLineRun(xtrace_mrid, 593);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError, kFeaturePolicyMessage);
    xtrace->LogLineRun(xtrace_mrid, 595);
    return;
  }

  // Grant permission by-default if extension has read/write permissions.
  xtrace->LogLineRun(xtrace_mrid, 599);
  if (GetLocalFrame()->GetContentSettingsClient() &&
      ((permission == mojom::blink::PermissionName::CLIPBOARD_READ &&
        GetLocalFrame()
            ->GetContentSettingsClient()
            ->AllowReadFromClipboard()) ||
       (permission == mojom::blink::PermissionName::CLIPBOARD_WRITE &&
        GetLocalFrame()
            ->GetContentSettingsClient()
            ->AllowWriteToClipboard()))) {
    xtrace->LogLineRun(xtrace_mrid, 608);
    GetClipboardTaskRunner()->PostTask(
        FROM_HERE, WTF::BindOnce(std::move(callback),
                                 mojom::blink::PermissionStatus::GRANTED));
    xtrace->LogLineRun(xtrace_mrid, 611);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 614);
  if ((permission == mojom::blink::PermissionName::CLIPBOARD_WRITE &&
       ClipboardCommands::IsExecutingCutOrCopy(*context)) ||
      (permission == mojom::blink::PermissionName::CLIPBOARD_READ &&
       ClipboardCommands::IsExecutingPaste(*context))) {
    xtrace->LogLineRun(xtrace_mrid, 618);
    GetClipboardTaskRunner()->PostTask(
        FROM_HERE, WTF::BindOnce(std::move(callback),
                                 mojom::blink::PermissionStatus::GRANTED));
    xtrace->LogLineRun(xtrace_mrid, 621);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 624);
  if (!GetPermissionService()) {
    xtrace->LogLineRun(xtrace_mrid, 625);
    script_promise_resolver_->RejectWithDOMException(
        DOMExceptionCode::kNotAllowedError,
        "Permission Service could not connect.");
    xtrace->LogLineRun(xtrace_mrid, 628);
    return;
  }

  xtrace->LogLineRun(xtrace_mrid, 631);
  bool has_transient_user_activation =
      LocalFrame::HasTransientUserActivation(GetLocalFrame());
  xtrace->LocalVarUpdate(xtrace_mrid, "has_transient_user_activation",
                         base::ToString(has_transient_user_activation));

  xtrace->LogLineRun(xtrace_mrid, 633);
  auto permission_descriptor = CreateClipboardPermissionDescriptor(
      permission, /*has_user_gesture=*/has_transient_user_activation,
      /*will_be_sanitized=*/will_be_sanitized);
  xtrace->LocalVarUpdate(xtrace_mrid, "permission_descriptor",
                         base::ToString(permission_descriptor));

  // Note that extra checks are performed browser-side in
  // `ContentBrowserClient::IsClipboardPasteAllowed()`.
  xtrace->LogLineRun(xtrace_mrid, 639);
  permission_service_->RequestPermission(
      std::move(permission_descriptor),
      /*user_gesture=*/has_transient_user_activation, std::move(callback));
}

LocalFrame *ClipboardPromise::GetLocalFrame() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::GetLocalFrame", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 645);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 646);
  ExecutionContext *context = GetExecutionContext();
  xtrace->LocalVarUpdate(xtrace_mrid, "context",
                         context ? base::ToString(*context) : "");

  // In case the context was destroyed and the caller didn't check for it, we
  // just return nullptr.
  xtrace->LogLineRun(xtrace_mrid, 649);
  if (!context) {
    xtrace->LogLineRun(xtrace_mrid, 650);
    return nullptr;
  }
  xtrace->LogLineRun(xtrace_mrid, 652);
  LocalFrame *local_frame = To<LocalDOMWindow>(context)->GetFrame();
  xtrace->LocalVarUpdate(xtrace_mrid, "local_frame",
                         local_frame ? base::ToString(*local_frame) : "");

  xtrace->LogLineRun(xtrace_mrid, 653);
  return local_frame;
}

ScriptState *ClipboardPromise::GetScriptState() const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::GetScriptState", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 657);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 658);
  return script_promise_resolver_->GetScriptState();
}

scoped_refptr<base::SingleThreadTaskRunner>
ClipboardPromise::GetClipboardTaskRunner() {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::GetClipboardTaskRunner",
      "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 663);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  xtrace->LogLineRun(xtrace_mrid, 664);
  return GetExecutionContext()->GetTaskRunner(TaskType::kClipboard);
}

// ExecutionContextLifecycleObserver implementation.
void ClipboardPromise::ContextDestroyed() {
  // This isn't the correct way to create a DOMException, but the correct way
  // probably wouldn't work at this point, and it probably doesn't matter.
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::ContextDestroyed", "GUID_FROM_TEST");
  xtrace->LogLineRun(xtrace_mrid, 671);
  script_promise_resolver_->Reject(MakeGarbageCollected<DOMException>(
      DOMExceptionCode::kNotAllowedError, "Document detached."));
  xtrace->LogLineRun(xtrace_mrid, 673);
  clipboard_writer_.Clear();
}

void ClipboardPromise::Trace(Visitor *visitor) const {
  blink::XTrace *xtrace = blink::XTrace::getInstance();
  std::string xtrace_mrid = xtrace->OnMethodEnter(
      "clip_prom.cc", "ClipboardPromise::Trace", "GUID_FROM_TEST");
  xtrace->LocalVarUpdate(xtrace_mrid, "visitor",
                         visitor ? base::ToString(*visitor) : "");
  xtrace->LogLineRun(xtrace_mrid, 677);
  visitor->Trace(script_promise_resolver_);
  xtrace->LogLineRun(xtrace_mrid, 678);
  visitor->Trace(clipboard_writer_);
  xtrace->LogLineRun(xtrace_mrid, 679);
  visitor->Trace(permission_service_);
  xtrace->LogLineRun(xtrace_mrid, 680);
  visitor->Trace(clipboard_item_data_);
  xtrace->LogLineRun(xtrace_mrid, 681);
  visitor->Trace(clipboard_item_data_with_promises_);
  xtrace->LogLineRun(xtrace_mrid, 682);
  ExecutionContextLifecycleObserver::Trace(visitor);
}

} // namespace blink

