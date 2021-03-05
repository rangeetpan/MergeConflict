// Copyright 2016 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/controls/focus_ring.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
<<<<<<< HEAD
#include "ui/base/edge_ui_base_features.h"
=======
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
>>>>>>> 32ea0b0591779cbd45618dc7e02bb9bd635354e9
#include "ui/gfx/canvas.h"
#include "ui/views/controls/focusable_border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/view_class_properties.h"

namespace views {

namespace {

bool IsPathUsable(const SkPath& path) {
  return !path.isEmpty() && (path.isRect(nullptr) || path.isOval(nullptr) ||
                             path.isRRect(nullptr));
}

ui::NativeTheme::ColorId ColorIdForValidity(bool valid) {
  return valid ? ui::NativeTheme::kColorId_FocusedBorderColor
               : ui::NativeTheme::kColorId_AlertSeverityHigh;
}

double GetCornerRadius() {
  return FocusableBorder::kCornerRadiusDp;
}

SkPath GetHighlightPathInternal(const View* view) {
  HighlightPathGenerator* path_generator =
      view->GetProperty(kHighlightPathGeneratorKey);

  if (path_generator) {
    SkPath highlight_path = path_generator->GetHighlightPath(view);
    // The generated path might be empty or otherwise unusable. If that's the
    // case we should fall back on the default path.
    if (IsPathUsable(highlight_path))
      return highlight_path;
  }

  const double corner_radius = GetCornerRadius();
  return SkPath().addRRect(SkRRect::MakeRectXY(
      RectToSkRect(view->GetLocalBounds()), corner_radius, corner_radius));
}

}  // namespace

// static
std::unique_ptr<FocusRing> FocusRing::Install(View* parent) {
  auto ring = base::WrapUnique<FocusRing>(new FocusRing());
  ring->set_owned_by_client();
  parent->AddChildView(ring.get());
  ring->InvalidateLayout();
  ring->SchedulePaint();
  return ring;
}

void FocusRing::SetPathGenerator(
    std::unique_ptr<HighlightPathGenerator> generator) {
  path_generator_ = std::move(generator);
  SchedulePaint();
}

void FocusRing::SetInvalid(bool invalid) {
  invalid_ = invalid;
  SchedulePaint();
}

void FocusRing::SetHasFocusPredicate(const ViewPredicate& predicate) {
  has_focus_predicate_ = predicate;
  RefreshLayer();
}

void FocusRing::SetColor(base::Optional<SkColor> color) {
  color_ = color;
  SchedulePaint();
}

SkColor FocusRing::GetColor() const {
  return color_.value_or(
      GetNativeTheme()->GetSystemColor(ColorIdForValidity(!invalid_)));
}

void FocusRing::SetInnerColor(base::Optional<SkColor> inner_color) {
  inner_color_ = inner_color;
  SchedulePaint();
}

SkColor FocusRing::GetInnerColor() const {
  return inner_color_.value_or(GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_FocusedBorderInnerColor));
}

void FocusRing::SetThickness(base::Optional<float> thickness) {
  thickness_ = thickness;
  SchedulePaint();
}

float FocusRing::GetThickness() const {
  return thickness_.value_or(PlatformStyle::kFocusHaloThickness);
}

void FocusRing::SetInset(base::Optional<float> inset) {
  inset_ = inset;
  SchedulePaint();
}

float FocusRing::GetInset() const {
  return inset_.value_or(PlatformStyle::kFocusHaloInset);
}

void FocusRing::Layout() {
  // The focus ring handles its own sizing, which is simply to fill the parent
  // and extend a little beyond its borders.
  gfx::Rect focus_bounds = parent()->GetLocalBounds();
  focus_bounds.Inset(gfx::Insets(GetInset()));
  SetBoundsRect(focus_bounds);

  // Need to match canvas direction with the parent. This is required to ensure
  // asymmetric focus ring shapes match their respective buttons in RTL mode.
  EnableCanvasFlippingForRTLUI(parent()->flip_canvas_on_paint_for_rtl_ui());
}

void FocusRing::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.child != this)
    return;

  if (details.is_add) {
    // Need to start observing the parent.
    details.parent->AddObserver(this);
  } else {
    // This view is being removed from its parent. It needs to remove itself
    // from its parent's observer list. Otherwise, since its |parent_| will
    // become a nullptr, it won't be able to do so in its destructor.
    details.parent->RemoveObserver(this);
  }
  RefreshLayer();
}

void FocusRing::OnPaint(gfx::Canvas* canvas) {
  is_showing_ = false;

  // TODO(pbos): Reevaluate if this can turn into a DCHECK, e.g. we should
  // never paint if there's no parent focus.
  if (has_focus_predicate_) {
    if (!(*has_focus_predicate_)(parent()))
      return;
  } else if (!parent()->HasFocus()) {
    return;
  }

  if (ShouldPaintFocusRing()) {
    is_showing_ = true;

    SkPath path;
    if (path_generator_)
      path = path_generator_->GetHighlightPath(parent());

    // If there's no path generator or the generated path is unusable, fall back
    // to the default.
    if (!IsPathUsable(path)) {
      path = GetHighlightPathInternal(parent());
      if (base::i18n::IsRTL() && !parent()->flip_canvas_on_paint_for_rtl_ui()) {
        gfx::Insets* padding = parent()->GetProperty(kInternalPaddingKey);
        if (padding) {
          path.offset(padding->right() - padding->left(), 0);
        }
      }
    }

    DCHECK(IsPathUsable(path));
    DCHECK_EQ(flip_canvas_on_paint_for_rtl_ui(),
            parent()->flip_canvas_on_paint_for_rtl_ui());
    SkRect bounds;
    SkRRect rbounds;
    if (path.isRect(&bounds)) {
      PaintFocusRing(canvas, RingRectFromPathRect(bounds));
    } else if (path.isOval(&bounds)) {
      PaintFocusRing(canvas, RingRectFromPathRect(SkRRect::MakeOval(bounds)));
    } else if (path.isRRect(&rbounds)) {
      PaintFocusRing(canvas, RingRectFromPathRect(rbounds));
    }
  }
}

void FocusRing::PaintFocusRing(gfx::Canvas* canvas, const SkRRect& rrect) {
  cc::PaintFlags paint;
  paint.setAntiAlias(true);
  paint.setStyle(cc::PaintFlags::kStroke_Style);
  float thickness = GetThickness();
  paint.setStrokeWidth(thickness);

  // Paint the outer ring.
  paint.setColor(GetColor());
  canvas->sk_canvas()->drawRRect(rrect, paint);

  // Paint the inner ring.
  if (show_inner_ring_) {
    // Since the inner ring is inside the outer ring and aligned to inside,
    // inset its path by the thickness of the outer ring.
    SkRRect rrect_inner = rrect;
    rrect_inner.inset(thickness, thickness);
    paint.setColor(GetInnerColor());
    canvas->sk_canvas()->drawRRect(rrect_inner, paint);
  }
}

void FocusRing::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  // Mark the focus ring in the accessibility tree as invisible so that it will
  // not be accessed by assistive technologies.
  node_data->AddState(ax::mojom::State::kInvisible);
}

void FocusRing::OnViewFocused(View* view) {
  RefreshLayer();
}

void FocusRing::OnViewBlurred(View* view) {
  RefreshLayer();
}

void FocusRing::OnViewBoundsChanged(View* view) {
  // FocusRing can install onto controls that don't necessarily know they have
  // been installed. To make sure the focus ring size stays up-to-date it
  // listens to the parent getting new bounds to re-layout.
  Layout();
}

bool FocusRing::IsHitTestVisible() const {
  return false;
}

FocusRing::FocusRing() {
  // Don't allow the view to process events.
  set_can_process_events_within_subtree(false);
}

FocusRing::~FocusRing() {
  if (parent())
    parent()->RemoveObserver(this);
}

void FocusRing::RefreshLayer() {
  // TODO(pbos): This always keeps the layer alive if |has_focus_predicate_| is
  // set. This is done because we're not notified when the predicate might
  // return a different result and there are call sites that call SchedulePaint
  // on FocusRings and expect that to be sufficient.
  // The cleanup would be to always call has_focus_predicate_ here and make sure
  // that RefreshLayer gets called somehow whenever |has_focused_predicate_|
  // returns a new value.
  const bool should_paint =
      has_focus_predicate_.has_value() || (parent() && parent()->HasFocus());
  SetVisible(should_paint);
  if (should_paint) {
    // A layer is necessary to paint beyond the parent's bounds.
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
  } else {
    DestroyLayer();
  }
}

SkRRect FocusRing::RingRectFromPathRect(const SkRect& rect) const {
  const double corner_radius = GetCornerRadius();
  return RingRectFromPathRect(
      SkRRect::MakeRectXY(rect, corner_radius, corner_radius));
}

SkRRect FocusRing::RingRectFromPathRect(const SkRRect& rrect) const {
  double thickness = GetThickness() / 2.f;
  gfx::RectF r = gfx::SkRectToRectF(rrect.rect());
  View::ConvertRectToTarget(parent(), this, &r);

  SkRRect skr =
      rrect.makeOffset(r.x() - rrect.rect().x(), r.y() - rrect.rect().y());

  // The focus indicator should hug the normal border, when present (as in the
  // case of text buttons). Since it's aligned to inside of the parent view,
  // inset the path by half the ring thickness.
  skr.inset(GetInset(), GetInset());
  skr.inset(thickness, thickness);

  return skr;
}

bool FocusRing::ShouldPaintFocusRing() {
  if (base::FeatureList::IsEnabled(features::edge::kKeyboardFocusRing)) {
    previous_reason_ = FocusManager::previous_focus_change_reason();
    current_reason_ = FocusManager::focus_change_reason();

    bool is_current_reason_keyboard_focus =
        current_reason_ ==
            views::FocusManager::FocusChangeReason::kFocusTraversal ||
        current_reason_ ==
            views::FocusManager::FocusChangeReason::kDirectKeyboardFocusChange;

    bool is_previous_reason_keyboard_focus =
        previous_reason_ ==
            views::FocusManager::FocusChangeReason::kFocusTraversal ||
        previous_reason_ ==
            views::FocusManager::FocusChangeReason::kDirectKeyboardFocusChange;

    return force_on_paint_ || is_current_reason_keyboard_focus ||
           (is_previous_reason_keyboard_focus &&
            current_reason_ ==
                views::FocusManager::FocusChangeReason::kFocusRestore);
  }
  return true;
}

SkPath GetHighlightPath(const View* view) {
  SkPath path = GetHighlightPathInternal(view);
  if (base::i18n::IsRTL()) {
    if (view->flip_canvas_on_paint_for_rtl_ui()) {
      gfx::Point center = view->GetLocalBounds().CenterPoint();
      SkMatrix flip;
      flip.setScale(-1, 1, center.x(), center.y());
      path.transform(flip);
    } else {
      // We need to account for the padding because when the browser is
      // maximized or fullscreen, a trailing margin is added to app menu button
      // in the toolbar. Without accounting for the offset, the highlight path
      // will be incorrect.
      gfx::Insets* padding = view->GetProperty(kInternalPaddingKey);
      if (padding) {
        path.offset(padding->right() - padding->left(), 0);
      }
    }
  }
  return path;
}

BEGIN_METADATA(FocusRing)
METADATA_PARENT_CLASS(View)
END_METADATA()

}  // namespace views
