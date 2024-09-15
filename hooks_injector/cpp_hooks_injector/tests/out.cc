
> tree-sitter-parser@1.0.0 inject
> node esbuild.config.js && node dist/out.js

Injecting.... // Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer.h"
#include "third_party/blink/renderer/core/clipboard/xtrace.h"
#include "third_party/blink/renderer/core/css/properties/longhands.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/page/drag_image.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/paint_test_configurations.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"

namespace blink {

void WriteBitmapToFile(const SkBitmap &bitmap, const std::string &path) {
  // Ensure the bitmap is valid

  SkFILEWStream fileStream(path.c_str());

  SkPngEncoder::Encode(&fileStream, bitmap.pixmap(), /*options=*/{});
}
class DataTransferTest : public PaintTestConfigurations, public RenderingTest {
 protected:
  Page& GetPage() const { return *GetDocument().GetPage(); }
  LocalFrame& GetFrame() const { return *GetDocument().GetFrame(); }
};

INSTANTIATE_PAINT_TEST_SUITE_P(DataTransferTest);

TEST_P(DataTransferTest, NodeImage) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
    </style>
    <div id=sample></div>
  )HTML");
  Element* sample = GetDocument().getElementById(AtomicString("sample"));
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(gfx::Size(100, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageWithNestedElement) {
  SetBodyInnerHTML(R"HTML(
    <style>
      div { -webkit-user-drag: element }
      span:-webkit-drag { color: #0F0 }
    </style>
    <div id=sample><span>Green when dragged</span></div>
  )HTML");
  Element* sample = GetDocument().getElementById(AtomicString("sample"));
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(Color::FromRGB(0, 255, 0),
            sample->firstChild()->GetLayoutObject()->ResolveColor(
                GetCSSPropertyColor()))
      << "Descendants node should have :-webkit-drag.";
}

TEST_P(DataTransferTest, NodeImageWithPsuedoClassWebKitDrag) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
      #sample:-webkit-drag { width: 200px; height: 200px; }
    </style>
    <div id=sample></div>
  )HTML");
  Element* sample = GetDocument().getElementById(AtomicString("sample"));
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(gfx::Size(200, 200), image->Size())
      << ":-webkit-drag should affect dragged image.";
}

TEST_P(DataTransferTest, NodeImageWithoutDraggedLayoutObject) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
      #sample:-webkit-drag { display:none }
    </style>
    <div id=sample></div>
  )HTML");
  Element* sample = GetDocument().getElementById(AtomicString("sample"));
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  EXPECT_EQ(nullptr, image.get()) << ":-webkit-drag blows away layout object";
}

TEST_P(DataTransferTest, NodeImageWithChangingLayoutObject) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #sample { color: blue; }
      #sample:-webkit-drag { display: inline-block; color: red; }
    </style>
    <span id=sample>foo</span>
  )HTML");
  Element* sample = GetDocument().getElementById(AtomicString("sample"));
  UpdateAllLifecyclePhasesForTest();
  LayoutObject* before_layout_object = sample->GetLayoutObject();
  const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);

  EXPECT_TRUE(sample->GetLayoutObject() != before_layout_object)
      << ":-webkit-drag causes sample to have different layout object.";
  EXPECT_EQ(Color::FromRGB(255, 0, 0),
            sample->GetLayoutObject()->ResolveColor(GetCSSPropertyColor()))
      << "#sample has :-webkit-drag.";

  // Layout w/o :-webkit-drag
  UpdateAllLifecyclePhasesForTest();

  EXPECT_EQ(Color::FromRGB(0, 0, 255),
            sample->GetLayoutObject()->ResolveColor(GetCSSPropertyColor()))
      << "#sample doesn't have :-webkit-drag.";
}

TEST_P(DataTransferTest, NodeImageExceedsViewportBounds) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #node { width: 2000px; height: 2000px; }
    </style>
    <div id='node'></div>
  )HTML");
  Element& node = *GetDocument().getElementById(AtomicString("node"));
  const auto image = DataTransfer::NodeImage(GetFrame(), node);
  EXPECT_EQ(gfx::Size(800, 600), image->Size());
}

TEST_P(DataTransferTest, NodeImageUnderScrollOffset) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #first { width: 500px; height: 500px; }
      #second { width: 800px; height: 900px; }
    </style>
    <div id='first'></div>
    <div id='second'></div>
  )HTML");

  const int scroll_amount = 10;
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);

  // The first div should be offset by the scroll offset.
  Element& first = *GetDocument().getElementById(AtomicString("first"));
  const auto first_image = DataTransfer::NodeImage(GetFrame(), first);
  const int first_height = 500;
  EXPECT_EQ(gfx::Size(500, first_height), first_image->Size());

  // The second div should also be offset by the scroll offset. In addition,
  // the second div should be clipped by the viewport.
  Element& second = *GetDocument().getElementById(AtomicString("second"));
  const auto second_image = DataTransfer::NodeImage(GetFrame(), second);
  const int viewport_height = 600;
  EXPECT_EQ(gfx::Size(800, viewport_height - (first_height - scroll_amount)),
            second_image->Size());
}

TEST_P(DataTransferTest, NodeImageSizeWithPageScaleFactor) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      html, body { height: 2000px; }
      #node { width: 200px; height: 141px; }
    </style>
    <div id='node'></div>
  )HTML");
  const int page_scale_factor = 2;
  GetPage().SetPageScaleFactor(page_scale_factor);
  Element& node = *GetDocument().getElementById(AtomicString("node"));
  const auto image = DataTransfer::NodeImage(GetFrame(), node);
  const int node_width = 200;
  const int node_height = 141;
  EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      node_height * page_scale_factor),
            image->Size());

  // Check that a scroll offset is scaled to device coordinates which includes
  // page scale factor.
  const int scroll_amount = 10;
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);
  const auto image_with_offset = DataTransfer::NodeImage(GetFrame(), node);
  EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      node_height * page_scale_factor),
            image_with_offset->Size());
}

TEST_P(DataTransferTest, NodeImageSizeWithPageScaleFactorTooLarge) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      html, body { height: 2000px; }
      #node { width: 800px; height: 601px; }
    </style>
    <div id='node'></div>
  )HTML");
  const int page_scale_factor = 2;
  GetPage().SetPageScaleFactor(page_scale_factor);
  Element& node = *GetDocument().getElementById(AtomicString("node"));
  const auto image = DataTransfer::NodeImage(GetFrame(), node);
  const int node_width = 800;
  const int node_height = 601;
  EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      (node_height - 1) * page_scale_factor),
            image->Size());

  // Check that a scroll offset is scaled to device coordinates which includes
  // page scale factor.
  const int scroll_amount = 10;
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);
  const auto image_with_offset = DataTransfer::NodeImage(GetFrame(), node);
  EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      (node_height - scroll_amount) * page_scale_factor),
            image_with_offset->Size());
}

TEST_P(DataTransferTest, NodeImageWithPageScaleFactor) {
  // #bluegreen is a 2x1 rectangle where the left pixel is blue and the right
  // pixel is green. The element is offset by a margin of 1px.
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #bluegreen {
        width: 1px;
        height: 1px;
        background: #0f0;
        border-left: 1px solid #00f;
        margin: 1px;
      }
    </style>
    <div id='bluegreen'></div>
  )HTML");
  const int page_scale_factor = 2;
  GetPage().SetPageScaleFactor(page_scale_factor);
  Element& blue_green =
      *GetDocument().getElementById(AtomicString("bluegreen"));
  const auto image = DataTransfer::NodeImage(GetFrame(), blue_green);
  const int blue_green_width = 2;
  const int blue_green_height = 1;
  EXPECT_EQ(gfx::Size(blue_green_width * page_scale_factor,
                      blue_green_height * page_scale_factor),
            image->Size());

  // Even though #bluegreen is offset by a margin of 1px (which is 2px in device
  // coordinates), we expect it to be painted at 0x0 and completely fill the 4x2
  // bitmap.
  SkBitmap expected_bitmap;
  expected_bitmap.allocN32Pixels(4, 2);
  expected_bitmap.eraseArea(SkIRect::MakeXYWH(0, 0, 2, 2), 0xFF0000FF);
  expected_bitmap.eraseArea(SkIRect::MakeXYWH(2, 0, 2, 2), 0xFF00FF00);
  const SkBitmap& bitmap = image->Bitmap();
  for (int x = 0; x < bitmap.width(); ++x)
    for (int y = 0; y < bitmap.height(); ++y)
      EXPECT_EQ(expected_bitmap.getColor(x, y), bitmap.getColor(x, y));
}

TEST_P(DataTransferTest, NodeImageFullyOffscreen) {
  SetBodyInnerHTML(R"HTML(
    <style>
    #target {
      position: absolute;
      top: 800px;
      left: 0;
      height: 100px;
      width: 200px;
      background: lightblue;
      isolation: isolate;
    }
    </style>
    <div id="target" draggable="true" ondragstart="drag(event)"></div>
  )HTML");

  const int scroll_amount = 800;
  LocalFrameView* frame_view = GetDocument().View();
  frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);

  Element& target = *GetDocument().getElementById(AtomicString("target"));
  const auto image = DataTransfer::NodeImage(GetFrame(), target);

  EXPECT_EQ(gfx::Size(200, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageWithScrolling) {
  SetBodyInnerHTML(R"HTML(
    <style>
    #target {
      position: absolute;
      top: 800px;
      left: 0;
      height: 100px;
      width: 200px;
      background: lightblue;
      isolation: isolate;
    }
    </style>
    <div id="target" draggable="true" ondragstart="drag(event)"></div>
  )HTML");

  Element& target = *GetDocument().getElementById(AtomicString("target"));
  const auto image = DataTransfer::NodeImage(GetFrame(), target);

  EXPECT_EQ(gfx::Size(200, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageInOffsetStackingContext) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #container {
        position: absolute;
        top: 4px;
        z-index: 10;
      }
      #drag {
        width: 5px;
        height: 5px;
        background: #0F0;
      }
    </style>
    <div id="container">
      <div id="drag" draggable="true"></div>
    </div>
  )HTML");
  Element& drag = *GetDocument().getElementById(AtomicString("drag"));
  const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  constexpr int drag_width = 5;
  constexpr int drag_height = 5;
  EXPECT_EQ(gfx::Size(drag_width, drag_height), image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  const SkBitmap& bitmap = image->Bitmap();
  for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y)
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageWithLargerPositionedDescendant) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #drag {
        position: absolute;
        top: 100px;
        left: 0;
        height: 1px;
        width: 1px;
        background: #00f;
      }
      #child {
        position: absolute;
        top: -1px;
        left: 0;
        height: 3px;
        width: 1px;
        background: #0f0;
      }
    </style>
    <div id="drag" draggable="true">
      <div id="child"></div>
    </div>
  )HTML");
  Element& drag = *GetDocument().getElementById(AtomicString("drag"));
  const auto image = DataTransfer::NodeImage(GetFrame(), drag);

  // The positioned #child should expand the dragged image's size.
  constexpr int drag_width = 1;
  constexpr int drag_height = 3;
  EXPECT_EQ(gfx::Size(drag_width, drag_height), image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green
  // which is the color of the #child which fully covers the dragged element.
  const SkBitmap& bitmap = image->Bitmap();
  for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y)
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageOutOfView) {
  SetBodyInnerHTML(R"HTML(
    <div id="drag" style="position: absolute; z-index: 1; top: -200px; left: 0;
                          width: 100px; height: 100px; background: green">
    </div>
  )HTML");

  auto image = DataTransfer::NodeImage(
      GetFrame(), *GetDocument().getElementById(AtomicString("drag")));
  EXPECT_EQ(gfx::Size(100, 100), image->Size());
  SkColor green = SkColorSetRGB(0, 0x80, 0);
  const SkBitmap& bitmap = image->Bitmap();
  for (int x = 0; x < 100; ++x) {
    for (int y = 0; y < 100; ++y)
      ASSERT_EQ(green, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageFixedChild) {
  SetBodyInnerHTML(R"HTML(
    <div id="drag" style="position: absolute; z-index: 1; top: 100px; left: 0;
                          width: 50px; height: 100px; background: green">
      <div style="position: fixed; top: 50px; width: 100px; height: 50px;
                  background: blue">
      </div>
    </div>
    <div style="height: 2000px"></div>
  )HTML");

  GetDocument().View()->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, 100), mojom::blink::ScrollType::kProgrammatic);

  auto image = DataTransfer::NodeImage(
      GetFrame(), *GetDocument().getElementById(AtomicString("drag")));
  EXPECT_EQ(gfx::Size(100, 100), image->Size());
  SkColor green = SkColorSetRGB(0, 0x80, 0);
  SkColor blue = SkColorSetRGB(0, 0, 0xFF);
  const SkBitmap& bitmap = image->Bitmap();
  for (int x = 0; x < 100; ++x) {
    for (int y = 0; y < 50; ++y) {
      ASSERT_EQ(x < 50 ? green : SK_ColorTRANSPARENT, bitmap.getColor(x, y));
    }
    for (int y = 50; y < 100; ++y)
      ASSERT_EQ(blue, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, CreateDragImageWithEmptyImageResource) {
  DataTransfer* data_transfer = DataTransfer::Create();
  data_transfer->SetDragImageResource(
      MakeGarbageCollected<ImageResourceContent>(nullptr), gfx::Point());

  gfx::Point drag_offset;
  std::unique_ptr<DragImage> drag_image = data_transfer->CreateDragImage(
      drag_offset, /* device_scale_factor*/ 1, &GetFrame());
  // The test passes if the above call does not crash.
}

TEST_P(DataTransferTest, NodeImageWithScaleFactor) {
  SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
        text-align: end; // 8 https://source.chromium.org/chromium/chromium/src/+/main:out/linux-Debug/gen/third_party/blink/renderer/core/style/computed_style_base_constants.h;drc=f9a11014f62ee4ffc170d8e8af48ac6e8125995b;l=564
      }
      #drag {
        transform: scale(2);
        transform-origin: left top;
        width: 5px;
        height: 5px;
        background: #0F0;
        text-align: center; // 2
      }
</style>
<div id="container">
  <div id="drag" draggable="true"></div>
</div>
  )HTML");
  Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  constexpr int scaleFactor = 2;
  constexpr int drag_width = 5;
  constexpr int drag_height = 5;
  EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  const SkBitmap &bitmap = image->Bitmap();
  for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y) {
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
    }
  }
}

TEST_P(DataTransferTest, NodeImageWithScaleFactorSelf) {
  SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
      }
      #drag {
        width: 5px;
        height: 5px;
        transform: scale(2);
        transform-origin: left top;
        background: #0F0;
      }
</style>
<div id="container">
  <div id="drag" draggable="true"></div>
</div>
  )HTML");
  Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  constexpr int scaleFactor = 2;
  constexpr int drag_width = 5;
  constexpr int drag_height = 5;
  EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  const SkBitmap &bitmap = image->Bitmap();
  for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y) {
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
    }
  }
}

TEST_P(DataTransferTest, NodeImageWithScaleFactorChild) {
  SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
      }
      #drag {
        width: 50px;
        height: 50px;


        background: #0F0;
      }
      #internal{
        transform: scale(2);
        transform-origin: left top;

        width: 10px;
        height: 10px;
        background: #00F;

      }
</style>
<div id="container">
  <div id="drag" draggable="true">
    <div id="internal"></div>
  </div>
</div>
  )HTML");
  Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  constexpr int scaleFactor = 1;
  constexpr int drag_width = 50;
  constexpr int drag_height = 50;
  EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  const SkBitmap &bitmap = image->Bitmap();

  WriteBitmapToFile(bitmap, "/tmp/blink.png");

  for (int x = 0; x < 20; ++x) {
    for (int y = 0; y < 20; ++y) {
      EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(x, y));
    }
  }
}

}  // namespace blink

// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer.h"
#include "third_party/blink/renderer/core/clipboard/xtrace.h"
#include "third_party/blink/renderer/core/css/properties/longhands.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/page/drag_image.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/paint_test_configurations.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/encode/SkPngEncoder.h"

namespace blink {

void WriteBitmapToFile(const SkBitmap &bitmap, const std::string &path) {
  // Ensure the bitmap is valid

  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "WriteBitmapToFile", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 26); SkFILEWStream fileStream(path.c_str());

  xtrace->LogLineRun(xtrace_mrid, 28); xtrace->FlushAllEventsToJSONFile(); SkPngEncoder::Encode(&fileStream, bitmap.pixmap(), /*options=*/{});
}
class DataTransferTest : public PaintTestConfigurations, public RenderingTest {
 protected:
  Page& GetPage() const { XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "GetPage", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 32); xtrace->FlushAllEventsToJSONFile(); return *GetDocument().GetPage(); }
  LocalFrame& GetFrame() const { XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "GetFrame", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 33); xtrace->FlushAllEventsToJSONFile(); return *GetDocument().GetFrame(); }
};

INSTANTIATE_PAINT_TEST_SUITE_P(DataTransferTest);

TEST_P(DataTransferTest, NodeImage) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 39); SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
    </style>
    <div id=sample></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 45); Element* sample = GetDocument().getElementById(AtomicString("sample"));
  xtrace->LogLineRun(xtrace_mrid, 46); const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  xtrace->LogLineRun(xtrace_mrid, 48); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(100, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageWithNestedElement) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 52); SetBodyInnerHTML(R"HTML(
    <style>
      div { -webkit-user-drag: element }
      span:-webkit-drag { color: #0F0 }
    </style>
    <div id=sample><span>Green when dragged</span></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 59); Element* sample = GetDocument().getElementById(AtomicString("sample"));
  xtrace->LogLineRun(xtrace_mrid, 60); const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  xtrace->LogLineRun(xtrace_mrid, 62); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(Color::FromRGB(0, 255, 0),
            sample->firstChild()->GetLayoutObject()->ResolveColor(
                GetCSSPropertyColor()))
      << "Descendants node should have :-webkit-drag.";
}

TEST_P(DataTransferTest, NodeImageWithPsuedoClassWebKitDrag) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 69); SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
      #sample:-webkit-drag { width: 200px; height: 200px; }
    </style>
    <div id=sample></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 76); Element* sample = GetDocument().getElementById(AtomicString("sample"));
  xtrace->LogLineRun(xtrace_mrid, 77); const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  xtrace->LogLineRun(xtrace_mrid, 79); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(200, 200), image->Size())
      << ":-webkit-drag should affect dragged image.";
}

TEST_P(DataTransferTest, NodeImageWithoutDraggedLayoutObject) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 84); SetBodyInnerHTML(R"HTML(
    <style>
      #sample { width: 100px; height: 100px; }
      #sample:-webkit-drag { display:none }
    </style>
    <div id=sample></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 91); Element* sample = GetDocument().getElementById(AtomicString("sample"));
  xtrace->LogLineRun(xtrace_mrid, 92); const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);
  xtrace->LogLineRun(xtrace_mrid, 94); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(nullptr, image.get()) << ":-webkit-drag blows away layout object";
}

TEST_P(DataTransferTest, NodeImageWithChangingLayoutObject) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 98); SetBodyInnerHTML(R"HTML(
    <style>
      #sample { color: blue; }
      #sample:-webkit-drag { display: inline-block; color: red; }
    </style>
    <span id=sample>foo</span>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 105); Element* sample = GetDocument().getElementById(AtomicString("sample"));
  xtrace->LogLineRun(xtrace_mrid, 106); UpdateAllLifecyclePhasesForTest();
  xtrace->LogLineRun(xtrace_mrid, 107); LayoutObject* before_layout_object = sample->GetLayoutObject();
  xtrace->LogLineRun(xtrace_mrid, 108); const std::unique_ptr<DragImage> image =
      DataTransfer::NodeImage(GetFrame(), *sample);

  xtrace->LogLineRun(xtrace_mrid, 111); EXPECT_TRUE(sample->GetLayoutObject() != before_layout_object)
      << ":-webkit-drag causes sample to have different layout object.";
  xtrace->LogLineRun(xtrace_mrid, 113); EXPECT_EQ(Color::FromRGB(255, 0, 0),
            sample->GetLayoutObject()->ResolveColor(GetCSSPropertyColor()))
      << "#sample has :-webkit-drag.";

  // Layout w/o :-webkit-drag
  xtrace->LogLineRun(xtrace_mrid, 118); UpdateAllLifecyclePhasesForTest();

  xtrace->LogLineRun(xtrace_mrid, 120); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(Color::FromRGB(0, 0, 255),
            sample->GetLayoutObject()->ResolveColor(GetCSSPropertyColor()))
      << "#sample doesn't have :-webkit-drag.";
}

TEST_P(DataTransferTest, NodeImageExceedsViewportBounds) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 126); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #node { width: 2000px; height: 2000px; }
    </style>
    <div id='node'></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 133); Element& node = *GetDocument().getElementById(AtomicString("node"));
  xtrace->LogLineRun(xtrace_mrid, 134); const auto image = DataTransfer::NodeImage(GetFrame(), node);
  xtrace->LogLineRun(xtrace_mrid, 135); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(800, 600), image->Size());
}

TEST_P(DataTransferTest, NodeImageUnderScrollOffset) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 139); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #first { width: 500px; height: 500px; }
      #second { width: 800px; height: 900px; }
    </style>
    <div id='first'></div>
    <div id='second'></div>
  )HTML");

  xtrace->LogLineRun(xtrace_mrid, 149); const int scroll_amount = 10;
  xtrace->LogLineRun(xtrace_mrid, 150); LocalFrameView* frame_view = GetDocument().View();
  xtrace->LogLineRun(xtrace_mrid, 151); frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);

  // The first div should be offset by the scroll offset.
  xtrace->LogLineRun(xtrace_mrid, 155); Element& first = *GetDocument().getElementById(AtomicString("first"));
  xtrace->LogLineRun(xtrace_mrid, 156); const auto first_image = DataTransfer::NodeImage(GetFrame(), first);
  xtrace->LogLineRun(xtrace_mrid, 157); const int first_height = 500;
  xtrace->LogLineRun(xtrace_mrid, 158); EXPECT_EQ(gfx::Size(500, first_height), first_image->Size());

  // The second div should also be offset by the scroll offset. In addition,
  // the second div should be clipped by the viewport.
  xtrace->LogLineRun(xtrace_mrid, 162); Element& second = *GetDocument().getElementById(AtomicString("second"));
  xtrace->LogLineRun(xtrace_mrid, 163); const auto second_image = DataTransfer::NodeImage(GetFrame(), second);
  xtrace->LogLineRun(xtrace_mrid, 164); const int viewport_height = 600;
  xtrace->LogLineRun(xtrace_mrid, 165); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(800, viewport_height - (first_height - scroll_amount)),
            second_image->Size());
}

TEST_P(DataTransferTest, NodeImageSizeWithPageScaleFactor) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 170); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      html, body { height: 2000px; }
      #node { width: 200px; height: 141px; }
    </style>
    <div id='node'></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 178); const int page_scale_factor = 2;
  xtrace->LogLineRun(xtrace_mrid, 179); GetPage().SetPageScaleFactor(page_scale_factor);
  xtrace->LogLineRun(xtrace_mrid, 180); Element& node = *GetDocument().getElementById(AtomicString("node"));
  xtrace->LogLineRun(xtrace_mrid, 181); const auto image = DataTransfer::NodeImage(GetFrame(), node);
  xtrace->LogLineRun(xtrace_mrid, 182); const int node_width = 200;
  xtrace->LogLineRun(xtrace_mrid, 183); const int node_height = 141;
  xtrace->LogLineRun(xtrace_mrid, 184); EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      node_height * page_scale_factor),
            image->Size());

  // Check that a scroll offset is scaled to device coordinates which includes
  // page scale factor.
  xtrace->LogLineRun(xtrace_mrid, 190); const int scroll_amount = 10;
  xtrace->LogLineRun(xtrace_mrid, 191); LocalFrameView* frame_view = GetDocument().View();
  xtrace->LogLineRun(xtrace_mrid, 192); frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);
  xtrace->LogLineRun(xtrace_mrid, 194); const auto image_with_offset = DataTransfer::NodeImage(GetFrame(), node);
  xtrace->LogLineRun(xtrace_mrid, 195); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      node_height * page_scale_factor),
            image_with_offset->Size());
}

TEST_P(DataTransferTest, NodeImageSizeWithPageScaleFactorTooLarge) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 201); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      html, body { height: 2000px; }
      #node { width: 800px; height: 601px; }
    </style>
    <div id='node'></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 209); const int page_scale_factor = 2;
  xtrace->LogLineRun(xtrace_mrid, 210); GetPage().SetPageScaleFactor(page_scale_factor);
  xtrace->LogLineRun(xtrace_mrid, 211); Element& node = *GetDocument().getElementById(AtomicString("node"));
  xtrace->LogLineRun(xtrace_mrid, 212); const auto image = DataTransfer::NodeImage(GetFrame(), node);
  xtrace->LogLineRun(xtrace_mrid, 213); const int node_width = 800;
  xtrace->LogLineRun(xtrace_mrid, 214); const int node_height = 601;
  xtrace->LogLineRun(xtrace_mrid, 215); EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      (node_height - 1) * page_scale_factor),
            image->Size());

  // Check that a scroll offset is scaled to device coordinates which includes
  // page scale factor.
  xtrace->LogLineRun(xtrace_mrid, 221); const int scroll_amount = 10;
  xtrace->LogLineRun(xtrace_mrid, 222); LocalFrameView* frame_view = GetDocument().View();
  xtrace->LogLineRun(xtrace_mrid, 223); frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);
  xtrace->LogLineRun(xtrace_mrid, 225); const auto image_with_offset = DataTransfer::NodeImage(GetFrame(), node);
  xtrace->LogLineRun(xtrace_mrid, 226); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(node_width * page_scale_factor,
                      (node_height - scroll_amount) * page_scale_factor),
            image_with_offset->Size());
}

TEST_P(DataTransferTest, NodeImageWithPageScaleFactor) {
  // #bluegreen is a 2x1 rectangle where the left pixel is blue and the right
  // pixel is green. The element is offset by a margin of 1px.
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 234); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #bluegreen {
        width: 1px;
        height: 1px;
        background: #0f0;
        border-left: 1px solid #00f;
        margin: 1px;
      }
    </style>
    <div id='bluegreen'></div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 247); const int page_scale_factor = 2;
  xtrace->LogLineRun(xtrace_mrid, 248); GetPage().SetPageScaleFactor(page_scale_factor);
  xtrace->LogLineRun(xtrace_mrid, 249); Element& blue_green =
      *GetDocument().getElementById(AtomicString("bluegreen"));
  xtrace->LogLineRun(xtrace_mrid, 251); const auto image = DataTransfer::NodeImage(GetFrame(), blue_green);
  xtrace->LogLineRun(xtrace_mrid, 252); const int blue_green_width = 2;
  xtrace->LogLineRun(xtrace_mrid, 253); const int blue_green_height = 1;
  xtrace->LogLineRun(xtrace_mrid, 254); EXPECT_EQ(gfx::Size(blue_green_width * page_scale_factor,
                      blue_green_height * page_scale_factor),
            image->Size());

  // Even though #bluegreen is offset by a margin of 1px (which is 2px in device
  // coordinates), we expect it to be painted at 0x0 and completely fill the 4x2
  // bitmap.
  xtrace->LogLineRun(xtrace_mrid, 261); SkBitmap expected_bitmap;
  xtrace->LogLineRun(xtrace_mrid, 262); expected_bitmap.allocN32Pixels(4, 2);
  xtrace->LogLineRun(xtrace_mrid, 263); expected_bitmap.eraseArea(SkIRect::MakeXYWH(0, 0, 2, 2), 0xFF0000FF);
  xtrace->LogLineRun(xtrace_mrid, 264); expected_bitmap.eraseArea(SkIRect::MakeXYWH(2, 0, 2, 2), 0xFF00FF00);
  xtrace->LogLineRun(xtrace_mrid, 265); const SkBitmap& bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 266); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < bitmap.width(); ++x)
    for (int y = 0; y < bitmap.height(); ++y)
      EXPECT_EQ(expected_bitmap.getColor(x, y), bitmap.getColor(x, y));
}

TEST_P(DataTransferTest, NodeImageFullyOffscreen) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 272); SetBodyInnerHTML(R"HTML(
    <style>
    #target {
      position: absolute;
      top: 800px;
      left: 0;
      height: 100px;
      width: 200px;
      background: lightblue;
      isolation: isolate;
    }
    </style>
    <div id="target" draggable="true" ondragstart="drag(event)"></div>
  )HTML");

  xtrace->LogLineRun(xtrace_mrid, 287); const int scroll_amount = 800;
  xtrace->LogLineRun(xtrace_mrid, 288); LocalFrameView* frame_view = GetDocument().View();
  xtrace->LogLineRun(xtrace_mrid, 289); frame_view->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, scroll_amount), mojom::blink::ScrollType::kProgrammatic);

  xtrace->LogLineRun(xtrace_mrid, 292); Element& target = *GetDocument().getElementById(AtomicString("target"));
  xtrace->LogLineRun(xtrace_mrid, 293); const auto image = DataTransfer::NodeImage(GetFrame(), target);

  xtrace->LogLineRun(xtrace_mrid, 295); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(200, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageWithScrolling) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 299); SetBodyInnerHTML(R"HTML(
    <style>
    #target {
      position: absolute;
      top: 800px;
      left: 0;
      height: 100px;
      width: 200px;
      background: lightblue;
      isolation: isolate;
    }
    </style>
    <div id="target" draggable="true" ondragstart="drag(event)"></div>
  )HTML");

  xtrace->LogLineRun(xtrace_mrid, 314); Element& target = *GetDocument().getElementById(AtomicString("target"));
  xtrace->LogLineRun(xtrace_mrid, 315); const auto image = DataTransfer::NodeImage(GetFrame(), target);

  xtrace->LogLineRun(xtrace_mrid, 317); xtrace->FlushAllEventsToJSONFile(); EXPECT_EQ(gfx::Size(200, 100), image->Size());
}

TEST_P(DataTransferTest, NodeImageInOffsetStackingContext) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 321); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #container {
        position: absolute;
        top: 4px;
        z-index: 10;
      }
      #drag {
        width: 5px;
        height: 5px;
        background: #0F0;
      }
    </style>
    <div id="container">
      <div id="drag" draggable="true"></div>
    </div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 339); Element& drag = *GetDocument().getElementById(AtomicString("drag"));
  xtrace->LogLineRun(xtrace_mrid, 340); const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  xtrace->LogLineRun(xtrace_mrid, 341); constexpr int drag_width = 5;
  xtrace->LogLineRun(xtrace_mrid, 342); constexpr int drag_height = 5;
  xtrace->LogLineRun(xtrace_mrid, 343); EXPECT_EQ(gfx::Size(drag_width, drag_height), image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  xtrace->LogLineRun(xtrace_mrid, 346); const SkBitmap& bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 347); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y)
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageWithLargerPositionedDescendant) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 354); SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #drag {
        position: absolute;
        top: 100px;
        left: 0;
        height: 1px;
        width: 1px;
        background: #00f;
      }
      #child {
        position: absolute;
        top: -1px;
        left: 0;
        height: 3px;
        width: 1px;
        background: #0f0;
      }
    </style>
    <div id="drag" draggable="true">
      <div id="child"></div>
    </div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 378); Element& drag = *GetDocument().getElementById(AtomicString("drag"));
  xtrace->LogLineRun(xtrace_mrid, 379); const auto image = DataTransfer::NodeImage(GetFrame(), drag);

  // The positioned #child should expand the dragged image's size.
  xtrace->LogLineRun(xtrace_mrid, 382); constexpr int drag_width = 1;
  xtrace->LogLineRun(xtrace_mrid, 383); constexpr int drag_height = 3;
  xtrace->LogLineRun(xtrace_mrid, 384); EXPECT_EQ(gfx::Size(drag_width, drag_height), image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green
  // which is the color of the #child which fully covers the dragged element.
  xtrace->LogLineRun(xtrace_mrid, 388); const SkBitmap& bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 389); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y)
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageOutOfView) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 396); SetBodyInnerHTML(R"HTML(
    <div id="drag" style="position: absolute; z-index: 1; top: -200px; left: 0;
                          width: 100px; height: 100px; background: green">
    </div>
  )HTML");

  xtrace->LogLineRun(xtrace_mrid, 402); auto image = DataTransfer::NodeImage(
      GetFrame(), *GetDocument().getElementById(AtomicString("drag")));
  xtrace->LogLineRun(xtrace_mrid, 404); EXPECT_EQ(gfx::Size(100, 100), image->Size());
  xtrace->LogLineRun(xtrace_mrid, 405); SkColor green = SkColorSetRGB(0, 0x80, 0);
  xtrace->LogLineRun(xtrace_mrid, 406); const SkBitmap& bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 407); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < 100; ++x) {
    for (int y = 0; y < 100; ++y)
      ASSERT_EQ(green, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, NodeImageFixedChild) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 414); SetBodyInnerHTML(R"HTML(
    <div id="drag" style="position: absolute; z-index: 1; top: 100px; left: 0;
                          width: 50px; height: 100px; background: green">
      <div style="position: fixed; top: 50px; width: 100px; height: 50px;
                  background: blue">
      </div>
    </div>
    <div style="height: 2000px"></div>
  )HTML");

  xtrace->LogLineRun(xtrace_mrid, 424); GetDocument().View()->LayoutViewport()->SetScrollOffset(
      ScrollOffset(0, 100), mojom::blink::ScrollType::kProgrammatic);

  xtrace->LogLineRun(xtrace_mrid, 427); auto image = DataTransfer::NodeImage(
      GetFrame(), *GetDocument().getElementById(AtomicString("drag")));
  xtrace->LogLineRun(xtrace_mrid, 429); EXPECT_EQ(gfx::Size(100, 100), image->Size());
  xtrace->LogLineRun(xtrace_mrid, 430); SkColor green = SkColorSetRGB(0, 0x80, 0);
  xtrace->LogLineRun(xtrace_mrid, 431); SkColor blue = SkColorSetRGB(0, 0, 0xFF);
  xtrace->LogLineRun(xtrace_mrid, 432); const SkBitmap& bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 433); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < 100; ++x) {
    for (int y = 0; y < 50; ++y) {
      ASSERT_EQ(x < 50 ? green : SK_ColorTRANSPARENT, bitmap.getColor(x, y));
    }
    for (int y = 50; y < 100; ++y)
      ASSERT_EQ(blue, bitmap.getColor(x, y));
  }
}

TEST_P(DataTransferTest, CreateDragImageWithEmptyImageResource) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 443); DataTransfer* data_transfer = DataTransfer::Create();
  xtrace->LogLineRun(xtrace_mrid, 444); data_transfer->SetDragImageResource(
      MakeGarbageCollected<ImageResourceContent>(nullptr), gfx::Point());

  xtrace->LogLineRun(xtrace_mrid, 447); gfx::Point drag_offset;
  xtrace->LogLineRun(xtrace_mrid, 448); xtrace->FlushAllEventsToJSONFile(); std::unique_ptr<DragImage> drag_image = data_transfer->CreateDragImage(
      drag_offset, /* device_scale_factor*/ 1, &GetFrame());
  // The test passes if the above call does not crash.
}

TEST_P(DataTransferTest, NodeImageWithScaleFactor) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 454); SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
        text-align: end; // 8 https://source.chromium.org/chromium/chromium/src/+/main:out/linux-Debug/gen/third_party/blink/renderer/core/style/computed_style_base_constants.h;drc=f9a11014f62ee4ffc170d8e8af48ac6e8125995b;l=564
      }
      #drag {
        transform: scale(2);
        transform-origin: left top;
        width: 5px;
        height: 5px;
        background: #0F0;
        text-align: center; // 2
      }
</style>
<div id="container">
  <div id="drag" draggable="true"></div>
</div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 474); Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  xtrace->LogLineRun(xtrace_mrid, 475); const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  xtrace->LogLineRun(xtrace_mrid, 476); constexpr int scaleFactor = 2;
  xtrace->LogLineRun(xtrace_mrid, 477); constexpr int drag_width = 5;
  xtrace->LogLineRun(xtrace_mrid, 478); constexpr int drag_height = 5;
  xtrace->LogLineRun(xtrace_mrid, 479); EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  xtrace->LogLineRun(xtrace_mrid, 483); const SkBitmap &bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 484); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y) {
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
    }
  }
}

TEST_P(DataTransferTest, NodeImageWithScaleFactorSelf) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 492); SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
      }
      #drag {
        width: 5px;
        height: 5px;
        transform: scale(2);
        transform-origin: left top;
        background: #0F0;
      }
</style>
<div id="container">
  <div id="drag" draggable="true"></div>
</div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 510); Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  xtrace->LogLineRun(xtrace_mrid, 511); const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  xtrace->LogLineRun(xtrace_mrid, 512); constexpr int scaleFactor = 2;
  xtrace->LogLineRun(xtrace_mrid, 513); constexpr int drag_width = 5;
  xtrace->LogLineRun(xtrace_mrid, 514); constexpr int drag_height = 5;
  xtrace->LogLineRun(xtrace_mrid, 515); EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  xtrace->LogLineRun(xtrace_mrid, 519); const SkBitmap &bitmap = image->Bitmap();
  xtrace->LogLineRun(xtrace_mrid, 520); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < drag_width; ++x) {
    for (int y = 0; y < drag_height; ++y) {
      EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(x, y));
    }
  }
}

TEST_P(DataTransferTest, NodeImageWithScaleFactorChild) {
  XTrace *xtrace = XTrace::getInstance(); std::string xtrace_mrid = xtrace->OnMethodEnter("main.cc", "TEST_P", "3c4e3b6b-2026-4b15-872c-07ce4463f59b" ); xtrace->LogLineRun(xtrace_mrid, 528); SetBodyInnerHTML(R"HTML(
<style>
      * { margin: 0; }
      #container {
        background: #F00;
      }
      #drag {
        width: 50px;
        height: 50px;


        background: #0F0;
      }
      #internal{
        transform: scale(2);
        transform-origin: left top;

        width: 10px;
        height: 10px;
        background: #00F;

      }
</style>
<div id="container">
  <div id="drag" draggable="true">
    <div id="internal"></div>
  </div>
</div>
  )HTML");
  xtrace->LogLineRun(xtrace_mrid, 557); Element &drag = *GetDocument().getElementById(AtomicString("drag"));
  xtrace->LogLineRun(xtrace_mrid, 558); const auto image = DataTransfer::NodeImage(GetFrame(), drag);
  xtrace->LogLineRun(xtrace_mrid, 559); constexpr int scaleFactor = 1;
  xtrace->LogLineRun(xtrace_mrid, 560); constexpr int drag_width = 50;
  xtrace->LogLineRun(xtrace_mrid, 561); constexpr int drag_height = 50;
  xtrace->LogLineRun(xtrace_mrid, 562); EXPECT_EQ(gfx::Size(drag_width * scaleFactor, drag_height * scaleFactor),
            image->Size());

  // The dragged image should be (drag_width x drag_height) and fully green.
  xtrace->LogLineRun(xtrace_mrid, 566); const SkBitmap &bitmap = image->Bitmap();

  xtrace->LogLineRun(xtrace_mrid, 568); WriteBitmapToFile(bitmap, "/tmp/blink.png");

  xtrace->LogLineRun(xtrace_mrid, 570); xtrace->FlushAllEventsToJSONFile(); for (int x = 0; x < 20; ++x) {
    for (int y = 0; y < 20; ++y) {
      EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(x, y));
    }
  }
}

}  // namespace blink

