// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_command_controller.h"

#include <stddef.h>

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/debugging_buildflags.h"
#include "base/debug/profiler.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/stl_util.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/edge_pinning_wizard_telemetry.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/installable/installable_metrics.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/metrics/tab_stats_tracker.h"
#include "chrome/browser/prefs/incognito_mode_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/apps/app_info_dialog.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/edge_notification_events_helper.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/web_applications/app_browser_controller.h"
#include "chrome/browser/ui/web_applications/web_app_dialog_utils.h"
#include "chrome/browser/ui/web_applications/web_app_launch_utils.h"
#include "chrome/browser/ui/webui/inspect_ui.h"
#include "chrome/common/content_restriction.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/dom_distiller/core/dom_distiller_features.h"
#include "components/feature_engagement/buildflags.h"
<<<<<<< HEAD
#include "components/pdf/common/pdf_constants.h"
=======
#include "components/omnibox/common/omnibox_features.h"
>>>>>>> 03be76dd6cbbb4ccaf41e6f183e1965f9c5bd0ae
#include "components/prefs/pref_service.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/wdag_win/buildflags.h"
#include "content/browser/open_page/open_page_preferences.h"
#include "content/browser/save_page/save_page_preferences.h"
#include "content/public/browser/dual_engine_support.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/profiling.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_system.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/edge_ui_base_features.h"
#include "ui/events/keycodes/keyboard_codes.h"

#if defined(OS_MACOSX)
#include "chrome/browser/ui/browser_commands_mac.h"
#endif

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "chrome/browser/dual_engine/dual_engine_service_factory_win.h"
#include "chrome/browser/dual_engine/dual_engine_service_win.h"
#include "content/public/browser/dual_engine_support.h"
#include "content/public/browser/gpu_data_manager.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_context_menu.h"
#include "chrome/browser/ui/browser_commands_chromeos.h"
#include "components/session_manager/core/session_manager.h"
#endif

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
#include "ui/base/ime/linux/text_edit_key_bindings_delegate_auralinux.h"
#endif

#if BUILDFLAG(ENABLE_LEGACY_DESKTOP_IN_PRODUCT_HELP)
#include "chrome/browser/feature_engagement/bookmark/bookmark_tracker.h"
#include "chrome/browser/feature_engagement/bookmark/bookmark_tracker_factory.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"
#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker_factory.h"
#endif

#if BUILDFLAG(ENABLE_WDAG)
#include "components/wdag_win/configuration.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

#if defined(MICROSOFT_EDGE_BUILD)
#include "components/edge_learning_tools/core/common/edge_learning_tools_features.h"
#include "components/edge_learning_tools/core/common/learning_tools_constants.h"
#include "components/edge_learning_tools/core/common/learning_tools_data_logger.h"
#endif

#if defined(MICROSOFT_EDGE_BUILD) && !defined(OS_ANDROID)
#include "components/dom_distiller/core/edge_reading_view_features.h"
#endif

#if defined(MICROSOFT_EDGE_BUILD) && defined(OS_WIN)
#include "components/edge_collections/core/common/collections_features.h"
#endif

using content::NavigationController;
using content::NavigationEntry;
using content::WebContents;

namespace chrome {

namespace {

// Ensures that - if we have not popped up an infobar to prompt the user to e.g.
// reload the current page - that the content pane of the browser is refocused.
void AppInfoDialogClosedCallback(content::WebContents* web_contents,
                                 views::Widget::ClosedReason closed_reason,
                                 bool reload_prompt) {
  if (reload_prompt)
    return;

  // If the user clicked on something specific or focus was changed, don't
  // override the focus.
  if (closed_reason != views::Widget::ClosedReason::kEscKeyPressed &&
      closed_reason != views::Widget::ClosedReason::kCloseButtonClicked) {
    return;
  }

  // Ensure that the web contents handle we have is still valid. It's possible
  // (though unlikely) that either the browser or web contents has been pulled
  // out from underneath us.
  Browser* const browser = chrome::FindBrowserWithWebContents(web_contents);
  if (!browser)
    return;

  // We want to focus the active web contents, which again, might not be the
  // original web contents (though it should be the vast majority of the time).
  content::WebContents* const active_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  if (active_contents)
    active_contents->Focus();
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// BrowserCommandController, public:

BrowserCommandController::BrowserCommandController(Browser* browser)
    : browser_(browser),
      save_page_prefs_(nullptr),
      open_page_prefs_(nullptr),
      command_updater_(nullptr) {
  browser_->tab_strip_model()->AddObserver(this);
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    local_pref_registrar_.Init(local_state);
    local_pref_registrar_.Add(
        prefs::kAllowFileSelectionDialogs,
        base::Bind(
            &BrowserCommandController::UpdateCommandsForFileSelectionDialogs,
            base::Unretained(this)));

#if defined(MICROSOFT_EDGE_BUILD) && defined(OS_WIN)
    local_pref_registrar_.Add(
        prefs::kPinningWizardAllowed,
        base::Bind(&BrowserCommandController::UpdateCommandsForPinningWizard,
                  base::Unretained(this)));
#endif
  }

  profile_pref_registrar_.Init(profile()->GetPrefs());
  profile_pref_registrar_.Add(
      prefs::kDevToolsAvailability,
      base::Bind(&BrowserCommandController::UpdateCommandsForDevTools,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      bookmarks::prefs::kEditBookmarksEnabled,
      base::Bind(&BrowserCommandController::UpdateCommandsForBookmarkEditing,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowBookmarkBar,
      base::Bind(&BrowserCommandController::UpdateCommandsForBookmarkBar,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowBookmarkBarOnlyNTP,
      base::Bind(&BrowserCommandController::UpdateCommandsForBookmarkBar,
                 base::Unretained(this)));
  IncognitoModePrefs::RegisterAvailabilityChangeListener(
      &profile_pref_registrar_,
      base::Bind(
          &BrowserCommandController::UpdateCommandsForIncognitoAvailability,
          base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kPrintingEnabled,
      base::Bind(&BrowserCommandController::UpdatePrintingState,
                 base::Unretained(this)));
#if !defined(OS_MACOSX)
  profile_pref_registrar_.Add(
      prefs::kFullscreenAllowed,
      base::Bind(&BrowserCommandController::UpdateCommandsForFullscreenMode,
                 base::Unretained(this)));
#endif
  pref_signin_allowed_.Init(
      prefs::kSigninAllowed, profile()->GetOriginalProfile()->GetPrefs(),
      base::Bind(&BrowserCommandController::OnSigninAllowedPrefChange,
                 base::Unretained(this)));

  InitCommandState();

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(profile());
  if (tab_restore_service) {
    tab_restore_service->AddObserver(this);
    if (!tab_restore_service->IsLoaded())
      tab_restore_service->LoadTabsFromLastSession();
  }
}

BrowserCommandController::~BrowserCommandController() {
  // TabRestoreService may have been shutdown by the time we get here. Don't
  // trigger creating it.
  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfileIfExisting(profile());
  if (tab_restore_service)
    tab_restore_service->RemoveObserver(this);
  profile_pref_registrar_.RemoveAll();
  local_pref_registrar_.RemoveAll();
  browser_->tab_strip_model()->RemoveObserver(this);
}

bool BrowserCommandController::IsReservedCommandOrKey(
    int command_id,
    const content::NativeWebKeyboardEvent& event) {
  // In Apps mode, no keys are reserved.
  if (browser_->is_type_app() || browser_->is_type_app_popup())
    return false;

#if defined(OS_CHROMEOS)
  // On Chrome OS, the top row of keys are mapped to browser actions like
  // back/forward or refresh. We don't want web pages to be able to change the
  // behavior of these keys.  Ash handles F4 and up; this leaves us needing to
  // reserve browser back/forward and refresh here.
  ui::KeyboardCode key_code =
      static_cast<ui::KeyboardCode>(event.windows_key_code);
  if ((key_code == ui::VKEY_BROWSER_BACK && command_id == IDC_BACK) ||
      (key_code == ui::VKEY_BROWSER_FORWARD && command_id == IDC_FORWARD) ||
      (key_code == ui::VKEY_BROWSER_REFRESH && command_id == IDC_RELOAD)) {
    return true;
  }
#endif

  if (window()->IsFullscreen()) {
    // In fullscreen, all commands except for IDC_FULLSCREEN and IDC_EXIT should
    // be delivered to the web page. The intent to implement and ship can be
    // found in http://crbug.com/680809.
    const bool is_exit_fullscreen =
        (command_id == IDC_EXIT || command_id == IDC_FULLSCREEN);
#if defined(OS_MACOSX)
    // This behavior is different on Mac OS, which has a unique user-initiated
    // full-screen mode. According to the discussion in http://crbug.com/702251,
    // the commands should be reserved for browser-side handling if the browser
    // window's toolbar is visible.
    if (window()->IsToolbarShowing()) {
      if (command_id == IDC_FULLSCREEN)
        return true;
    } else {
      return is_exit_fullscreen;
    }
#else
    return is_exit_fullscreen;
#endif
  }

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  // If this key was registered by the user as a content editing hotkey, then
  // it is not reserved.
  ui::TextEditKeyBindingsDelegateAuraLinux* delegate =
      ui::GetTextEditKeyBindingsDelegate();
  if (delegate && event.os_event && delegate->MatchEvent(*event.os_event, NULL))
    return false;
#endif

  return command_id == IDC_CLOSE_TAB || command_id == IDC_CLOSE_WINDOW ||
         command_id == IDC_NEW_INCOGNITO_WINDOW || command_id == IDC_NEW_TAB ||
         command_id == IDC_NEW_APPLICATION_GUARD_WINDOW ||
         command_id == IDC_NEW_WINDOW || command_id == IDC_RESTORE_TAB ||
         command_id == IDC_SELECT_NEXT_TAB ||
         command_id == IDC_SELECT_PREVIOUS_TAB || command_id == IDC_EXIT;
}

void BrowserCommandController::TabStateChanged() {
  UpdateCommandsForTabState();
}

void BrowserCommandController::ZoomStateChanged() {
  UpdateCommandsForZoomState();
}

void BrowserCommandController::PDFStateChanged() {
  UpdateCommandsForPDF();
}

void BrowserCommandController::ContentRestrictionsChanged() {
  UpdateCommandsForContentRestrictionState();
}

void BrowserCommandController::FullscreenStateChanged() {
  UpdateCommandsForFullscreenMode();
}

#if defined(OS_CHROMEOS)
void BrowserCommandController::LockedFullscreenStateChanged() {
  UpdateCommandsForLockedFullscreenMode();
}
#endif

void BrowserCommandController::PrintingStateChanged() {
  UpdatePrintingState();
}

void BrowserCommandController::LoadingStateChanged(bool is_loading,
                                                   bool force) {
  UpdateReloadStopState(is_loading, force);
}

void BrowserCommandController::FindBarVisibilityChanged() {
  if (is_locked_fullscreen_)
    return;
  UpdateCloseFindOrStop();
}

void BrowserCommandController::ExtensionStateChanged() {
  // Extensions may disable the bookmark editing commands.
  UpdateCommandsForBookmarkEditing();
}

void BrowserCommandController::TabKeyboardFocusChangedTo(
    base::Optional<int> index) {
  UpdateCommandsForTabKeyboardFocus(index);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCommandController, CommandUpdater implementation:

bool BrowserCommandController::SupportsCommand(int id) const {
  return command_updater_.SupportsCommand(id);
}

bool BrowserCommandController::IsCommandEnabled(int id) const {
  return command_updater_.IsCommandEnabled(id);
}

bool BrowserCommandController::ExecuteCommand(int id,
                                              base::TimeTicks time_stamp) {
  return ExecuteCommandWithDisposition(id, WindowOpenDisposition::CURRENT_TAB,
                                       time_stamp);
}

bool BrowserCommandController::ExecuteCommandWithDisposition(
    int id,
    WindowOpenDisposition disposition,
    base::TimeTicks time_stamp) {

  // Send confirmation of (in)action as appropriate for
  // disabled commands.
  HandleDisabledCommandConfirmationOfAction(id);

  // Doesn't go through the command_updater_ to avoid dealing with having a
  // naming collision for ExecuteCommandWithDisposition (both
  // CommandUpdaterDelegate and CommandUpdater declare this function so we
  // choose to not implement CommandUpdaterDelegate inside this class and
  // therefore command_updater_ doesn't have the delegate set).
   if (!SupportsCommand(id) || !IsCommandEnabled(id))
    return false;

  // No commands are enabled if there is not yet any selected tab.
  // TODO(pkasting): It seems like we should not need this, because either
  // most/all commands should not have been enabled yet anyway or the ones that
  // are enabled should be global, or safe themselves against having no selected
  // tab.  However, Ben says he tried removing this before and got lots of
  // crashes, e.g. from Windows sending WM_COMMANDs at random times during
  // window construction.  This probably could use closer examination someday.
  if (browser_->tab_strip_model()->active_index() == TabStripModel::kNoTab)
    return true;

  DCHECK(command_updater_.IsCommandEnabled(id))
      << "Invalid/disabled command " << id;

  // Drop down ShyUI before the command so it can receive focus.
  browser_->window()->MaybeStartImmersiveReveal();

  // The order of commands in this switch statement must match the function
  // declaration order in browser.h!
  switch (id) {
    // Navigation commands
    case IDC_BACK:
      edge::a11y::RaisePageNavigationEvent(
          browser_->tab_strip_model()->GetActiveWebContents(),
          IDS_GOING_BACK_NARRATION, L"GoingBack");
      GoBack(browser_, disposition);
      break;
    case IDC_FORWARD:
      edge::a11y::RaisePageNavigationEvent(
          browser_->tab_strip_model()->GetActiveWebContents(),
          IDS_GOING_FORWARD_NARRATION, L"GoingForward");
      GoForward(browser_, disposition);
      break;
    case IDC_RELOAD:
      Reload(browser_, disposition);
      break;
    case IDC_RELOAD_CLEARING_CACHE:
      ClearCache(browser_);
      FALLTHROUGH;
    case IDC_RELOAD_BYPASSING_CACHE:
      ReloadBypassingCache(browser_, disposition);
      break;
    case IDC_HOME:
      Home(browser_, disposition);
      break;
    case IDC_OPEN_CURRENT_URL:
      OpenCurrentURL(browser_);
      break;
    case IDC_STOP:
      Stop(browser_);
      break;
    case IDC_PASTE_AND_GO:
      PasteAndGo(browser_);
      break;

      // Window management commands
    case IDC_NEW_WINDOW:
      NewWindow(browser_);
      break;
    case IDC_NEW_INCOGNITO_WINDOW:
      NewIncognitoWindow(profile());
      break;
#if BUILDFLAG(ENABLE_WDAG)
    case IDC_NEW_APPLICATION_GUARD_WINDOW:
      NewAppGuardWindow(browser_);
      break;
#endif
    case IDC_CLOSE_WINDOW:
      base::RecordAction(base::UserMetricsAction("CloseWindowByKey"));
      CloseWindow(browser_);
      break;
    case IDC_NEW_TAB: {
      base::RecordAction(base::UserMetricsAction("Microsoft.OpenTabByKey"));
      metrics::CheckAndReportTabCountThresholdCrossed(
          browser_->tab_strip_model()->count());
      NewTab(browser_);
#if BUILDFLAG(ENABLE_LEGACY_DESKTOP_IN_PRODUCT_HELP)
      // This is not in NewTab() to avoid tracking programmatic creation of new
      // tabs by extensions.
      auto* new_tab_tracker =
          feature_engagement::NewTabTrackerFactory::GetInstance()
              ->GetForProfile(profile());

      new_tab_tracker->OnNewTabOpened();
      new_tab_tracker->CloseBubble();
#endif
      break;
    }
    case IDC_CLOSE_TAB:
      base::RecordAction(base::UserMetricsAction("CloseTabByKey"));
      CloseTab(browser_);
      break;
    case IDC_SELECT_NEXT_TAB:
      base::RecordAction(base::UserMetricsAction("Accel_SelectNextTab"));
      SelectNextTab(browser_,
                    {TabStripModel::GestureType::kKeyboard, time_stamp});
      break;
    case IDC_SELECT_PREVIOUS_TAB:
      base::RecordAction(base::UserMetricsAction("Accel_SelectPreviousTab"));
      SelectPreviousTab(browser_,
                        {TabStripModel::GestureType::kKeyboard, time_stamp});
      break;
    case IDC_MOVE_TAB_NEXT:
      MoveTabNext(browser_);
      break;
    case IDC_MOVE_TAB_PREVIOUS:
      MoveTabPrevious(browser_);
      break;
    case IDC_SELECT_TAB_0:
    case IDC_SELECT_TAB_1:
    case IDC_SELECT_TAB_2:
    case IDC_SELECT_TAB_3:
    case IDC_SELECT_TAB_4:
    case IDC_SELECT_TAB_5:
    case IDC_SELECT_TAB_6:
    case IDC_SELECT_TAB_7:
      base::RecordAction(base::UserMetricsAction("Accel_SelectNumberedTab"));
      SelectNumberedTab(browser_, id - IDC_SELECT_TAB_0,
                        {TabStripModel::GestureType::kKeyboard, time_stamp});
      break;
    case IDC_SELECT_LAST_TAB:
      base::RecordAction(base::UserMetricsAction("Accel_SelectNumberedTab"));
      SelectLastTab(browser_,
                    {TabStripModel::GestureType::kKeyboard, time_stamp});
      break;
    case IDC_DUPLICATE_TAB:
      DuplicateTab(browser_);
      break;
    case IDC_RESTORE_TAB:
      RestoreTab(browser_);
      browser_->window()->OnTabRestored(IDC_RESTORE_TAB);
      break;
    case IDC_SHOW_AS_TAB:
      ConvertPopupToTabbedBrowser(browser_);
      break;
    case IDC_FULLSCREEN:
      chrome::ToggleFullscreenMode(browser_);
      break;
    case IDC_OPEN_SITE:
      ReparentSecureSiteAsAppForActiveTab(browser_);
      break;
    case IDC_OPEN_IN_PWA_WINDOW:
      base::RecordAction(base::UserMetricsAction("OpenActiveTabInPwaWindow"));
      web_app::ReparentWebAppForActiveTab(browser_);
      break;
    case IDC_MOVE_TAB_TO_NEW_WINDOW:
      MoveActiveTabToNewWindow(browser_);
      break;

#if defined(OS_CHROMEOS)
    case IDC_VISIT_DESKTOP_OF_LRU_USER_2:
    case IDC_VISIT_DESKTOP_OF_LRU_USER_3:
    case IDC_VISIT_DESKTOP_OF_LRU_USER_4:
    case IDC_VISIT_DESKTOP_OF_LRU_USER_5:
      ExecuteVisitDesktopCommand(id, window()->GetNativeWindow());
      break;
#endif

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
    case IDC_MINIMIZE_WINDOW:
      browser_->window()->Minimize();
      break;
    case IDC_MAXIMIZE_WINDOW:
      browser_->window()->Maximize();
      break;
    case IDC_RESTORE_WINDOW:
      browser_->window()->Restore();
      break;
    case IDC_USE_SYSTEM_TITLE_BAR: {
      PrefService* prefs = profile()->GetPrefs();
      prefs->SetBoolean(prefs::kUseCustomChromeFrame,
                        !prefs->GetBoolean(prefs::kUseCustomChromeFrame));
      break;
    }
#endif

#if defined(OS_MACOSX)
    case IDC_TOGGLE_FULLSCREEN_TOOLBAR:
      chrome::ToggleFullscreenToolbar(browser_);
      break;
    case IDC_TOGGLE_JAVASCRIPT_APPLE_EVENTS: {
      chrome::ToggleJavaScriptFromAppleEventsAllowed(browser_);
      break;
    }
#endif
    case IDC_EXIT:
      Exit();
      break;

    // Page-related commands
    case IDC_SAVE_PAGE:
      SavePage(browser_, save_page_prefs_);
      break;
    case IDC_BOOKMARK_THIS_TAB:
#if BUILDFLAG(ENABLE_LEGACY_DESKTOP_IN_PRODUCT_HELP)
      feature_engagement::BookmarkTrackerFactory::GetInstance()
          ->GetForProfile(profile())
          ->OnBookmarkAdded();
#endif
      BookmarkCurrentTab(browser_);
      break;
    case IDC_BOOKMARK_ALL_TABS:
#if BUILDFLAG(ENABLE_LEGACY_DESKTOP_IN_PRODUCT_HELP)
      feature_engagement::BookmarkTrackerFactory::GetInstance()
          ->GetForProfile(profile())
          ->OnBookmarkAdded();
#endif
      BookmarkAllTabs(browser_);
      break;
    case IDC_VIEW_SOURCE:
      browser_->tab_strip_model()
          ->GetActiveWebContents()
          ->GetMainFrame()
          ->ViewSource();
      break;
    case IDC_EMAIL_PAGE_LOCATION:
      EmailPageLocation(browser_);
      break;
    case IDC_PRINT:
      Print(browser_);
      break;

#if BUILDFLAG(ENABLE_PRINTING)
    case IDC_BASIC_PRINT:
      base::RecordAction(base::UserMetricsAction("Accel_Advanced_Print"));
      BasicPrint(browser_);
      break;
#endif  // ENABLE_PRINTING

    case IDC_SAVE_CREDIT_CARD_FOR_PAGE:
      SaveCreditCard(browser_);
      break;
    case IDC_MIGRATE_LOCAL_CREDIT_CARD_FOR_PAGE:
#if EXCLUDED_FROM_EDGE
      MigrateLocalCards(browser_);
#endif
      NOTREACHED();
      break;
    case IDC_SHOW_SAVE_LOCAL_CARD_SIGN_IN_PROMO_IF_APPLICABLE:
      MaybeShowSaveLocalCardSignInPromo(browser_);
      break;
    case IDC_CLOSE_SIGN_IN_PROMO:
      CloseSaveLocalCardSignInPromo(browser_);
      break;
    case IDC_TRANSLATE_PAGE:
      Translate(browser_);
      break;
    case IDC_MANAGE_PASSWORDS_FOR_PAGE:
      ManagePasswordsForPage(browser_);
      break;
    case IDC_SEND_TAB_TO_SELF:
      SendTabToSelfFromPageAction(browser_);
      break;

    // Toggle caret browsing command
    case IDC_TOGGLE_CARET_BROWSING:
      chrome::ToggleCaretBrowsingMode(browser_);
      break;

    // Clipboard commands
    case IDC_CUT:
    case IDC_COPY:
    case IDC_PASTE:
      CutCopyPaste(browser_, id);
      break;

    // Find-in-page
    case IDC_FIND:
      Find(browser_);
      break;
    case IDC_FIND_NEXT:
      FindNext(browser_);
      break;
    case IDC_FIND_PREVIOUS:
      FindPrevious(browser_);
      break;
    case IDC_CLOSE_FIND_OR_STOP:
      if (CanCloseFind(browser_))
        CloseFind(browser_);
      else if (IsCommandEnabled(IDC_STOP))
        ExecuteCommand(IDC_STOP);
      break;

    // Zoom
    case IDC_ZOOM_PLUS:
      Zoom(browser_, content::PAGE_ZOOM_IN);
      break;
    case IDC_ZOOM_NORMAL:
      Zoom(browser_, content::PAGE_ZOOM_RESET);
      break;
    case IDC_ZOOM_MINUS:
      Zoom(browser_, content::PAGE_ZOOM_OUT);
      break;

#if defined(MICROSOFT_EDGE_BUILD)
    case IDC_ADD_TO_READING_LIST:
      AddToReadingList(browser_);
      break;
    case IDC_READING_LIST_MOVED_TO_FAVORITES_TIP:
      ShowReadingListItemsInFavorites(browser_);
      break;
#endif

#if defined(MICROSOFT_EDGE_BUILD)
    case IDC_READ_ALOUD_START:
      ReadAloud(browser_, edge::learning_tools::kReadAloudStart,
                edge::learning_tools::LearningToolsDataLogger::kAppMenu);
      break;
    case IDC_READ_ALOUD_TOGGLE_START:
      ReadAloud(browser_,
                IsReadAloudStopped(browser_)
                    ? edge::learning_tools::kReadAloudStart
                    : edge::learning_tools::kReadAloudStop,
                edge::learning_tools::LearningToolsDataLogger::kMacMenuBar);
      break;
    case IDC_READ_ALOUD_TOGGLE_RESUME:
      ReadAloud(browser_,
                IsReadAloudPlaying(browser_)
                    ? edge::learning_tools::kReadAloudPause
                    : edge::learning_tools::kReadAloudResume,
                edge::learning_tools::LearningToolsDataLogger::kMacMenuBar);
      break;
    case IDC_READ_ALOUD_NEXT:
      ReadAloud(browser_, edge::learning_tools::kReadAloudNext,
                edge::learning_tools::LearningToolsDataLogger::kMacMenuBar);
      break;
    case IDC_READ_ALOUD_PREVIOUS:
      ReadAloud(browser_, edge::learning_tools::kReadAloudPrevious,
                edge::learning_tools::LearningToolsDataLogger::kMacMenuBar);
      break;
    case IDC_READ_ALOUD_TOGGLE_VOICE_OPTIONS:
      // For now we are hardcoding caller as Keyboard (param as false) since keycode
      // event is not flowing to Menu bar command handling. Change to Hide if
      // voice options are visible. Bug# 21049988: Initiator of event (Mouse or
      // Keyboard) when clicking Read Aloud menu
      ReadAloud(browser_,
                chrome::IsReadAloudSettingsPaneOpen(browser_)
                    ? edge::learning_tools::kReadAloudHideSettings
                    : edge::learning_tools::kReadAloudShowSettings,
                {"false"} /*params*/,
                edge::learning_tools::LearningToolsDataLogger::kMacMenuBar);
      break;
    case IDC_READ_ALOUD_TOGGLE_START_SHORTCUT:
      ReadAloud(browser_,
                IsReadAloudPlaying(browser_)
                    ? edge::learning_tools::kReadAloudPause
                    : edge::learning_tools::kReadAloudStart,
                edge::learning_tools::LearningToolsDataLogger::kAppMenu);
      break;
#endif
    case IDC_FIT_TO_VIEW:
      InvokePDFAction(browser_, pdf::kPdfFitToChanged);
      break;
    case IDC_ROTATE_CW:
      InvokePDFAction(browser_, pdf::kPdfRotateCW);
      break;
    // Focus various bits of UI
    case IDC_FOCUS_TOOLBAR:
      base::RecordAction(base::UserMetricsAction("Accel_Focus_Toolbar"));
      FocusToolbar(browser_);
      break;
    case IDC_FOCUS_LOCATION:
      base::RecordAction(base::UserMetricsAction("Accel_Focus_Location"));
      FocusLocationBar(browser_);
      break;
    case IDC_FOCUS_SEARCH:
      base::RecordAction(base::UserMetricsAction("Accel_Focus_Search"));
      FocusSearch(browser_);
      break;
    case IDC_FOCUS_MENU_BAR:
      FocusAppMenu(browser_);
      break;
    case IDC_FOCUS_BOOKMARKS:
      base::RecordAction(base::UserMetricsAction("Accel_Focus_Bookmarks"));
      FocusBookmarksToolbar(browser_);
      break;
    case IDC_FOCUS_INACTIVE_POPUP_FOR_ACCESSIBILITY:
      FocusInactivePopupForAccessibility(browser_);
      break;
    case IDC_FOCUS_NEXT_PANE:
      FocusNextPane(browser_);
      break;
    case IDC_FOCUS_PREVIOUS_PANE:
      FocusPreviousPane(browser_);
      break;

    // Show various bits of UI
    case IDC_OPEN_FILE:
      browser_->OpenFile(open_page_prefs_);
      break;
    case IDC_CREATE_SHORTCUT:
      base::RecordAction(base::UserMetricsAction("CreateShortcut"));
      InstallableMetrics::TrackBrowserMenuInstallAction(
          WebappBrowserMenuInstallAction::kPinnedSiteInstallationInvoked);
      web_app::CreateWebAppFromCurrentWebContents(
          browser_, true /* force_shortcut_app */);
      break;
    case IDC_PIN_TO_TASKBAR:
      web_app::PinWebAppToTaskbar(browser_);
      break;
    case IDC_INSTALL_SITE:
      web_app::set_use_dialog(web_app::USE_DIALOG::INSTALL_SITE_DIALOG);
      InstallableMetrics::TrackBrowserMenuInstallAction(
          WebappBrowserMenuInstallAction::kSiteAsAppInstallationInvoked);
      web_app::CreateWebAppFromCurrentWebContents(
          browser_, false /* force_shortcut_app */);
      break;
    case IDC_INSTALL_PWA:
      base::RecordAction(base::UserMetricsAction("InstallWebAppFromMenu"));
      InstallableMetrics::TrackBrowserMenuInstallAction(
          WebappBrowserMenuInstallAction::kPwaInstallationInvoked);
      web_app::CreateWebAppFromCurrentWebContents(
          browser_, false /* force_shortcut_app */);
      break;
    case IDC_DEV_TOOLS:
      ToggleDevToolsWindow(browser_, DevToolsToggleAction::Show());
      break;
    case IDC_DEV_TOOLS_CONSOLE:
      ToggleDevToolsWindow(browser_, DevToolsToggleAction::ShowConsolePanel());
      break;
    case IDC_DEV_TOOLS_DEVICES:
      InspectUI::InspectDevices(browser_);
      break;
    case IDC_DEV_TOOLS_INSPECT:
      ToggleDevToolsWindow(browser_, DevToolsToggleAction::Inspect());
      break;
    case IDC_DEV_TOOLS_TOGGLE:
      ToggleDevToolsWindow(browser_, DevToolsToggleAction::Toggle());
      break;
    case IDC_TASK_MANAGER:
      OpenTaskManager(browser_);
      break;
#if defined(MICROSOFT_EDGE_BUILD) && defined(OS_WIN)
    case IDC_SHOW_PINNING_WIZARD:
      edge_pinning_wizard_telemetry::EdgePinningWizardTelemetry::
          FirePinningWizardLaunchHistogram(
              edge_pinning_wizard_telemetry::PinningWizardEntryPoint ::
                  menuLaunch);
      OpenPinningWizard(browser_);
      break;
#endif
#if defined(OS_CHROMEOS)
    case IDC_TAKE_SCREENSHOT:
      TakeScreenshot();
      break;
#endif
#if defined(MICROSOFT_EDGE_BUILD)
    case IDC_TOGGLE_COLLECTIONS:
      ToggleCollectionsVisibility(browser_);
      break;
#endif
    case IDC_TOGGLE_HUB:
      ToggleHubVisibility(browser_);
      break;
    case IDC_EDGE_FEEDBACK:
      // Show regular feedback dialog when clicked on "Send feedback"
      // under Wrench -> Help & Feedback or under the more menu in a PWA
      if (web_app::AppBrowserController::IsForWebAppBrowser(browser_)) {
        OpenEdgeFeedbackDialog(browser_, microsoft_edge::kFeedbackEdgePwa);
      } else {
        OpenEdgeFeedbackDialog(browser_,
                               microsoft_edge::kFeedbackSourceBrowserCommand);
      }
      break;
    case IDC_EDGE_FEEDBACK_VIA_KEYBOARD:
      if (web_app::AppBrowserController::IsForWebAppBrowser(browser_)) {
        OpenEdgeFeedbackDialog(browser_,
                              microsoft_edge::kFeedbackEdgePwaShortcut);
      } else {
        OpenEdgeFeedbackDialog(browser_,
                              microsoft_edge::kFeedbackSourceShortcut);
      }
      break;
    case IDC_EDGE_FEEDBACK_SMILE_BUTTON:
      // Show regular feedback dialog when clicked on Smile button on toolbar.
      OpenEdgeFeedbackDialog(
          browser_, microsoft_edge::kFeedbackSourceEdgeFeedbackSmileButton);
      break;
    case IDC_SHOW_TOOLBAR_FEEDBACK_BUTTON:
      ToggleToolbarFeedbackButtonVisibility(browser_);
      break;
    case IDC_SHOW_TOOLBAR_COLLECTIONS_BUTTON:
      ToggleToolbarCollectionsButtonVisibility(browser_);
      break;
    case IDC_SHOW_TOOLBAR_HOME_BUTTON:
      ToggleToolbarHomeButtonVisibility(browser_);
      break;
#if BUILDFLAG(ENABLE_WDAG)
    case IDC_WDAG_FLYOUT_BUTTON:
      ShowWdagFlyoutDialog(browser_);
      break;
#endif
    case IDC_SHOW_BOOKMARK_BAR:
      ToggleBookmarkBar(browser_);
      break;
<<<<<<< HEAD
    case IDC_SHOW_BOOKMARK_BAR_ALWAYS:
      ShowBookmarkBarAlways(browser_);
      break;
    case IDC_SHOW_BOOKMARK_BAR_NEVER:
      ShowBookmarkBarNever(browser_);
      break;
    case IDC_SHOW_BOOKMARK_BAR_ONLY_ON_NTP:
      ShowBookmarkBarOnlyOnNTP(browser_);
      break;

    case IDC_SHOW_TOOLBAR_BOOKMARKS_BUTTON:
      ToggleToolbarBookmarksButtonVisibility(browser_);
      break;

    case IDC_SHOW_TOOLBAR_SHARE_BUTTON:
      ToggleToolbarShareButtonVisibility(browser_);
      break;

    case IDC_SHOW_CUSTOMIZE_TOOLBAR_PAGE:
      ShowCustomizeBrowser(browser_);
      break;

=======
    case IDC_SHOW_FULL_URLS:
      ToggleShowFullURLs(browser_);
      break;
>>>>>>> 03be76dd6cbbb4ccaf41e6f183e1965f9c5bd0ae
    case IDC_PROFILING_ENABLED:
      content::Profiling::Toggle();
      break;

    case IDC_SHOW_BOOKMARK_MANAGER:
      ShowBookmarkManager(browser_);
      break;

    case IDC_BOOKMARK_MANAGER_REMOVE_DUPLICATES:
      ShowBookmarkManagerRemoveDuplicates(browser_);
      break;

    case IDC_SHOW_APPS_MANAGER:
      ShowAppsManager(browser_);
      break;
    // Command handling for more menu
    case IDC_SHOW_APP_MENU:
      base::RecordAction(base::UserMetricsAction("Accel_Show_App_Menu"));
      ShowAppMenu(browser_);
      break;
    case IDC_SHOW_AVATAR_MENU:
      ShowAvatarMenu(browser_);
      break;
    case IDC_SHOW_HISTORY:
      ShowHistory(browser_);
      break;
    case IDC_SHOW_DOWNLOADS:
      ShowDownloads(browser_);
      break;
    case IDC_TOGGLE_MUTE_ACTIVE_TAB:
      ToggleMuteCurrentTab(browser_);
      break;
    case IDC_SHARE:
      ShowShare(browser_);
      break;
    case IDC_MANAGE_EXTENSIONS:
      ShowExtensions(browser_, std::string());
      break;
    case IDC_OPTIONS:
      ShowSettings(browser_);
      break;
    case IDC_EDIT_SEARCH_ENGINES:
      ShowSearchEngineSettings(browser_);
      break;
    case IDC_VIEW_PASSWORDS:
      ShowPasswordManager(browser_);
      break;
    case IDC_CLEAR_BROWSING_DATA:
      ShowClearBrowsingDataDialog(browser_);
      break;
    case IDC_IMPORT_SETTINGS:
      ShowImportDialog(browser_);
      break;
    case IDC_TOGGLE_REQUEST_TABLET_SITE:
      ToggleRequestTabletSite(browser_);
      break;
    case IDC_ABOUT:
      ShowAboutChrome(browser_);
      break;
    case IDC_UPGRADE_DIALOG:
      OpenUpdateChromeDialog(browser_);
      break;
    case IDC_HELP_PAGE_VIA_KEYBOARD:
      ShowHelp(browser_, HELP_SOURCE_KEYBOARD);
      break;
    case IDC_HELP_PAGE_VIA_MENU:
      ShowHelp(browser_, HELP_SOURCE_MENU);
      break;
    case IDC_WHATS_NEW_AND_TIPS:
      ShowWhatsNewAndTips(browser_);
      break;
    case IDC_SHOW_BETA_FORUM:
      ShowBetaForum(browser_);
      break;
#if !defined(OS_CHROMEOS)
    case IDC_SHOW_SIGNIN:
      ShowBrowserSigninOrSettings(
          browser_, signin_metrics::AccessPoint::ACCESS_POINT_MENU);
      break;
#endif
    case IDC_DISTILL_PAGE:
      ToggleDistilledView(browser_);
      break;
#if defined(MICROSOFT_EDGE_BUILD) && !defined(OS_ANDROID)
    case IDC_TOGGLE_READING_VIEW:
      ToggleReadingView(browser_);
      break;
#endif

    case IDC_QUICK_SEARCH:
      ShowQuickSearch(browser_);
      break;

    case IDC_ROUTE_MEDIA:
      RouteMediaInvokedFromAppMenu(browser_);
      break;
    case IDC_WINDOW_MUTE_SITE:
      MuteSite(browser_);
      break;
    case IDC_WINDOW_PIN_TAB:
      PinTab(browser_);
      break;
    case IDC_WINDOW_GROUP_TAB:
      GroupTab(browser_);
      break;
    case IDC_WINDOW_CLOSE_TABS_TO_RIGHT:
      CloseTabsToRight(browser_);
      break;
    case IDC_WINDOW_CLOSE_OTHER_TABS:
      CloseOtherTabs(browser_);
      break;
    case IDC_SHOW_MANAGEMENT_PAGE: {
      ShowSingletonTab(browser_, GURL(kChromeUIManagementURL));
      break;
    }
    case IDC_SHOW_FAMILY_SAFETY_PAGE: {
      ShowSingletonTab(browser_, GURL(kFamilySafetyPageURL));
      break;
    }
    case IDC_MUTE_TARGET_SITE:
      MuteSiteForKeyboardFocusedTab(browser_);
      break;
    case IDC_PIN_TARGET_TAB:
      PinKeyboardFocusedTab(browser_);
      break;
    case IDC_GROUP_TARGET_TAB:
      GroupKeyboardFocusedTab(browser_);
      break;
    case IDC_DUPLICATE_TARGET_TAB:
      DuplicateKeyboardFocusedTab(browser_);
      break;
    // Hosted App commands
    case IDC_COPY_URL:
      CopyURL(browser_);
      break;
    case IDC_OPEN_IN_CHROME:
      OpenInChrome(browser_);
      break;
    case IDC_SITE_SETTINGS:
      ShowSiteSettings(
          browser_,
          browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL());
      break;
    case IDC_WEB_APP_MENU_APP_INFO: {
      content::WebContents* const web_contents =
          browser_->tab_strip_model()->GetActiveWebContents();
      if (web_contents) {
        ShowPageInfoDialog(web_contents,
                           base::BindOnce(&AppInfoDialogClosedCallback,
                                          base::Unretained(web_contents)),
                           bubble_anchor_util::kAppMenuButton);
      }
      break;
    }

#if defined(OS_WIN)
    // Dual Engines
    case IDC_DUALENGINE_HOST_IN_IE:
      AddDualEngineForcedModeTab(
          browser_,
          browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL(),
          dual_engine::ForcedEngine::kInternetExplorer);
      break;
    case IDC_DUALENGINE_HOST_IN_EDGE:
      AddDualEngineForcedModeTab(
          browser_,
          browser_->tab_strip_model()->GetActiveWebContents()->GetVisibleURL(),
          dual_engine::ForcedEngine::kEdge);
      break;
    case IDC_DUALENGINE_REPORT_SITE:
      DoDualEngineReporting(browser_);
      break;
#endif

    // SmartScreen
    case IDC_SMARTSCREEN_REPORT_SITE:
      ReportSite(browser_->tab_strip_model()->GetActiveWebContents());
      break;

    default:
      LOG(WARNING) << "Received Unimplemented Command: " << id;
      break;
  }

  return true;
}

void BrowserCommandController::AddCommandObserver(int id,
                                                  CommandObserver* observer) {
  command_updater_.AddCommandObserver(id, observer);
}

void BrowserCommandController::RemoveCommandObserver(
    int id,
    CommandObserver* observer) {
  command_updater_.RemoveCommandObserver(id, observer);
}

void BrowserCommandController::RemoveCommandObserver(
    CommandObserver* observer) {
  command_updater_.RemoveCommandObserver(observer);
}

bool BrowserCommandController::UpdateCommandEnabled(int id, bool state) {
  if (is_locked_fullscreen_)
    return false;

  return command_updater_.UpdateCommandEnabled(id, state);
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCommandController, SigninPrefObserver implementation:

void BrowserCommandController::OnSigninAllowedPrefChange() {
  // For unit tests, we don't have a window.
  if (!window())
    return;
  UpdateShowSyncState(IsShowingMainUI());
}

// BrowserCommandController, TabStripModelObserver implementation:

void BrowserCommandController::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  std::vector<content::WebContents*> new_contents;
  std::vector<content::WebContents*> old_contents;

  switch (change.type()) {
    case TabStripModelChange::kInserted:
      for (const auto& contents : change.GetInsert()->contents)
        new_contents.push_back(contents.contents);
      break;
    case TabStripModelChange::kReplaced: {
      auto* replace = change.GetReplace();
      new_contents.push_back(replace->new_contents);
      old_contents.push_back(replace->old_contents);
      break;
    }
    case TabStripModelChange::kRemoved:
      for (const auto& contents : change.GetRemove()->contents)
        old_contents.push_back(contents.contents);
      break;
    default:
      break;
  }

  for (auto* contents : old_contents)
    RemoveInterstitialObservers(contents);
  for (auto* contents : new_contents)
    AddInterstitialObservers(contents);
}

void BrowserCommandController::TabBlockedStateChanged(
    content::WebContents* contents,
    int index) {
  PrintingStateChanged();
  FullscreenStateChanged();
  UpdateCommandsForFind();
  UpdateCommandsForMediaRouter();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCommandController, TabRestoreServiceObserver implementation:

void BrowserCommandController::TabRestoreServiceChanged(
    sessions::TabRestoreService* service) {
  UpdateTabRestoreCommandState();
}

void BrowserCommandController::TabRestoreServiceDestroyed(
    sessions::TabRestoreService* service) {
  service->RemoveObserver(this);
}

void BrowserCommandController::TabRestoreServiceLoaded(
    sessions::TabRestoreService* service) {
  UpdateTabRestoreCommandState();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCommandController, private:

class BrowserCommandController::InterstitialObserver
    : public content::WebContentsObserver {
 public:
  InterstitialObserver(BrowserCommandController* controller,
                       content::WebContents* web_contents)
      : WebContentsObserver(web_contents), controller_(controller) {}

  void DidAttachInterstitialPage() override {
    controller_->UpdateCommandsForTabState();
  }

  void DidDetachInterstitialPage() override {
    controller_->UpdateCommandsForTabState();
  }

 private:
  BrowserCommandController* controller_;

  DISALLOW_COPY_AND_ASSIGN(InterstitialObserver);
};

bool BrowserCommandController::IsShowingMainUI() {
  return browser_->SupportsWindowFeature(Browser::FEATURE_TABSTRIP);
}

bool BrowserCommandController::IsShowingLocationBar() {
  return browser_->SupportsWindowFeature(Browser::FEATURE_LOCATIONBAR);
}

void BrowserCommandController::InitCommandState() {
  // All browser commands whose state isn't set automagically some other way
  // (like Back & Forward with initial page load) must have their state
  // initialized here, otherwise they will be forever disabled.

  if (is_locked_fullscreen_)
    return;

  // Navigation commands
  command_updater_.UpdateCommandEnabled(IDC_RELOAD, true);
  command_updater_.UpdateCommandEnabled(IDC_RELOAD_BYPASSING_CACHE, true);
  command_updater_.UpdateCommandEnabled(IDC_RELOAD_CLEARING_CACHE, true);

  // Window management commands
  command_updater_.UpdateCommandEnabled(IDC_CLOSE_WINDOW, true);
  command_updater_.UpdateCommandEnabled(IDC_NEW_TAB, true);
  command_updater_.UpdateCommandEnabled(IDC_CLOSE_TAB, true);
  command_updater_.UpdateCommandEnabled(IDC_DUPLICATE_TAB, true);
  command_updater_.UpdateCommandEnabled(IDC_FIT_TO_VIEW, false);
  command_updater_.UpdateCommandEnabled(IDC_ROTATE_CW, false);
#if BUILDFLAG(ENABLE_WDAG)
  if (wdag::IsHostInstance() && wdag::host::IsEnabledAndSupported())
    command_updater_.UpdateCommandEnabled(
        IDC_NEW_APPLICATION_GUARD_WINDOW, true);
#endif
  UpdateTabRestoreCommandState();
  command_updater_.UpdateCommandEnabled(IDC_EXIT, true);
  command_updater_.UpdateCommandEnabled(IDC_DEBUG_FRAME_TOGGLE, true);
#if defined(OS_CHROMEOS)
  command_updater_.UpdateCommandEnabled(IDC_MINIMIZE_WINDOW, true);
  // The VisitDesktop command is only supported for up to 5 logged in users
  // because that's the max number of user sessions. If that number is increased
  // the IDC_VISIT_DESKTOP_OF_LRU_USER_ command ids should be updated as well.
  // crbug.com/940461
  static_assert(
      session_manager::kMaximumNumberOfUserSessions <=
          IDC_VISIT_DESKTOP_OF_LRU_USER_LAST -
              IDC_VISIT_DESKTOP_OF_LRU_USER_NEXT + 2,
      "The max number of user sessions exceeds the number of users supported.");
  command_updater_.UpdateCommandEnabled(IDC_VISIT_DESKTOP_OF_LRU_USER_2, true);
  command_updater_.UpdateCommandEnabled(IDC_VISIT_DESKTOP_OF_LRU_USER_3, true);
  command_updater_.UpdateCommandEnabled(IDC_VISIT_DESKTOP_OF_LRU_USER_4, true);
  command_updater_.UpdateCommandEnabled(IDC_VISIT_DESKTOP_OF_LRU_USER_5, true);
#endif
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  command_updater_.UpdateCommandEnabled(IDC_MINIMIZE_WINDOW, true);
  command_updater_.UpdateCommandEnabled(IDC_MAXIMIZE_WINDOW, true);
  command_updater_.UpdateCommandEnabled(IDC_RESTORE_WINDOW, true);
  bool use_system_title_bar = true;
#if defined(USE_OZONE)
  use_system_title_bar = ui::OzonePlatform::GetInstance()
                             ->GetPlatformProperties()
                             .use_system_title_bar;
#endif
  command_updater_.UpdateCommandEnabled(IDC_USE_SYSTEM_TITLE_BAR,
                                        use_system_title_bar);
#endif
  command_updater_.UpdateCommandEnabled(IDC_OPEN_IN_PWA_WINDOW, true);

  // Page-related commands
  command_updater_.UpdateCommandEnabled(IDC_EMAIL_PAGE_LOCATION, true);
  command_updater_.UpdateCommandEnabled(IDC_MANAGE_PASSWORDS_FOR_PAGE, true);

// ReadAloud
#if defined(MICROSOFT_EDGE_BUILD)
  UpdateCommandsForReadAloud();
#endif

  UpdateQuickSearchState();

  // Zoom
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_MENU, true);
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_PLUS, true);
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_NORMAL, false);
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_MINUS, true);

  // Show various bits of UI
  const bool guest_session =
      profile()->IsGuestSession() || profile()->IsSystemProfile();
  DCHECK(!profile()->IsSystemProfile())
      << "Ought to never have browser for the system profile.";
  const bool normal_window = browser_->is_type_normal();
  UpdateOpenFileState(&command_updater_);
  UpdateCommandsForDevTools();
  command_updater_.UpdateCommandEnabled(IDC_TASK_MANAGER, CanOpenTaskManager());
  command_updater_.UpdateCommandEnabled(IDC_SHOW_HISTORY, !guest_session);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_DOWNLOADS, true);
  command_updater_.UpdateCommandEnabled(IDC_HELP_MENU, true);
  command_updater_.UpdateCommandEnabled(IDC_HELP_PAGE_VIA_KEYBOARD, true);
  command_updater_.UpdateCommandEnabled(IDC_HELP_PAGE_VIA_MENU, true);
  command_updater_.UpdateCommandEnabled(IDC_WHATS_NEW_AND_TIPS, true);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_BETA_FORUM, true);
  command_updater_.UpdateCommandEnabled(IDC_BOOKMARKS_MENU,
#if BUILDFLAG(ENABLE_WDAG)
      (wdag::IsHostInstance() ||
           wdag::container::IsContainerPersistencePolicyEnabled()) &&
#endif
      !guest_session);
  command_updater_.UpdateCommandEnabled(IDC_RECENT_TABS_MENU,
#if BUILDFLAG(ENABLE_WDAG)
      (wdag::IsHostInstance() ||
           wdag::container::IsContainerPersistencePolicyEnabled()) &&
#endif
      !guest_session &&
      !profile()->IsOffTheRecord());
  command_updater_.UpdateCommandEnabled(IDC_APPS_MENU, !guest_session);
  command_updater_.UpdateCommandEnabled(
      IDC_CLEAR_BROWSING_DATA,
      !guest_session && !profile()->IsIncognitoProfile());
#if defined(OS_CHROMEOS)
  command_updater_.UpdateCommandEnabled(IDC_TAKE_SCREENSHOT, true);
  // Chrome OS uses the system tray menu to handle multi-profiles. Avatar menu
  // is only required in incognito mode.
  if (profile()->IsIncognitoProfile())
    command_updater_.UpdateCommandEnabled(IDC_SHOW_AVATAR_MENU, true);
#else
  if (normal_window)
    command_updater_.UpdateCommandEnabled(IDC_SHOW_AVATAR_MENU, true);
#endif
  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_SAVE_LOCAL_CARD_SIGN_IN_PROMO_IF_APPLICABLE, true);
  command_updater_.UpdateCommandEnabled(IDC_CLOSE_SIGN_IN_PROMO, true);

  UpdateShowSyncState(true);

  // Navigation commands
  command_updater_.UpdateCommandEnabled(
      IDC_HOME, normal_window || browser_->deprecated_is_app());

  const bool is_web_app =
      web_app::AppBrowserController::IsForWebAppBrowser(browser_);
  // Hosted app browser commands.
  command_updater_.UpdateCommandEnabled(IDC_COPY_URL, is_web_app);
  command_updater_.UpdateCommandEnabled(IDC_OPEN_IN_CHROME, is_web_app);
  command_updater_.UpdateCommandEnabled(IDC_SITE_SETTINGS, is_web_app);
  command_updater_.UpdateCommandEnabled(IDC_WEB_APP_MENU_APP_INFO, is_web_app);

  // Tab management commands
  const bool supports_tabs =
      browser_->SupportsWindowFeature(Browser::FEATURE_TABSTRIP);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_NEXT_TAB, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_PREVIOUS_TAB, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_MOVE_TAB_NEXT, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_MOVE_TAB_PREVIOUS, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_0, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_1, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_2, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_3, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_4, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_5, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_6, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_TAB_7, supports_tabs);
  command_updater_.UpdateCommandEnabled(IDC_SELECT_LAST_TAB, supports_tabs);

  // Caret browsing mode toggling command.
  command_updater_.UpdateCommandEnabled(IDC_TOGGLE_CARET_BROWSING, true);

  // These are always enabled; the menu determines their menu item visibility.
  command_updater_.UpdateCommandEnabled(IDC_UPGRADE_DIALOG, true);
  // Distill current page.
#if !defined(MICROSOFT_EDGE_BUILD) || defined(OS_ANDROID)
  command_updater_.UpdateCommandEnabled(IDC_DISTILL_PAGE,
                                        dom_distiller::IsDomDistillerEnabled());
#endif

  command_updater_.UpdateCommandEnabled(IDC_WINDOW_MUTE_SITE, normal_window);
  command_updater_.UpdateCommandEnabled(IDC_TOGGLE_MUTE_ACTIVE_TAB,
                                        normal_window);
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_PIN_TAB, normal_window);
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_GROUP_TAB, normal_window);
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_RIGHT,
                                        normal_window);
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_CLOSE_OTHER_TABS,
                                        normal_window);

  // Dual engine
#if defined(OS_WIN)
  const bool ie_mode_testing_enabled =
      (dual_engine::InternetExplorerIntegrationLevel(profile()) ==
       dual_engine::IntegrationLevel::IEMode) &&
      dual_engine::IsInternetExplorerModeTestingEnabled();
  command_updater_.UpdateCommandEnabled(IDC_DUALENGINE_HOST_IN_IE,
                                        ie_mode_testing_enabled);
  command_updater_.UpdateCommandEnabled(IDC_DUALENGINE_HOST_IN_EDGE,
                                        ie_mode_testing_enabled);
  command_updater_.UpdateCommandEnabled(
      IDC_DUALENGINE_REPORT_SITE,
      dual_engine::InternetExplorerIntegrationLevel(profile()) ==
          dual_engine::IntegrationLevel::IEMode);
#endif

  // SmartScreen
  command_updater_.UpdateCommandEnabled(IDC_SMARTSCREEN_REPORT_SITE, true);

  // Initialize other commands whose state changes based on various conditions.
  UpdateCommandsForFullscreenMode();
  UpdateCommandsForContentRestrictionState();
  UpdateCommandsForBookmarkEditing();
  UpdateCommandsForIncognitoAvailability();
  UpdateCommandsForTabKeyboardFocus(GetKeyboardFocusedTabIndex(browser_));
#if defined(MICROSOFT_EDGE_BUILD) && defined(OS_WIN)
  UpdateCommandsForPinningWizard();
#endif
}

// static
void BrowserCommandController::UpdateSharedCommandsForIncognitoAvailability(
    CommandUpdater* command_updater,
    Profile* profile) {
  const bool guest_session = profile->IsGuestSession();
  // TODO(mlerman): Make GetAvailability account for profile->IsGuestSession().
  IncognitoModePrefs::Availability incognito_availability =
      IncognitoModePrefs::GetAvailability(profile->GetPrefs());
  command_updater->UpdateCommandEnabled(
      IDC_NEW_WINDOW, incognito_availability != IncognitoModePrefs::FORCED);
  command_updater->UpdateCommandEnabled(
      IDC_NEW_INCOGNITO_WINDOW,
      incognito_availability != IncognitoModePrefs::DISABLED && !guest_session);

  const bool forced_incognito =
      incognito_availability == IncognitoModePrefs::FORCED ||
      guest_session;  // Guest always runs in Incognito mode.
  command_updater->UpdateCommandEnabled(
      IDC_SHOW_BOOKMARK_MANAGER,
      browser_defaults::bookmarks_enabled && !forced_incognito);
  command_updater->UpdateCommandEnabled(
      IDC_BOOKMARK_MANAGER_REMOVE_DUPLICATES,
      browser_defaults::bookmarks_enabled && !forced_incognito);
  command_updater->UpdateCommandEnabled(IDC_SHOW_APPS_MANAGER,
                                        !forced_incognito);
  extensions::ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  const bool enable_extensions =
      extension_service && extension_service->extensions_enabled();

  command_updater->UpdateCommandEnabled(
      IDC_SHOW_FULL_URLS,
      base::FeatureList::IsEnabled(omnibox::kOmniboxContextMenuShowFullUrls));

  // Bookmark manager and settings page/subpages are forced to open in normal
  // mode. For this reason we disable these commands when incognito is forced.
  command_updater->UpdateCommandEnabled(IDC_MANAGE_EXTENSIONS,
                                        enable_extensions && !forced_incognito);

  command_updater->UpdateCommandEnabled(IDC_IMPORT_SETTINGS, !forced_incognito);
  command_updater->UpdateCommandEnabled(IDC_OPTIONS,
                                        !forced_incognito || guest_session);
  command_updater->UpdateCommandEnabled(IDC_SHOW_SIGNIN, !forced_incognito);
}

void BrowserCommandController::UpdateCommandsForIncognitoAvailability() {
  if (is_locked_fullscreen_)
    return;

  UpdateSharedCommandsForIncognitoAvailability(&command_updater_, profile());

  if (!IsShowingMainUI()) {
    command_updater_.UpdateCommandEnabled(IDC_IMPORT_SETTINGS, false);
    command_updater_.UpdateCommandEnabled(IDC_OPTIONS, false);
  }
}

void BrowserCommandController::UpdateCommandsForTabState() {
  if (is_locked_fullscreen_)
    return;

  WebContents* current_web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();
  if (!current_web_contents)  // May be NULL during tab restore.
    return;

  // Navigation commands
  command_updater_.UpdateCommandEnabled(IDC_BACK, CanGoBack(browser_));
  command_updater_.UpdateCommandEnabled(IDC_FORWARD, CanGoForward(browser_));
  command_updater_.UpdateCommandEnabled(IDC_RELOAD, CanReload(browser_));
  command_updater_.UpdateCommandEnabled(IDC_RELOAD_BYPASSING_CACHE,
                                        CanReload(browser_));
  command_updater_.UpdateCommandEnabled(IDC_RELOAD_CLEARING_CACHE,
                                        CanReload(browser_));

  // Window management commands
  command_updater_.UpdateCommandEnabled(
      IDC_DUPLICATE_TAB,
      !browser_->deprecated_is_app() && CanDuplicateTab(browser_));
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_MUTE_SITE,
                                        !browser_->deprecated_is_app());
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_PIN_TAB,
                                        !browser_->deprecated_is_app());
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_GROUP_TAB,
                                        !browser_->deprecated_is_app());
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_CLOSE_TABS_TO_RIGHT,
                                        CanCloseTabsToRight(browser_));
  command_updater_.UpdateCommandEnabled(IDC_WINDOW_CLOSE_OTHER_TABS,
                                        CanCloseOtherTabs(browser_));
  command_updater_.UpdateCommandEnabled(IDC_MOVE_TAB_TO_NEW_WINDOW,
                                        CanMoveActiveTabToNewWindow(browser_));

  // Page-related commands
  window()->SetStarredState(
      BookmarkTabHelper::FromWebContents(current_web_contents)->is_starred());
  window()->ZoomChangedForActiveTab(false);
  command_updater_.UpdateCommandEnabled(IDC_VIEW_SOURCE,
                                        CanViewSource(browser_));
  command_updater_.UpdateCommandEnabled(IDC_EMAIL_PAGE_LOCATION,
                                        CanEmailPageLocation(browser_));
  if (browser_->is_type_devtools())
    command_updater_.UpdateCommandEnabled(IDC_OPEN_FILE, false);

#if defined(OS_WIN)
  command_updater_.UpdateCommandEnabled(IDC_SHARE,
                                        CanShareFromBrowser(browser_));
#endif

  // Note: The enable states for PWA menu items must be kept in sync with those
  // in AppsSubMenuModel::IsCommandIdEnabled. See VSTS bug
  // #25291353.
  const bool can_create_web_app = web_app::CanCreateWebApp(browser_);
  command_updater_.UpdateCommandEnabled(IDC_INSTALL_PWA, can_create_web_app);

  // owner: Edge\Browser Experience\SHIN - Shell Integration
  if (features::IsPinToTaskbarNoInstallEnabled()) {
    command_updater_.UpdateCommandEnabled(
        IDC_PIN_TO_TASKBAR, can_create_web_app);
  } else {
    command_updater_.UpdateCommandEnabled(
        IDC_CREATE_SHORTCUT, can_create_web_app);
  }

  // Note that additional logic in AppMenuModel::Build() controls the presence
  // of this command.
  const bool can_pop_out_web_app = web_app::CanPopOutWebApp(profile());
  command_updater_.UpdateCommandEnabled(IDC_OPEN_IN_PWA_WINDOW,
                                        can_pop_out_web_app);

  // Install/Run as Site commands
  command_updater_.UpdateCommandEnabled(IDC_INSTALL_SITE, can_create_web_app);
  command_updater_.UpdateCommandEnabled(IDC_OPEN_SITE, can_pop_out_web_app);

  command_updater_.UpdateCommandEnabled(
      IDC_TOGGLE_REQUEST_TABLET_SITE,
      CanRequestTabletSite(current_web_contents));

  UpdateCommandsForContentRestrictionState();
  UpdateCommandsForBookmarkEditing();
  UpdateCommandsForFind();
  UpdateCommandsForReadingList();

#if defined(MICROSOFT_EDGE_BUILD)
  UpdateCommandsForReadAloud();

#if defined(OS_WIN)
  // If IE tab is current tab, disable Collections toggle button.
  if (features::edge::IsCollectionsFeatureEnabled(
          features::edge::kEdgeCollections) &&
      browser_->profile()->GetPrefs()->GetBoolean(
          prefs::kEnableEdgeCollections) &&
      !browser_->profile()->IsOffTheRecord()) {
    command_updater_.UpdateCommandEnabled(
        IDC_TOGGLE_COLLECTIONS,
        !current_web_contents->IsHostingInternetExplorer());
  }
#endif
#endif
  UpdateCommandsForPDF();
  UpdateCommandsForMediaRouter();
  // Update the zoom commands when an active tab is selected.
  UpdateCommandsForZoomState();
  UpdateCommandsForTabKeyboardFocus(GetKeyboardFocusedTabIndex(browser_));
}

void BrowserCommandController::UpdateCommandsForZoomState() {
  WebContents* contents = browser_->tab_strip_model()->GetActiveWebContents();
  if (!contents)
    return;
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_PLUS, CanZoomIn(contents));
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_NORMAL,
                                        CanResetZoom(contents));
  command_updater_.UpdateCommandEnabled(IDC_ZOOM_MINUS, CanZoomOut(contents));
}

#if defined(MICROSOFT_EDGE_BUILD)
void BrowserCommandController::UpdateCommandsForReadAloud() {
  bool can_read_aloud = CanReadAloud(browser_);
  bool is_stopped = false;
  bool is_playing = false;
  if (can_read_aloud) {
    is_stopped = IsReadAloudStopped(browser_);
    is_playing = IsReadAloudPlaying(browser_);
  }
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_START, can_read_aloud && !is_playing);
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_RESUME,
                                        (can_read_aloud & !is_stopped));
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_PAUSE,
                                        (can_read_aloud & !is_stopped));
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_PREVIOUS,
                                        (can_read_aloud & is_playing));
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_NEXT,
                                        (can_read_aloud & is_playing));
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_STOP,
                                        (can_read_aloud & !is_stopped));
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_TOGGLE_START,
                                        can_read_aloud);
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_TOGGLE_VOICE_OPTIONS,
                                        can_read_aloud & !is_stopped);
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_TOGGLE_RESUME,
                                        can_read_aloud & !is_stopped);
  command_updater_.UpdateCommandEnabled(IDC_READ_ALOUD_TOGGLE_START_SHORTCUT,
                                        can_read_aloud);
}

#endif
void BrowserCommandController::UpdateCommandsForPDF() {
  bool is_pdf_content = IsPDFWebContents(browser_);
  command_updater_.UpdateCommandEnabled(IDC_FIT_TO_VIEW, is_pdf_content);
  command_updater_.UpdateCommandEnabled(IDC_ROTATE_CW, is_pdf_content);
}

void BrowserCommandController::UpdateCommandsForContentRestrictionState() {
  int restrictions = GetContentRestrictions(browser_);

  command_updater_.UpdateCommandEnabled(
      IDC_COPY, !(restrictions & CONTENT_RESTRICTION_COPY));
  command_updater_.UpdateCommandEnabled(
      IDC_CUT, !(restrictions & CONTENT_RESTRICTION_CUT));
  command_updater_.UpdateCommandEnabled(
      IDC_PASTE, !(restrictions & CONTENT_RESTRICTION_PASTE));
  UpdateSaveAsState();
  UpdatePrintingState();
}

void BrowserCommandController::UpdateCommandsForDevTools() {
  if (is_locked_fullscreen_)
    return;

  bool dev_tools_enabled = DevToolsWindow::AllowDevToolsFor(
      profile(), browser_->tab_strip_model()->GetActiveWebContents());
  command_updater_.UpdateCommandEnabled(IDC_DEV_TOOLS, dev_tools_enabled);
  command_updater_.UpdateCommandEnabled(IDC_DEV_TOOLS_CONSOLE,
                                        dev_tools_enabled);
  command_updater_.UpdateCommandEnabled(IDC_DEV_TOOLS_DEVICES,
                                        dev_tools_enabled);
  command_updater_.UpdateCommandEnabled(IDC_DEV_TOOLS_INSPECT,
                                        dev_tools_enabled);
  command_updater_.UpdateCommandEnabled(IDC_DEV_TOOLS_TOGGLE,
                                        dev_tools_enabled);
#if defined(OS_MACOSX)
  command_updater_.UpdateCommandEnabled(IDC_TOGGLE_JAVASCRIPT_APPLE_EVENTS,
                                        dev_tools_enabled);
#endif
}

void BrowserCommandController::UpdateCommandsForBookmarkEditing() {
  if (is_locked_fullscreen_)
    return;

  command_updater_.UpdateCommandEnabled(IDC_BOOKMARK_THIS_TAB,
#if BUILDFLAG(ENABLE_WDAG)
      (wdag::IsHostInstance() ||
           wdag::container::IsContainerPersistencePolicyEnabled()) &&
#endif
      CanBookmarkCurrentTab(browser_));
  command_updater_.UpdateCommandEnabled(IDC_BOOKMARK_ALL_TABS,
#if BUILDFLAG(ENABLE_WDAG)
      (wdag::IsHostInstance() ||
           wdag::container::IsContainerPersistencePolicyEnabled()) &&
#endif
      CanBookmarkAllTabs(browser_));

#if defined(OS_WIN)
  command_updater_.UpdateCommandEnabled(IDC_PIN_TO_START_SCREEN, true);
#endif
}

void BrowserCommandController::UpdateCommandsForBookmarkBar() {
  if (is_locked_fullscreen_)
    return;

  const bool common_reasons =
      browser_defaults::bookmarks_enabled && !profile()->IsGuestSession() &&
      !profile()->IsSystemProfile() && IsShowingMainUI();

  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_BOOKMARK_BAR,
      common_reasons && !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBar));

  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_BOOKMARK_BAR_ALWAYS,
      common_reasons &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBar) &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBarOnlyNTP));

  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_BOOKMARK_BAR_NEVER,
      common_reasons &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBar) &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBarOnlyNTP));

  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_BOOKMARK_BAR_ONLY_ON_NTP,
      common_reasons &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBar) &&
          !profile()->GetPrefs()->IsManagedPreference(
              bookmarks::prefs::kShowBookmarkBarOnlyNTP));
}

void BrowserCommandController::UpdateCommandsForFileSelectionDialogs() {
  if (is_locked_fullscreen_)
    return;

  UpdateSaveAsState();
  UpdateOpenFileState(&command_updater_);
}

void BrowserCommandController::UpdateCommandsForFullscreenMode() {
  if (is_locked_fullscreen_)
    return;

  const bool is_fullscreen = window() && window()->IsFullscreen();
  const bool show_main_ui = IsShowingMainUI();
  const bool show_location_bar = IsShowingLocationBar();
  const bool show_feedback =
      show_main_ui || browser_->is_type_devtools() ||
      (web_app::AppBrowserController::IsForWebAppBrowser(browser_) &&
       browser_->app_controller()->IsWebAppFeedbackEnabled());
  const bool main_not_fullscreen = show_main_ui && !is_fullscreen;

  // Navigation commands
  command_updater_.UpdateCommandEnabled(IDC_OPEN_CURRENT_URL, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_PASTE_AND_GO, show_location_bar);

  // Window management commands
  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_AS_TAB, !browser_->is_type_normal() && !is_fullscreen);

  // Focus various bits of UI
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_TOOLBAR, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_LOCATION, show_location_bar);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_SEARCH, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_MENU_BAR,
                                        main_not_fullscreen);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_NEXT_PANE,
                                        main_not_fullscreen);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_PREVIOUS_PANE,
                                        main_not_fullscreen);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_BOOKMARKS,
                                        main_not_fullscreen);
  command_updater_.UpdateCommandEnabled(
      IDC_FOCUS_INACTIVE_POPUP_FOR_ACCESSIBILITY, main_not_fullscreen);

  // Show various bits of UI
  command_updater_.UpdateCommandEnabled(IDC_DEVELOPER_MENU, show_main_ui);

  // Show Edge Feedback in menu item only when group policy allows.
  if (profile()->GetPrefs()->GetBoolean(prefs::kEdgeFeedbackEnabled)) {
    command_updater_.UpdateCommandEnabled(IDC_EDGE_FEEDBACK, show_feedback);
    command_updater_.UpdateCommandEnabled(IDC_EDGE_FEEDBACK_VIA_KEYBOARD,
      show_feedback);
  }

#if BUILDFLAG(GOOGLE_CHROME_BRANDING)
  command_updater_.UpdateCommandEnabled(
      IDC_FEEDBACK, show_main_ui || browser_->is_type_devtools());
#endif
  UpdateShowSyncState(show_main_ui);

  command_updater_.UpdateCommandEnabled(IDC_EDIT_SEARCH_ENGINES, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_VIEW_PASSWORDS, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_ABOUT, show_main_ui);
  // adding command to command_updater which will enable the command
  // if not added then command will be in disabled state
  command_updater_.UpdateCommandEnabled(IDC_SHOW_APP_MENU, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_SEND_TAB_TO_SELF, show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_SEND_TAB_TO_SELF_SINGLE_TARGET,
                                        show_main_ui);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_MANAGEMENT_PAGE, true);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_FAMILY_SAFETY_PAGE, true);
  command_updater_.UpdateCommandEnabled(IDC_READING_LIST_MOVED_TO_FAVORITES_TIP,
                                        true);

  if (base::debug::IsProfilingSupported())
    command_updater_.UpdateCommandEnabled(IDC_PROFILING_ENABLED, show_main_ui);

#if !defined(OS_MACOSX)
  // Disable toggling into fullscreen mode if disallowed by pref.
  const bool fullscreen_enabled =
      is_fullscreen ||
      profile()->GetPrefs()->GetBoolean(prefs::kFullscreenAllowed);
#else
  const bool fullscreen_enabled = true;
#endif

  command_updater_.UpdateCommandEnabled(IDC_FULLSCREEN, fullscreen_enabled);
  command_updater_.UpdateCommandEnabled(IDC_TOGGLE_FULLSCREEN_TOOLBAR,
                                        fullscreen_enabled);

  UpdateCommandsForBookmarkBar();
  UpdateCommandsForIncognitoAvailability();
  UpdateCommandsForHostedAppAvailability();
}

void BrowserCommandController::UpdateCommandsForHostedAppAvailability() {
  bool has_toolbar =
      browser_->is_type_normal() ||
      web_app::AppBrowserController::IsForWebAppBrowser(browser_);
  if (window() && window()->ShouldHideUIForFullscreen())
    has_toolbar = false;
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_TOOLBAR, has_toolbar);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_NEXT_PANE, has_toolbar);
  command_updater_.UpdateCommandEnabled(IDC_FOCUS_PREVIOUS_PANE, has_toolbar);
  command_updater_.UpdateCommandEnabled(IDC_SHOW_APP_MENU, has_toolbar);
}

#if defined(OS_CHROMEOS)
namespace {

#if DCHECK_IS_ON()
// Makes sure that all commands that are not whitelisted are disabled. DCHECKs
// otherwise. Compiled only in debug mode.
void NonWhitelistedCommandsAreDisabled(CommandUpdaterImpl* command_updater) {
  constexpr int kWhitelistedIds[] = {IDC_CUT, IDC_COPY, IDC_PASTE};

  // Go through all the command ids, skip the whitelisted ones.
  for (int id : command_updater->GetAllIds()) {
    if (base::Contains(kWhitelistedIds, id)) {
      continue;
    }
    DCHECK(!command_updater->IsCommandEnabled(id));
  }
}
#endif

}  // namespace

void BrowserCommandController::UpdateCommandsForLockedFullscreenMode() {
  bool is_locked_fullscreen =
      platform_util::IsBrowserLockedFullscreen(browser_);
  // Sanity check to make sure this function is called only on state change.
  DCHECK_NE(is_locked_fullscreen, is_locked_fullscreen_);
  if (is_locked_fullscreen == is_locked_fullscreen_)
    return;
  is_locked_fullscreen_ = is_locked_fullscreen;

  if (is_locked_fullscreen_) {
    command_updater_.DisableAllCommands();
    // Update the state of whitelisted commands:
    // IDC_CUT/IDC_COPY/IDC_PASTE,
    UpdateCommandsForContentRestrictionState();
    // TODO(crbug.com/904637): Re-enable Find and Zoom in locked fullscreen.
    // All other commands will be disabled (there is an early return in their
    // corresponding UpdateCommandsFor* functions).
#if DCHECK_IS_ON()
    NonWhitelistedCommandsAreDisabled(&command_updater_);
#endif
  } else {
    // Do an init call to re-initialize command state after the
    // DisableAllCommands.
    InitCommandState();
  }
}
#endif

void BrowserCommandController::UpdateQuickSearchState() {
  bool quicksearch_enabled = CanQuickSearch(browser_);
  command_updater_.UpdateCommandEnabled(IDC_QUICK_SEARCH, quicksearch_enabled);
}

void BrowserCommandController::UpdatePrintingState() {
  if (is_locked_fullscreen_)
    return;

  bool print_enabled = CanPrint(browser_);
  command_updater_.UpdateCommandEnabled(IDC_PRINT, print_enabled);
#if BUILDFLAG(ENABLE_PRINTING)
  command_updater_.UpdateCommandEnabled(IDC_BASIC_PRINT,
                                        CanBasicPrint(browser_));
#endif
}

void BrowserCommandController::UpdateSaveAsState() {
  if (is_locked_fullscreen_)
    return;

  command_updater_.UpdateCommandEnabled(IDC_SAVE_PAGE,
#if BUILDFLAG(ENABLE_WDAG)
      (wdag::IsHostInstance() ||
           wdag::container::IsContainerPersistencePolicyEnabled()) &&
#endif
      CanSavePage(browser_));
}

void BrowserCommandController::UpdateShowSyncState(bool show_main_ui) {
  if (is_locked_fullscreen_)
    return;

  command_updater_.UpdateCommandEnabled(
      IDC_SHOW_SIGNIN, show_main_ui && pref_signin_allowed_.GetValue());
}

// static
void BrowserCommandController::UpdateOpenFileState(
    CommandUpdater* command_updater) {
  bool enabled = true;
  PrefService* local_state = g_browser_process->local_state();
  if (local_state)
    enabled = local_state->GetBoolean(prefs::kAllowFileSelectionDialogs);

  command_updater->UpdateCommandEnabled(IDC_OPEN_FILE, enabled);
}

void BrowserCommandController::UpdateReloadStopState(bool is_loading,
                                                     bool force) {
  if (is_locked_fullscreen_)
    return;

  window()->UpdateReloadStopState(is_loading, force);
  command_updater_.UpdateCommandEnabled(IDC_STOP, is_loading);
  UpdateCloseFindOrStop();
}

void BrowserCommandController::UpdateTabRestoreCommandState() {
  if (is_locked_fullscreen_)
    return;

  sessions::TabRestoreService* tab_restore_service =
      TabRestoreServiceFactory::GetForProfile(profile());
  // The command is enabled if the service hasn't loaded yet to trigger loading.
  // The command is updated once the load completes.
  command_updater_.UpdateCommandEnabled(
      IDC_RESTORE_TAB,
      tab_restore_service &&
          (!tab_restore_service->IsLoaded() ||
           GetRestoreTabType(browser_) != TabStripModelDelegate::RESTORE_NONE));
}

void BrowserCommandController::UpdateCommandsForFind() {
  TabStripModel* model = browser_->tab_strip_model();
  bool enabled = !model->IsTabBlocked(model->active_index()) &&
                 !browser_->is_type_devtools();

  command_updater_.UpdateCommandEnabled(IDC_FIND, enabled);
  command_updater_.UpdateCommandEnabled(IDC_FIND_NEXT, enabled);
  command_updater_.UpdateCommandEnabled(IDC_FIND_PREVIOUS, enabled);
}

void BrowserCommandController::UpdateCommandsForReadingList() {
  // TODO: Add logic to figure out whether the content can be added to reading
  // list or not
  command_updater_.UpdateCommandEnabled(IDC_ADD_TO_READING_LIST, true);
}

void BrowserCommandController::UpdateCloseFindOrStop() {
  bool enabled = CanCloseFind(browser_) || IsCommandEnabled(IDC_STOP);
  command_updater_.UpdateCommandEnabled(IDC_CLOSE_FIND_OR_STOP, enabled);
}

void BrowserCommandController::UpdateCommandsForMediaRouter() {
  if (is_locked_fullscreen_)
    return;

  command_updater_.UpdateCommandEnabled(IDC_ROUTE_MEDIA,
                                        CanRouteMedia(browser_));
}

void BrowserCommandController::UpdateCommandsForTabKeyboardFocus(
    base::Optional<int> target_index) {
  command_updater_.UpdateCommandEnabled(
      IDC_DUPLICATE_TARGET_TAB, !browser_->deprecated_is_app() &&
                                    target_index.has_value() &&
                                    CanDuplicateTabAt(browser_, *target_index));
  const bool normal_window = browser_->is_type_normal();
  command_updater_.UpdateCommandEnabled(
      IDC_MUTE_TARGET_SITE, normal_window && target_index.has_value());
  command_updater_.UpdateCommandEnabled(
      IDC_PIN_TARGET_TAB, normal_window && target_index.has_value());
  command_updater_.UpdateCommandEnabled(
      IDC_GROUP_TARGET_TAB, normal_window && target_index.has_value());
}

#if defined(MICROSOFT_EDGE_BUILD) && defined(OS_WIN)
void BrowserCommandController::UpdateCommandsForPinningWizard() {
  bool isPinningWizardAllowed = true;
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    isPinningWizardAllowed =
        local_state->GetBoolean(prefs::kPinningWizardAllowed);
  }
  command_updater_.UpdateCommandEnabled(IDC_SHOW_PINNING_WIZARD,
                                        isPinningWizardAllowed);
}
#endif

void BrowserCommandController::AddInterstitialObservers(WebContents* contents) {
  interstitial_observers_.push_back(new InterstitialObserver(this, contents));
}

void BrowserCommandController::RemoveInterstitialObservers(
    WebContents* contents) {
  for (size_t i = 0; i < interstitial_observers_.size(); i++) {
    if (interstitial_observers_[i]->web_contents() != contents)
      continue;

    delete interstitial_observers_[i];
    interstitial_observers_.erase(interstitial_observers_.begin() + i);
    return;
  }
}

BrowserWindow* BrowserCommandController::window() {
  return browser_->window();
}

Profile* BrowserCommandController::profile() {
  return browser_->profile();
}

void BrowserCommandController::HandleDisabledCommandConfirmationOfAction(
      int command_id) {
  // Command is not supported, in which case we don't want to notify or
  // it is enabled in which case we don't need do anything.
  if (!SupportsCommand(command_id) || IsCommandEnabled(command_id)) {
    return;
  }

  switch (command_id) {
    // Navigation commands
    case IDC_BACK:
      if (browser_ && browser_->tab_strip_model()) {
        edge::a11y::RaisePageNavigationEvent(
            browser_->tab_strip_model()->GetActiveWebContents(),
            IDS_UNABLE_TO_BACK_NARRATION, L"CantGoBack");
      }
      break;
    case IDC_FORWARD:
      if (browser_ && browser_->tab_strip_model()) {
        edge::a11y::RaisePageNavigationEvent(
            browser_->tab_strip_model()->GetActiveWebContents(),
            IDS_UNABLE_TO_FORWARD_NARRATION, L"CantGoForward");
      }
      break;
    default:
      break;
  }
}

}  // namespace chrome
