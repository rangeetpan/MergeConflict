// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/tab_specific_content_settings.h"

#include <list>
#include <vector>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
<<<<<<< HEAD
#include "chrome/browser/browser_process.h"
=======
#include "chrome/browser/browsing_data/browsing_data_file_system_util.h"
>>>>>>> b05fa3a2a40082ce5fa4c82e4c80e0690d59ec98
#include "chrome/browser/browsing_data/cookies_tree_model.h"
#include "chrome/browser/content_settings/chrome_content_settings_utils.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/edge_trust/trust_manager/manager.h"
#include "chrome/browser/edge_trust/trust_utils.h"
#include "chrome/browser/media/webrtc/media_capture_devices_dispatcher.h"
#include "chrome/browser/media/webrtc/media_stream_capture_indicator.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/renderer_configuration.mojom.h"
#include "components/browsing_data/content/appcache_helper.h"
#include "components/browsing_data/content/cookie_helper.h"
#include "components/browsing_data/content/database_helper.h"
#include "components/browsing_data/content/file_system_helper.h"
#include "components/browsing_data/content/indexed_db_helper.h"
#include "components/browsing_data/content/local_storage_helper.h"
#include "components/content_settings/common/content_settings_agent.mojom.h"
#include "components/content_settings/core/browser/content_settings_details.h"
#include "components/content_settings/core/browser/content_settings_info.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/edge_trust/common/features.h"
#include "components/edge_trust/tracking_protection/options.h"
#include "components/edge_trust/tracking_protection/tracking_checker.h"
#include "components/prefs/pref_service.h"
#include "components/security_state/core/security_state_pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_constants.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/cookies/canonical_cookie.h"
#include "storage/common/file_system/file_system_types.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/origin.h"

using content::BrowserThread;
using content::NavigationController;
using content::NavigationEntry;
using content::WebContents;

namespace {

static TabSpecificContentSettings* GetForWCGetter(
    const base::Callback<content::WebContents*(void)>& wc_getter) {
  WebContents* web_contents = wc_getter.Run();
  if (!web_contents)
    return nullptr;

  return TabSpecificContentSettings::FromWebContents(web_contents);
}

bool ShouldSendUpdatedContentSettingsRulesToRenderer(
    ContentSettingsType content_type) {
  // ContentSettingsType::DEFAULT signals that multiple content settings may
  // have been updated, e.g. by the PolicyProvider. This should always be sent
  // to the renderer in case a relevant setting is updated.
  if (content_type == ContentSettingsType::DEFAULT)
    return true;

  return RendererContentSettingRules::IsRendererContentSetting((content_type));
}

}  // namespace

TabSpecificContentSettings::SiteDataObserver::SiteDataObserver(
    TabSpecificContentSettings* tab_specific_content_settings)
    : tab_specific_content_settings_(tab_specific_content_settings) {
  tab_specific_content_settings_->AddSiteDataObserver(this);
}

TabSpecificContentSettings::SiteDataObserver::~SiteDataObserver() {
  if (tab_specific_content_settings_)
    tab_specific_content_settings_->RemoveSiteDataObserver(this);
}

void TabSpecificContentSettings::SiteDataObserver::ContentSettingsDestroyed() {
  tab_specific_content_settings_ = NULL;
}

TabSpecificContentSettings::TabSpecificContentSettings(WebContents* tab)
    : content::WebContentsObserver(tab),
#if defined(OS_WIN)
      content::WebContentsDualEngineBrowserObserver(tab),
#endif
      map_(HostContentSettingsMapFactory::GetForProfile(
          Profile::FromBrowserContext(tab->GetBrowserContext()))),
      allowed_local_shared_objects_(
          Profile::FromBrowserContext(tab->GetBrowserContext()),
          browsing_data_file_system_util::GetAdditionalFileSystemTypes()),
      blocked_local_shared_objects_(
          Profile::FromBrowserContext(tab->GetBrowserContext()),
          browsing_data_file_system_util::GetAdditionalFileSystemTypes()),
      geolocation_usages_state_(map_, ContentSettingsType::GEOLOCATION),
      midi_usages_state_(map_, ContentSettingsType::MIDI_SYSEX),
      pending_protocol_handler_(ProtocolHandler::EmptyProtocolHandler()),
      previous_protocol_handler_(ProtocolHandler::EmptyProtocolHandler()),
      pending_protocol_handler_setting_(CONTENT_SETTING_DEFAULT),
      load_plugins_link_enabled_(true),
      microphone_camera_state_(MICROPHONE_CAMERA_NOT_ACCESSED) {
  ClearContentSettingsExceptForNavigationRelatedSettings();
  ClearNavigationRelatedContentSettings();

  observer_.Add(map_);
}

TabSpecificContentSettings::~TabSpecificContentSettings() {
  for (SiteDataObserver& observer : observer_list_)
    observer.ContentSettingsDestroyed();
}

TabSpecificContentSettings* TabSpecificContentSettings::GetForFrame(
    int render_process_id,
    int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::RenderFrameHost* frame = content::RenderFrameHost::FromID(
      render_process_id, render_frame_id);
  WebContents* web_contents = WebContents::FromRenderFrameHost(frame);
  if (!web_contents)
    return nullptr;

  return TabSpecificContentSettings::FromWebContents(web_contents);
}

// static
void TabSpecificContentSettings::TrackerBlocked(
    const base::Callback<content::WebContents*(void)>& wc_getter,
    const GURL& url,
    const GURL& first_party_url,
    tracking_prevention::TrackerType tracker_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  WebContents* web_contents = wc_getter.Run();
  if (!web_contents) {
    return;
  }

  // Make sure our first party url of this object is same origin with
  // |first_party_url|. If it's not then we can no-op this report. There is
  // still the potential for us to be navigating within the same-origin but
  // different documents where we might show the wrong tracker count. This is
  // less bad as it is still the same origin that is generating the report.
  // Ultimately this is an architecture issue upstream that affects all
  // cookie/storage reporting on the PageInfo flyout though.
  // https://crbug.com/1010367 is tracking the issue.
  GURL current_page_url = web_contents->GetVisibleURL();

  // If we ended up redirecting to an edge:// url we may have a real url behind
  // this. We'll use that instead.
  if (current_page_url.scheme() == "edge") {
    current_page_url = web_contents->GetOriginalRequestURL();
  }

  url::Origin current_page_origin = url::Origin::Create(current_page_url);
  if (!current_page_origin.IsSameOriginWith(
          url::Origin::Create(first_party_url))) {
    return;
  }

  TabSpecificContentSettings* settings = GetForWCGetter(wc_getter);
  if (settings) {
    settings->OnTrackerBlocked(url, first_party_url, tracker_type);
  }

  tracking_prevention::DisplayDevToolsBlockedTrackerMessage(
      wc_getter, tracker_type, url);
}

// static
void TabSpecificContentSettings::WebDatabaseAccessed(
    int render_process_id,
    int render_frame_id,
    const GURL& url,
    bool blocked_by_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings = GetForFrame(
      render_process_id, render_frame_id);
  if (settings)
    settings->OnWebDatabaseAccessed(url, blocked_by_policy);
}

// static
void TabSpecificContentSettings::IndexedDBAccessed(
    int render_process_id,
    int render_frame_id,
    const GURL& url,
    bool blocked_by_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings = GetForFrame(
      render_process_id, render_frame_id);
  if (settings)
    settings->OnIndexedDBAccessed(url, blocked_by_policy);
}

// static
void TabSpecificContentSettings::CacheStorageAccessed(int render_process_id,
                                                      int render_frame_id,
                                                      const GURL& url,
                                                      bool blocked_by_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings =
      GetForFrame(render_process_id, render_frame_id);
  if (settings)
    settings->OnCacheStorageAccessed(url, blocked_by_policy);
}

// static
void TabSpecificContentSettings::FileSystemAccessed(int render_process_id,
                                                    int render_frame_id,
                                                    const GURL& url,
                                                    bool blocked_by_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings = GetForFrame(
      render_process_id, render_frame_id);
  if (settings)
    settings->OnFileSystemAccessed(url, blocked_by_policy);
}

// static
void TabSpecificContentSettings::ServiceWorkerAccessed(
    const base::Callback<content::WebContents*(void)>& wc_getter,
    const GURL& scope,
    bool blocked_by_policy_javascript,
    bool blocked_by_policy_cookie) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings = GetForWCGetter(wc_getter);
  if (settings)
    settings->OnServiceWorkerAccessed(scope, blocked_by_policy_javascript,
                                      blocked_by_policy_cookie);
}

// static
void TabSpecificContentSettings::SharedWorkerAccessed(
    int render_process_id,
    int render_frame_id,
    const GURL& worker_url,
    const std::string& name,
    const url::Origin& constructor_origin,
    bool blocked_by_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TabSpecificContentSettings* settings =
      GetForFrame(render_process_id, render_frame_id);
  if (settings)
    settings->OnSharedWorkerAccessed(worker_url, name, constructor_origin,
                                     blocked_by_policy);
}

bool TabSpecificContentSettings::IsContentBlocked(
    ContentSettingsType content_type) const {
  DCHECK_NE(ContentSettingsType::GEOLOCATION, content_type)
      << "Geolocation settings handled by ContentSettingGeolocationImageModel";
  DCHECK_NE(ContentSettingsType::NOTIFICATIONS, content_type)
      << "Notifications settings handled by "
      << "ContentSettingsNotificationsImageModel";
  DCHECK_NE(ContentSettingsType::AUTOMATIC_DOWNLOADS, content_type)
      << "Automatic downloads handled by DownloadRequestLimiter";

  if (content_type == ContentSettingsType::IMAGES ||
      content_type == ContentSettingsType::JAVASCRIPT ||
      content_type == ContentSettingsType::PLUGINS ||
      content_type == ContentSettingsType::COOKIES ||
      content_type == ContentSettingsType::POPUPS ||
      content_type == ContentSettingsType::MIXEDSCRIPT ||
      content_type == ContentSettingsType::MEDIASTREAM_MIC ||
      content_type == ContentSettingsType::MEDIASTREAM_CAMERA ||
      content_type == ContentSettingsType::PPAPI_BROKER ||
      content_type == ContentSettingsType::MIDI_SYSEX ||
      content_type == ContentSettingsType::ADS ||
      content_type == ContentSettingsType::SOUND ||
      content_type == ContentSettingsType::CLIPBOARD_READ_WRITE ||
      content_type == ContentSettingsType::SENSORS ||
      (content_type == ContentSettingsType::TRACKERS &&
       features::edge::IsEnhancedTrackingPreventionEnabled())) {
    const auto& it = content_settings_status_.find(content_type);
    if (it != content_settings_status_.end())
      return it->second.blocked;
  }

  return false;
}

bool TabSpecificContentSettings::IsContentAllowed(
    ContentSettingsType content_type) const {
  DCHECK_NE(ContentSettingsType::AUTOMATIC_DOWNLOADS, content_type)
      << "Automatic downloads handled by DownloadRequestLimiter";

  // This method currently only returns meaningful values for the content type
  // cookies, media, PPAPI broker, downloads, MIDI sysex, clipboard, and
  // sensors.
  if (content_type != ContentSettingsType::COOKIES &&
      content_type != ContentSettingsType::MEDIASTREAM_MIC &&
      content_type != ContentSettingsType::MEDIASTREAM_CAMERA &&
      content_type != ContentSettingsType::PPAPI_BROKER &&
      content_type != ContentSettingsType::MIDI_SYSEX &&
      content_type != ContentSettingsType::CLIPBOARD_READ_WRITE &&
      content_type != ContentSettingsType::SENSORS) {
    return false;
  }

  const auto& it = content_settings_status_.find(content_type);
  if (it != content_settings_status_.end())
    return it->second.allowed;
  return false;
}

void TabSpecificContentSettings::OnContentBlocked(ContentSettingsType type) {
  DCHECK(type != ContentSettingsType::GEOLOCATION)
      << "Geolocation settings handled by OnGeolocationPermissionSet";
  DCHECK(type != ContentSettingsType::MEDIASTREAM_MIC &&
         type != ContentSettingsType::MEDIASTREAM_CAMERA)
      << "Media stream settings handled by OnMediaStreamPermissionSet";
  if (!content_settings::ContentSettingsRegistry::GetInstance()->Get(type))
    return;

  ContentSettingsStatus& status = content_settings_status_[type];

  if (!status.blocked) {
    status.blocked = true;
    content_settings::UpdateLocationBarUiForWebContents(web_contents());

    if (type == ContentSettingsType::PLUGINS) {
      content_settings::RecordPluginsAction(
          content_settings::PLUGINS_ACTION_DISPLAYED_BLOCKED_ICON_IN_OMNIBOX);
    } else if (type == ContentSettingsType::POPUPS) {
      content_settings::RecordPopupsAction(
          content_settings::POPUPS_ACTION_DISPLAYED_BLOCKED_ICON_IN_OMNIBOX);
    }
  }
}

void TabSpecificContentSettings::OnContentAllowed(ContentSettingsType type) {
  DCHECK(type != ContentSettingsType::GEOLOCATION)
      << "Geolocation settings handled by OnGeolocationPermissionSet";
  DCHECK(type != ContentSettingsType::MEDIASTREAM_MIC &&
         type != ContentSettingsType::MEDIASTREAM_CAMERA)
      << "Media stream settings handled by OnMediaStreamPermissionSet";
  bool access_changed = false;
  ContentSettingsStatus& status = content_settings_status_[type];

  // Whether to reset status for the |blocked| setting to avoid ending up
  // with both |allowed| and |blocked| set, which can mean multiple things
  // (allowed setting that got disabled, disabled setting that got enabled).
  bool must_reset_blocked_status = false;

  // For sensors, the status with both allowed/blocked flags set means that
  // access was previously allowed but the last decision was to block.
  // Reset the blocked flag so that the UI will properly indicate that the
  // last decision here instead was to allow sensor access.
  if (type == ContentSettingsType::SENSORS)
    must_reset_blocked_status = true;

#if defined(OS_ANDROID)
  // content_settings_status_[type].allowed is always set to true in
  // OnContentBlocked, so we have to use
  // content_settings_status_[type].blocked to detect whether the protected
  // media setting has changed.
  if (type == ContentSettingsType::PROTECTED_MEDIA_IDENTIFIER)
    must_reset_blocked_status = true;
#endif

  if (must_reset_blocked_status && status.blocked) {
    status.blocked = false;
    access_changed = true;
  }

  if (!status.allowed) {
    status.allowed = true;
    access_changed = true;
  }

  if (access_changed)
    content_settings::UpdateLocationBarUiForWebContents(web_contents());
}

void TabSpecificContentSettings::IncrementBlockedTrackerCount(
    const GURL& tracker_url) {
  int tracker_count =
      tracking_prevention::GetTrackerData(tracker_url, map_);

  tracking_prevention::SetTrackerData(tracker_url, ++tracker_count,
                                               map_);
}

void TabSpecificContentSettings::OnTrackerBlocked(
    const GURL& url,
    const GURL& first_party_url,
    tracking_prevention::TrackerType tracker_type) {
  // In order to show an accurate count of the trackers blocked we'll increase
  // our count of the types of trackers we've seen blocked for this tab.
  switch (tracker_type) {
    case tracking_prevention::TrackerType::STORAGE:
      if (storage_tracking_attempts_blocked_ == 0) {
        // Reset trackers blocked status to update blocked cookie notification
        content_settings_status_[ContentSettingsType::TRACKERS].blocked = false;
      }
      storage_tracking_attempts_blocked_++;
      break;
    case tracking_prevention::TrackerType::CSPREPORT_RESOURCE:
    case tracking_prevention::TrackerType::FAVICON_RESOURCE:
    case tracking_prevention::TrackerType::FONT_RESOURCE:
    case tracking_prevention::TrackerType::IFRAME_RESOURCE:
    case tracking_prevention::TrackerType::IMAGE_RESOURCE:
    case tracking_prevention::TrackerType::MEDIA_RESOURCE:
    case tracking_prevention::TrackerType::OBJECT_RESOURCE:
    case tracking_prevention::TrackerType::OTHER_RESOURCE:
    case tracking_prevention::TrackerType::PING_RESOURCE:
    case tracking_prevention::TrackerType::PLUGIN_RESOURCE:
    case tracking_prevention::TrackerType::PREFETCH_RESOURCE:
    case tracking_prevention::TrackerType::PRELOAD_RESOURCE:
    case tracking_prevention::TrackerType::SCRIPT_RESOURCE:
    case tracking_prevention::TrackerType::SERVICE_WORKER_RESOURCE:
    case tracking_prevention::TrackerType::STYLESHEET_RESOURCE:
    case tracking_prevention::TrackerType::WORKER_RESOURCE:
    case tracking_prevention::TrackerType::XHR_RESOURCE:
      // If we don't have a `STORAGE` type then all other types are resource
      // types so increment the appropriate counter.
      resource_tracking_attempts_blocked_++;
      break;
    default:
      NOTREACHED();
      break;
  }

  if (url_trackers_blocked_.find(url.host()) == url_trackers_blocked_.end()) {
    // Found a new tracker. Get organization and increment value for UI.
    DCHECK(g_browser_process->trust_manager());
    const std::string organization = g_browser_process->trust_manager()
                                         ->TrackingChecker()
                                         ->GetTrackerOrganization(url);
    const int organization_tracker_count =
        organization_trackers_blocked_[organization];
    organization_trackers_blocked_[organization] =
        organization_tracker_count + 1;

    url_trackers_blocked_.insert(url.host());
    blocked_trackers_count_++;

    if (features::edge::IsBlockedTrackerStatsEnabled()) {
      IncrementBlockedTrackerCount(url);
    }
  }

  OnContentBlocked(ContentSettingsType::TRACKERS);
  NotifySiteDataObservers();
}

#if defined(OS_WIN)
void TabSpecificContentSettings::OnInternetExplorerDocumentModeSet(
    uint32_t document_mode) {
  switch (document_mode) {
    case htmlDocumentModeIE5:
      internet_explorer_document_mode_ = 5;
      break;
    case htmlDocumentModeIE7:
      internet_explorer_document_mode_ = 7;
      break;
    case htmlDocumentModeIE8:
      internet_explorer_document_mode_ = 8;
      break;
    case htmlDocumentModeIE9:
      internet_explorer_document_mode_ = 9;
      break;
    case htmlDocumentModeIE10:
      internet_explorer_document_mode_ = 10;
      break;
    case htmlDocumentModeIE11:
      internet_explorer_document_mode_ = 11;
      break;
    default:
      internet_explorer_document_mode_ = 0;
      break;
  }
}

uint32_t TabSpecificContentSettings::GetInternetExplorerDocumentMode() {
  return internet_explorer_document_mode_;
}

void TabSpecificContentSettings::OnInternetExplorerIndicatorItemsSet(
    dual_engine::InternetExplorerProtectedMode protected_mode,
    dual_engine::InternetExplorerUrlZone url_zone,
    bool enterprise_mode) {
  internet_explorer_protected_mode_ = protected_mode;
  url_zone_ = url_zone;
  enterprise_mode_ = enterprise_mode;
}
#endif

void TabSpecificContentSettings::OnDomStorageAccessed(const GURL& url,
                                                      bool local,
                                                      bool blocked_by_policy) {
  browsing_data::LocalSharedObjectsContainer& container =
      blocked_by_policy ? blocked_local_shared_objects_
                        : allowed_local_shared_objects_;
  browsing_data::CannedLocalStorageHelper* helper =
      local ? container.local_storages() : container.session_storages();
  helper->Add(url::Origin::Create(url));

  if (blocked_by_policy)
    OnContentBlocked(ContentSettingsType::COOKIES);
  else
    OnContentAllowed(ContentSettingsType::COOKIES);

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnCookiesRead(
    const GURL& url,
    const GURL& frame_url,
    const net::CookieList& cookie_list,
    bool blocked_by_policy) {
  if (cookie_list.empty())
    return;
  if (blocked_by_policy) {
    blocked_local_shared_objects_.cookies()->AddReadCookies(
        frame_url, url, cookie_list);
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.cookies()->AddReadCookies(
        frame_url, url, cookie_list);
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnCookieChange(
    const GURL& url,
    const GURL& frame_url,
    const net::CanonicalCookie& cookie,
    bool blocked_by_policy) {
  if (blocked_by_policy) {
    blocked_local_shared_objects_.cookies()->AddChangedCookie(frame_url, url,
                                                              cookie);
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.cookies()->AddChangedCookie(frame_url, url,
                                                              cookie);
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnIndexedDBAccessed(const GURL& url,
                                                     bool blocked_by_policy) {
  if (blocked_by_policy) {
    blocked_local_shared_objects_.indexed_dbs()->Add(url::Origin::Create(url));
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.indexed_dbs()->Add(url::Origin::Create(url));
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnCacheStorageAccessed(
    const GURL& url,
    bool blocked_by_policy) {
  if (blocked_by_policy) {
    blocked_local_shared_objects_.cache_storages()->Add(
        url::Origin::Create(url));
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.cache_storages()->Add(
        url::Origin::Create(url));
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnServiceWorkerAccessed(
    const GURL& scope,
    bool blocked_by_policy_javascript,
    bool blocked_by_policy_cookie) {
  DCHECK(scope.is_valid());
  if (blocked_by_policy_javascript || blocked_by_policy_cookie) {
    blocked_local_shared_objects_.service_workers()->Add(
        url::Origin::Create(scope));
  } else {
    allowed_local_shared_objects_.service_workers()->Add(
        url::Origin::Create(scope));
  }

  if (blocked_by_policy_javascript) {
    OnContentBlocked(ContentSettingsType::JAVASCRIPT);
  } else {
    OnContentAllowed(ContentSettingsType::JAVASCRIPT);
  }
  if (blocked_by_policy_cookie) {
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    OnContentAllowed(ContentSettingsType::COOKIES);
  }
}

void TabSpecificContentSettings::OnSharedWorkerAccessed(
    const GURL& worker_url,
    const std::string& name,
    const url::Origin& constructor_origin,
    bool blocked_by_policy) {
  DCHECK(worker_url.is_valid());
  if (blocked_by_policy) {
    blocked_local_shared_objects_.shared_workers()->AddSharedWorker(
        worker_url, name, constructor_origin);
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.shared_workers()->AddSharedWorker(
        worker_url, name, constructor_origin);
    OnContentAllowed(ContentSettingsType::COOKIES);
  }
}

void TabSpecificContentSettings::OnWebDatabaseAccessed(
    const GURL& url,
    bool blocked_by_policy) {
  if (blocked_by_policy) {
    blocked_local_shared_objects_.databases()->Add(url::Origin::Create(url));
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.databases()->Add(url::Origin::Create(url));
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnFileSystemAccessed(
    const GURL& url,
    bool blocked_by_policy) {
  // Note that all sandboxed file system access is recorded here as
  // kTemporary; the distinction between temporary (default) and persistent
  // storage is not made in the UI that presents this data.
  if (blocked_by_policy) {
    blocked_local_shared_objects_.file_systems()->Add(url::Origin::Create(url));
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.file_systems()->Add(url::Origin::Create(url));
    OnContentAllowed(ContentSettingsType::COOKIES);
  }

  NotifySiteDataObservers();
}

void TabSpecificContentSettings::OnGeolocationPermissionSet(
    const GURL& requesting_origin,
    bool allowed) {
  geolocation_usages_state_.OnPermissionSet(requesting_origin, allowed);
  content_settings::UpdateLocationBarUiForWebContents(web_contents());
}

#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
void TabSpecificContentSettings::OnProtectedMediaIdentifierPermissionSet(
    const GURL& requesting_origin,
    bool allowed) {
  if (allowed) {
    OnContentAllowed(ContentSettingsType::PROTECTED_MEDIA_IDENTIFIER);
  } else {
    OnContentBlocked(ContentSettingsType::PROTECTED_MEDIA_IDENTIFIER);
  }
}
#endif

TabSpecificContentSettings::MicrophoneCameraState
TabSpecificContentSettings::GetMicrophoneCameraState() const {
  MicrophoneCameraState state = microphone_camera_state_;

  // Include capture devices in the state if there are still consumers of the
  // approved media stream.
  scoped_refptr<MediaStreamCaptureIndicator> media_indicator =
      MediaCaptureDevicesDispatcher::GetInstance()->
        GetMediaStreamCaptureIndicator();
  if (media_indicator->IsCapturingAudio(web_contents()))
    state |= MICROPHONE_ACCESSED;
  if (media_indicator->IsCapturingVideo(web_contents()))
    state |= CAMERA_ACCESSED;

  return state;
}

bool TabSpecificContentSettings::IsMicrophoneCameraStateChanged() const {
  if ((microphone_camera_state_ & MICROPHONE_ACCESSED) &&
      ((microphone_camera_state_ & MICROPHONE_BLOCKED)
           ? !IsContentBlocked(ContentSettingsType::MEDIASTREAM_MIC)
           : !IsContentAllowed(ContentSettingsType::MEDIASTREAM_MIC)))
    return true;

  if ((microphone_camera_state_ & CAMERA_ACCESSED) &&
      ((microphone_camera_state_ & CAMERA_BLOCKED)
           ? !IsContentBlocked(ContentSettingsType::MEDIASTREAM_CAMERA)
           : !IsContentAllowed(ContentSettingsType::MEDIASTREAM_CAMERA)))
    return true;

  PrefService* prefs =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext())->
          GetPrefs();
  scoped_refptr<MediaStreamCaptureIndicator> media_indicator =
      MediaCaptureDevicesDispatcher::GetInstance()->
          GetMediaStreamCaptureIndicator();

  if ((microphone_camera_state_ & MICROPHONE_ACCESSED) &&
      prefs->GetString(prefs::kDefaultAudioCaptureDevice) !=
      media_stream_selected_audio_device() &&
      media_indicator->IsCapturingAudio(web_contents()))
    return true;

  if ((microphone_camera_state_ & CAMERA_ACCESSED) &&
      prefs->GetString(prefs::kDefaultVideoCaptureDevice) !=
      media_stream_selected_video_device() &&
      media_indicator->IsCapturingVideo(web_contents()))
    return true;

  return false;
}

void TabSpecificContentSettings::OnMediaStreamPermissionSet(
    const GURL& request_origin,
    MicrophoneCameraState new_microphone_camera_state,
    const std::string& media_stream_selected_audio_device,
    const std::string& media_stream_selected_video_device,
    const std::string& media_stream_requested_audio_device,
    const std::string& media_stream_requested_video_device) {
  media_stream_access_origin_ = request_origin;

  if (new_microphone_camera_state & MICROPHONE_ACCESSED) {
    media_stream_requested_audio_device_ = media_stream_requested_audio_device;
    media_stream_selected_audio_device_ = media_stream_selected_audio_device;
    bool mic_blocked = (new_microphone_camera_state & MICROPHONE_BLOCKED) != 0;
    ContentSettingsStatus& status =
        content_settings_status_[ContentSettingsType::MEDIASTREAM_MIC];
    status.allowed = !mic_blocked;
    status.blocked = mic_blocked;
  }

  if (new_microphone_camera_state & CAMERA_ACCESSED) {
    media_stream_requested_video_device_ = media_stream_requested_video_device;
    media_stream_selected_video_device_ = media_stream_selected_video_device;
    bool cam_blocked = (new_microphone_camera_state & CAMERA_BLOCKED) != 0;
    ContentSettingsStatus& status =
        content_settings_status_[ContentSettingsType::MEDIASTREAM_CAMERA];
    status.allowed = !cam_blocked;
    status.blocked = cam_blocked;
  }

  if (microphone_camera_state_ != new_microphone_camera_state) {
    microphone_camera_state_ = new_microphone_camera_state;
    content_settings::UpdateLocationBarUiForWebContents(web_contents());
  }
}

void TabSpecificContentSettings::OnMidiSysExAccessed(
    const GURL& requesting_origin) {
  midi_usages_state_.OnPermissionSet(requesting_origin, true);
  OnContentAllowed(ContentSettingsType::MIDI_SYSEX);
}

void TabSpecificContentSettings::OnMidiSysExAccessBlocked(
    const GURL& requesting_origin) {
  midi_usages_state_.OnPermissionSet(requesting_origin, false);
  OnContentBlocked(ContentSettingsType::MIDI_SYSEX);
}

void TabSpecificContentSettings::
ClearContentSettingsExceptForNavigationRelatedSettings() {
  for (auto& status : content_settings_status_) {
    if (status.first == ContentSettingsType::COOKIES ||
        status.first == ContentSettingsType::JAVASCRIPT)
      continue;
    status.second.blocked = false;
    status.second.allowed = false;
  }
  microphone_camera_state_ = MICROPHONE_CAMERA_NOT_ACCESSED;
  camera_was_just_granted_on_site_level_ = false;
  mic_was_just_granted_on_site_level_ = false;
  load_plugins_link_enabled_ = true;
  content_settings::UpdateLocationBarUiForWebContents(web_contents());
}

void TabSpecificContentSettings::ClearNavigationRelatedContentSettings() {
  blocked_local_shared_objects_.Reset();
  allowed_local_shared_objects_.Reset();
  storage_tracking_attempts_blocked_ = 0;
  resource_tracking_attempts_blocked_ = 0;
  blocked_trackers_count_ = 0;
  url_trackers_blocked_.clear();
  organization_trackers_blocked_.clear();
  for (ContentSettingsType type :
       {ContentSettingsType::COOKIES, ContentSettingsType::JAVASCRIPT,
        ContentSettingsType::TRACKERS}) {
    ContentSettingsStatus& status = content_settings_status_[type];
    status.blocked = false;
    status.allowed = false;
  }
  content_settings::UpdateLocationBarUiForWebContents(web_contents());
}

void TabSpecificContentSettings::FlashDownloadBlocked() {
  OnContentBlocked(ContentSettingsType::PLUGINS);
}

void TabSpecificContentSettings::ClearPopupsBlocked() {
  ContentSettingsStatus& status =
      content_settings_status_[ContentSettingsType::POPUPS];
  status.blocked = false;
  content_settings::UpdateLocationBarUiForWebContents(web_contents());
}

void TabSpecificContentSettings::OnAudioBlocked() {
  OnContentBlocked(ContentSettingsType::SOUND);
}

void TabSpecificContentSettings::SetPepperBrokerAllowed(bool allowed) {
  if (allowed) {
    OnContentAllowed(ContentSettingsType::PPAPI_BROKER);
  } else {
    OnContentBlocked(ContentSettingsType::PPAPI_BROKER);
  }
}

void TabSpecificContentSettings::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type,
    const std::string& resource_identifier) {
  const ContentSettingsDetails details(
      primary_pattern, secondary_pattern, content_type, resource_identifier);
  if (!details.update_all() &&
      // The visible URL is the URL in the URL field of a tab.
      // Currently this should be matched by the |primary_pattern|.
      !details.primary_pattern().Matches(web_contents()->GetVisibleURL())) {
    return;
  }

  ContentSettingsStatus& status = content_settings_status_[content_type];
  switch (content_type) {
    case ContentSettingsType::MEDIASTREAM_MIC:
    case ContentSettingsType::MEDIASTREAM_CAMERA: {
      const GURL media_origin = media_stream_access_origin();
      ContentSetting setting = map_->GetContentSetting(
          media_origin, media_origin, content_type, std::string());

      if (content_type == ContentSettingsType::MEDIASTREAM_MIC &&
          setting == CONTENT_SETTING_ALLOW) {
        mic_was_just_granted_on_site_level_ = true;
      }

      if (content_type == ContentSettingsType::MEDIASTREAM_CAMERA &&
          setting == CONTENT_SETTING_ALLOW) {
        camera_was_just_granted_on_site_level_ = true;
      }

      status.allowed = setting == CONTENT_SETTING_ALLOW;
      status.blocked = setting == CONTENT_SETTING_BLOCK;
      break;
    }
    case ContentSettingsType::IMAGES:
    case ContentSettingsType::JAVASCRIPT:
    case ContentSettingsType::PLUGINS:
    case ContentSettingsType::COOKIES:
    case ContentSettingsType::POPUPS:
    case ContentSettingsType::MIXEDSCRIPT:
    case ContentSettingsType::PPAPI_BROKER:
    case ContentSettingsType::MIDI_SYSEX:
    case ContentSettingsType::ADS:
    case ContentSettingsType::SOUND:
    case ContentSettingsType::CLIPBOARD_READ_WRITE:
    case ContentSettingsType::SENSORS: {
      ContentSetting setting = map_->GetContentSetting(
          web_contents()->GetVisibleURL(), web_contents()->GetVisibleURL(),
          content_type, std::string());
      // If an indicator is shown and the content settings has changed, swap the
      // indicator for the one with the opposite meaning (allowed <=> blocked).
      if (setting == CONTENT_SETTING_BLOCK && status.allowed) {
        status.blocked = false;
        status.allowed = false;
        OnContentBlocked(content_type);
      } else if (setting == CONTENT_SETTING_ALLOW && status.blocked) {
        status.blocked = false;
        status.allowed = false;
        OnContentAllowed(content_type);
      }
      break;
    }
    default:
      break;
  }

  if (!ShouldSendUpdatedContentSettingsRulesToRenderer(content_type))
    return;

  MaybeSendRendererContentSettingsRules(web_contents());
}

void TabSpecificContentSettings::MaybeSendRendererContentSettingsRules(
    content::WebContents* web_contents) {
  // Only send a message to the renderer if it is initialised and not dead.
  // Otherwise, the IPC messages will be queued in the browser process,
  // potentially causing large memory leaks. See https://crbug.com/875937.
  content::RenderProcessHost* process =
      web_contents->GetMainFrame()->GetProcess();
  if (!process->IsInitializedAndNotDead())
    return;

  // |channel| may be null in tests.
  IPC::ChannelProxy* channel = process->GetChannel();
  if (!channel)
    return;

  RendererContentSettingRules rules;
  GetRendererContentSettingRules(map_, &rules);

  mojo::AssociatedRemote<chrome::mojom::RendererConfiguration> rc_interface;
  channel->GetRemoteAssociatedInterface(&rc_interface);
  rc_interface->SetContentSettingRules(rules);
}

void TabSpecificContentSettings::RenderFrameForInterstitialPageCreated(
    content::RenderFrameHost* render_frame_host) {
  // We want to tell the renderer-side code to ignore content settings for this
  // page.
  mojo::AssociatedRemote<content_settings::mojom::ContentSettingsAgent>
      content_settings_agent;
  render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(
      &content_settings_agent);
  content_settings_agent->SetAsInterstitial();
}

void TabSpecificContentSettings::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  ClearNavigationRelatedContentSettings();
}

void TabSpecificContentSettings::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  Profile* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());

  if (profile &&
      !profile->GetPrefs()->GetBoolean(
          security_state::prefs::kStricterMixedContentTreatmentEnabled)) {
    auto* render_frame_host = navigation_handle->GetRenderFrameHost();
    mojo::AssociatedRemote<content_settings::mojom::ContentSettingsAgent>
        content_settings_agent;
    render_frame_host->GetRemoteAssociatedInterfaces()->GetInterface(
        &content_settings_agent);
    content_settings_agent->SetDisabledMixedContentUpgrades();
  }

  // There may be content settings that were updated for the navigated URL.
  // These would not have been sent before if we're navigating cross-origin.
  // Ensure up to date rules are sent before navigation commits.
  MaybeSendRendererContentSettingsRules(navigation_handle->GetWebContents());
}

void TabSpecificContentSettings::DidFinishNavigation(
      content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  ClearContentSettingsExceptForNavigationRelatedSettings();
  GeolocationDidNavigate(navigation_handle);
  MidiDidNavigate(navigation_handle);
  ClearPendingProtocolHandler();
  ClearContentSettingsChangedViaPageInfo();

  if (web_contents()->GetVisibleURL().SchemeIsHTTPOrHTTPS()) {
    content_settings::RecordPluginsAction(
        content_settings::PLUGINS_ACTION_TOTAL_NAVIGATIONS);
  }
}

#if defined(OS_WIN)
void TabSpecificContentSettings::DidStartInternetExplorerNavigation(
    bool is_main_frame,
    bool is_same_document,
    bool is_error_page) {
  if (!is_main_frame || is_same_document) {
    return;
  }

  // If we're displaying a network error page do not reset the content
  // settings delegate's cookies so the user has a chance to modify cookie
  // settings.
  if (!is_error_page)
    ClearNavigationRelatedContentSettings();
}

void TabSpecificContentSettings::DidFinishInternetExplorerNavigation(
    bool is_main_frame,
    bool is_same_document,
    bool is_error_page) {
  if (!is_main_frame || is_same_document) {
    return;
  }
  // Clear "blocked" flags.
  ClearContentSettingsExceptForNavigationRelatedSettings();

  // This should be updated to get the actual previous URL when that data
  // becomes available.
  GURL previous_url;

  geolocation_usages_state_.DidNavigate(web_contents()->GetLastCommittedURL(),
                                        previous_url);
}
#endif

void TabSpecificContentSettings::AppCacheAccessed(const GURL& manifest_url,
                                                  bool blocked_by_policy) {
  if (blocked_by_policy) {
    blocked_local_shared_objects_.appcaches()->Add(
        url::Origin::Create(manifest_url));
    OnContentBlocked(ContentSettingsType::COOKIES);
  } else {
    allowed_local_shared_objects_.appcaches()->Add(
        url::Origin::Create(manifest_url));
    OnContentAllowed(ContentSettingsType::COOKIES);
  }
}

void TabSpecificContentSettings::AddSiteDataObserver(
    SiteDataObserver* observer) {
  observer_list_.AddObserver(observer);
}

void TabSpecificContentSettings::RemoveSiteDataObserver(
    SiteDataObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

void TabSpecificContentSettings::NotifySiteDataObservers() {
  for (SiteDataObserver& observer : observer_list_)
    observer.OnSiteDataAccessed();
}

void TabSpecificContentSettings::ClearContentSettingsChangedViaPageInfo() {
  content_settings_changed_via_page_info_.clear();
}

void TabSpecificContentSettings::GeolocationDidNavigate(
    content::NavigationHandle* navigation_handle) {
  geolocation_usages_state_.ClearStateMap();
  geolocation_usages_state_.DidNavigate(navigation_handle->GetURL(),
                                        navigation_handle->GetPreviousURL());
}

void TabSpecificContentSettings::MidiDidNavigate(
    content::NavigationHandle* navigation_handle) {
  midi_usages_state_.ClearStateMap();
  midi_usages_state_.DidNavigate(navigation_handle->GetURL(),
                                 navigation_handle->GetPreviousURL());
}

void TabSpecificContentSettings::BlockAllContentForTesting() {
  content_settings::ContentSettingsRegistry* registry =
      content_settings::ContentSettingsRegistry::GetInstance();
  for (const content_settings::ContentSettingsInfo* info : *registry) {
    ContentSettingsType type = info->website_settings_info()->type();
    if (type != ContentSettingsType::GEOLOCATION &&
        type != ContentSettingsType::MEDIASTREAM_MIC &&
        type != ContentSettingsType::MEDIASTREAM_CAMERA) {
      OnContentBlocked(type);
    }
  }

  // Geolocation and media must be blocked separately, as the generic
  // TabSpecificContentSettings::OnContentBlocked does not apply to them.
  OnGeolocationPermissionSet(web_contents()->GetLastCommittedURL(), false);
  MicrophoneCameraStateFlags media_blocked =
      static_cast<MicrophoneCameraStateFlags>(
          TabSpecificContentSettings::MICROPHONE_ACCESSED |
          TabSpecificContentSettings::MICROPHONE_BLOCKED |
          TabSpecificContentSettings::CAMERA_ACCESSED |
          TabSpecificContentSettings::CAMERA_BLOCKED);
  OnMediaStreamPermissionSet(
      web_contents()->GetLastCommittedURL(),
      media_blocked,
      std::string(), std::string(), std::string(), std::string());
}

void TabSpecificContentSettings::ContentSettingChangedViaPageInfo(
    ContentSettingsType type) {
  content_settings_changed_via_page_info_.insert(type);
}

bool TabSpecificContentSettings::HasContentSettingChangedViaPageInfo(
    ContentSettingsType type) const {
  return content_settings_changed_via_page_info_.find(type) !=
         content_settings_changed_via_page_info_.end();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(TabSpecificContentSettings)
