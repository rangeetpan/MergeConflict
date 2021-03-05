// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/chrome_pages.h"

#include <stddef.h>

#include <memory>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/user_metrics.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/system/sys_info.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/apps/app_service/app_launch_params.h"
#include "chrome/browser/apps/app_service/app_service_proxy.h"
#include "chrome/browser/apps/app_service/app_service_proxy_factory.h"
#include "chrome/browser/apps/app_service/browser_app_launcher.h"
#include "chrome/browser/apps/app_service/launch_utils.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/web_applications/default_web_app_ids.h"
#include "chrome/browser/download/download_shelf.h"
#include "chrome/browser/edge_ui_features.h"
#include "chrome/browser/hub/hub_manager.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/share/share_system.h"
#include "chrome/browser/share/share_system_helpers.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
<<<<<<< HEAD
#include "chrome/browser/ui/ui_features.h"
=======
#include "chrome/browser/ui/web_applications/system_web_app_ui_utils.h"
>>>>>>> 16307825352720ae04d898f37efa5449ad68b606
#include "chrome/browser/ui/webui/bookmarks/bookmarks_ui.h"
#include "chrome/browser/ui/webui/settings/site_settings_helper.h"
#include "chrome/browser/web_applications/components/app_registrar.h"
#include "chrome/browser/web_applications/components/web_app_constants.h"
#include "chrome/browser/web_applications/components/web_app_id.h"
#include "chrome/browser/web_applications/components/web_app_provider_base.h"
#include "chrome/browser/web_applications/system_web_app_manager.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chromeos/login/login_state/login_state.h"
#include "chromeos/system/statistics_provider.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/common/constants.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_util.h"

#if defined(OS_CHROMEOS)
#include "base/metrics/histogram_functions.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "chrome/browser/ui/webui/settings/chromeos/app_management/app_management_uma.h"
#include "chrome/browser/ui/webui/settings/chromeos/constants/routes.mojom.h"
#include "chrome/browser/ui/webui/settings/chromeos/constants/routes_util.h"
#include "chrome/common/webui_url_constants.h"
#include "chromeos/constants/chromeos_features.h"
#include "components/version_info/version_info.h"
#else
#include "chrome/browser/ui/signin_view_controller.h"
#endif

#if !defined(OS_ANDROID)
#include "chrome/browser/signin/identity_manager_factory.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#endif

#if defined(OS_WIN)
#include "content/public/browser/browser_context.h"
#endif

using base::UserMetricsAction;

namespace chrome {

namespace microsoft_edge {
void NavigateToPage(const GURL& url,
              Browser* browser,
              Profile* profile,
              WindowOpenDisposition disposition) {
  NavigateParams params(profile, url, ui::PAGE_TRANSITION_TYPED);
  params.disposition = disposition;
  params.browser = browser;
  Navigate(&params);
}

void ShowPrivacyStatement(Browser* browser,
                          Profile* profile,
                          WindowOpenDisposition disposition) {
  const GURL url(chrome::kEdgePrivacyStatementURL);
  NavigateToPage(url, browser, profile, disposition);
}

void ShowPersonalizedDataCollectionLearnMore(Browser* browser,
                                             Profile* profile) {
  const GURL url(chrome::kEdgePersonalizedDataCollectionLearnMoreURL);
  NavigateToPage(url, browser, profile, WindowOpenDisposition::NEW_POPUP);
}
}  // namespace microsoft_edge

namespace {

const char kHashMark[] = "#";

void FocusWebContents(Browser* browser) {
  auto* const contents = browser->tab_strip_model()->GetActiveWebContents();
  if (contents)
    contents->Focus();
}

// Shows |url| in a tab in |browser|. If a tab is already open to |url|,
// ignoring the URL path, then that tab becomes selected. Overwrites the new tab
// page if it is open.
void ShowSingletonTabIgnorePathOverwriteNTP(Browser* browser, const GURL& url) {
  NavigateParams params(GetSingletonTabNavigateParams(browser, url));
  params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  ShowSingletonTabOverwritingNTP(browser, std::move(params));
}

void OpenBookmarkManagerForNode(Browser* browser, int64_t node_id) {
  const GURL url = GURL(kChromeUIBookmarksURL)
                       .Resolve(base::StringPrintf(
                           "/?id=%s", base::NumberToString(node_id).c_str()));
  ShowSingletonTabIgnorePathOverwriteNTP(browser, url);
}

#if defined(OS_CHROMEOS) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

const std::string BuildQueryString(Profile* profile) {
  const std::string board_name = base::SysInfo::GetLsbReleaseBoard();
  std::string region;
  chromeos::system::StatisticsProvider::GetInstance()->GetMachineStatistic(
      "region", &region);
  const std::string language = g_browser_process->GetApplicationLocale();
  const std::string version = version_info::GetVersionNumber();
  const std::string milestone = version_info::GetMajorVersionNumber();
  std::string channel_name =
      chrome::GetChannelName();  // beta, dev, canary, unknown, or empty string
                                 // for stable
  if (channel_name.empty())
    channel_name = "stable";
  const std::string username = profile->GetProfileUserName();
  std::string user_type;
  if (gaia::IsGoogleInternalAccountEmail(username)) {
    user_type = "googler";
  } else if (profile->GetProfilePolicyConnector()->IsManaged()) {
    user_type = "managed";
  } else {
    user_type = "general";
  }

  const std::string query_string = base::StrCat(
      {kChromeReleaseNotesURL, "?version=", milestone, "&tags=", board_name,
       ",", region, ",", language, ",", channel_name, ",", user_type});
  return query_string;
}

void LaunchReleaseNotesInTab(Profile* profile) {
  GURL url(BuildQueryString(profile));
  auto displayer = std::make_unique<ScopedTabbedBrowserDisplayer>(profile);
  ShowSingletonTab(displayer->browser(), url);
}

void LaunchReleaseNotesImpl(Profile* profile) {
  base::RecordAction(UserMetricsAction("ReleaseNotes.ShowReleaseNotes"));
  auto* provider = web_app::WebAppProviderBase::GetProviderBase(profile);
  if (provider && provider->registrar().IsInstalled(
                      chromeos::default_web_apps::kReleaseNotesAppId)) {
    web_app::DisplayMode display_mode =
        provider->registrar().GetAppEffectiveDisplayMode(
            chromeos::default_web_apps::kReleaseNotesAppId);
    apps::AppLaunchParams params = apps::CreateAppIdLaunchParamsWithEventFlags(
        chromeos::default_web_apps::kReleaseNotesAppId,
        /*event_flags=*/0, apps::mojom::AppLaunchSource::kSourceUntracked,
        /*display_id=*/-1,
        web_app::ConvertDisplayModeToAppLaunchContainer(display_mode));

    params.override_url = GURL(BuildQueryString(profile));
    apps::AppServiceProxyFactory::GetForProfile(profile)
        ->BrowserAppLauncher()
        .LaunchAppWithParams(params);
    return;
  }
  DVLOG(1) << "ReleaseNotes App Not Found";
  LaunchReleaseNotesInTab(profile);
}

#endif

// Shows either the help app or the appropriate help page for |source|. If
// |browser| is NULL and the help page is used (vs the app), the help page is
// shown in the last active browser. If there is no such browser, a new browser
// is created.
void ShowHelpImpl(Browser* browser, Profile* profile, HelpSource source) {
  base::RecordAction(UserMetricsAction("ShowHelpTab"));
#if defined(OS_CHROMEOS) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  auto app_launch_source = apps::mojom::LaunchSource::kUnknown;
  switch (source) {
    case HELP_SOURCE_KEYBOARD:
      app_launch_source = apps::mojom::LaunchSource::kFromKeyboard;
      break;
    case HELP_SOURCE_MENU:
      app_launch_source = apps::mojom::LaunchSource::kFromMenu;
      break;
    case HELP_SOURCE_WEBUI:
    case HELP_SOURCE_WEBUI_CHROME_OS:
      app_launch_source = apps::mojom::LaunchSource::kFromOtherApp;
      break;
    default:
      NOTREACHED() << "Unhandled help source" << source;
  }
  apps::AppServiceProxy* proxy =
      apps::AppServiceProxyFactory::GetForProfile(profile);
  DCHECK(proxy);

  const char* app_id =
      base::FeatureList::IsEnabled(chromeos::features::kHelpAppV2)
          ? chromeos::default_web_apps::kHelpAppId
          : extension_misc::kGeniusAppId;
  proxy->Launch(app_id, ui::EventFlags::EF_NONE, app_launch_source,
                display::kDefaultDisplayId);
#else
  GURL url;

  // Check to see if Policy provides a Custom help url.
  // The switch blocks for F1 and Help menu will
  // check for this value, and if present, will set the URL.
  // This effectively makes the policy override the default
  // and the UseNewHelpLinkUrls experiment (if active).
  Profile* original_profile = browser->profile()->GetOriginalProfile();
  PrefService* pref_service = original_profile->GetPrefs();
  std::string custom_help_link_policy_url =
      pref_service->GetString(prefs::kCustomHelpLink);

  // Edge Stable and Non-Stable have different help content/endpoints
  std::string help_keyboard_url(
      chrome::GetChannel() == version_info::Channel::STABLE
                             ? kChromeHelpViaKeyboardURLStable
                             : kChromeHelpViaKeyboardURL);

  std::string help_menu_url(
      chrome::GetChannel() == version_info::Channel::STABLE
                             ? kChromeHelpViaMenuURLStable
                             : kChromeHelpViaMenuURL);

  switch (source) {
    case HELP_SOURCE_KEYBOARD:
      if (base::FeatureList::IsEnabled(
              features::edge::kEdgeUseNewHelpLinkUrls)) {
        std::string help_link_f1_key_url =
            base::GetFieldTrialParamValueByFeature(
                features::edge::kEdgeUseNewHelpLinkUrls,
                features::edge::kEdgeNewHelpLinkF1KeyURL);
        if (!help_link_f1_key_url.empty()) {
          help_keyboard_url = help_link_f1_key_url;
        }
      }

      if (!custom_help_link_policy_url.empty()) {
        help_keyboard_url = custom_help_link_policy_url;
      }
      url = GURL(help_keyboard_url);
      break;
    case HELP_SOURCE_MENU:
      if (base::FeatureList::IsEnabled(
              features::edge::kEdgeUseNewHelpLinkUrls)) {
        std::string help_link_help_menu_url =
            base::GetFieldTrialParamValueByFeature(
                features::edge::kEdgeUseNewHelpLinkUrls,
                features::edge::kEdgeNewHelpLinkHelpMenuURL);
        if (!help_link_help_menu_url.empty()) {
          help_menu_url = help_link_help_menu_url;
        }
      }

      if (!custom_help_link_policy_url.empty()) {
        help_menu_url = custom_help_link_policy_url;
      }
      url = GURL(help_menu_url);
      break;
#if defined(OS_CHROMEOS)
    case HELP_SOURCE_WEBUI:
      url = GURL(kChromeHelpViaWebUIURL);
      break;
    case HELP_SOURCE_WEBUI_CHROME_OS:
      url = GURL(kChromeOsHelpViaWebUIURL);
      break;
#else
    case HELP_SOURCE_WEBUI:
      url = GURL(kChromeHelpViaWebUIURL);
      break;
#endif
    default:
      NOTREACHED() << "Unhandled help source " << source;
  }
  std::unique_ptr<ScopedTabbedBrowserDisplayer> displayer;
  if (!browser) {
    displayer = std::make_unique<ScopedTabbedBrowserDisplayer>(profile);
    browser = displayer->browser();
  }
  ShowSingletonTab(browser, url);
#endif
}

std::string GenerateContentSettingsExceptionsSubPage(ContentSettingsType type) {
  // In MD Settings, the exceptions no longer have a separate subpage.
  // This list overrides the group names defined in site_settings_helper for the
  // purposes of URL generation for MD Settings only. We need this because some
  // of the old group names are no longer appropriate: i.e. "plugins" =>
  // "flash".
  //
  // TODO(crbug.com/728353): Update the group names defined in
  // site_settings_helper once Options is removed from Chrome. Then this list
  // will no longer be needed.
  static base::NoDestructor<std::map<ContentSettingsType, std::string>>
      kSettingsPathOverrides(
          {{ContentSettingsType::AUTOMATIC_DOWNLOADS, "automaticDownloads"},
           {ContentSettingsType::BACKGROUND_SYNC, "backgroundSync"},
           {ContentSettingsType::MEDIASTREAM_MIC, "microphone"},
           {ContentSettingsType::MEDIASTREAM_CAMERA, "camera"},
           {ContentSettingsType::MIDI_SYSEX, "midiDevices"},
           {ContentSettingsType::PLUGINS, "flash"},
           {ContentSettingsType::ADS, "ads"},
           {ContentSettingsType::PPAPI_BROKER, "unsandboxedPlugins"}});
  const auto it = kSettingsPathOverrides->find(type);
  const std::string content_type_path =
      (it == kSettingsPathOverrides->end())
          ? site_settings::ContentSettingsTypeToGroupName(type)
          : it->second;

  return std::string(kContentSettingsSubPage) + "/" + content_type_path;
}

void ShowSiteSettingsImpl(Browser* browser, Profile* profile, const GURL& url) {
  // If a valid non-file origin, open a settings page specific to the current
  // origin of the page. Otherwise, open Content Settings.
  url::Origin site_origin = url::Origin::Create(url);
  std::string link_destination(chrome::kChromeUIContentSettingsURL);
  // TODO(https://crbug.com/444047): Site Details should work with file:// urls
  // when this bug is fixed, so add it to the whitelist when that happens.
  if (!site_origin.opaque() && (url.SchemeIsHTTPOrHTTPS() ||
                                url.SchemeIs(extensions::kExtensionScheme))) {
    std::string origin_string = site_origin.Serialize();
    url::RawCanonOutputT<char> percent_encoded_origin;
    url::EncodeURIComponent(origin_string.c_str(), origin_string.length(),
                            &percent_encoded_origin);
    link_destination = chrome::kChromeUISiteDetailsPrefixURL +
                       std::string(percent_encoded_origin.data(),
                                   percent_encoded_origin.length());
  }
  NavigateParams params(profile, GURL(link_destination),
                        ui::PAGE_TRANSITION_TYPED);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.browser = browser;
  Navigate(&params);
}

}  // namespace

void ShowBookmarkManager(Browser* browser) {
  base::RecordAction(UserMetricsAction("ShowBookmarkManager"));
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(kChromeUIBookmarksURL));
}

void ShowBookmarkManagerRemoveDuplicates(Browser* browser) {
  std::string remove_duplicates_url = std::string(kChromeUIBookmarksURL) +
                            std::string(kChromeUIBookmarksRemoveDuplicatesPath);
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(remove_duplicates_url));
}

void ShowBookmarkManagerForNode(Browser* browser, int64_t node_id) {
  base::RecordAction(UserMetricsAction("ShowBookmarkManager"));
  OpenBookmarkManagerForNode(browser, node_id);
}

void ShowHistory(Browser* browser) {
  base::RecordAction(UserMetricsAction("ShowHistory"));
  if (base::FeatureList::IsEnabled(features::edge::kHub)) {
    if (!HubManager::IsVisible(browser, HubContentID::History)) {
      BrowserWindow* activeWindow = browser->window();
      if (activeWindow) {
        activeWindow->ShowHubPopup(HubContentID::History);
      }
    }
    return;
  }

  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(kChromeUIHistoryURL));
}

void ShowDownloads(Browser* browser) {
  base::RecordAction(UserMetricsAction("ShowDownloads"));
  if (browser->window() && browser->window()->IsDownloadShelfVisible())
    browser->window()->GetDownloadShelf()->Close(DownloadShelf::USER_ACTION);

  if (base::FeatureList::IsEnabled(features::edge::kHub)) {
    if (!HubManager::IsVisible(browser, HubContentID::Downloads)) {
      BrowserWindow* activeWindow = browser->window();
      if (activeWindow) {
        activeWindow->ShowHubPopup(HubContentID::Downloads);
      }
    }
    return;
  }

  // If we aren't an IE tab, then we should just do the same thing we always do!
  #if defined(OS_WIN)
  content::WebContents* current_tab = browser->tab_strip_model()->GetActiveWebContents();
  if (current_tab &&
    current_tab->IsHostingInternetExplorer()) {
      content::BrowserContext::ShowInternetExplorerDownloads(current_tab->GetBrowserContext());
  }
  else
  {
    ShowSingletonTabOverwritingNTP(
        browser,
        GetSingletonTabNavigateParams(browser, GURL(kChromeUIDownloadsURL)));
  }
  #else
  ShowSingletonTabOverwritingNTP(
    browser,
    GetSingletonTabNavigateParams(browser, GURL(kChromeUIDownloadsURL)));
  #endif
}

void ShowAppsManager(Browser* browser) {
  base::RecordAction(UserMetricsAction("ShowAppsManager"));
  NavigateParams params(
      GetSingletonTabNavigateParams(browser, GURL(kChromeUIAppsURL)));
  params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  ShowSingletonTabOverwritingNTP(browser, std::move(params));
}

void ShowExtensions(Browser* browser,
                    const std::string& extension_to_highlight) {
  base::RecordAction(UserMetricsAction("ShowExtensions"));
  GURL url(kChromeUIExtensionsURL);
  if (!extension_to_highlight.empty()) {
    GURL::Replacements replacements;
    std::string query("id=");
    query += extension_to_highlight;
    replacements.SetQueryStr(query);
    url = url.ReplaceComponents(replacements);
  }
  ShowSingletonTabIgnorePathOverwriteNTP(browser, url);
}

void ShowShare(Browser* browser) {
  #if defined(OS_WIN)
    // Make sharesystem a scoped ptr which can be retained in callback.
    auto sharesystem = base::MakeRefCounted<ShareSystem>(
        browser, base::BindOnce(&share_system::helper::CallbackOnShareResult));
    sharesystem->GetMetadataForTab();
  #endif
}

void ShowEdgeShareCopyPaste(Browser* browser) {
  base::RecordAction(UserMetricsAction("ShowEdgeShareCopyPaste"));
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(kEdgeShareCopyPasteURL));
}

void ShowHelp(Browser* browser, HelpSource source) {
  ShowHelpImpl(browser, browser->profile(), source);
}

void ShowHelpForProfile(Profile* profile, HelpSource source) {
  ShowHelpImpl(NULL, profile, source);
}

void ShowWhatsNewAndTips(Browser* browser) {
  ShowSingletonTab(browser,
                   chrome::GetChannel() == version_info::Channel::STABLE
                       ? GURL(kChromeWhatsNewAndTipsStable)
                       : GURL(kChromeWhatsNewAndTips));
}

void LaunchReleaseNotes(Profile* profile) {
#if defined(OS_CHROMEOS) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  LaunchReleaseNotesImpl(profile);
#endif
}

void ShowBetaForum(Browser* browser) {
  ShowSingletonTab(browser, GURL(kChromeBetaForumURL));
}

void ShowPolicy(Browser* browser) {
  ShowSingletonTab(browser, GURL(kChromeUIPolicyURL));
}

void ShowSlow(Browser* browser) {
#if defined(OS_CHROMEOS)
  ShowSingletonTab(browser, GURL(kChromeUISlowURL));
#endif
}

GURL GetSettingsUrl(const std::string& sub_page) {
  return GURL(std::string(kChromeUISettingsURL) + sub_page);
}

bool IsTrustedPopupWindowWithScheme(const Browser* browser,
                                    const std::string& scheme) {
  if (browser->is_type_normal() || !browser->is_trusted_source())
    return false;
  if (scheme.empty())  // Any trusted popup window
    return true;
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetWebContentsAt(0);
  if (!web_contents)
    return false;
  GURL url(web_contents->GetURL());
  return url.SchemeIs(scheme);
}

void ShowSettings(Browser* browser) {
  ShowSettingsSubPage(browser, std::string());
}

void ShowSettingsSubPage(Browser* browser, const std::string& sub_page) {
#if defined(OS_CHROMEOS)
  ShowSettingsSubPageForProfile(browser->profile(), sub_page);
#else
  ShowSettingsSubPageInTabbedBrowser(browser, sub_page);
#endif
}

void ShowSettingsSubPageForProfile(Profile* profile,
                                   const std::string& sub_page) {
#if defined(OS_CHROMEOS)
  // OS settings sub-pages are handled else where and should never be
  // encountered here.
  DCHECK(!chromeos::settings::IsOSSettingsSubPage(sub_page)) << sub_page;
#endif
  Browser* browser = chrome::FindTabbedBrowser(profile, false);
  if (!browser)
    browser = new Browser(Browser::CreateParams(profile, true));
  ShowSettingsSubPageInTabbedBrowser(browser, sub_page);
}

void ShowSettingsSubPageInTabbedBrowser(Browser* browser,
                                        const std::string& sub_page) {
  base::RecordAction(UserMetricsAction("ShowOptions"));

  // Since the user may be triggering navigation from another UI element such as
  // a menu, ensure the web contents (and therefore the settings page that is
  // about to be shown) is focused. (See crbug/926492 for motivation.)
  FocusWebContents(browser);
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GetSettingsUrl(sub_page));
}

void ShowContentSettingsExceptions(Browser* browser,
                                   ContentSettingsType content_settings_type) {
  ShowSettingsSubPage(
      browser, GenerateContentSettingsExceptionsSubPage(content_settings_type));
}

void ShowContentSettingsExceptionsForProfile(
    Profile* profile,
    ContentSettingsType content_settings_type) {
  ShowSettingsSubPageForProfile(
      profile, GenerateContentSettingsExceptionsSubPage(content_settings_type));
}

void ShowSiteSettings(Browser* browser, const GURL& url) {
  ShowSiteSettingsImpl(browser, browser->profile(), url);
}

void ShowSiteSettings(Profile* profile, const GURL& url) {
  DCHECK(profile);
  ShowSiteSettingsImpl(nullptr, profile, url);
}

void ShowContentSettings(Browser* browser,
                         ContentSettingsType content_settings_type) {
  ShowSettingsSubPage(
      browser,
      kContentSettingsSubPage + std::string(kHashMark) +
          site_settings::ContentSettingsTypeToGroupName(content_settings_type));
}

void ShowClearBrowsingDataDialog(Browser* browser) {
  base::RecordAction(UserMetricsAction("ClearBrowsingData_ShowDlg"));
  ShowSettingsSubPage(browser, kClearBrowserDataSubPage);
}

void ShowPasswordManager(Browser* browser) {
  base::RecordAction(UserMetricsAction("Options_ShowPasswordManager"));
  ShowSettingsSubPage(browser, kPasswordManagerSubPage);
}

void ShowPasswordCheck(Browser* browser) {
  base::RecordAction(UserMetricsAction("Options_ShowPasswordCheck"));
  ShowSettingsSubPage(browser, kPasswordCheckSubPage);
}

void ShowImportDialog(Browser* browser) {
  base::RecordAction(UserMetricsAction("Import_ShowDlg"));
  ShowSettingsSubPage(browser, kImportDataSubPage);
}

void ShowAboutChrome(Browser* browser) {
  base::RecordAction(UserMetricsAction("AboutChrome"));
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(kChromeUIHelpURL));
}

void ShowSearchEngineSettings(Browser* browser) {
  base::RecordAction(UserMetricsAction("EditSearchEngines"));
  ShowSettingsSubPage(browser, kSearchEnginesSubPage);
}

#if defined(OS_CHROMEOS)
void ShowEnterpriseManagementPageInTabbedBrowser(Browser* browser) {
  // Management shows in a tab because it has a "back" arrow that takes the
  // user to the Chrome browser about page, which is part of browser settings.
  ShowSingletonTabIgnorePathOverwriteNTP(browser, GURL(kChromeUIManagementURL));
}

void ShowAppManagementPage(Profile* profile,
                           const std::string& app_id,
                           AppManagementEntryPoint entry_point) {
  // This histogram is also declared and used at chrome/browser/resources/
  // settings/chrome_os/os_apps_page/app_management_page/constants.js.
  constexpr char kAppManagementEntryPointsHistogramName[] =
      "AppManagement.EntryPoints";

  base::UmaHistogramEnumeration(kAppManagementEntryPointsHistogramName,
                                entry_point);
  std::string sub_page = base::StrCat(
      {chromeos::settings::mojom::kAppDetailsSubpagePath, "?id=", app_id});
  chrome::SettingsWindowManager::GetInstance()->ShowOSSettings(profile,
                                                               sub_page);
}

void ShowPrintManagementApp(Profile* profile) {
  DCHECK(
      base::FeatureList::IsEnabled(chromeos::features::kPrintJobManagementApp));
  LaunchSystemWebApp(profile, web_app::SystemAppType::PRINT_MANAGEMENT,
                     GURL(chrome::kChromeUIPrintManagementUrl));
}

GURL GetOSSettingsUrl(const std::string& sub_page) {
  DCHECK(sub_page.empty() || chromeos::settings::IsOSSettingsSubPage(sub_page))
      << sub_page;
  std::string url = kChromeUIOSSettingsURL;
  return GURL(url + sub_page);
}
#endif

#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
void ShowBrowserSignin(Browser* browser,
                       signin_metrics::AccessPoint access_point) {
  Profile* original_profile = browser->profile()->GetOriginalProfile();
  DCHECK(original_profile->GetPrefs()->GetBoolean(prefs::kSigninAllowed));

  // If the browser's profile is an incognito profile, make sure to use
  // a browser window from the original profile. The user cannot sign in
  // from an incognito window.
  auto displayer =
      std::make_unique<ScopedTabbedBrowserDisplayer>(original_profile);
  browser = displayer->browser();

  profiles::BubbleViewMode bubble_view_mode =
      IdentityManagerFactory::GetForProfile(original_profile)
              ->HasPrimaryAccount()
          ? profiles::BUBBLE_VIEW_MODE_GAIA_REAUTH
          : profiles::BUBBLE_VIEW_MODE_GAIA_SIGNIN;
  browser->signin_view_controller()->ShowSignin(bubble_view_mode, access_point);
}

void ShowBrowserSigninOrSettings(Browser* browser,
                                 signin_metrics::AccessPoint access_point) {
  Profile* original_profile = browser->profile()->GetOriginalProfile();
  DCHECK(original_profile->GetPrefs()->GetBoolean(prefs::kSigninAllowed));
  if (IdentityManagerFactory::GetForProfile(original_profile)
          ->HasPrimaryAccount())
    ShowSettings(browser);
  else
    ShowBrowserSignin(browser, access_point);
}
#endif

#if BUILDFLAG(ENABLE_SMARTSCREEN)
void ShowDownloadsSubPage(Browser* browser,
                          const std::string& sub_page,
                          const int download_id) {
  base::RecordAction(UserMetricsAction("ShowDownloads"));
  auto* window = browser->window();
  if (window && window->IsDownloadShelfVisible())
    window->GetDownloadShelf()->Close(DownloadShelf::USER_ACTION);

  const std::string query_parameter =
      std::string("?id=") + base::NumberToString(download_id);
  const GURL url =
      GURL(std::string(kChromeUIDownloadsURL) + sub_page + query_parameter);

  NavigateParams params(GetSingletonTabNavigateParams(browser, url));
  // Because we use URL query parameters to identify which download the modal is
  // referring to, we should also take query parameters into consideration when
  // comparing this URL with other tab URLs.
  params.should_ignore_url_query = false;

  ShowSingletonTabOverwritingNTP(browser, std::move(params));
}

void ShowDownloadsAppRepModal(Browser* browser, const int download_id) {
  ShowDownloadsSubPage(browser, std::string(kDownloadsAppRepSubPage),
                       download_id);
}

void ShowDownloadsUrlRepModal(Browser* browser, const int download_id) {
  ShowDownloadsSubPage(browser, std::string(kDownloadsUrlRepSubPage),
                       download_id);
}

void ShowDownloadsPuaRepModal(Browser* browser, const int download_id) {
  ShowDownloadsSubPage(browser, std::string(kDownloadsPuaRepSubPage),
                       download_id);
}
#endif // BUILDFLAG(ENABLE_SMARTSCREEN)

}  // namespace chrome
