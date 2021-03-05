// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/virtualkeyboard/virtual_keyboard.h"

#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/ime/input_method_controller.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
<<<<<<< HEAD
#include "third_party/blink/renderer/core/frame/web_feature.h"
=======
>>>>>>> 96415363c28f5a0958b580e9d1351550bc6a120a
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/virtualkeyboard/virtual_keyboard_overlay_geometry_change_event.h"
<<<<<<< HEAD
#include "third_party/blink/renderer/platform/heap/heap.h"
=======
>>>>>>> 96415363c28f5a0958b580e9d1351550bc6a120a
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"

namespace blink {

VirtualKeyboard::VirtualKeyboard(LocalFrame* frame)
    : ExecutionContextClient(frame ? frame->DomWindow()->GetExecutionContext()
                                   : nullptr),
<<<<<<< HEAD
      VirtualKeyboardOverlayChangeObserver(frame ? frame->GetPage() : nullptr) {
}
=======
      VirtualKeyboardOverlayChangedObserver(frame) {}
>>>>>>> 96415363c28f5a0958b580e9d1351550bc6a120a

ExecutionContext* VirtualKeyboard::GetExecutionContext() const {
  return ExecutionContextClient::GetExecutionContext();
}

const AtomicString& VirtualKeyboard::InterfaceName() const {
  return event_target_names::kVirtualKeyboard;
}

VirtualKeyboard::~VirtualKeyboard() = default;

bool VirtualKeyboard::overlaysContent() const {
  return overlays_content_;
}

void VirtualKeyboard::setOverlaysContent(bool overlays_content) {
  LocalFrame* frame = GetFrame();
  if (frame && frame->IsMainFrame()) {
    if (overlays_content != overlays_content_) {
      auto& local_frame_host = frame->GetLocalFrameHostRemote();
      local_frame_host.SetVirtualKeyboardOverlayPolicy(overlays_content);
      overlays_content_ = overlays_content;
    }
  } else {
    GetExecutionContext()->AddConsoleMessage(
        MakeGarbageCollected<ConsoleMessage>(
<<<<<<< HEAD
            mojom::ConsoleMessageSource::kJavaScript,
            mojom::ConsoleMessageLevel::kWarning,
            "Setting overlaysContent is only supported from "
            "the top level browsing context"));
  }

  if (frame && frame->GetDocument()) {
    UseCounter::CountEdge(frame->GetDocument(),
        EdgeWebFeature::kVirtualKeyboardOverlayPolicy);
  }
=======
            mojom::blink::ConsoleMessageSource::kJavaScript,
            mojom::blink::ConsoleMessageLevel::kWarning,
            "Setting overlaysContent is only supported from "
            "the top level browsing context"));
  }
>>>>>>> 96415363c28f5a0958b580e9d1351550bc6a120a
}

void VirtualKeyboard::VirtualKeyboardOverlayChanged(
    const gfx::Rect& keyboard_rect) {
<<<<<<< HEAD
  TRACE_EVENT0("vk", "VirtualKeyboard::VirtualKeyboardOverlayChanged");
  LocalFrame* frame = GetFrame();
  if (frame && frame->GetDocument()) {
    UseCounter::CountEdge(frame->GetDocument(),
        EdgeWebFeature::kVirtualKeyboardOverlayChangeFired);
  }

  DispatchEvent(*VirtualKeyboardOverlayGeometryChangeEvent::Create(
      event_type_names::kOverlaygeometrychange,
      DOMRect::FromFloatRect(
          FloatRect(gfx::RectF(keyboard_rect)))));
=======
  DispatchEvent(
      *(MakeGarbageCollected<VirtualKeyboardOverlayGeometryChangeEvent>(
          event_type_names::kOverlaygeometrychange,
          DOMRect::FromFloatRect(FloatRect(gfx::RectF(keyboard_rect))))));
>>>>>>> 96415363c28f5a0958b580e9d1351550bc6a120a
}

void VirtualKeyboard::show() {
  TRACE_EVENT0("vk", "VirtualKeyboard::show");
  LocalFrame* frame = GetFrame();
  if (frame) {
    frame->GetInputMethodController().SetVirtualKeyboardVisibilityRequest(
        WebVirtualKeyboardVisibilityRequest::
            kWebVirtualKeyboardVisibilityRequestShow);
    if (frame->GetDocument()) {
      UseCounter::CountEdge(frame->GetDocument(),
                            EdgeWebFeature::kVirtualKeyboardShowAPI);
    }
  }
}

void VirtualKeyboard::hide() {
  TRACE_EVENT0("vk", "VirtualKeyboard::hide");
  LocalFrame* frame = GetFrame();
  if (frame) {
    frame->GetInputMethodController().SetVirtualKeyboardVisibilityRequest(
        WebVirtualKeyboardVisibilityRequest::
            kWebVirtualKeyboardVisibilityRequestHide);
    if (frame->GetDocument()) {
      UseCounter::CountEdge(frame->GetDocument(),
                            EdgeWebFeature::kVirtualKeyboardHideAPI);
    }
  }
}

void VirtualKeyboard::Trace(Visitor* visitor) {
  EventTargetWithInlineData::Trace(visitor);
  ExecutionContextClient::Trace(visitor);
}

}  // namespace blink
