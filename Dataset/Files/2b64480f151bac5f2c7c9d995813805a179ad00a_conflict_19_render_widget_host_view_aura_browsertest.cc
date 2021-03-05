// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_aura.h"

#include "base/bind.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/browser/devtools/protocol/devtools_protocol_test_support.h"
#include "content/browser/renderer_host/delegated_frame_host.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
<<<<<<< HEAD
#include "content/public/common/content_switches.h"
=======
#include "content/public/test/browser_test.h"
>>>>>>> eaac5b5035fe189b6706e1637122e37134206059
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "ui/events/event_utils.h"

#if defined(OS_WIN)
#include "content/browser/window_segments/window_segments_delegate_win.h"
#endif

namespace content {
namespace {

#if defined(OS_WIN)

const char kTestPageURL[] =
    "data:text/html,"
    "<style>"
    "  .target {"
    "    margin-top: env(fold-top, 0px);"
    "    margin-left: env(fold-left, 0px);"
    "    width: env(fold-width, 100px);"
    "    height: env(fold-height, 100px);"
    "  }"
    "</style>"
    "<body>"
    "<div class='target'></div>"
    "<script>"
    "  const e = document.getElementsByClassName('target')[0];"
    "  const style = window.getComputedStyle(e, null);"
    "</script></body>";
#endif
#if defined(OS_CHROMEOS)

const char kMinimalPageDataURL[] =
    "data:text/html,<html><head></head><body>Hello, world</body></html>";

// Run the current message loop for a short time without unwinding the current
// call stack.
void GiveItSomeTime() {
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromMilliseconds(250));
  run_loop.Run();
}
#endif  // defined(OS_CHROMEOS)

class FakeWebContentsDelegate : public WebContentsDelegate {
 public:
  FakeWebContentsDelegate() = default;
  ~FakeWebContentsDelegate() override = default;

  void SetShowStaleContentOnEviction(bool value) {
    show_stale_content_on_eviction_ = value;
  }

  bool ShouldShowStaleContentOnEviction(WebContents* source) override {
    return show_stale_content_on_eviction_;
  }

 private:
  bool show_stale_content_on_eviction_ = false;

  DISALLOW_COPY_AND_ASSIGN(FakeWebContentsDelegate);
};

}  // namespace

class RenderWidgetHostViewAuraBrowserTest : public ContentBrowserTest {
 public:
  RenderViewHost* GetRenderViewHost() const {
    RenderViewHost* const rvh = shell()->web_contents()->GetRenderViewHost();
    CHECK(rvh);
    return rvh;
  }

  RenderWidgetHostViewAura* GetRenderWidgetHostView() const {
    return static_cast<RenderWidgetHostViewAura*>(
        GetRenderViewHost()->GetWidget()->GetView());
  }

  DelegatedFrameHost* GetDelegatedFrameHost() const {
    return GetRenderWidgetHostView()->delegated_frame_host_.get();
  }

#if defined(OS_WIN)
  // GetContentRectsDoubleLandscape tries to mock GetContentRects(an api
  // provided by WIN OS to get the content rects of an app on dual screen
  // devices). This function will return two window segments that represent the
  // web content split by the zero width hinge, when an app is spanned across
  // two screens in landscape mode.
  BOOL GetContentRectsDoubleLandscape(HWND hwnd, UINT* count, RECT* pRect) {
    if (!pRect) {
      *count = 2;
      return true;
    }
    gfx::Rect root_rect =
        GetRenderWidgetHostView()->GetRootView()->GetBoundsInRootWindow();
    pRect[0] =
        gfx::Rect(0, 0, root_rect.width() / 2, root_rect.height()).ToRECT();
    pRect[1] = gfx::Rect(root_rect.width() / 2, 0, root_rect.width() / 2,
                         root_rect.height())
                   .ToRECT();
    return true;
  }

  // GetContentRectsDoublePortrait tries to mock GetContentRects(an api provided
  // by WIN OS to get the content rects of an app on dual screen devices). This
  // function will return two window segments that represent the web content
  // split by the zero height hinge, when an app is spanned across two screens
  // in portrait mode.
  BOOL GetContentRectsDoublePortrait(HWND hwnd, UINT* count, RECT* pRect) {
    if (!pRect) {
      *count = 2;
      return true;
    }
    gfx::Rect root_rect =
        GetRenderWidgetHostView()->GetRootView()->GetBoundsInRootWindow();
    pRect[0] =
        gfx::Rect(0, 0, root_rect.width(), root_rect.height() / 2).ToRECT();
    pRect[1] = gfx::Rect(0, root_rect.height() / 2, root_rect.width(),
                         root_rect.height() / 2)
                   .ToRECT();
    return true;
  }

  // GetContentRectsDoubleLandscapeWithNonZeroCut tries to mock
  // GetContentRects(an api provided by WIN OS to get the content rects of an
  // app on dual screen devices). This function will return two window segments
  // that represent the web content split by the non-zero width hinge, when an
  // app is spanned across two screens in landscape mode.
  BOOL GetContentRectsDoubleLandscapeWithNonZeroCut(HWND hwnd,
                                                    UINT* count,
                                                    RECT* pRect) {
    if (!pRect) {
      *count = 2;
      return true;
    }
    gfx::Rect root_rect =
        GetRenderWidgetHostView()->GetRootView()->GetBoundsInRootWindow();
    pRect[0] = gfx::Rect(0, 0, 390, root_rect.height()).ToRECT();
    pRect[1] = gfx::Rect(410, 0, 390, root_rect.height()).ToRECT();
    return true;
  }

  // GetContentRectsWithZeroSegments tries to mock  GetContentRects(an api
  // provided by WIN OS to get the content rects of an app on dual screen
  // devices) on windows OS version where it is not available. This function
  // will return zero segment.
  BOOL GetContentRectsWithZeroSegments(HWND hwnd, UINT* count, RECT* pRect) {
    if (!pRect) {
      *count = 0;
      return true;
    }

    return false;
  }
#endif
};

#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       StaleFrameContentOnEvictionNormal) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL(kMinimalPageDataURL)));

  // Make sure the renderer submits at least one frame before hiding it.
  RenderFrameSubmissionObserver submission_observer(shell()->web_contents());
  if (!submission_observer.render_frame_count())
    submission_observer.WaitForAnyFrameSubmission();

  FakeWebContentsDelegate delegate;
  delegate.SetShowStaleContentOnEviction(true);
  shell()->web_contents()->SetDelegate(&delegate);

  // Initially there should be no stale content set.
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Hide the view and evict the frame. This should trigger a copy of the stale
  // frame content.
  GetRenderWidgetHostView()->Hide();
  static_cast<viz::FrameEvictorClient*>(GetDelegatedFrameHost())
      ->EvictDelegatedFrame();
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kPendingEvictionRequests);

  // Wait until the stale frame content is copied and set onto the layer.
  while (!GetDelegatedFrameHost()->stale_content_layer_->has_external_content())
    GiveItSomeTime();

  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Unhidding the view should reset the stale content layer to show the new
  // frame content.
  GetRenderWidgetHostView()->Show();
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
}

IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       StaleFrameContentOnEvictionRejected) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL(kMinimalPageDataURL)));

  // Wait for first frame activation when a surface is embedded.
  while (!GetDelegatedFrameHost()->HasSavedFrame())
    GiveItSomeTime();

  FakeWebContentsDelegate delegate;
  delegate.SetShowStaleContentOnEviction(true);
  shell()->web_contents()->SetDelegate(&delegate);

  // Initially there should be no stale content set.
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Hide the view and evict the frame. This should trigger a copy of the stale
  // frame content.
  GetRenderWidgetHostView()->Hide();
  static_cast<viz::FrameEvictorClient*>(GetDelegatedFrameHost())
      ->EvictDelegatedFrame();
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kPendingEvictionRequests);

  GetRenderWidgetHostView()->Show();
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Wait until the stale frame content is copied and the result callback is
  // complete.
  GiveItSomeTime();

  // This should however not set the stale content as the view is visible and
  // new frames are being submitted.
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
}

IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       StaleFrameContentOnEvictionNone) {
  EXPECT_TRUE(NavigateToURL(shell(), GURL(kMinimalPageDataURL)));

  // Wait for first frame activation when a surface is embedded.
  while (!GetDelegatedFrameHost()->HasSavedFrame())
    GiveItSomeTime();

  FakeWebContentsDelegate delegate;
  delegate.SetShowStaleContentOnEviction(false);
  shell()->web_contents()->SetDelegate(&delegate);

  // Initially there should be no stale content set.
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Hide the view and evict the frame. This should not trigger a copy of the
  // stale frame content as the WebContentDelegate returns false.
  GetRenderWidgetHostView()->Hide();
  static_cast<viz::FrameEvictorClient*>(GetDelegatedFrameHost())
      ->EvictDelegatedFrame();

  EXPECT_EQ(GetDelegatedFrameHost()->frame_eviction_state_,
            DelegatedFrameHost::FrameEvictionState::kNotStarted);

  // Wait for a while to ensure any copy requests that were sent out are not
  // completed. There shouldnt be any requests sent however.
  GiveItSomeTime();
  EXPECT_FALSE(
      GetDelegatedFrameHost()->stale_content_layer_->has_external_content());
}
#endif  // #if defined(OS_CHROMEOS)

#if defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest, GetWindowSegments) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);

  EXPECT_TRUE(NavigateToURL(shell(), GURL(kTestPageURL)));
  auto* wc = shell()->web_contents();
  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoubleLandscape,
      base::Unretained(this)));

  // |GetContentRectsDoubleLandscape| function will result in change in
  // ScreenInfo/VisualProperties. Thus |SynchronizeVisualProperties| call will
  // make sure that renderer is aware of such change.
  host_view_aura->host()->SynchronizeVisualProperties();
  gfx::Rect root_view_rect = host_view_aura->GetRootView()->GetViewBounds();
  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  int segment1_width =
      EvalJs(shell(), "window.getWindowSegments()[0].width").ExtractInt();
  int segment2_width =
      EvalJs(shell(), "window.getWindowSegments()[1].width").ExtractInt();

  EXPECT_EQ(root_view_rect.width(), segment1_width + segment2_width);
  EXPECT_EQ(root_view_rect.height(),
            EvalJs(shell(), "window.getWindowSegments()[0].height"));
  EXPECT_EQ(root_view_rect.height(),
            EvalJs(shell(), "window.getWindowSegments()[1].height"));

  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[0].x"));
  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[0].y"));
  EXPECT_EQ(segment1_width, EvalJs(shell(), "window.getWindowSegments()[1].x"));
  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[1].y"));

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoublePortrait,
      base::Unretained(this)));

  // |GetContentRectsDoublePortrait| function will result in change in
  // ScreenInfo/VisualProperties. Thus |SynchronizeVisualProperties| call will
  // make sure that renderer is aware of such change.
  host_view_aura->host()->SynchronizeVisualProperties();
  root_view_rect = host_view_aura->GetRootView()->GetViewBounds();
  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  int segment1_height =
      EvalJs(shell(), "window.getWindowSegments()[0].height").ExtractInt();
  int segment2_height =
      EvalJs(shell(), "window.getWindowSegments()[1].height").ExtractInt();

  EXPECT_EQ(root_view_rect.height(), segment1_height + segment2_height);
  EXPECT_EQ(root_view_rect.width(),
            EvalJs(shell(), "window.getWindowSegments()[0].width"));
  EXPECT_EQ(root_view_rect.width(),
            EvalJs(shell(), "window.getWindowSegments()[1].width"));

  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[0].x"));
  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[0].y"));
  EXPECT_EQ(0, EvalJs(shell(), "window.getWindowSegments()[1].x"));
  EXPECT_EQ(segment1_height,
            EvalJs(shell(), "window.getWindowSegments()[1].y"));
}

IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       GetFoldCssEnviromentVariables) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);

  EXPECT_TRUE(NavigateToURL(shell(), GURL(kTestPageURL)));
  auto* wc = shell()->web_contents();

  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("100px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "100px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());

  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoubleLandscape,
      base::Unretained(this)));

  // |GetContentRectsDoubleLandscape| function will result in change in
  // ScreenInfo/VisualProperties. Thus |SynchronizeVisualProperties| call will
  // make sure that renderer is aware of such change.
  host_view_aura->host()->SynchronizeVisualProperties();
  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  EXPECT_EQ(
      "400px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("0px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "600px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());

  host_view_aura->SetGetContentRectFunctionForTesting(
      base::BindRepeating(&RenderWidgetHostViewAuraBrowserTest::
                              GetContentRectsDoubleLandscapeWithNonZeroCut,
                          base::Unretained(this)));

  // |GetContentRectsDoubleLandscapeWithNonZeroCut| function will result in
  // change in ScreenInfo/VisualProperties. Thus |SynchronizeVisualProperties|
  // call will make sure that renderer is aware of such change.
  host_view_aura->host()->SynchronizeVisualProperties();
  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  int expected_margin_left =
      EvalJs(shell(), "window.getWindowSegments()[0].width").ExtractInt();
  EXPECT_EQ(
      base::NumberToString(expected_margin_left) + "px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("20px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "600px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsWithZeroSegments,
      base::Unretained(this)));

  // |GetContentRectsWithZeroSegments| function will result in change in
  // ScreenInfo/VisualProperties. Thus |SynchronizeVisualProperties| call will
  // make sure that renderer is aware of such change.
  host_view_aura->host()->SynchronizeVisualProperties();
  EXPECT_EQ(1,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  EXPECT_EQ(
      "0px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("100px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "100px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());
}

IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       VerifyMediaSpanningFeature) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);

  const char kTestPageForMediaFeatureURL[] =
      "data:text/html,"
      "<style>"
      "  .target {"
      "    margin-top: env(fold-top, 20px);"
      "    margin-left: env(fold-left, 20px);"
      "    width: env(fold-width, 100px);"
      "    height: env(fold-height, 100px);"
      "  }"
      " @media(spanning: single-fold-vertical) {"
      " .target { "
      "           background: yellow;"
      "      }"
      "   }"
      " @media(spanning: single-fold-horizontal) {"
      " .target { "
      "           background:blue;"
      "      }"
      "   }"
      " @media(spanning: none) {"
      " .target { "
      "           background:red;"
      "      }"
      "   }"
      "</style>"
      "<body>"
      "<div class='target'></div>"
      "<script>"
      "  const e = document.getElementsByClassName('target')[0];"
      "  const style = window.getComputedStyle(e, null);"
      "</script></body>";

  EXPECT_TRUE(NavigateToURL(shell(), GURL(kTestPageForMediaFeatureURL)));
  auto* wc = shell()->web_contents();
  MainThreadFrameObserver frame_observer(GetRenderViewHost()->GetWidget());

  EXPECT_EQ(
      "20px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  EXPECT_EQ(
      "20px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("100px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "100px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());

  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoubleLandscape,
      base::Unretained(this)));

  // Resize the host window to trigger media query evaluation.
  host_view_aura->host()->GetView()->SetSize(gfx::Size(600, 600));
  frame_observer.Wait();

  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ("rgb(255, 255, 0)",
            EvalJs(shell(), "style.getPropertyValue('background-color')")
                .ExtractString());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoublePortrait,
      base::Unretained(this)));

  // Resize the host window to trigger media query evaluation.
  host_view_aura->host()->GetView()->SetSize(gfx::Size(800, 600));
  frame_observer.Wait();

  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ("rgb(0, 0, 255)",
            EvalJs(shell(), "style.getPropertyValue('background-color')")
                .ExtractString());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsWithZeroSegments,
      base::Unretained(this)));

  // Resize the host window to trigger media query evaluation.
  host_view_aura->host()->GetView()->SetSize(gfx::Size(600, 600));
  frame_observer.Wait();

  EXPECT_EQ(1,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ("rgb(255, 0, 0)",
            EvalJs(shell(), "style.getPropertyValue('background-color')")
                .ExtractString());
}

// This test verifies that @media(spanning) return true by correctly identifying
// either single-fold-vertical or single-fold-horizontal.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       VerifyMediaSpanningFeatureWithoutArgument) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kEnableExperimentalWebPlatformFeatures);

  const char kTestPageForMediaFeatureURL[] =
      "data:text/html,"
      "<style>"
      "  .target {"
      "    margin-top: env(fold-top, 20px);"
      "    margin-left: env(fold-left, 20px);"
      "    width: env(fold-width, 100px);"
      "    height: env(fold-height, 100px);"
      "  }"
      " @media(spanning) {"
      " .target { "
      "           background: yellow;"
      "      }"
      "   }"
      "</style>"
      "<body>"
      "<div class='target'></div>"
      "<script>"
      "  const e = document.getElementsByClassName('target')[0];"
      "  const style = window.getComputedStyle(e, null);"
      "</script></body>";

  EXPECT_TRUE(NavigateToURL(shell(), GURL(kTestPageForMediaFeatureURL)));
  auto* wc = shell()->web_contents();

  EXPECT_EQ(
      "20px",
      EvalJs(shell(), "style.getPropertyValue('margin-top')").ExtractString());
  EXPECT_EQ(
      "20px",
      EvalJs(shell(), "style.getPropertyValue('margin-left')").ExtractString());
  EXPECT_EQ("100px",
            EvalJs(shell(), "style.getPropertyValue('width')").ExtractString());
  EXPECT_EQ(
      "100px",
      EvalJs(shell(), "style.getPropertyValue('height')").ExtractString());

  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());

  MainThreadFrameObserver frame_observer(GetRenderViewHost()->GetWidget());

  host_view_aura->SetGetContentRectFunctionForTesting(base::BindRepeating(
      &RenderWidgetHostViewAuraBrowserTest::GetContentRectsDoubleLandscape,
      base::Unretained(this)));

  // Resize the host window to trigger media query evaluation.
  host_view_aura->host()->GetView()->SetSize(gfx::Size(600, 600));
  frame_observer.Wait();

  EXPECT_EQ(2,
            EvalJs(shell(), "window.getWindowSegments().length").ExtractInt());
  EXPECT_EQ("rgb(255, 255, 0)",
            EvalJs(shell(), "style.getPropertyValue('background-color')")
                .ExtractString());
}

// Test the pen button events flow.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraBrowserTest,
                       AllPenButtonEvents) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                  "PenButtonEvents");

  const char kTestPageForPenButtonEventsURL[] =
      "data:text/html,<!DOCTYPE html>"
      "<body>"
      "<script>"
      "  let click = false, double_click = false, press_and_hold = false;"
      "  window.addEventListener('penbuttonclick', evt => {"
      "  click = true;"
      "  });"
      "  window.addEventListener('penbuttondoubleclick', evt => {"
      "  double_click = true;"
      "  });"
      "  window.addEventListener('penbuttonpressandhold', evt => {"
      "  press_and_hold = true;"
      "  });"
      "</script>"
      "</body>";

  EXPECT_TRUE(NavigateToURL(shell(), GURL(kTestPageForPenButtonEventsURL)));
  auto* wc = shell()->web_contents();
  MainThreadFrameObserver frame_observer(GetRenderViewHost()->GetWidget());
  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());

  EXPECT_EQ(false, EvalJs(shell(), "click").ExtractBool());
  EXPECT_EQ(false, EvalJs(shell(), "double_click").ExtractBool());
  EXPECT_EQ(false, EvalJs(shell(), "press_and_hold").ExtractBool());

  ui::PenButtonEvent pen_button_click(ui::ET_PEN_BUTTON_CLICK,
                                      base::TimeTicks::Now(), 0);
  host_view_aura->OnPenButtonEvent(&pen_button_click);
  frame_observer.Wait();
  EXPECT_EQ(true, EvalJs(shell(), "click").ExtractBool());
  EXPECT_EQ(false, EvalJs(shell(), "double_click").ExtractBool());
  EXPECT_EQ(false, EvalJs(shell(), "press_and_hold").ExtractBool());

  ui::PenButtonEvent pen_button_double_click(ui::ET_PEN_BUTTON_DOUBLE_CLICK,
                                             base::TimeTicks::Now(), 0);
  host_view_aura->OnPenButtonEvent(&pen_button_double_click);
  frame_observer.Wait();
  EXPECT_EQ(true, EvalJs(shell(), "double_click").ExtractBool());
  EXPECT_EQ(false, EvalJs(shell(), "press_and_hold").ExtractBool());

  ui::PenButtonEvent pen_button_press_and_hold(ui::ET_PEN_BUTTON_PRESS_AND_HOLD,
                                               base::TimeTicks::Now(), 0);
  host_view_aura->OnPenButtonEvent(&pen_button_press_and_hold);
  frame_observer.Wait();
  EXPECT_EQ(true, EvalJs(shell(), "press_and_hold").ExtractBool());
}
#endif

class RenderWidgetHostViewAuraDevtoolsBrowserTest
    : public content::DevToolsProtocolTest {
 public:
  RenderWidgetHostViewAuraDevtoolsBrowserTest() = default;

 protected:
  aura::Window* window() {
    return reinterpret_cast<content::RenderWidgetHostViewAura*>(
               shell()->web_contents()->GetRenderWidgetHostView())
        ->window();
  }

  bool HasChildPopup() const {
    return reinterpret_cast<content::RenderWidgetHostViewAura*>(
               shell()->web_contents()->GetRenderWidgetHostView())
        ->popup_child_host_view_;
  }
};

// This test opens a select popup which inside breaks the debugger
// which enters a nested event loop.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewAuraDevtoolsBrowserTest,
                       NoCrashOnSelect) {
  GURL page(
      "data:text/html;charset=utf-8,"
      "<!DOCTYPE html>"
      "<html>"
      "<body>"
      "<select id=\"ddlChoose\">"
      " <option value=\"\">Choose</option>"
      " <option value=\"A\">A</option>"
      " <option value=\"B\">B</option>"
      " <option value=\"C\">C</option>"
      "</select>"
      "<script type=\"text/javascript\">"
      "  document.getElementById('ddlChoose').addEventListener('change', "
      "    function () {"
      "      debugger;"
      "    });"
      "  function focusSelectMenu() {"
      "    document.getElementById('ddlChoose').focus();"
      "  }"
      "  function noop() {"
      "  }"
      "</script>"
      "</body>"
      "</html>");

  EXPECT_TRUE(NavigateToURL(shell(), page));
  auto* wc = shell()->web_contents();
  Attach();
  SendCommand("Debugger.enable", nullptr);

  ASSERT_TRUE(ExecuteScript(wc, "focusSelectMenu();"));
  SimulateKeyPress(wc, ui::DomKey::FromCharacter(' '), ui::DomCode::SPACE,
                   ui::VKEY_SPACE, false, false, false, false);

  // Wait until popup is opened.
  while (!HasChildPopup()) {
    base::RunLoop().RunUntilIdle();
    base::PlatformThread::Sleep(TestTimeouts::tiny_timeout());
  }

  // Send down and enter to select next item and cause change listener to fire.
  // The event listener causes devtools to break (and enter a nested event
  // loop).
  ui::KeyEvent press_down(ui::ET_KEY_PRESSED, ui::VKEY_DOWN,
                          ui::DomCode::ARROW_DOWN, ui::EF_NONE,
                          ui::DomKey::ARROW_DOWN, ui::EventTimeForNow());
  ui::KeyEvent release_down(ui::ET_KEY_RELEASED, ui::VKEY_DOWN,
                            ui::DomCode::ARROW_DOWN, ui::EF_NONE,
                            ui::DomKey::ARROW_DOWN, ui::EventTimeForNow());
  ui::KeyEvent press_enter(ui::ET_KEY_PRESSED, ui::VKEY_RETURN,
                           ui::DomCode::ENTER, ui::EF_NONE, ui::DomKey::ENTER,
                           ui::EventTimeForNow());
  ui::KeyEvent release_enter(ui::ET_KEY_RELEASED, ui::VKEY_RETURN,
                             ui::DomCode::ENTER, ui::EF_NONE, ui::DomKey::ENTER,
                             ui::EventTimeForNow());
  auto* host_view_aura = static_cast<content::RenderWidgetHostViewAura*>(
      wc->GetRenderWidgetHostView());
  host_view_aura->OnKeyEvent(&press_down);
  host_view_aura->OnKeyEvent(&release_down);
  host_view_aura->OnKeyEvent(&press_enter);
  host_view_aura->OnKeyEvent(&release_enter);
  WaitForNotification("Debugger.paused");

  // Close the widget window while inside the nested event loop.
  // This will cause the RenderWidget to be destroyed while we are inside a
  // method and used to cause UAF when the nested event loop unwinds.
  window()->Hide();

  // Disconnect devtools causes script to resume. This causes the unwind of the
  // nested event loop. The RenderWidget that was entered has been destroyed,
  // make sure that we detect this and don't touch any members in the class.
  Detach();

  // Try to access the renderer process, it would have died if
  // crbug.com/1032984 wasn't fixed.
  ASSERT_TRUE(ExecuteScript(wc, "noop();"));
}

}  // namespace content
