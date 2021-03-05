// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/web_applications/web_app_menu_model.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/edge_webapp_features.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/base/l10n/l10n_util.h"
<<<<<<< HEAD
#include "ui/native_theme/native_theme.h"
=======
#include "ui/base/models/image_model.h"
>>>>>>> a93b2daf64f5bbe249a8117375599854e477ebc8
#include "url/gurl.h"

constexpr int WebAppMenuModel::kUninstallAppCommandId;

WebAppMenuModel::WebAppMenuModel(ui::AcceleratorProvider* provider,
                                 Browser* browser)
    : AppMenuModel(provider, browser) {}

WebAppMenuModel::~WebAppMenuModel() {}

constexpr int kDefaultHostedAppMenuIconSize = 16;

void WebAppMenuModel::AddItemWithStringIdAndIcon(
  int id, int localization_id, const gfx::VectorIcon& icon)
{
  SimpleMenuModel::AddItemWithStringIdAndIcon(
      id, localization_id,
      CreateVectorIcon(
          icon, kDefaultHostedAppMenuIconSize,
          ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
              ui::NativeTheme::kColorId_EnabledMenuItemForegroundColor)));
}

void WebAppMenuModel::Build() {
  if (CreateActionToolbarOverflowMenu())
  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringId(IDC_WEB_APP_MENU_APP_INFO,
                      IDS_APP_CONTEXT_MENU_SHOW_INFO);
  int app_info_index = GetItemCount() - 1;
  SetMinorText(app_info_index, web_app::AppBrowserController::FormatUrlOrigin(
                                   browser()
                                       ->tab_strip_model()
                                       ->GetActiveWebContents()
                                       ->GetVisibleURL()));
  SetMinorIcon(app_info_index,
               ui::ImageModel::FromVectorIcon(
                   browser()->location_bar_model()->GetVectorIcon()));

  AddSeparator(ui::NORMAL_SEPARATOR);
  AddItemWithStringIdAndIcon(IDC_COPY_URL, IDS_COPY_URL,
                             vector_icons::kCopyIcon);
  AddItemWithStringIdAndIcon(IDC_OPEN_IN_CHROME, IDS_OPEN_IN_CHROME,
                             kAboutEdgeIcon);

  // Edge specific.
  // The triggered event used for filtering which users are considered as part
  // of an experiment's control and treatment group.
  base::FeatureList::TriggerUsage(features::edge::kEdgePwaFeedback);

  DCHECK(browser()->app_controller());
  if (browser()->app_controller()->IsWebAppFeedbackEnabled()) {
    AddItemWithStringIdAndIcon(IDC_EDGE_FEEDBACK, IDS_EDGE_FEEDBACK_PWA,
                               kSendFeedbackIcon);
  }

// Chrome OS's app list is prominent enough to not need a separate uninstall
// option in the app menu.
#if !defined(OS_CHROMEOS)
  if (browser()->app_controller()->IsInstalled()) {
    AddSeparator(ui::NORMAL_SEPARATOR);
    AddItem(
        kUninstallAppCommandId,
        l10n_util::GetStringFUTF16(
            IDS_UNINSTALL_FROM_OS_LAUNCH_SURFACE,
            base::UTF8ToUTF16(browser()->app_controller()->GetAppShortName())));
  }
#endif  // !defined(OS_CHROMEOS)
  AddSeparator(ui::LOWER_SEPARATOR);

  CreateZoomMenu();
  AddSeparator(ui::UPPER_SEPARATOR);

  AddItemWithStringIdAndIcon(IDC_PRINT, IDS_PRINT, kPrintIcon);
  AddItemWithStringIdAndIcon(IDC_FIND, IDS_FIND, kFindInPageIcon);
  if (media_router::MediaRouterEnabled(browser()->profile()))
    AddItemWithStringIdAndIcon(IDC_ROUTE_MEDIA,
                               IDS_MEDIA_ROUTER_MENU_ITEM_TITLE, kCastIcon);
  //AddSeparator(ui::LOWER_SEPARATOR);
  //CreateCutCopyPasteMenu();
}

bool WebAppMenuModel::IsCommandIdEnabled(int command_id) const {
  return command_id == kUninstallAppCommandId
             ? browser()->app_controller()->CanUninstall()
             : AppMenuModel::IsCommandIdEnabled(command_id);
}

void WebAppMenuModel::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == kUninstallAppCommandId) {
    browser()->app_controller()->Uninstall();
  } else {
    AppMenuModel::ExecuteCommand(command_id, event_flags);
  }
}

void WebAppMenuModel::LogMenuAction(AppMenuAction action_id) {
  UMA_HISTOGRAM_ENUMERATION("Microsoft.HostedAppFrame.MoreMenu.MenuAction",
                            action_id, LIMIT_MENU_ACTION);
}
