#include "base/strings/to_string.h"
#include "third_party/xtrace/xtrace.h"
// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/clipboard/clipboard.h"

#include <utility>

#include "net/base/mime_util.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/clipboard/clipboard_promise.h"
#include "ui/base/clipboard/clipboard_constants.h"

namespace blink {

// static
const char Clipboard::kSupplementName[] = "Clipboard";

Clipboard *Clipboard::clipboard(Navigator &navigator) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::clipboard",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "navigator", base::ToString(navigator));
  xtrace->LogLineRun(xtrace_mrid, 22);
  Clipboard *clipboard = Supplement<Navigator>::From<Clipboard>(navigator);
  xtrace->LocalVarUpdate(xtrace_mrid, "clipboard",
                         clipboard ? base::ToString(*clipboard) : "");

  xtrace->LogLineRun(xtrace_mrid, 23);
  if (!clipboard) {
    xtrace->LogLineRun(xtrace_mrid, 24);
    clipboard = MakeGarbageCollected<Clipboard>(navigator);
    xtrace->LogLineRun(xtrace_mrid, 25);
    ProvideTo(navigator, clipboard);
  }
  xtrace->LogLineRun(xtrace_mrid, 27);
  xtrace->FlushAllEventsToJSONFile();
  return clipboard;
}

Clipboard::Clipboard(Navigator &navigator) : Supplement<Navigator>(navigator) {}

ScriptPromise<IDLSequence<ClipboardItem>>
Clipboard::read(ScriptState *script_state, ClipboardUnsanitizedFormats *formats,
                ExceptionState &exception_state) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::read",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "formats",
                         formats ? base::ToString(*formats) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 36);
  LocalDOMWindow *window = GetSupplementable()->DomWindow();
  xtrace->LocalVarUpdate(xtrace_mrid, "window",
                         window ? base::ToString(*window) : "");

  xtrace->LogLineRun(xtrace_mrid, 37);
  LocalFrame *local_frame = window ? window->GetFrame() : nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "local_frame",
                         local_frame ? base::ToString(*local_frame) : "");

  xtrace->LogLineRun(xtrace_mrid, 38);
  if (local_frame && local_frame->IsAdScriptInStack()) {
    xtrace->LogLineRun(xtrace_mrid, 39);
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kAdScriptInStackOnClipboardRead);
  }

  xtrace->LogLineRun(xtrace_mrid, 43);
  xtrace->FlushAllEventsToJSONFile();
  return ClipboardPromise::CreateForRead(GetExecutionContext(), script_state,
                                         formats, exception_state);
}

ScriptPromise<IDLString> Clipboard::readText(ScriptState *script_state,
                                             ExceptionState &exception_state) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::readText",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 49);
  LocalDOMWindow *window = GetSupplementable()->DomWindow();
  xtrace->LocalVarUpdate(xtrace_mrid, "window",
                         window ? base::ToString(*window) : "");

  xtrace->LogLineRun(xtrace_mrid, 50);
  LocalFrame *local_frame = window ? window->GetFrame() : nullptr;
  xtrace->LocalVarUpdate(xtrace_mrid, "local_frame",
                         local_frame ? base::ToString(*local_frame) : "");

  xtrace->LogLineRun(xtrace_mrid, 51);
  if (local_frame && local_frame->IsAdScriptInStack()) {
    xtrace->LogLineRun(xtrace_mrid, 52);
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kAdScriptInStackOnClipboardRead);
  }

  xtrace->LogLineRun(xtrace_mrid, 56);
  xtrace->FlushAllEventsToJSONFile();
  return ClipboardPromise::CreateForReadText(GetExecutionContext(),
                                             script_state, exception_state);
}

ScriptPromise<IDLUndefined>
Clipboard::write(ScriptState *script_state,
                 const HeapVector<Member<ClipboardItem>> &data,
                 ExceptionState &exception_state) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::write",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "data", base::ToString(data));
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 64);
  xtrace->FlushAllEventsToJSONFile();
  return ClipboardPromise::CreateForWrite(GetExecutionContext(), script_state,
                                          std::move(data), exception_state);
}

ScriptPromise<IDLUndefined>
Clipboard::writeText(ScriptState *script_state, const String &data,
                     ExceptionState &exception_state) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::writeText",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "script_state",
                         script_state ? base::ToString(*script_state) : "");
  xtrace->LocalVarUpdate(xtrace_mrid, "data", base::ToString(data));
  xtrace->LocalVarUpdate(xtrace_mrid, "exception_state",
                         base::ToString(exception_state));
  xtrace->LogLineRun(xtrace_mrid, 72);
  xtrace->FlushAllEventsToJSONFile();
  return ClipboardPromise::CreateForWriteText(
      GetExecutionContext(), script_state, data, exception_state);
}

const AtomicString &Clipboard::InterfaceName() const {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::InterfaceName",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LogLineRun(xtrace_mrid, 77);
  xtrace->FlushAllEventsToJSONFile();
  return event_target_names::kClipboard;
}

ExecutionContext *Clipboard::GetExecutionContext() const {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::GetExecutionContext",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LogLineRun(xtrace_mrid, 81);
  xtrace->FlushAllEventsToJSONFile();
  return GetSupplementable()->DomWindow();
}

// static
String Clipboard::ParseWebCustomFormat(const String &format) {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::ParseWebCustomFormat",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "format", base::ToString(format));
  xtrace->LogLineRun(xtrace_mrid, 86);
  if (format.StartsWith(ui::kWebClipboardFormatPrefix)) {
    xtrace->LogLineRun(xtrace_mrid, 87);
    String web_custom_format_suffix = format.Substring(
        static_cast<unsigned>(std::strlen(ui::kWebClipboardFormatPrefix)));
    xtrace->LogLineRun(xtrace_mrid, 89);
    std::string web_top_level_mime_type;
    xtrace->LogLineRun(xtrace_mrid, 90);
    std::string web_mime_sub_type;
    xtrace->LogLineRun(xtrace_mrid, 91);
    if (net::ParseMimeTypeWithoutParameter(web_custom_format_suffix.Utf8(),
                                           &web_top_level_mime_type,
                                           &web_mime_sub_type)) {
      xtrace->LogLineRun(xtrace_mrid, 94);
      return String::Format("%s/%s", web_top_level_mime_type.c_str(),
                            web_mime_sub_type.c_str());
    }
  }
  xtrace->LogLineRun(xtrace_mrid, 98);
  xtrace->FlushAllEventsToJSONFile();
  return g_empty_string;
}

void Clipboard::Trace(Visitor *visitor) const {
  XTrace *xtrace = XTrace::getInstance();
  std::string xtrace_mrid =
      xtrace->OnMethodEnter("clipboard.cc", "Clipboard::Trace",
                            "00000000-0000-0000-0000-000000000000");
  xtrace->LocalVarUpdate(xtrace_mrid, "visitor",
                         visitor ? base::ToString(*visitor) : "");
  xtrace->LogLineRun(xtrace_mrid, 102);
  EventTarget::Trace(visitor);
  xtrace->LogLineRun(xtrace_mrid, 103);
  xtrace->FlushAllEventsToJSONFile();
  Supplement<Navigator>::Trace(visitor);
}

} // namespace blink

