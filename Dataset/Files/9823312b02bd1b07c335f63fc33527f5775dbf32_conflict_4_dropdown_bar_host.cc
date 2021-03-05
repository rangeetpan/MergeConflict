// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/dropdown_bar_host.h"

#include <algorithm>

#include "build/build_config.h"
<<<<<<< HEAD
#include "chrome/browser/edge_ui_features.h"
=======
>>>>>>> 8daefb17828c95f6cc0a77ead7bfcaa9359a9569
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/dropdown_bar_host_delegate.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "content/public/common/edge_content_features.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/scrollbar_size.h"
#include "ui/views/focus/external_focus_tracker.h"
#include "ui/views/widget/widget.h"

#if defined(OS_WIN)
#include "chrome/browser/ui/views/dropdown_bar_host_frame_view_win.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#endif

// static
bool DropdownBarHost::disable_animations_during_testing_ = false;

////////////////////////////////////////////////////////////////////////////////
// DropdownBarHost, public:

DropdownBarHost::DropdownBarHost(BrowserView* browser_view)
    : AnimationDelegateViews(browser_view), browser_view_(browser_view) {}

DropdownBarHost::~DropdownBarHost() {
  focus_manager_->RemoveFocusChangeListener(this);
  ResetFocusTracker();
}

void DropdownBarHost::Init(views::View* host_view,
                           std::unique_ptr<views::View> view,
                           DropdownBarHostDelegate* delegate) {
  DCHECK(view);
  DCHECK(delegate);

  delegate_ = delegate;

<<<<<<< HEAD
  if (features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    // The |clip_view| exists to paint to a layer so that it can clip descendent
    // Views which also paint to a Layer. See http://crbug.com/589497
    auto clip_view = std::make_unique<views::View>();
    clip_view->SetPaintToLayer();
    clip_view->layer()->SetFillsBoundsOpaquely(false);
    clip_view->layer()->SetMasksToBounds(true);
    view_ = clip_view->AddChildView(std::move(view));

    // Initialize the host.
    host_ = std::make_unique<ThemeCopyingWidget>(browser_view_->GetWidget());

    views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
    params.delegate = this;
    params.name = "DropdownBarHost";
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.parent = browser_view_->GetWidget()->GetNativeView();
    params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;

    host_->Init(std::move(params));
    host_->SetContentsView(clip_view.release());
  } else {
    // For the windowed find bar, the |view| pointer will become owned by the
    // windowed widget once the view is attached as the widget's contents.
    view_ = view.release();
#if defined(OS_WIN)
    InitDesktopNativeViewAura();
#else
    NOTREACHED() << "Unsupported configuration";
#endif
  }
=======
  // The |clip_view| exists to paint to a layer so that it can clip descendent
  // Views which also paint to a Layer. See http://crbug.com/589497
  auto clip_view = std::make_unique<views::View>();
  clip_view->SetPaintToLayer();
  clip_view->layer()->SetFillsBoundsOpaquely(false);
  clip_view->layer()->SetMasksToBounds(true);
  view_ = clip_view->AddChildView(std::move(view));

  // Initialize the host.
  host_ = std::make_unique<ThemeCopyingWidget>(browser_view_->GetWidget());
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_CONTROL);
  params.delegate = this;
  params.name = "DropdownBarHost";
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = browser_view_->GetWidget()->GetNativeView();
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
#if defined(OS_MACOSX)
  params.activatable = views::Widget::InitParams::ACTIVATABLE_YES;
#endif
  host_->Init(std::move(params));
  host_->SetContentsView(clip_view.release());
>>>>>>> 8daefb17828c95f6cc0a77ead7bfcaa9359a9569

  SetHostViewNative(host_view);

  // Start listening to focus changes, so we can register and unregister our
  // own handler for Escape.
  focus_manager_ = host_->GetFocusManager();
  if (focus_manager_) {
    focus_manager_->AddFocusChangeListener(this);
  } else {
    // In some cases (see bug http://crbug.com/17056) it seems we may not have
    // a focus manager.  Please reopen the bug if you hit this.
    NOTREACHED();
  }

  animation_ = std::make_unique<gfx::SlideAnimation>(this);
  if (!gfx::Animation::ShouldRenderRichAnimation())
    animation_->SetSlideDuration(base::TimeDelta());

  // Update the widget and |view_| bounds to the hidden state.
  AnimationProgressed(animation_.get());
}

bool DropdownBarHost::IsAnimating() const {
  return animation_->is_animating();
}

bool DropdownBarHost::IsVisible() const {
  return is_visible_;
}

void DropdownBarHost::SetFocus() {
  delegate_->Focus();
}

void DropdownBarHost::SetFocusAndSelection() {
  delegate_->FocusAndSelectAll();
}

void DropdownBarHost::StopAnimation() {
  animation_->End();
}

void DropdownBarHost::Show(bool animate) {
  // Stores the currently focused view, and tracks focus changes so that we can
  // restore focus when the dropdown widget is closed.
  focus_tracker_ =
      std::make_unique<views::ExternalFocusTracker>(view_, focus_manager_);

  SetDialogPosition(GetDialogPosition(gfx::Rect()));

  // If we're in the middle of a close animation, stop it and skip to the end.
  // This ensures that the state is consistent and prepared to show the drop-
  // down bar.
  if (animation_->IsClosing())
    StopAnimation();

  host_->Show();

  bool was_visible = is_visible_;
  is_visible_ = true;
  if (!animate || disable_animations_during_testing_) {
    animation_->Reset(1);
    AnimationProgressed(animation_.get());
  } else if (!was_visible) {
    // Don't re-start the animation.
    animation_->Reset();
    animation_->Show();
  }

#if defined(OS_WIN)
  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    // Enable activation when visible so mouse clicks activate the window.
    HWND hwnd = GetWidget()->GetNativeWindow()->GetHost()->
        GetAcceleratedWidget();
    SetWindowLong(hwnd, GWL_EXSTYLE,
      GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_NOACTIVATE);
  }
#endif

  if (!was_visible)
    OnVisibilityChanged();
}

void DropdownBarHost::Hide(bool animate) {
  if (!IsVisible())
    return;
  if (animate && !disable_animations_during_testing_ &&
      !animation_->IsClosing()) {
    animation_->Hide();
  } else {
    if (animation_->IsClosing()) {
      // If we're in the middle of a close animation, skip immediately to the
      // end of the animation.
      StopAnimation();
    } else {
      // Otherwise we need to set both the animation state to ended and the
      // DropdownBarHost state to ended/hidden, otherwise the next time we try
      // to show the bar, it might refuse to do so. Note that we call
      // AnimationEnded ourselves as Reset does not call it if we are not
      // animating here.
      animation_->Reset();
      AnimationEnded(animation_.get());
    }
  }

#if defined(OS_WIN)
  // If we're using a window, activate the main browser so its focus manager
  // can restore focus properly.
  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    // Prevent default activation when the find bar is shown. The find bar gets
    // activated manually after deciding it should have focus.
    SetNoActivate();

    ActivateBrowserView();
  }
#endif
}

#if defined(OS_WIN)
void DropdownBarHost::SetNoActivate() {
  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    HWND hwnd = GetWidget()->GetNativeWindow()->GetHost()->
        GetAcceleratedWidget();

    if (GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP) {
      SetWindowLong(hwnd, GWL_EXSTYLE,
                    GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
    } else {
      NOTREACHED()
          << "WS_EX_NOACTIVATE attempted to be set on a non-popup window.";
    }
  }
}
#endif

void DropdownBarHost::SetDialogPosition(const gfx::Rect& new_pos) {
  view_->SetSize(new_pos.size());

  if (new_pos.IsEmpty())
    return;

  gfx::Rect new_screen_pos(new_pos);

  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    // Transform from local browser to screen coordinates.
    views::View::ConvertRectToScreen(browser_view_->GetWidget()->GetRootView(),
                                     &new_screen_pos);
  }

  host()->SetBounds(new_screen_pos);
}

void DropdownBarHost::OnWillChangeFocus(views::View* focused_before,
                                        views::View* focused_now) {
  // First we need to determine if one or both of the views passed in are child
  // views of our view.
  bool our_view_before = focused_before && view_->Contains(focused_before);
  bool our_view_now = focused_now && view_->Contains(focused_now);

  // When both our_view_before and our_view_now are false, it means focus is
  // changing hands elsewhere in the application (and we shouldn't do anything).
  // Similarly, when both are true, focus is changing hands within the dropdown
  // widget (and again, we should not do anything). We therefore only need to
  // look at when we gain initial focus and when we loose it.
  if (!our_view_before && our_view_now) {
    // We are gaining focus from outside the dropdown widget so we must register
    // a handler for Escape.
    RegisterAccelerators();
  } else if (our_view_before && !our_view_now) {
    // We are losing focus to something outside our widget so we restore the
    // original handler for Escape.
    UnregisterAccelerators();
  }
}

void DropdownBarHost::OnDidChangeFocus(views::View* focused_before,
                                       views::View* focused_now) {
}

void DropdownBarHost::AnimationProgressed(const gfx::Animation* animation) {
  // First, we calculate how many pixels to slide the widget.
  gfx::Size pref_size = view_->GetPreferredSize();
  int view_offset = static_cast<int>((animation_->GetCurrentValue() - 1.0) *
                                     pref_size.height());

  // This call makes sure |view_| appears in the right location, the size and
  // shape is correct and that it slides in the right direction.
  view_->SetPosition(gfx::Point(0, view_offset));
}

void DropdownBarHost::AnimationEnded(const gfx::Animation* animation) {
  if (!animation_->IsShowing()) {
    // Animation has finished closing.
    host_->Hide();
    is_visible_ = false;
    OnVisibilityChanged();
  } else {
    // Animation has finished opening.
  }
}

void DropdownBarHost::RegisterAccelerators() {
  DCHECK(!esc_accel_target_registered_);
  ui::Accelerator escape(ui::VKEY_ESCAPE, ui::EF_NONE);
  focus_manager_->RegisterAccelerator(
      escape, ui::AcceleratorManager::kNormalPriority, this);

  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    ui::Accelerator f6(ui::VKEY_F6, ui::EF_NONE);
    focus_manager_->RegisterAccelerator(
        f6, ui::AcceleratorManager::kNormalPriority, this);
  }

  esc_accel_target_registered_ = true;
}

void DropdownBarHost::UnregisterAccelerators() {
  DCHECK(esc_accel_target_registered_);
  ui::Accelerator escape(ui::VKEY_ESCAPE, ui::EF_NONE);
  focus_manager_->UnregisterAccelerator(escape, this);

  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    ui::Accelerator f6(ui::VKEY_F6, ui::EF_NONE);
    focus_manager_->UnregisterAccelerator(f6, this);
  }

  esc_accel_target_registered_ = false;
}

#if defined(OS_WIN)
void DropdownBarHost::InitDesktopNativeViewAura() {
  // Initialize the host.
  host_.reset(new ThemeCopyingWidget(browser_view_->GetWidget()));

  views::Widget::InitParams params(views::Widget::InitParams::TYPE_BUBBLE);
  params.delegate = this;
  params.name = "DropdownBarHost";
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.parent = browser_view_->GetWidget()->GetNativeView();
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.remove_standard_frame = true;

  // To ensure ChromeViewsDelegate::CreateNativeWidget doesn't force the widget
  // back to being native Aura (windowless) on Windows 7 with the DWM disabled,
  // turn off software compositing. See also, comments relating to
  // http://crbug.com/125248.
  //
  // Nothing within the find bar is expected to take advantage of Aero
  // composition, so this isn't expect to have a perf impact.
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.force_software_compositing = true;

  host_->Init(std::move(params));

  // We do our own animation and don't want any from the system.
  host_->SetVisibilityChangedAnimationsEnabled(false);
}

bool DropdownBarHost::CanActivate() const {
  if (features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    return false;
  }
  return true;
}

void DropdownBarHost::ActivateBrowserView() {
  browser_view_->GetWidget()->Activate();
}

views::NonClientFrameView* DropdownBarHost::CreateNonClientFrameView(
    views::Widget* widget) {
  return new DropdownBarHostFrameView(this);
}

views::View* DropdownBarHost::GetContentsView(){
  if (!features::edge::UseAuraForFindBar(browser_view_->GetProfile())) {
    return view_;
  }
  return WidgetDelegate::GetContentsView();
}
#endif

void DropdownBarHost::OnVisibilityChanged() {}

void DropdownBarHost::ResetFocusTracker() {
  focus_tracker_.reset();
}

void DropdownBarHost::GetWidgetBounds(gfx::Rect* bounds) {
  DCHECK(bounds);
  *bounds = browser_view_->bounds();
}

views::Widget* DropdownBarHost::GetWidget() {
  return host_.get();
}

const views::Widget* DropdownBarHost::GetWidget() const {
  return host_.get();
}
