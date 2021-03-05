// Copyright 2015 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/importer/edge_importer_win.h"
#include "components/autofill/core/common/password_form.h"

#include <Shlobj.h>

#include <stdint.h>
#include <algorithm>
#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "base/base_paths_win.h"
#include "base/debug/debugger.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
<<<<<<< HEAD
#include "base/json/json_file_value_serializer.h"
=======
#include "base/logging.h"
>>>>>>> da9549bd3df6d6b9ea16435b9f9cf5c415a98fc4
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "base/win/core_winrt_util.h"
#include "base/win/shlwapi.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/edge_cookie_import_feature.h"
#include "chrome/common/edge_onramp_features.h"
#include "chrome/common/importer/edge_importer_autofill_credit_card_data_entry.h"
#include "chrome/common/importer/edge_importer_autofill_profile_data_entry.h"
#include "chrome/common/importer/edge_importer_profile_metrics.h"
#include "chrome/common/importer/edge_importer_utils.h"
#include "chrome/common/importer/edge_importer_utils_win.h"
#include "chrome/common/importer/extensions/edge_extension_import_group.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_autofill_form_data_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/utility/importer/edge_async_storage_win.h"
#include "chrome/utility/importer/edge_cookie_importer.h"
#include "chrome/utility/importer/edge_importer_common_win.h"
#include "chrome/utility/importer/edge_recovery_store_win.h"
#include "chrome/utility/importer/favicon_reencode.h"

#if defined(MICROSOFT_EDGE_BUILD)
#include "components/edge_onramp/edge_onramp_api_win.h"
#endif

#include "chrome/utility/importer/extensions/edge_extension_importer.h"
#include "components/os_crypt/os_crypt.h"
#include "components/search_engines/template_url.h"
#include "content/public/common/page_zoom.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using namespace windows_runtime;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace std;

namespace {
// Toolbar favorites are placed under this special folder name.
const base::char16 kSpartanDatabaseFile[] = L"spartan.edb";
const base::char16 kSpartanHistoryDatabaseFile[] = L"WebCacheV01.dat";
const base::char16 kInternetUrlsHistoryContainer[] =
    L"microsoft.microsoftedge_8wekyb3d8bbwe\\AC\\#!"
    L"001\\MicrosoftEdge\\History";
const base::char16 kIntranetUrlsHistoryContainer[] =
    L"microsoft.microsoftedge_8wekyb3d8bbwe\\AC\\#!"
    L"121\\MicrosoftEdge\\History";
const base::char16 kDownloadHistoryContainerTitle[] = L"\\DownloadHistory";

// Registry key paths from which we import Edge search scopes settings.
constexpr base::char16 kEdgeProtectedDataPath[] =
    L"Software\\Classes\\Local "
    L"Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Sto"
    L"rage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Protected - "
    L"It is a violation of Windows Policy to modify. See aka.ms/browserpolicy";

constexpr base::char16 kEdgeHomeButtonPagePath[] =
    L"Software\\Classes\\Local "
    L"Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Sto"
    L"rage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge\\Main";

constexpr base::char16 kEdgeSettingsRegPath[] =
    L"Software\\Classes\\Local "
    L"Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Sto"
    L"rage\\microsoft.microsoftedge_8wekyb3d8bbwe\\MicrosoftEdge";

// Webnotes created in spartan are saved in this relative path
constexpr base::char16 webnotes_old_rel_path[] =
    L"\\MicrosoftEdge\\User\\Default\\WebNotes\\";

// These values are copied from Spartan
const int kTitleKey = 16;
const int kVisitCount = 6;
const int kLocalPreviousVisitTime = 40;
const int kInsiteFlags = 9;
const int kMinDataSize = 5;

#define ISF_HISTORY 0x10000000

#define RETURN_FALSE_IF_FAILED(exp) \
  {                                 \
    if (FAILED(((HRESULT)(exp)))) { \
      return false;                 \
    }                               \
  }

#define LOG_ERROR_AND_CONTINUE(exp) \
  {                                 \
    VLOG(1) << exp;                 \
    continue;                       \
  }

#define LOG_ERROR_METRIC_AND_CONTINUE(item, error, error_count)      \
  {                                                                  \
    error_count++;                                                   \
    ImporterProfileMetrics::LogImporterErrorMetric(                  \
        importer::TYPE_EDGE, item, error);                           \
    continue;                                                        \
  }

enum PasswordErrorType {
  ERROR_READ_FROM_VAULT = 0,
  ERROR_READ_PASSWORD_LIST = 1,
  TOTAL
};

struct AutofillAddressComponent {
  base::string16 street_address;
  base::string16 dependent_locality;
  base::string16 city;
  base::string16 state;
  base::string16 zipcode;
  base::string16 sorting_code;
  base::string16 country_code;
  base::string16 language_code;
};

// The name of the database file is spartan.edb, however it isn't clear how
// the intermediate path between the DataStore and the database is generated.
// Therefore we just do a simple recursive search until we find a matching name.
base::FilePath FindSpartanDatabase(const base::FilePath& profile_path) {
  base::FilePath data_path =
      profile_path.empty() ? importer::GetEdgeDataFilePath() : profile_path;
  if (data_path.empty())
    return base::FilePath();

  base::FileEnumerator enumerator(data_path.Append(L"DataStore\\Data"), true,
                                  base::FileEnumerator::FILES);
  base::FilePath path = enumerator.Next();
  while (!path.empty()) {
    if (base::EqualsCaseInsensitiveASCII(path.BaseName().value(),
                                         kSpartanDatabaseFile))
      return path;
    path = enumerator.Next();
  }
  return base::FilePath();
}

base::FilePath FindRecoveryStore(const base::FilePath& profile_path) {
  base::FilePath recovery_path =
      profile_path.empty() ? importer::GetEdgeDataFilePath() : profile_path;
  if (recovery_path.empty())
    return base::FilePath();

  return recovery_path.Append(L"Recovery\\Active");
}

unique_ptr<EdgeDatabaseTableEnumerator> GetDatabaseTableEnumerator(
    EdgeDatabaseReader& database,
    const base::string16& table_name) {
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database.OpenTableEnumerator(table_name);
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database.GetErrorMessage();
    return nullptr;
  }
  return enumerator;
}

bool RetrieveColumn(EdgeDatabaseTableEnumerator* enumerator,
                    const base::string16& column_name,
                    base::string16& value) {
  value.clear();
  std::string encrypted_value;
  if (!enumerator->RetrieveColumn(column_name, &encrypted_value))
    return false;

  if (!encrypted_value.empty()) {
    std::string decrypted_byte;
    if (!OSCrypt::DecryptString(encrypted_value, &decrypted_byte) ||
        (decrypted_byte.size() % sizeof(base::char16) != 0)) {
      return false;
    }

    if (decrypted_byte.size() > 0) {
      value.assign(reinterpret_cast<wchar_t*>(&decrypted_byte[0]),
                   decrypted_byte.size() / sizeof(base::char16));
    }
  }
  return true;
}

bool GetAllAutofillAddress(
    EdgeDatabaseReader& database,
    map<GUID, shared_ptr<AutofillAddressComponent>, importer::GuidComparator>&
        addressDataMap) {
  unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      GetDatabaseTableEnumerator(database, L"AutoFormFillAddressStorage");

  if (!enumerator)
    return false;
  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  do {
    bool is_deleted = false;
    if (!enumerator->RetrieveColumn(kIsDeleted, &is_deleted) || is_deleted)
      continue;

    shared_ptr<AutofillAddressComponent> entry(new AutofillAddressComponent);

    base::string16 address_line1;
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kAddressLine1, address_line1));
    if (!address_line1.empty())
      entry->street_address.swap(address_line1);

    base::string16 address_line2;
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kAddressLine2, address_line2));
    if (!address_line2.empty()) {
      entry->street_address.append(kSpace);
      entry->street_address.append(address_line2);
    }

    base::string16 address_line3;
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kAddressLine3, address_line3));
    if (!address_line3.empty()) {
      entry->street_address.append(kSpace);
      entry->street_address.append(address_line3);
    }

    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kLocality, entry->dependent_locality));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kCity, entry->city));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kState, entry->state));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kZipcode, entry->zipcode));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(enumerator.get(), kCountry, entry->country_code));

    GUID address_id;
    CONTINUE_LOOP_IF_FALSE(enumerator->RetrieveColumn(kItemId, &address_id));

    // Push back entry.
    addressDataMap.emplace(address_id, entry);

  } while (enumerator->Next());

  return true;
}
}  // namespace

namespace importer {
template <>
CookieContentSetting
ConvertImportedCookiePref<importer::ImporterType::TYPE_EDGE>(
    int extracted_cookie_pref) {
  switch (extracted_cookie_pref) {
      /* BLOCK_ALL_COOKIES */
    case 0:
      return CookieContentSetting::BLOCK_COOKIES;
      /* DONT_BLOCK_COOKIES */
    case 2:
      return CookieContentSetting::ALLOW_COOKIES;
    default:
      return CookieContentSetting::UNKNOWN;
  };
}
}  // namespace importer

EdgeImporter::EdgeImporter() {}

void EdgeImporter::StartImport(const importer::SourceProfile& source_profile,
                               uint16_t items,
                               ImporterBridge* bridge) {
  bridge_ = bridge;
  bridge_->NotifyStarted();
  source_path_ = source_profile.source_path;
  bool is_fatal_error = false;

  // Import startup_page and search_engine settings at the earliest for SAN
  // experimentation.
  StartSearchEngineMigration(items);
  StartStartupPageMigration(items);

  // Please note do not change the order of import of Importer items.
  // Favorites should be imported first, followed by settings and
  // open tabs. History should be at the end.
  if ((items & importer::FAVORITES || items & importer::READING_LIST ||
       items & importer::PAYMENTS || items & importer::AUTOFILL_FORM_DATA ||
       items & importer::TAB_SWEEP || items & importer::TOP_SITES) ||
      (base::FeatureList::IsEnabled(features::edge::kEdgeSettingsImportV2) &&
       (items & importer::SETTINGS))) {
    if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport)) {
      if (!OpenSpartanDatabase(false /*is_history_db*/)) {
        VLOG(1) << "Error opening database " << source_path_.value().c_str()
                << " Error " << database_.GetErrorMessage();
        is_fatal_error = true;
      }
    } else {
      // When experiment key is off, do not copy / recover database.
      base::FilePath database_path = FindSpartanDatabase(source_path_);
      if (!database_.OpenDatabase(database_path.value().c_str(),
                                  false /*is_history_db*/)) {
        VLOG(1) << "Error opening database " << source_path_.value().c_str()
                << " Error " << database_.GetErrorMessage();
        is_fatal_error = true;
      }
    }
    // Even if OpenDatabase call fails we should do NotifyItemStarted and
    // NotifyItemEnded with correct status, hence we should reset status to
    // false after each dataype import
    StartFavoritesMigration(items, is_fatal_error);
    StartPaymentsMigration(items, is_fatal_error);
    StartAutofillMigration(items, is_fatal_error);
    StartReadingListMigration(items, is_fatal_error);
    StartWebNotesMigration();
    StartTabSweepMigration(items, is_fatal_error);
    StartTopSitesMigration(items, is_fatal_error);
    StartSettingsMigration(items);
    // Releasing the database so that the
    // temp files are not locked by this process when it is getting deleted.
    database_.ReleaseDatabase();
  }

  StartCookiesMigration(items);
  StartOpenTabsMigration(items);
  StartPasswordsMigration(items);
  StartHomePageMigration(items);
  StartExtensionsMigration(source_profile, items);

  // Import history should be done after rest of the imports since spartan
  // history is stored in WebCacheV01.dat instead of spartan.edb and at
  // a time only one instance of database_ (opened database) could be there
  // also import of history takes time.
  StartHistoryMigration(items);

  // Release |recovery_store_| so it re-initializes with the most recenty
  // active one if StartImport is invoked again on the same EdgeImporter object.
  recovery_store_.Release();

  bridge_->NotifyEnded(
      true /*succeeded*/);  // Pass cumulative status of each datatype import
                            // TODO: Bug 20676064
}

EdgeImporter::~EdgeImporter() {}

bool EdgeImporter::ImportFavorites() {
  std::vector<ImportedBookmarkEntry> bookmarks;
  favicon_base::FaviconUsageDataList favicons;
  bool should_show_fav_bar_on_ntp = false;
  if (!ParseFavoritesDatabase(&bookmarks, &favicons,
                              &should_show_fav_bar_on_ntp))
    return false;

  if (!bookmarks.empty() && !cancelled()) {
    const base::string16& first_folder_name =
        l10n_util::GetStringUTF16(IDS_BOOKMARK_GROUP_FROM_EDGE);
    bridge_->AddBookmarks(bookmarks, first_folder_name,
                          should_show_fav_bar_on_ntp);
  }
  if (!favicons.empty() && !cancelled())
    bridge_->SetFavicons(favicons);
  return true;
}

// make a full or partial copy of property store
HRESULT CopyPropertyStore(IPropertyStore* source, IPropertyStore* target) {
  if ((source == nullptr) || (target == nullptr))
    return E_FAIL;
  DWORD properties;
  HRESULT hr = source->GetCount(&properties);
  for (DWORD i = 0; SUCCEEDED(hr) && (i < properties); i++) {
    PROPERTYKEY prop_key;
    hr = source->GetAt(i, &prop_key);
    if (SUCCEEDED(hr)) {
      PROPVARIANT value = {{{0}}};
      hr = source->GetValue(prop_key, &value);
      if (SUCCEEDED(hr)) {
        hr = target->SetValue(prop_key, value);
        PropVariantClear(&value);
      }
    }
  }
  return hr;
}

bool EdgeImporter::GetPreviousVisitTime(FILETIME local_file_time,
                                        base::Time& previous_visit_time) {
  // Spartan stores local file time
  // Anaheim expects GMT based time hence conversion required
  TIME_ZONE_INFORMATION time_zone_information;
  if (GetTimeZoneInformation(&time_zone_information) == TIME_ZONE_ID_INVALID) {
    VLOG(1) << "GetPreviousVisitTime: Error getting time zone information: "
            << GetLastError();
    return false;
  }
  SYSTEMTIME system_time;
  if (!FileTimeToSystemTime(&local_file_time, &system_time)) {
    VLOG(1) << "GetPreviousVisitTime: Error file time to system time : "
            << GetLastError();
    return false;
  }
  SYSTEMTIME universal_time;
  if (!TzSpecificLocalTimeToSystemTime(&time_zone_information, &system_time,
                                       &universal_time)) {
    VLOG(1) << "GetPreviousVisitTime: Error local time to system time : "
            << GetLastError();
    return false;
  }
  FILETIME universal_file_time;
  if (!SystemTimeToFileTime(&universal_time, &universal_file_time)) {
    VLOG(1) << "GetPreviousVisitTime: Error system time to file time : "
            << GetLastError();
    return false;
  }
  previous_visit_time = base::Time::FromFileTime(universal_file_time);
  return true;
}

bool EdgeImporter::FetchHistoryProperties(IPropertyStore* history_store,
                                          WCHAR (&url_title)[MAX_PATH],
                                          unsigned long& visit_count,
                                          base::Time& previous_visit_time) {
  if (history_store == nullptr)
    return false;
  // ISF_HISTORY flag is used in spartan to check if it is a top url or not
  PROPVARIANT insite_flags_prop_var = {{{0}}};
  PROPERTYKEY const insite_flags_prop_key = {FMTID_InternetSite, kInsiteFlags};
  HRESULT hr =
      history_store->GetValue(insite_flags_prop_key, &insite_flags_prop_var);
  if (SUCCEEDED(hr) && (insite_flags_prop_var.vt != VT_EMPTY)) {
    if (!(insite_flags_prop_var.ulVal & ISF_HISTORY)) {
      PropVariantClear(&insite_flags_prop_var);
      return false;
    }
  }
  // Get previous visit time
  PROPVARIANT local_previous_visit_prop_var = {{{0}}};
  PROPERTYKEY const local_previous_visit_prop_key = {FMTID_InternetSite,
                                                     kLocalPreviousVisitTime};
  hr = history_store->GetValue(local_previous_visit_prop_key,
                               &local_previous_visit_prop_var);
  if (!(SUCCEEDED(hr) && (local_previous_visit_prop_var.vt == VT_FILETIME) &&
        GetPreviousVisitTime(local_previous_visit_prop_var.filetime,
                             previous_visit_time))) {
    PropVariantClear(&insite_flags_prop_var);
    PropVariantClear(&local_previous_visit_prop_var);
    return false;
  }
  // Get visit count from history_store
  PROPVARIANT visit_count_prop_var = {{{0}}};
  PROPERTYKEY const visit_count_prop_key = {FMTID_InternetSite, kVisitCount};
  hr = history_store->GetValue(visit_count_prop_key, &visit_count_prop_var);
  if (SUCCEEDED(hr) && (visit_count_prop_var.vt != VT_EMPTY)) {
    visit_count = visit_count_prop_var.ulVal;
  }

  // Get title
  PROPVARIANT title_prop_var = {{{0}}};
  PROPERTYKEY const title_prop_key = {FMTID_InternetSite, kTitleKey};
  hr = history_store->GetValue(title_prop_key, &title_prop_var);
  if (SUCCEEDED(hr) && title_prop_var.vt != VT_EMPTY) {
    if (wcscpy_s(url_title, wcslen(title_prop_var.pwszVal) + 1,
                 title_prop_var.pwszVal) != 0) {
      VLOG(1) << "FetchHistoryProperties: Error copying url title";
    }
  }
  PropVariantClear(&insite_flags_prop_var);
  PropVariantClear(&local_previous_visit_prop_var);
  PropVariantClear(&visit_count_prop_var);
  PropVariantClear(&title_prop_var);
  return true;
}

bool EdgeImporter::GetTitleAndVisitCountFromResposeHeader(
    std::vector<uint8_t> response_header,
    unsigned long response_header_size,
    WCHAR (&url_title)[MAX_PATH],
    unsigned long& visit_count,
    base::Time& previous_visit_time) {
  // Fetch roam property store from response headers
  Microsoft::WRL::ComPtr<IPropertyStore> history_property_store;
  RETURN_FALSE_IF_FAILED(
      PSCreateMemoryPropertyStore(IID_PPV_ARGS(&history_property_store)));
  Microsoft::WRL::ComPtr<IPersistStream> persist_roaming_store;
  RETURN_FALSE_IF_FAILED(history_property_store->QueryInterface(
      IID_PPV_ARGS(&persist_roaming_store)));
  Microsoft::WRL::ComPtr<IStream> memory_stream = SHCreateMemStream(
      static_cast<const BYTE*>(response_header.data()), response_header_size);
  if (memory_stream == nullptr)
    return false;
  const LARGE_INTEGER c_li0 = {{0, 0}};
  RETURN_FALSE_IF_FAILED(
      memory_stream->Seek(c_li0, STREAM_SEEK_SET, nullptr /*plibNewPosition*/));
  RETURN_FALSE_IF_FAILED(persist_roaming_store->Load(memory_stream.Get()));
  Microsoft::WRL::ComPtr<IPropertyStore> roam_store;
  RETURN_FALSE_IF_FAILED(persist_roaming_store.As(&roam_store));
  if (!FetchHistoryProperties(roam_store.Get(), url_title, visit_count,
                              previous_visit_time)) {
    // Fetch local property store if metadata not present in roam store
    Microsoft::WRL::ComPtr<IPersistStream> persist_local_store;
    RETURN_FALSE_IF_FAILED(
        PSCreateMemoryPropertyStore(IID_PPV_ARGS(&persist_local_store)));
    RETURN_FALSE_IF_FAILED(persist_local_store->Load(memory_stream.Get()));
    Microsoft::WRL::ComPtr<IPropertyStore> local_store;
    RETURN_FALSE_IF_FAILED(persist_local_store.As(&local_store));
    RETURN_FALSE_IF_FAILED(
        CopyPropertyStore(local_store.Get(), history_property_store.Get()));
    if (!FetchHistoryProperties(history_property_store.Get(), url_title,
                                visit_count, previous_visit_time)) {
      return false;
    }
  }
  return true;
}

// param container: Name of the container to be opened since various apps
// store data in different containers in Webcache
bool EdgeImporter::ParseHistoryContainers(
    std::unique_ptr<EdgeDatabaseTableEnumerator>& enumerator,
    std::vector<ImporterURLRow>& rows,
    const base::char16* container) {
  const std::string kSchemes[] = {url::kHttpScheme, url::kHttpsScheme,
                                  url::kFtpScheme, url::kFileScheme};
  do {
    base::string16 edge_history_dir;
    // get the Directory column in Containers table
    CONTINUE_LOOP_IF_FALSE(
        enumerator->RetrieveColumn(L"Directory", &edge_history_dir));
    // If Directory column matches with the edge history directory then fetch
    // container id for the same (this will be the container containing spatan
    // browsing history)
    if (edge_history_dir.find(container) != std::string::npos) {
      int64_t history_container_id = -1;
      if (!enumerator->RetrieveColumn(L"ContainerId", &history_container_id)) {
        VLOG(1) << "ParseHistoryContainers: Error fetching edge history "
                   "container id";
        return false;
      }
      base::string16 container = L"Container_";
      // open history_container_id
      std::unique_ptr<EdgeDatabaseTableEnumerator>
          history_container_enumerator = database_.OpenTableEnumerator(
              container.append(std::to_wstring(history_container_id)));
      if (history_container_enumerator == nullptr) {
        VLOG(1) << "ParseHistoryContainers: Error opening database table "
                << database_.GetErrorMessage();
        return false;
      }
      // Return true when there is no record.
      if (!history_container_enumerator->Reset()) {
        bool status = (history_container_enumerator->last_error() ==
                       JET_errNoCurrentRecord);
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::HISTORY,
            status ? ImportErrorState::kEdgeEnumeratorNoRecords
                   : ImportErrorState::kEdgeEnumeratorResetError);
        return status;
      }
      // parse container_id table
      do {
        base::string16 url_string;
        CONTINUE_LOOP_IF_FALSE(
            history_container_enumerator->RetrieveColumn(L"URL", &url_string));
        base::string16 trimmed_url_string =
            url_string.substr(url_string.find(L"@") + 1);
        GURL url = GURL(trimmed_url_string);
        // Filter out Url's having protocols mentioned in kSchemes
        if (!url.is_valid() || !base::Contains(kSchemes, url.scheme()))
          continue;
        // fetch response header from history table which contains url title,
        // visit count and previous visit time
        WCHAR url_title[MAX_PATH] = {0};
        unsigned long visit_count = 0;
        unsigned long response_header_size = 0;
        std::vector<uint8_t> response_header;
        bool return_value =
            history_container_enumerator->GetLongBinaryDataColumn(
                response_header, L"ResponseHeaders", &response_header_size);
        if (!return_value || (response_header.size() != response_header_size)) {
          VLOG(1) << "ParseHistoryContainers: Error getting response header";
          continue;
        }
        base::Time previous_visit_time;
        if (!GetTitleAndVisitCountFromResposeHeader(
                response_header, response_header_size, url_title, visit_count,
                previous_visit_time)) {
          VLOG(1) << "ParseHistoryContainers: Error getting title and visit "
                     "count from response header";
          continue;
        }
        // If the visit count is greater than 0, add the url in
        // ImporterURLRow, otherwise it is a redirected url.

        ImporterURLRow row(url);
        row.last_visit = previous_visit_time;
        row.visit_count = visit_count;
        row.hidden = false;
        row.title = url_title;
        rows.push_back(row);
      } while (history_container_enumerator->Next() && !cancelled());
      break;
    }
  } while (enumerator->Next() && !cancelled());
  return true;
}

// parse the headers received from the response header field of the container
bool EdgeImporter::ParseDownloadResponseHeader(
    const std::vector<uint8_t>& response_header,
    const unsigned long int& response_header_size,
    history::DownloadRow& download_row) {
  DownloadHistoryParseUtils::DownloadHistoryData* download_history_data =
      new DownloadHistoryParseUtils::DownloadHistoryData();
  if (!DownloadHistoryParseUtils::UnpackDownloadHistoryData(
          const_cast<LPBYTE>(response_header.data()),
          static_cast<const DWORD>(response_header_size),
          download_history_data)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HISTORY,
        ImportErrorState::kDownloadHistoryDataUnpackError);
    return false;
  }
  base::Time previous_visit_time = base::Time::Now();
  EdgeImporter::GetPreviousVisitTime(download_history_data->download_begin_time,
                                     previous_visit_time);
  download_row.current_path = base::FilePath::FromUTF16Unsafe(
      download_history_data->destination_file_path);
  download_row.target_path = base::FilePath::FromUTF16Unsafe(
      download_history_data->destination_file_path);
  download_row.referrer_url = GURL(download_history_data->origin_download_url);
  download_row.site_url = GURL(download_history_data->referrer_url);
  download_row.tab_url = download_row.site_url;
  download_row.tab_referrer_url = download_row.tab_url;
  download_row.http_method = "HTTP";
  download_row.mime_type = "";
  download_row.original_mime_type = "";
  download_row.start_time = previous_visit_time;
  download_row.end_time = previous_visit_time;
  download_row.etag = "";
  download_row.received_bytes = download_history_data->file_size;
  download_row.total_bytes = download_history_data->file_size;
  download_row.state = history::DownloadState::COMPLETE;
  download_row.danger_type = history::DownloadDangerType::NOT_DANGEROUS;
  history::DownloadInterruptReason reason =
      history::IntToDownloadInterruptReason(0);
  download_row.interrupt_reason = reason;
  download_row.hash = "";
  download_row.opened = false;
  download_row.last_access_time = download_row.end_time;
  download_row.by_ext_id = "";
  download_row.by_ext_name = "";
  std::vector<GURL> url_chain;
  url_chain.push_back(GURL(download_history_data->referrer_url));
  url_chain.push_back(GURL(download_history_data->origin_download_url));
  download_row.url_chain = url_chain;
  DownloadHistoryParseUtils::DeleteDownloadHistoryData(download_history_data);
  return true;
}

// Go over the available containers to find the download history container and
// parse information from the particular container
bool EdgeImporter::ParseDownloadHistoryContainer(
    std::unique_ptr<EdgeDatabaseTableEnumerator>& enumerator,
    std::vector<history::DownloadRow>& download_rows) {
  do {
    base::string16 current_dir;
    // get the Directory column in Containers table
    CONTINUE_LOOP_IF_FALSE(
        enumerator->RetrieveColumn(L"Directory", &current_dir));
    if (current_dir.find(source_path_.value() +
                         kDownloadHistoryContainerTitle) != std::string::npos) {
      int64_t container_id = -1;
      if (!enumerator->RetrieveColumn(L"ContainerId", &container_id)) {
        VLOG(1) << "DownloadHistoryLog : Error fetching edge "
                   "download history "
                   "container id";
        return false;
      }
      base::string16 container = L"Container_";
      // open history_container_id
      std::unique_ptr<EdgeDatabaseTableEnumerator>
          download_history_container_enumerator = database_.OpenTableEnumerator(
              container.append(std::to_wstring(container_id)));
      if (download_history_container_enumerator == nullptr) {
        VLOG(1) << "DownloadHistoryLog : Error opening database "
                   "table "
                << database_.GetErrorMessage();
        return false;
      }
      // Return true when there is no record.
      if (!download_history_container_enumerator->Reset()) {
        bool status = (download_history_container_enumerator->last_error() ==
                       JET_errNoCurrentRecord);
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::HISTORY,
            status ? ImportErrorState::kEdgeEnumeratorNoRecords
                   : ImportErrorState::kEdgeEnumeratorResetError);
        return status;
      }
      // parse container_id table
      do {
        unsigned long response_header_size = 0;
        std::vector<uint8_t> response_header;
        if (!download_history_container_enumerator->GetLongBinaryDataColumn(
                response_header, L"ResponseHeaders", &response_header_size) ||
            (response_header.size() != response_header_size)) {
          VLOG(1) << "DownloadHistoryLog: Error when fetching response header "
                     "from the data enumerator";
          continue;
        }

        history::DownloadRow download_row;
        if (!ParseDownloadResponseHeader(response_header, response_header_size,
                                         download_row)) {
          ImporterProfileMetrics::LogImporterErrorMetric(
              importer::TYPE_EDGE, importer::HISTORY,
              ImportErrorState::kParseDownloadResponseHeaderError);
          continue;
        }
        // hash is a string , the datatype: int64_t
        int64_t hash_value = 0;
        if (!download_history_container_enumerator->RetrieveColumn(
                L"UrlHash", &hash_value)) {
          VLOG(1) << "DownloadHistoryLog : Unable to obtain "
                     "URL hash value";
          continue;
        }

        FILETIME last_access_filetime;
        if (!download_history_container_enumerator->RetrieveColumn(
                L"AccessedTime", &last_access_filetime)) {
          VLOG(1) << "DownloadHistoryLog : Unable to fetch "
                     "last access file time value";
          continue;
        }

        base::Time last_access = base::Time::FromFileTime(last_access_filetime);
        download_row.hash = base::NumberToString(hash_value);
        download_row.guid = base::GenerateGUID();
        download_row.last_access_time = last_access;

        base::Time::Exploded* exploded_time_value = new base::Time::Exploded();
        last_access.LocalExplode(exploded_time_value);
        std::string full_inflated_time =
            DownloadHistoryParseUtils::GetWeekday(
                exploded_time_value->day_of_week) +
            ", " + to_string(exploded_time_value->day_of_month) + " " +
            DownloadHistoryParseUtils::GetMonth(exploded_time_value->month) +
            " " + to_string(exploded_time_value->year) + " " +
            to_string(exploded_time_value->hour) + ":" +
            to_string(exploded_time_value->minute) + ":" +
            to_string(exploded_time_value->second);
        download_row.last_modified = full_inflated_time;
        download_rows.push_back(download_row);
      } while (download_history_container_enumerator->Next() && !cancelled());
      break;
    }
  } while (enumerator->Next() && !cancelled());
  return true;
}

bool EdgeImporter::ImportHistory() {
  std::vector<ImporterURLRow> rows;
  // open Containers table in WebCacheV01.dat
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"Containers");
  if (enumerator == nullptr) {
    VLOG(1) << "ImportHistory: Error opening database table "
            << database_.GetErrorMessage();
    return false;
  }
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HISTORY,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  // Fetching all internet URL's
  if (!ParseHistoryContainers(enumerator, rows,
                              kInternetUrlsHistoryContainer)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HISTORY,
        ImportErrorState::kParseSpartanInternetContainerError);
    return false;
  }

  // Fetching all intranet URL's
  if (!ParseHistoryContainers(enumerator, rows,
                              kIntranetUrlsHistoryContainer)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HISTORY,
        ImportErrorState::kParseSpartanIntranetContainerError);
    return false;
  }

  if (!cancelled()) {
    bridge_->SetHistoryItems(rows, importer::VISIT_SOURCE_EDGE_IMPORTED);
  }

  if (base::FeatureList::IsEnabled(
          features::edge::kEdgeDownloadHistoryImport)) {
    std::vector<history::DownloadRow> download_rows;
    base::TimeTicks download_history_import_start_time = base::TimeTicks::Now();
    if (!enumerator->Reset()) {
      bool status = (enumerator->last_error() ==
                     JET_errNoCurrentRecord);
      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, importer::HISTORY,
          status ? ImportErrorState::kEdgeEnumeratorNoRecords
                 : ImportErrorState::kEdgeEnumeratorResetError);
      return status;
    }
    VLOG(1) << "DownloadHistoryLog : Importing download history from Spartan";
    if (!ParseDownloadHistoryContainer(enumerator, download_rows)) {
      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, importer::HISTORY,
          ImportErrorState::kParseDownloadHistoryContainerError);
      return false;
    }
    if (!cancelled()) {
      bridge_->SetDownloadHistoryItem(download_rows);
      ImporterProfileMetrics::LogEdgeDownloadHistoryImportPerfMetric(
          base::TimeTicks::Now() - download_history_import_start_time);
    }
  }
  return true;
}

bool EdgeImporter::ImportCreditCards() {
  std::vector<ImporterAutofillCreditCardDataEntry> payment_cards;
  if (!ParseAutoFormFillPaymentStorageDatabase(&payment_cards))
    return false;

  if (!payment_cards.empty() && !cancelled()) {
    bridge_->SetAutofillCreditCardData(payment_cards);
  }
  return true;
}

bool EdgeImporter::ImportAutofillFormData() {
  std::vector<ImporterAutofillFormDataEntry> form_entries;
  ParseGenericAutoSuggestStorageDatabase(&form_entries);

  if (!form_entries.empty() && !cancelled()) {
    bridge_->SetAutofillFormData(form_entries);
  }
  return true;
}

bool EdgeImporter::ImportReadingList() {
  favicon_base::FaviconUsageDataList favicons;
  std::vector<ImportedBookmarkEntry> rl_bookmarks;
  if (!ParseReadingListDatabase(&rl_bookmarks, &favicons)) {
    return false;
  }

  if (!rl_bookmarks.empty() && !cancelled()) {
    const base::string16& reading_list_folder_name =
        l10n_util::GetStringUTF16(IDS_BOOKMARK_GROUP_READING_LIST);
    bridge_->AddBookmarks(rl_bookmarks, reading_list_folder_name);
  }

  if (!favicons.empty() && !cancelled())
    bridge_->SetFavicons(favicons);
  return true;
}

bool EdgeImporter::ImportTabSweep() {
  favicon_base::FaviconUsageDataList favicons;
  std::vector<ImportedBookmarkEntry> ts_bookmarks;
  if (!ParseTabSweepDatabase(&ts_bookmarks, &favicons)) {
    return false;
  }

  if (!ts_bookmarks.empty() && !cancelled()) {
    const base::string16& tab_sweep_folder_name =
        l10n_util::GetStringUTF16(IDS_BOOKMARK_GROUP_TAB_SWEEP);
    bridge_->AddBookmarks(ts_bookmarks, tab_sweep_folder_name);
  }
  if (!favicons.empty() && !cancelled()) {
    bridge_->SetFavicons(favicons);
  }

  return true;
}

bool EdgeImporter::ImportTopSites() {
  std::vector<ImporterURLRow> dynamic_top_sites;
  std::vector<ImporterURLRow> custom_top_sites;
  if (!ParseTopSitesDatabase(&dynamic_top_sites, &custom_top_sites)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TOP_SITES,
        ImportErrorState::kSpartanTopSitesParseError);
    return false;
  }

  if (!custom_top_sites.empty() && !cancelled()) {
    bridge_->SetCustomLinks(custom_top_sites);
  }

  if (!dynamic_top_sites.empty() && !cancelled()) {
    // Segments need to be added to urls table before
    // being added to the segments table
    bridge_->SetHistoryItems(dynamic_top_sites,
                             importer::VISIT_SOURCE_EDGE_IMPORTED);
    bridge_->SetSegmentItems(dynamic_top_sites);
  }

  return true;
}

bool EdgeImporter::ImportTabs(importer::ImportItem types) {
  if (!recovery_store_) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TAB_SWEEP,
        ImportErrorState::kEdgeRecoveryStoreNotInitialized);
    return false;
  }

  unsigned int frame_count = 0;
  if (FAILED(recovery_store_->GetFrameCount(&frame_count))) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TAB_SWEEP,
        ImportErrorState::kEdgeFramesNotFound);
    return false;
  }

  // Tabs should be acquired in order relative to their frame's activation time.
  std::vector<unsigned int> frame_indices;
  if (FAILED(recovery_store_->GetFrameIndicesSortedByActivationTime(
          &frame_indices))) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TAB_SWEEP,
        ImportErrorState::kEdgeFrameIndicesNotOrdered);
    return false;
  }

  std::vector<importer::TabInfo> tabs;
  int discovered_count = 0;
  uint32_t error_count = 0;
  int insertion_error_count = 0;
  for (unsigned int frame_index : frame_indices)
    recovery_store_->GetFrameTabData(frame_index, &tabs, &error_count);

  discovered_count = tabs.size() + error_count;

  // Copy over only visible, non-private tabs.
  std::vector<importer::TabInfo> visible_tabs;
  std::copy_if(tabs.begin(), tabs.end(), std::back_inserter(visible_tabs),
               [](importer::TabInfo item) {
                 return !(item.is_hidden || item.is_private);
               });

  // Switch out Spartan NTP for Anaheim NTP.
  for (auto it = visible_tabs.begin(); it != visible_tabs.end(); ++it) {
    if (IsSpartanNewTabPageURL(it->url)) {
      it->url = GURL(chrome::kChromeUINewTabURL);
    }
  }

  if (!visible_tabs.empty() && !cancelled())
    bridge_->SetTabs(visible_tabs);

  insertion_error_count = tabs.size() - visible_tabs.size();

  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::OPEN_TABS, importer::DISCOVERED,
      discovered_count);
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::OPEN_TABS, importer::EXTRACTED,
      tabs.size());
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::OPEN_TABS, importer::EXTRACTION_ERROR,
      error_count);
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::OPEN_TABS, importer::INSERTED,
      visible_tabs.size());
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::OPEN_TABS, importer::INSERTION_ERROR,
      insertion_error_count);
  if (discovered_count > 0) {
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::OPEN_TABS, importer::EXTRACTED,
        static_cast<int>((tabs.size() * 100) / discovered_count));
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::OPEN_TABS, importer::EXTRACTION_ERROR,
        static_cast<int>((error_count * 100) / discovered_count));
  }
  if (tabs.size() > 0) {
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::OPEN_TABS, importer::INSERTED,
        static_cast<int>((visible_tabs.size() * 100) / tabs.size()));
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::OPEN_TABS, importer::INSERTION_ERROR,
        static_cast<int>((insertion_error_count * 100) / tabs.size()));
  }

  return true;
}

bool EdgeImporter::ImportCookies() {
  std::unique_ptr<base::Value> json;

  // Testing hook to read from a json file instead of IPC during tests
  if (base::FeatureList::IsEnabled(
          features::edge::kEnableVirtualCookieImportForTesting)) {
    JSONFileValueDeserializer json_deserializer(
        source_path_.AppendASCII("cookies.json"));
    int error_code = 0;
    std::string error_message;
    json = json_deserializer.Deserialize(&error_code, &error_message);
    return OnCookieImport(std::move(json));
  } else {
    EdgeCookieImporter cookie_importer(source_path_);
    CookieImportResult result = cookie_importer.ImportCookies();

    if (result.first) {
      DCHECK(!result.second);

      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, importer::COOKIES, result.first.value());
      return OnCookieImport(nullptr);
    } else {
      // The Microsoft.ImportProfile.ImportTypeSuccess.*Import.EDGE.Cookies
      // histogram tracks cookie import success so no emission is necessary
      // here.
      return OnCookieImport(
          std::make_unique<base::Value>(std::move(result.second.value())));
    }
  }
}

bool EdgeImporter::OnCookieImport(std::unique_ptr<base::Value> json) {
  std::vector<ImporterCanonicalCookie> cookies;

  if (!json) {
    return false;
  }

  std::unique_ptr<base::ListValue> json_list =
      base::ListValue::From(std::move(json));

  if (!json_list) {
    return false;
  }

  int error_count = 0;

  for (const auto& json_cookie_dict : json_list->GetList()) {
    base::Optional<ImporterCanonicalCookie> cookie =
        CreateImporterCanonicalCookieFromJson(json_cookie_dict);
    if (cookie) {
      cookies.emplace_back(std::move(cookie.value()));
    } else {
      error_count++;
      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, importer::COOKIES,
          ImportErrorState::kCanonicalCookieImporterCreationFailed);
    }
  }

  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::COOKIES, importer::DISCOVERED,
      json_list->GetList().size());
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::COOKIES, importer::EXTRACTED,
      cookies.size());
  ImporterProfileMetrics::LogImportDetailCount(
      importer::TYPE_EDGE, importer::COOKIES, importer::EXTRACTION_ERROR,
      error_count);
  int json_size = json_list->GetList().size();
  if (json_size > 0) {
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::COOKIES, importer::EXTRACTED,
        static_cast<int>((cookies.size() * 100) / json_size));

    ImporterProfileMetrics::LogImportDetailRatio(
        importer::TYPE_EDGE, importer::COOKIES, importer::EXTRACTION_ERROR,
        static_cast<int>((error_count * 100) / json_size));
  }

  if (!cancelled()) {
    bridge_->SetCookies(std::move(cookies));
  }

  return true;
}

// From Edge 13 (released with Windows 10 TH2), Favorites are stored in a JET
// database within the Edge local storage. The import uses the ESE library to
// open and read the data file. The data is stored in a Favorites table with
// the following schema.
// Column Name          Column Type
// ------------------------------------------
// DateUpdated          LongLong - FILETIME
// FaviconFile          LongText - Relative path
// HashedUrl            ULong
// IsDeleted            Bit
// IsFolder             Bit
// ItemId               Guid
// OrderNumber          LongLong
// ParentId             Guid
// RoamDisabled         Bit
// RowId                Long
// Title                LongText
// URL                  LongText
bool EdgeImporter::ParseFavoritesDatabase(
    std::vector<ImportedBookmarkEntry>* bookmarks,
    favicon_base::FaviconUsageDataList* favicons,
    bool* should_show_fav_bar_on_ntp) {
  DCHECK(should_show_fav_bar_on_ntp);
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"Favorites");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::FAVORITES,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::FAVORITES,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  std::map<GUID, importer::EdgeFavoriteEntry, importer::GuidComparator>
      database_entries;
  base::FilePath favicon_base =
      source_path_.empty() ? importer::GetEdgeDataFilePath() : source_path_;
  favicon_base = favicon_base.Append(L"DataStore");

  bool has_atleast_one_entry_in_fav_bar = false;
  int discovered_count = 0;
  int extracted_count = 0;
  int error_count = 0;
  do {
    importer::EdgeFavoriteEntry entry;
    bool is_deleted = false;
    if (!enumerator->RetrieveColumn(L"IsDeleted", &is_deleted))
      LOG_ERROR_AND_CONTINUE("IsDeleted column not found.");
    if (is_deleted)
      LOG_ERROR_AND_CONTINUE("Favorites entry marked with IsDeleted.");
    bool is_orphaned = false;
    if (enumerator->RetrieveColumn(L"IsOrphaned", &is_orphaned)) {
      if (is_orphaned)
        LOG_ERROR_AND_CONTINUE("Favorites entry marked with IsOrphaned.");
    }
    if (!enumerator->RetrieveColumn(L"IsFolder", &entry.is_folder))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kIsFolderColumnNotFound, error_count);
    if (!entry.is_folder)
      discovered_count++;
    base::string16 url;
    if (!enumerator->RetrieveColumn(L"URL", &url))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kUrlColumnNotFound, error_count);
    entry.url = GURL(url);
    bool is_webnote = false;
    // No column with IsWebNotes is completely valid. Hence not logging error.
    enumerator->RetrieveColumn(L"IsWebNotes", &is_webnote);
    if (is_webnote)
      CollectWebnotes(url, entry.source,
                      webnotes::WebnoteMigrationState::FAVORITES_WEBNOTE_FOUND);
    if (!entry.is_folder && !entry.url.is_valid())
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kInvalidUrl, error_count);
    if (!enumerator->RetrieveColumn(L"Title", &entry.title))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kTitleColumnNotFound, error_count);
    base::string16 favicon_file;
    if (!enumerator->RetrieveColumn(L"FaviconFile", &favicon_file))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kFaviconFileColumnNotFound, error_count);
    if (!favicon_file.empty())
      entry.favicon_file = favicon_base.Append(favicon_file);
    if (!enumerator->RetrieveColumn(L"ParentId", &entry.parent_id))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kParentIdColumnNotFound, error_count);
        // Check if there is a bookmark in favorites bar.
    if (!has_atleast_one_entry_in_fav_bar &&
        memcmp(&entry.parent_id, &kFavBarGUID, sizeof(GUID)) == 0) {
      has_atleast_one_entry_in_fav_bar = true;
    }
    if (!enumerator->RetrieveColumn(L"ItemId", &entry.item_id))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kItemIdColumnNotFound, error_count);
    if (!enumerator->RetrieveColumn(L"OrderNumber", &entry.order_number))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kOrderNumberColumnNotFound, error_count);
    FILETIME data_updated;
    if (!enumerator->RetrieveColumn(L"DateUpdated", &data_updated))
      LOG_ERROR_METRIC_AND_CONTINUE(importer::FAVORITES,
        ImportErrorState::kDateUpdatedColumnNotFound, error_count);
    entry.date_updated = base::Time::FromFileTime(data_updated);
    if (!entry.is_folder)
      extracted_count++;
    database_entries[entry.item_id] = entry;
  } while (enumerator->Next() && !cancelled());

  // Count values for metrics
  ImporterProfileMetrics::LogImportDetailCount(
      importer::ImporterType::TYPE_EDGE, importer::ImportItem::FAVORITES,
      importer::ImportDetailType::DISCOVERED, discovered_count);
  ImporterProfileMetrics::LogImportDetailCount(
      importer::ImporterType::TYPE_EDGE, importer::ImportItem::FAVORITES,
      importer::ImportDetailType::EXTRACTED, extracted_count);
  ImporterProfileMetrics::LogImportDetailCount(
      importer::ImporterType::TYPE_EDGE, importer::ImportItem::FAVORITES,
      importer::ImportDetailType::EXTRACTION_ERROR, error_count);
  // Ratio values for metrics
  if (discovered_count > 0) {
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::ImporterType::TYPE_EDGE, importer::ImportItem::FAVORITES,
        importer::ImportDetailType::EXTRACTED,
        static_cast<int>((extracted_count * 100) / discovered_count));
    ImporterProfileMetrics::LogImportDetailRatio(
        importer::ImporterType::TYPE_EDGE, importer::ImportItem::FAVORITES,
        importer::ImportDetailType::EXTRACTION_ERROR,
        static_cast<int>((error_count * 100) / discovered_count));
  }

  importer::EdgeFavoriteEntry root_entry;
  BuildFavoritesTree(database_entries, &root_entry);

  std::vector<base::string16> path;
  *should_show_fav_bar_on_ntp = has_atleast_one_entry_in_fav_bar;
  // If favorites exists in favorites bar of spartan then import hub favorites
  // to other favorites. If favorites doesn't exists in favorites bar then
  // import hub favorites to favorites bar. This is controlled by
  // |has_atleast_one_entry_in_fav_bar|
  BuildBookmarkEntries(root_entry, !has_atleast_one_entry_in_fav_bar, bookmarks,
                       favicons, &path);

  return true;
}
bool EdgeImporter::ParseReadingListDatabase(
    std::vector<ImportedBookmarkEntry>* bookmarks,
    favicon_base::FaviconUsageDataList* favicons) {
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"ReadingList");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::READING_LIST,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::READING_LIST,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  std::map<GUID, importer::EdgeFavoriteEntry, importer::GuidComparator>
      database_entries;

  // Root entry folder for reading list links
  importer::EdgeFavoriteEntry rootEntry;
  rootEntry.is_folder = true;
  rootEntry.title = l10n_util::GetStringUTF16(IDS_BOOKMARK_GROUP_READING_LIST);
  rootEntry.source = bookmarks::BookmarkNode::Source::IMPORT_READING_LIST;
  if (CoCreateGuid(&rootEntry.parent_id) != S_OK) {
    return false;
  }

  if (CoCreateGuid(&rootEntry.item_id) != S_OK) {
    return false;
  }

  base::FilePath favicon_base =
      source_path_.empty() ? importer::GetEdgeDataFilePath() : source_path_;
  favicon_base = favicon_base.Append(L"DataStore");

  do {
    importer::EdgeFavoriteEntry entry;
    entry.is_folder = false;
    entry.parent_id = rootEntry.item_id;
    entry.in_other_bookmarks = true;
    entry.source = bookmarks::BookmarkNode::Source::IMPORT_READING_LIST;

    if (CoCreateGuid(&entry.item_id) != S_OK) {
      return false;
    }

    bool is_deleted = false;
    if (!enumerator->RetrieveColumn(L"IsDeleted", &is_deleted) || is_deleted)
      LOG_ERROR_AND_CONTINUE("ReadingList - IsDeleted column not found.");
    base::string16 url;
    if (!enumerator->RetrieveColumn(L"URL", &url))
      LOG_ERROR_AND_CONTINUE("ReadingList - URL column not found.");
    entry.url = GURL(url);
    if (IsReadingListEntryAWebNote(url))
      CollectWebnotes(
          url, entry.source,
          webnotes::WebnoteMigrationState::READING_LIST_WEBNOTE_FOUND);
    if (!enumerator->RetrieveColumn(L"Title", &entry.title))
      LOG_ERROR_AND_CONTINUE("ReadingList - Title column not found.");
    base::string16 favicon_file;
    if (!enumerator->RetrieveColumn(L"DominantImageFile", &favicon_file))
      LOG_ERROR_AND_CONTINUE(
          "ReadingList - DominantImageFile column not found.");
    if (!favicon_file.empty())
      entry.favicon_file = favicon_base.Append(favicon_file);
    FILETIME data_updated;
    if (!enumerator->RetrieveColumn(L"UpdatedDate", &data_updated)) {
      LOG_ERROR_AND_CONTINUE("ReadingList - UpdatedDate column not found.");
    }
    entry.date_updated = base::Time::FromFileTime(data_updated);
    database_entries[entry.item_id] = entry;
  } while (enumerator->Next() && !cancelled());

  if (database_entries.empty()) {
    // Reading List entries are stored in the data base. When a delete happens
    // 1) "is_deleted" entry in data base is updated first
    // 2) the actual deletion happens later asynchronously
    // so, the previous check of no records will be passed in some cases like -
    // all the reading list items are deleted.

    // Return true when there are no valid records, stop migrating RL.
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::READING_LIST,
        ImportErrorState::kEdgeEnumeratorNoRecords);
    return true;
  }
  database_entries[rootEntry.item_id] = rootEntry;

  // Build simple tree.
  importer::EdgeFavoriteEntry root_entry;
  for (auto& entry : database_entries) {
    auto found_parent = database_entries.find(entry.second.parent_id);
    if (found_parent == database_entries.end() ||
        !found_parent->second.is_folder) {
      root_entry.children.push_back(&entry.second);
    } else {
      found_parent->second.children.push_back(&entry.second);
    }
  }
  // With tree built sort the children of each node including the root.
  std::sort(root_entry.children.begin(), root_entry.children.end(),
            importer::EdgeFavoriteEntryComparator());
  for (auto& entry : database_entries) {
    std::sort(entry.second.children.begin(), entry.second.children.end(),
              importer::EdgeFavoriteEntryComparator());
  }
  std::vector<base::string16> path;
  BuildBookmarkEntries(root_entry, false, bookmarks, favicons, &path);
  return true;
}

bool EdgeImporter::IsReadingListEntryAWebNote(const base::string16& url) {
  // There is NO column in the Reading list table of spartan DB to differentiate
  // between reading lists and web notes, hence going by file path prefix and
  // .html extension as web notes are the only html files among reading list.

  // Considering the fact that webnotes will be in less number,
  // the first check of html extension helps ignoring non-webnotes fast as
  // EndsWith is way faster than find.
  if (base::EndsWith(url, L".html", base::CompareCase::SENSITIVE) &&
      (url.find(webnotes_old_rel_path)) != base::string16::npos)
    return true;
  else
    return false;
}

void EdgeImporter::CollectWebnotes(
    const base::string16& url,
    bookmarks::BookmarkNode::Source& source,
    webnotes::WebnoteMigrationState migration_state) {
  webnote_spartan_paths_.push_back(url);
  // Updating the source of webnotes to IMPORT_WEBNOTES.
  source = bookmarks::BookmarkNode::Source::IMPORT_WEBNOTES;
  webnotes::LogWebnoteMigrationMetric(migration_state);
}

// Swept tabs are stored in a JET database within the Edge local storage.
// The import uses the ESE library to open and read the data file.
// The data is stored in a Swept Tabs table with the following schema.
// Column Name          Column Type
// ------------------------------------------
// DateSwept          LongLong
// SweepGroupId       Guid
// Title              LongText
// URL                LongText

bool EdgeImporter::ParseTabSweepDatabase(
    std::vector<ImportedBookmarkEntry>* bookmarks,
    favicon_base::FaviconUsageDataList* favicons) {
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"SweptTabs");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TAB_SWEEP,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TAB_SWEEP,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  // Root entry folder for tab sweep data
  importer::EdgeFavoriteEntry folder_root_entry;
  folder_root_entry.is_folder = true;
  folder_root_entry.title =
      l10n_util::GetStringUTF16(IDS_BOOKMARK_GROUP_TAB_SWEEP);
  folder_root_entry.source =
      bookmarks::BookmarkNode::Source::IMPORT_TABS_SET_ASIDE;

  if (CoCreateGuid(&folder_root_entry.parent_id) != S_OK) {
    return false;
  }

  if (CoCreateGuid(&folder_root_entry.item_id) != S_OK) {
    return false;
  }

  std::map<GUID, importer::EdgeFavoriteEntry, importer::GuidComparator>
      database_entries;
  database_entries[folder_root_entry.item_id] = folder_root_entry;

  std::set<GUID, importer::GuidComparator> swept_tab_groups;

  // Create tab sweep entries in folders
  do {
    // Create folders for tab sweep groups
    GUID group_id;
    if (!enumerator->RetrieveColumn(L"SweepGroupId", &group_id))
      continue;

    // Check if group already exists; if not, create a new folder entry.
    if (swept_tab_groups.find(group_id) == swept_tab_groups.end()) {
      importer::EdgeFavoriteEntry folder_entry;
      folder_entry.is_folder = true;
      folder_entry.source =
          bookmarks::BookmarkNode::Source::IMPORT_TABS_SET_ASIDE;
      folder_entry.item_id = group_id;
      folder_entry.parent_id = folder_root_entry.item_id;

      const base::string16 group_number =
          base::NumberToString16(swept_tab_groups.size() + 1);
      folder_entry.title = l10n_util::GetStringFUTF16(
          IDS_BOOKMARK_GROUP_TAB_SWEEP_GROUP, group_number);
      swept_tab_groups.insert(group_id);
      database_entries[folder_entry.item_id] = folder_entry;

      folder_root_entry.children.push_back(
          &database_entries[folder_entry.item_id]);
    }

    importer::EdgeFavoriteEntry entry;
    entry.is_folder = false;
    entry.source = bookmarks::BookmarkNode::Source::IMPORT_TABS_SET_ASIDE;

    if (CoCreateGuid(&entry.item_id) != S_OK) {
      return false;
    }

    entry.parent_id = group_id;
    if (!enumerator->RetrieveColumn(L"Title", &entry.title))
      continue;
    base::string16 url;
    if (!enumerator->RetrieveColumn(L"URL", &url))
      continue;
    entry.url = GURL(url);
    FILETIME data_updated;
    if (!enumerator->RetrieveColumn(L"DateSwept", &data_updated))
      continue;
    entry.date_updated = base::Time::FromFileTime(data_updated);
    entry.in_other_bookmarks = true;

    // Extract favicon if available.
    GUID tab_id;
    if (recovery_store_ && enumerator->RetrieveColumn(L"RecoveryGuid", &tab_id))
      recovery_store_->GetTabFavicon(tab_id, &entry.favicon_data);

    database_entries[entry.item_id] = entry;

    auto found_parent = database_entries.find(entry.parent_id);
    found_parent->second.children.push_back(&database_entries[entry.item_id]);
  } while (enumerator->Next() && !cancelled());

  // With tree built, sort the children of each node including the root.
  std::sort(folder_root_entry.children.begin(),
            folder_root_entry.children.end(),
            importer::EdgeFavoriteEntryComparator());
  for (auto& entry : database_entries) {
    std::sort(entry.second.children.begin(), entry.second.children.end(),
              importer::EdgeFavoriteEntryComparator());
  }

  // Creating this root_entry is necessary because BuildBookmarkEntries only
  // loops through the children of the root entry we pass it. One of these
  // children needs to be our actual Tab Sweep folder entry, folder_root_entry.
  importer::EdgeFavoriteEntry root_entry;
  root_entry.children.push_back(&folder_root_entry);

  std::vector<base::string16> path;
  BuildBookmarkEntries(root_entry, false, bookmarks, favicons, &path);
  return true;
}

bool EdgeImporter::ParseTopSitesDatabase(
    std::vector<ImporterURLRow>* dynamic_rows,
    std::vector<ImporterURLRow>* custom_rows) {
  const std::string kSchemes[] = {url::kHttpScheme, url::kHttpsScheme,
                                  url::kFtpScheme, url::kFileScheme};
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"TopSites");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TOP_SITES,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::TOP_SITES,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  do {
    // Filter out tombstoned urls
    bool is_tombstoned = false;
    CONTINUE_LOOP_IF_FALSE(
        enumerator->RetrieveColumn(L"IsTombstoned", &is_tombstoned));
    if (is_tombstoned)
      continue;

    base::string16 url_string;
    CONTINUE_LOOP_IF_FALSE(enumerator->RetrieveColumn(L"URL", &url_string));
    GURL url = GURL(url_string);

    // Filter out URLs having protocols mentioned in kSchemes
    if (!url.is_valid() || !base::Contains(kSchemes, url.scheme()))
      LOG_ERROR_AND_CONTINUE("Invalid top site URL");

    ImporterURLRow row(url);

    // Ideally this value should be the RVC count from Spartan,
    // hardcoding this for now since it will be eventually overwritten
    row.visit_count = 7;

    int64_t last_visit = 0;
    CONTINUE_LOOP_IF_FALSE(
        enumerator->RetrieveColumn(L"UpdatedDateTimeUTC", &last_visit));
    row.last_visit = base::Time::FromDeltaSinceWindowsEpoch(
        base::TimeDelta::FromMicroseconds(last_visit));

    base::string16 title;
    enumerator->RetrieveColumn(L"Title", &title);
    row.title = title;

    row.hidden = 1;

    int source_type;
    CONTINUE_LOOP_IF_FALSE(
        enumerator->RetrieveColumn(L"SourceType", &source_type));
    if (source_type == TS_SOURCE_TYPE_CUSTOM)
      custom_rows->push_back(row);
    else if (source_type == TS_SOURCE_TYPE_DYNAMIC)
      dynamic_rows->push_back(row);
  } while (enumerator->Next() && !cancelled());

  return true;
}

base::Optional<ImporterCanonicalCookie>
EdgeImporter::CreateImporterCanonicalCookieFromJson(const base::Value& json) {
  ImporterCanonicalCookie cookie;

  const base::Value* name =
      json.FindKeyOfType("name", base::Value::Type::STRING);
  const base::Value* value =
      json.FindKeyOfType("value", base::Value::Type::STRING);
  const base::Value* domain =
      json.FindKeyOfType("domain", base::Value::Type::STRING);
  const base::Value* path =
      json.FindKeyOfType("path", base::Value::Type::STRING);
  const base::Value* creation =
      json.FindKeyOfType("creation", base::Value::Type::DOUBLE);
  const base::Value* expiration =
      json.FindKeyOfType("expiration", base::Value::Type::DOUBLE);
  const base::Value* last_access =
      json.FindKeyOfType("last_access", base::Value::Type::DOUBLE);
  const base::Value* secure =
      json.FindKeyOfType("secure", base::Value::Type::BOOLEAN);
  const base::Value* httponly =
      json.FindKeyOfType("httponly", base::Value::Type::BOOLEAN);
  const base::Value* hostonly =
      json.FindKeyOfType("hostonly", base::Value::Type::BOOLEAN);
  const base::Value* edgelegacycookie =
      json.FindKeyOfType("edgelegacy", base::Value::Type::BOOLEAN);
  const base::Value* same_site =
      json.FindKeyOfType("same_site", base::Value::Type::INTEGER);

  // base::Value requires callers to null check before extracting values.
  // The cookie values in the null check here define the ones necessary
  // for proper cookie function, and the cookie should be skipped if these
  // json properties are omitted
  if (!name || !value || !domain || !path || !expiration || !secure ||
      !httponly || !hostonly) {
    return base::nullopt;
  }
  cookie.name = name->GetString();
  cookie.value = value->GetString();
  cookie.domain = domain->GetString();
  cookie.path = path->GetString();
  cookie.creation = creation ? base::Time::FromJsTime(creation->GetDouble())
                             : base::Time::Now();
  cookie.expiration = base::Time::FromJsTime(expiration->GetDouble());
  cookie.last_access = last_access
                           ? base::Time::FromJsTime(last_access->GetDouble())
                           : base::Time::Now();
  cookie.secure = secure->GetBool();
  cookie.httponly = httponly->GetBool();
  cookie.hostonly = hostonly->GetBool();
  cookie.edgelegacycookie =
      edgelegacycookie ? edgelegacycookie->GetBool() : false;
  if (same_site) {
    cookie.same_site = static_cast<net::CookieSameSite>(same_site->GetInt());
  }
  return cookie;
}

bool EdgeImporter::ParseAutoFormFillPaymentStorageDatabase(
    std::vector<ImporterAutofillCreditCardDataEntry>* payment_cards) {
  // AutoFormFillPaymentStorage was added in RS4. Import
  // payment cards only if windows version is RS4 or above.
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"AutoFormFillPaymentStorage");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::PAYMENTS,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::PAYMENTS,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  do {
    ImporterAutofillCreditCardDataEntry entry;
    bool is_deleted = false;
    if (!enumerator->RetrieveColumn(L"IsDeleted", &is_deleted) || is_deleted)
      continue;

    // TODO (Bug 19700235) :
    // microsoft.visualstudio.com/Edge/_workitems/edit/19700235
    // Handles schema change in AutoFormFillPaymentStorage from RS4 to RS5
    // where card holder full name moved from CardHolderFirstName to a new
    // column CardHolderFullName. However sets last_error_ to
    // JET_errColumnNotFound in RS4.
    if (!RetrieveColumn(enumerator.get(), L"CardHolderFullName",
                        entry.full_name))
      if (!RetrieveColumn(enumerator.get(), L"CardHolderFirstName",
                          entry.full_name))
        continue;
    if (!RetrieveColumn(enumerator.get(), L"CreditCardNumber",
                        entry.credit_card_number))
      continue;
    base::string16 expiry_month;
    if (!RetrieveColumn(enumerator.get(), L"ExpiryMonth", expiry_month))
      continue;
    entry.expiration_month = _wtoi(expiry_month.c_str());
    base::string16 expiry_year;
    if (!RetrieveColumn(enumerator.get(), L"ExpiryYear", expiry_year))
      continue;
    entry.expiration_year = _wtoi(expiry_year.c_str());
    FILETIME date_used;
    if (!enumerator->RetrieveColumn(L"DateAccessed", &date_used))
      continue;
    entry.use_date = base::Time::FromFileTime(date_used);
    payment_cards->push_back(std::move(entry));
  } while (enumerator->Next() && !cancelled());

  return true;
}

bool EdgeImporter::ImportAutofillProfiles() {
  map<GUID, shared_ptr<AutofillAddressComponent>, importer::GuidComparator>
      addressDataMap;

  if (!GetAllAutofillAddress(database_, addressDataMap)) {
    VLOG(1) << "Error querying address entries from spartan DB.";
  }

  std::unique_ptr<EdgeDatabaseTableEnumerator> contactEnumerator =
      GetDatabaseTableEnumerator(database_, L"AutoFormFillStorage");
  if (!contactEnumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!contactEnumerator->Reset()) {
    bool status = (contactEnumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  vector<ImporterAutofillProfileDataEntry> profiles;
  do {
    ImporterAutofillProfileDataEntry entry;
    bool is_deleted = false;
    if (!contactEnumerator->RetrieveColumn(L"IsDeleted", &is_deleted) ||
        is_deleted)
      continue;

    GUID addressId;
    CONTINUE_LOOP_IF_FALSE(
        contactEnumerator->RetrieveColumn(kAddressId, &addressId));

    if (addressId != GUID_NULL) {
      std::map<GUID, shared_ptr<AutofillAddressComponent>,
               importer::GuidComparator>::iterator addressProfile =
          addressDataMap.find(addressId);

      if (addressProfile != addressDataMap.end()) {
        entry.street_address = addressProfile->second->street_address;
        entry.dependent_locality = addressProfile->second->dependent_locality;
        entry.city = addressProfile->second->city;
        entry.state = addressProfile->second->state;
        entry.zipcode = addressProfile->second->zipcode;
        entry.country_code = addressProfile->second->country_code;
      } else {
        continue;
      }
    }

    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(contactEnumerator.get(), kFirstName, entry.first_name));
    CONTINUE_LOOP_IF_FALSE(RetrieveColumn(contactEnumerator.get(), kMiddleName,
                                          entry.middle_name));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(contactEnumerator.get(), kLastName, entry.last_name));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(contactEnumerator.get(), kEmailId, entry.email));
    CONTINUE_LOOP_IF_FALSE(
        RetrieveColumn(contactEnumerator.get(), kPhone, entry.phone));
    CONTINUE_LOOP_IF_FALSE(RetrieveColumn(contactEnumerator.get(), kCompanyName,
                                          entry.company_name));

    FILETIME use_date;
    CONTINUE_LOOP_IF_FALSE(
        contactEnumerator->RetrieveColumn(kDateUsed, &use_date));
    entry.use_date = base::Time::FromFileTime(use_date);

    CONTINUE_LOOP_IF_FALSE(
        contactEnumerator->RetrieveColumn(kUseCount, &entry.use_count));
    profiles.push_back(entry);

  } while (contactEnumerator->Next() && !cancelled());

  bridge_->SetAutofillProfileData(profiles);
  return true;
}

bool EdgeImporter::ParseGenericAutoSuggestStorageDatabase(
    std::vector<ImporterAutofillFormDataEntry>* form_entries) {
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"GenericAutoSuggestStorage");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
        ImportErrorState::kEdgeEnumeratorNotFound);
    return true;
  }

  // Return true when there is no record.
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }

  do {
    ImporterAutofillFormDataEntry entry;
    bool is_deleted = false;
    if (!enumerator->RetrieveColumn(L"IsDeleted", &is_deleted) || is_deleted)
      continue;

    // Pre RS5 generic data used to be stored in registry with key SHAed.
    // Any such entry in Edge database wont be migrated.
    bool is_hashed = false;
    if (!enumerator->RetrieveColumn(L"IsShaGenerated", &is_hashed) || is_hashed)
      continue;

    if (!enumerator->RetrieveColumn(L"Key", &entry.name))
      continue;
    if (!RetrieveColumn(enumerator.get(), L"Value", entry.value) ||
        entry.value.empty())
      continue;
    if (!enumerator->RetrieveColumn(
            L"UseCount", reinterpret_cast<uint32_t*>(&entry.times_used)))
      continue;

    FILETIME date_used;
    if (!enumerator->RetrieveColumn(L"DateAccessed", &date_used))
      continue;
    entry.last_used = base::Time::FromFileTime(date_used);
    FILETIME date_updated;
    if (!enumerator->RetrieveColumn(L"DateUpdated", &date_updated))
      continue;
    entry.first_used = base::Time::FromFileTime(date_updated);
    form_entries->push_back(std::move(entry));
  } while (enumerator->Next() && !cancelled());

  return true;
}

bool EdgeImporter::OpenSpartanDatabase(bool is_history_db) {
  database_.ReleaseDatabase();
  if (source_path_.empty()) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE,
        is_history_db ? importer::HISTORY : importer::FAVORITES,
        ImportErrorState::kEdgeSourcePathEmpty);
    return false;
  }
  base::FilePath database_path = is_history_db
                                     ? FindSpartanHistoryDatabase()
                                     : FindSpartanDatabase(source_path_);

  importer::ImportItem data_type =
      is_history_db ? importer::HISTORY : importer::FAVORITES;
  if (database_path.empty()) {
    VLOG(1) << "Error finding database from source path"
            << database_path.value();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type, ImportErrorState::kOriginalDbNotFound);
    return false;
  }

  // Create a temporary directory to copy the database and log files in
  // source_path_.

  if (!EnsureTempDir(data_type)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type,
        ImportErrorState::kEdgeTempPathNotFound);
    return false;
  }

  base::FilePath database_folder = base::FilePath(
      importer::GetDatabaseFolderFromPath(database_path.value()));
  if (!base::PathExists(database_folder)) {
    VLOG(1) << "OpenSpartanDatabase: Path doesn't exists: " << database_folder;
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type,
        ImportErrorState::kOriginalPathNotFound);
    return false;
  }

  if (!importer::CopyAllFiles(database_folder, importer_temp_dir_)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type, ImportErrorState::kCopyDbFailed);
    return false;
  }

  base::FilePath copied_database_file_path;
  if (is_history_db) {
    copied_database_file_path =
        importer_temp_dir_.Append(kSpartanHistoryDatabaseFile);
  } else {
    copied_database_file_path = importer_temp_dir_.Append(kSpartanDatabaseFile);
    // For spartan.edb recovery, Logfiles need to be copied in a folder as per
    // original location hence creating new folder. The new folder will be
    // created in case of manual import and incase of auto import if the file
    // exists we will use the cached files for better perf.
    if (base::PathExists(importer_temp_dir_.Append(L"LogFiles")) ||
        base::CreateDirectoryW(importer_temp_dir_.Append(L"LogFiles"))) {
      if (!importer::CopyAllFiles(database_folder.Append(L"LogFiles"),
                                  importer_temp_dir_.Append(L"LogFiles"))) {
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, data_type,
            ImportErrorState::kCopyLogFilesFailed);
        return false;
      }
    } else {
      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, data_type,
          ImportErrorState::kCreateTempLogFilePathFailed);
      return false;
    }
  }

  if (!database_.OpenDatabase(copied_database_file_path.value(),
                              is_history_db)) {
    ImporterProfileMetrics::LogSpartanOpenDatabaseError(
        is_history_db ? importer::HISTORY : importer::FAVORITES,
        database_.last_error());
    VLOG(1) << "Error in OpenDatabase from "
            << copied_database_file_path.value().c_str();
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type, ImportErrorState::kOpenDbFailed);
    return false;
  } else {
    VLOG(1) << "Successfully opened database from "
            << copied_database_file_path.value().c_str();
  }
  return true;
}

base::FilePath EdgeImporter::FindSpartanHistoryDatabase() {
  return importer::FindSpartanHistoryDatabase();
}

bool EdgeImporter::CopySpartanRecoveryStoreToTempDir(
    importer::ImportItem data_type) {
  if (source_path_.empty())
    return false;

  if (!EnsureTempDir(data_type)) {
    return false;
  }

  base::FilePath original_recovery_path = FindRecoveryStore(source_path_);

  if (!base::PathExists(original_recovery_path)) {
    VLOG(1) << "Path doesn't exist: " << original_recovery_path;
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type,
        ImportErrorState::kOriginalPathNotFound);
    return false;
  }

  if (!importer::CopyAllFiles(original_recovery_path, importer_temp_dir_)) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type, ImportErrorState::kCopyDbFailed);
    return false;
  }

  return true;
}

bool EdgeImporter::IsSpartanNewTabPageURL(const GURL& url) {
  const char kSpartanNewTabUrl[] = "https://www.msn.com/spartan/";
  const char kSpartanNewTabUrlCN[] = "https://www.msn.cn/spartan/";

  return base::StartsWith(url.spec(), kSpartanNewTabUrl,
                          base::CompareCase::INSENSITIVE_ASCII) ||
         base::StartsWith(url.spec(), kSpartanNewTabUrlCN,
                          base::CompareCase::INSENSITIVE_ASCII);
}

bool EdgeImporter::EnsureTempDir(importer::ImportItem data_type) {
  if (!importer_temp_dir_.empty()) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type, ImportErrorState::kEdgeTempPathReused);
    return true;
  }

  if (source_path_.empty()) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type,
        ImportErrorState::kEdgeTempSourcePathNotFound);
    return false;
  }

  if (!importer::GetTempDirForImporter(importer::ImporterType::TYPE_EDGE,
                                       source_path_.BaseName().value(),
                                       &importer_temp_dir_)) {
    VLOG(1) << "Error creating temp directory";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, data_type,
        ImportErrorState::kTempPathCreateFailed);
    return false;
  }
  bridge_->SetTempFilePath(importer_temp_dir_);
  return true;
}

HRESULT EdgeImporter::ParseEdgeSearchScopes(
    BYTE* unobfuscated_data,
    DWORD decrypted_data_size,
    std::vector<importer::SearchEngineInfo>& search_engines,
    base::string16& default_search_engine_id) {
  HRESULT hr = S_OK;
  DWORD top_level_position = 0;
  UINT32 top_level_type = 0;
  BLOB top_level_value = {};
  wchar_t default_id[MAX_PATH] = {};
  while (SUCCEEDED(hr) && top_level_position < decrypted_data_size) {
    hr = FetchSearchEngineData(unobfuscated_data, decrypted_data_size,
                               &top_level_position, &top_level_type,
                               &top_level_value);
    if (SUCCEEDED(hr)) {
      DWORD search_scope_field_position = 0;
      UINT32 search_scope_field_type = 0;
      BLOB search_scope_field_value = {};
      while (SUCCEEDED(hr) &&
             search_scope_field_position < top_level_value.cbSize) {
        hr = FetchSearchEngineData(
            top_level_value.pBlobData, top_level_value.cbSize,
            &search_scope_field_position, &search_scope_field_type,
            &search_scope_field_value);
        if (SUCCEEDED(hr)) {
          BLOB raw_data = {};
          hr = GetSearchEngineRawData(&search_scope_field_value,
                                      search_scope_field_type, &raw_data,
                                      default_id);
          if (SUCCEEDED(hr) && (raw_data.cbSize != 0)) {
            default_search_engine_id.assign(default_id);
            DWORD search_scope_data_position = 0;
            UINT32 search_scope_prop_id = 0;
            BLOB search_scope_data_value;
            importer::SearchEngineInfo search_engine = {};
            while (SUCCEEDED(hr) &&
                   search_scope_data_position < raw_data.cbSize) {
              hr = FetchSearchEngineData(raw_data.pBlobData, raw_data.cbSize,
                                         &search_scope_data_position,
                                         &search_scope_prop_id,
                                         &search_scope_data_value);
              if (SUCCEEDED(hr)) {
                int search_engine_data_type =
                    GetSearchEngineDataType(search_scope_prop_id);
                if (search_engine_data_type == 0) {
                  switch (search_scope_prop_id) {
                    case 1 /*id*/: {
                      search_engine.id = reinterpret_cast<PWSTR>(
                          search_scope_data_value.pBlobData);
                      if (wcscmp(search_engine.id.c_str(),
                                 default_search_engine_id.c_str()) == 0)
                        search_engine.is_default = true;
                    } break;
                    case 2 /*display_name*/:
                      search_engine.display_name = reinterpret_cast<PWSTR>(
                          search_scope_data_value.pBlobData);
                      search_engine.keyword = reinterpret_cast<PWSTR>(
                          search_scope_data_value.pBlobData);
                      break;
                    case 3 /*url*/: {
                      search_engine.url = reinterpret_cast<PWSTR>(
                          search_scope_data_value.pBlobData);
                      // If we have a valid search url. Change the keyword to
                      // host. Spartan tags search providers with the
                      // same keyword for different regions.
                      // eg., in.search.yahoo.com and fr.search.yahoo.com comes
                      // with the same keyword Yahoo.
                      GURL search_engine_url(search_engine.url);
                      if (search_engine_url.is_valid())
                        search_engine.keyword =
                            TemplateURL::GenerateKeyword(search_engine_url);
                    } break;
                    default:
                      break;
                  }
                }
              }
            }
            search_engines.push_back(search_engine);
          }
        }
      }
    }
  }
  return (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) ? S_OK : hr;
}

bool EdgeImporter::ParseSearchEngines(
    BYTE* encrypted_data,
    DWORD data_size,
    std::vector<importer::SearchEngineInfo>& edge_search_engines,
    base::string16& default_search_engine_id) {
  HRESULT hr = S_OK;
  edge_search_engines.clear();
  edge_search_engines.reserve(kMinDataSize /*expected_search_engine_size*/);
  if ((encrypted_data != nullptr) && (data_size > 4 /*min_data_size*/)) {
    DWORD unobfuscated_data_size = data_size * sizeof(DWORD);
    BYTE* unobfuscated_data = (BYTE*)malloc(unobfuscated_data_size);
    memset(unobfuscated_data, 0, unobfuscated_data_size);
    hr = UnobfuscateData(encrypted_data + 4 /*data_offset*/,
                         data_size - 4 /*data_offset*/, unobfuscated_data,
                         &unobfuscated_data_size);
    if (SUCCEEDED(hr)) {
      hr = ParseEdgeSearchScopes(unobfuscated_data, unobfuscated_data_size,
                                 edge_search_engines, default_search_engine_id);
    }
    if (unobfuscated_data)
      free(unobfuscated_data);
  }
  return SUCCEEDED(hr);
}

bool EdgeImporter::ImportSearchEngines() {
  base::win::RegKey key(HKEY_CURRENT_USER, kEdgeProtectedDataPath, KEY_READ);
  DWORD data_type = REG_BINARY;
  DWORD data_size = 0;
  LONG result = key.ReadValue(L"ProtectedSearchScopes", nullptr /*data*/,
                              &data_size, &data_type);
  if ((result != ERROR_SUCCESS) || !data_size || data_type != REG_BINARY) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::SEARCH_ENGINES,
        ImportErrorState::kRegistryReadError);
    return false;
  }
  BYTE* encrypted_data = (BYTE*)malloc(data_size);
  result = key.ReadValue(L"ProtectedSearchScopes", encrypted_data, &data_size,
                         &data_type);
  bool status = true;
  if (result != ERROR_SUCCESS) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::SEARCH_ENGINES,
        ImportErrorState::kRegistryReadError);
    status = false;
  } else {
    std::vector<importer::SearchEngineInfo> edge_search_engines;
    base::string16 default_search_engine_id;
    status = ParseSearchEngines(encrypted_data, data_size, edge_search_engines,
                                default_search_engine_id);
    if (status) {
      SetDefaultSearchEnginePref(edge_search_engines);
      bridge_->SetKeywords(edge_search_engines,
                           true /*unique_on_host_and_path*/);
    }
    edge_search_engines.clear();
  }
  if (encrypted_data)
    free(encrypted_data);
  return status;
}

bool EdgeImporter::ImportHomePage() {
  base::win::RegKey key(HKEY_CURRENT_USER, kEdgeHomeButtonPagePath, KEY_READ);
  base::string16 homepage_url;
  if (key.ReadValue(L"HomeButtonPage", &homepage_url) != ERROR_SUCCESS) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HOME_PAGE,
        ImportErrorState::kRegistryReadError);
    // If the registry entry is not found, it implies that user did not set it
    // on Edge.
    return true;
  }
  if (homepage_url.empty()) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HOME_PAGE,
        ImportErrorState::kRegistryValueEmpty);
    return true;
  }
  GURL homepage = GURL(homepage_url);
  if (!homepage.is_valid()) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::HOME_PAGE,
        ImportErrorState::kRegistryValueInvalid);
    return false;
  }
  if (!cancelled()) {
    bridge_->AddHomePage(homepage);
  }
  return true;
}

bool EdgeImporter::OpenSpartanRecoveryStore(importer::ImportItem item) {
  if (recovery_store_)
    return true;  // Recovery store already opened.

  if (!CopySpartanRecoveryStoreToTempDir(item)) {
    VLOG(1) << "Error copying recovery store";
    return false;
  }

  if (!OpenSpartanRecoveryStore(item, importer_temp_dir_)) {
    VLOG(1) << "Error opening recovery store";
    return false;
  }

  return true;
}

bool EdgeImporter::OpenSpartanRecoveryStore(
    importer::ImportItem item,
    const base::FilePath& recovery_path) {
  HRESULT hr = EdgeRecovery::GetRecoveryStore(recovery_path, &recovery_store_);

  if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
    VLOG(1) << "Error: kEdgeRecoveryStoreFileNotFound";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item,
        ImportErrorState::kEdgeRecoveryStoreFileNotFound);
  } else if (hr == HRESULT_FROM_WIN32(ERROR_BADKEY)) {
    VLOG(1) << "Error: kRegistryValueInvalid";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item, ImportErrorState::kRegistryValueInvalid);
  } else if (hr == E_EDGE_RECOVERY_NO_DATA) {
    VLOG(1) << "Error: kEdgeRecoveryStoreNoData";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item, ImportErrorState::kEdgeRecoveryStoreNoData);
  } else if (hr == E_EDGE_RECOVERY_INVALID_DATA) {
    VLOG(1) << "Error: kEdgeRecoveryStoreInvalidData";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item,
        ImportErrorState::kEdgeRecoveryStoreInvalidData);
  } else if (hr == E_EDGE_RECOVERY_INVALID_DATA_VERSION) {
    VLOG(1) << "Error: kEdgeRecoveryStoreInvalidDataVersion";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item,
        ImportErrorState::kEdgeRecoveryStoreInvalidDataVersion);
  } else if (FAILED(hr)) {
    VLOG(1) << "Error: kParseDbFailed";
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, item, ImportErrorState::kParseDbFailed);
  }

  return (SUCCEEDED(hr));
}

bool EdgeImporter::ReadDWORDFromRegistry(const base::win::RegKey& key,
                                         base::string16 name,
                                         DWORD& value) {
  if (key.ReadValueDW(name.c_str(), &value) != ERROR_SUCCESS) {
    VLOG(1) << name.c_str()
            << " setting not found. some settings are not set in registry"
               "when they are not changed by user";
    return false;
  }
  return true;
}

bool EdgeImporter::ReadStringFromRegistry(const base::win::RegKey& key,
                                          base::string16 name,
                                          base::string16& value) {
  if (key.ReadValue(name.c_str(), &value) != ERROR_SUCCESS) {
    VLOG(1) << name.c_str()
            << " setting not found. some settings are not set in registry"
               "when they are not changed by user";
    value.clear();
    return false;
  }
  return true;
}

bool EdgeImporter::ReadValuesFromRegistry(const base::string16& path,
                                          std::map<std::string, bool>& url_map,
                                          uint16_t default_value) {
  url_map.clear();
  base::string16 settings_path(kEdgeSettingsRegPath);
  base::win::RegKey key(HKEY_CURRENT_USER, settings_path.append(path).c_str(),
                        KEY_READ);
  base::win::RegistryValueIterator reg_iterator(HKEY_CURRENT_USER,
                                                settings_path.c_str());
  size_t value_count = 0;
  bool value_found = false;
  while (reg_iterator.Valid() && !cancelled()) {
    value_count++;
    if (reg_iterator.Name() == nullptr) {
      VLOG(1) << "setting not found";
    } else {
      DWORD value = default_value;
      value = (key.ReadValueDW(reg_iterator.Name(), &value) == ERROR_SUCCESS)
                  ? value
                  : default_value;
      base::string16 url = reg_iterator.Name();
      if (url.empty()) {
        VLOG(1) << "setting name in registry is empty";
      } else {
        // Edge saves url with * prefix to set permissions on every url
        // beginning with a sequence. Remove the prefix(*.) to get the base url.
        size_t wild_card_offset = url.rfind('*');
        if ((std::string::npos != wild_card_offset) &&
            (wild_card_offset == 0)) {
          url = url.substr(wild_card_offset + 2 /*offset of two chars*/,
                           url.length());
        }
        url_map.insert(std::pair<std::string, bool>(base::WideToUTF8(url),
                                                    (value == default_value)));
        value_found = true;
      }
    }
    ++reg_iterator;
  }
  if (value_count >= 1000) {
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::SETTINGS,
        ImportErrorState::kRegValuesMaxLimit);
  }
  return value_found;
}

bool EdgeImporter::ImportSettings() {
  ImporterSettingsType settings_group = {};
  base::string16 settings_path(kEdgeSettingsRegPath);
  base::win::RegKey main_key(HKEY_CURRENT_USER,
                             settings_path.append(L"\\Main").c_str(), KEY_READ);
  DWORD settings_int_type_value = 0;
  if (ReadDWORDFromRegistry(main_key, L"HomeButtonEnabled",
                            settings_int_type_value))
    settings_group.show_homebutton = settings_int_type_value;

  base::string16 settings_string_type_value;
  if (ReadStringFromRegistry(main_key, L"Use FormSuggest",
                             settings_string_type_value))
    settings_group.save_form_data_enabled =
        settings_string_type_value.empty()
            ? true
            : (wcscmp(settings_string_type_value.c_str(), L"yes") == 0);

  if (ReadStringFromRegistry(main_key, L"FormSuggest Passwords",
                             settings_string_type_value))
    settings_group.save_passwords_enabled =
        settings_string_type_value.empty()
            ? true
            : (wcscmp(settings_string_type_value.c_str(), L"yes") == 0);

  if (ReadStringFromRegistry(main_key, L"UsePaymentFormFill",
                             settings_string_type_value))
    settings_group.save_payment_info_enabled =
        settings_string_type_value.empty()
            ? true
            : (wcscmp(settings_string_type_value.c_str(), L"yes") == 0);

  if (ReadDWORDFromRegistry(main_key, L"DoNotTrack", settings_int_type_value))
    settings_group.enable_do_not_track = settings_int_type_value;

  if (ReadDWORDFromRegistry(main_key, L"ShowSearchSuggestionsGlobal",
                            settings_int_type_value))
    settings_group.search_suggestions_enabled = settings_int_type_value;

  if (ReadDWORDFromRegistry(main_key, L"Cookies", settings_int_type_value)) {
    settings_group.cookies_pref =
        importer::ConvertImportedCookiePref<importer::ImporterType::TYPE_EDGE>(
            settings_int_type_value);
    // Since cookie settings are mutually exclusive in Spartan, block
    // third_party_cookies only if set explicitly
    settings_group.block_third_party_cookies =
        settings_int_type_value == 1 ? 1 : -1;
  }

  if (ReadStringFromRegistry(main_key, L"Default Download Directory",
                             settings_string_type_value))
    settings_group.download_dir_path =
        base::WideToUTF8(settings_string_type_value.data());

  if (ReadStringFromRegistry(main_key, L"HomeButtonPage",
                             settings_string_type_value))
    MapHomepageURL(settings_string_type_value, settings_group.homepage_url);
  settings_group.is_homepage_newtabpage =
      (settings_group.homepage_url == chrome::kChromeUINewTabHost);

  base::string16 download_path(kEdgeSettingsRegPath);
  base::win::RegKey download_key(
      HKEY_CURRENT_USER, download_path.append(L"\\Download").c_str(), KEY_READ);
  if (ReadDWORDFromRegistry(download_key, L"EnableSavePrompt",
                            settings_int_type_value))
    settings_group.prompt_download = settings_int_type_value;

  if (ReadDWORDFromRegistry(main_key, L"Theme", settings_int_type_value))
    settings_group.theme_pref = settings_int_type_value;

  base::string16 fav_path(kEdgeSettingsRegPath);
  base::win::RegKey favbar_key(
      HKEY_CURRENT_USER, fav_path.append(L"\\LinksBar").c_str(), KEY_READ);
  if (ReadDWORDFromRegistry(favbar_key, L"Enabled", settings_int_type_value))
    settings_group.show_favbar = settings_int_type_value;

  base::string16 service_ui_path(kEdgeSettingsRegPath);
  base::win::RegKey ntp_key(HKEY_CURRENT_USER,
                            service_ui_path.append(L"\\ServiceUI").c_str(),
                            KEY_READ);
  if (ReadDWORDFromRegistry(ntp_key, L"NewTabPageDisplayOption",
                            settings_int_type_value))
    settings_group.show_top_sites = settings_int_type_value;

  base::string16 filter_path(kEdgeSettingsRegPath);
  base::win::RegKey safe_browsing_key(
      HKEY_CURRENT_USER, filter_path.append(L"\\PhishingFilter").c_str(),
      KEY_READ);
  if (ReadDWORDFromRegistry(safe_browsing_key, L"EnabledV9",
                            settings_int_type_value))
    settings_group.safe_browsing = (settings_int_type_value != 0);

  base::string16 popup_path(kEdgeSettingsRegPath);
  base::win::RegKey popups_key(
      HKEY_CURRENT_USER, popup_path.append(L"\\New Windows").c_str(), KEY_READ);
  if (ReadStringFromRegistry(popups_key, L"PopupMgr",
                             settings_string_type_value))
    settings_group.show_popups =
        settings_string_type_value.empty()
            ? false
            : (settings_string_type_value.compare(L"no") == 0);

  base::string16 flash_player_path(kEdgeSettingsRegPath);
  base::win::RegKey flash_player_key(
      HKEY_CURRENT_USER, flash_player_path.append(L"\\Addons").c_str(),
      KEY_READ);
  if (ReadDWORDFromRegistry(flash_player_key, L"FlashPlayerEnabled",
                            settings_int_type_value))
    settings_group.flash_player_enabled = settings_int_type_value;

  base::string16 zoom_path(kEdgeSettingsRegPath);
  base::win::RegKey zoom_factor_key(
      HKEY_CURRENT_USER, zoom_path.append(L"\\Zoom").c_str(), KEY_READ);
  if (ReadDWORDFromRegistry(zoom_factor_key, L"ZoomFactor",
                            settings_int_type_value)) {
    // Edge saves zoom factor as a percentage multiplied by 1000.
    double zoom_factor = (settings_int_type_value / 100000.0);
    settings_group.zoom_factor = zoom_factor;
  }

  ImportWebsitePermissions(settings_group);

  bridge_->SetSettingsData(settings_group);
  return true;
}

HRESULT EdgeImporter::ImportObfuscatedHomePages(
    BYTE* obfuscated_data,
    DWORD data_size,
    std::vector<std::string>& edge_dhps_to_import) {
  edge_dhps_to_import.clear();
  edge_dhps_to_import.reserve(
      kMinDataSize /*expected_default_home_pages_size*/);
  std::vector<std::string> edge_dhps;
  HRESULT hr =
      UnobfuscateDefaultHomePages(obfuscated_data, data_size, edge_dhps);
  if (SUCCEEDED(hr)) {
    for (auto& dhp : edge_dhps) {
      if (!dhp.empty()) {
        if (dhp.compare("about:start") == 0) {
          ImporterProfileMetrics::LogSpartanStartPageConfiguration(
              StartPageType::kMSN);
        } else if (dhp.compare("about:tabs") == 0) {
          ImporterProfileMetrics::LogSpartanStartPageConfiguration(
              StartPageType::kNTP);
        } else {
          edge_dhps_to_import.push_back(dhp);
          ImporterProfileMetrics::LogSpartanStartPageConfiguration(
              StartPageType::kOther);
        }
      }
    }
  }
  return hr;
}

void EdgeImporter::ImportDefaultHomePages(
    ImporterSettingsType& settings_group) {
  base::string16 settings_path(kEdgeSettingsRegPath);
  base::win::RegKey session_restore_key(
      HKEY_CURRENT_USER, settings_path.append(L"\\ContinuousBrowsing").c_str(),
      KEY_READ);
  DWORD settings_int_type_value = 0;
  // If the setting is not found in registry, Default Home Page in Spartan is
  // NOT set to Previous Tabs.
  ReadDWORDFromRegistry(session_restore_key, L"Enabled",
                        settings_int_type_value);
  uint16_t session_pref_type;
  session_pref_type = settings_int_type_value
                          ? importer::SessionStartupPrefType::LAST
                          : importer::SessionStartupPrefType::DEFAULT;
  if (session_pref_type != importer::SessionStartupPrefType::LAST) {
    base::win::RegKey protected_home_pages_key(
        HKEY_CURRENT_USER, kEdgeProtectedDataPath, KEY_READ);
    DWORD data_type = REG_BINARY;
    DWORD data_size = 0;
    LONG result = protected_home_pages_key.ReadValue(
        L"ProtectedHomePages", nullptr /*data*/, &data_size, &data_type);
    if ((result != ERROR_SUCCESS) || !data_size || data_type != REG_BINARY) {
      VLOG(1) << "StartTabs data not found";
      return;
    }
    BYTE* encrypted_data = static_cast<BYTE*>(malloc(data_size));
    memset(encrypted_data, 0, data_size);
    result = protected_home_pages_key.ReadValue(
        L"ProtectedHomePages", encrypted_data, &data_size, &data_type);
    if (result != ERROR_SUCCESS) {
      VLOG(1) << "StartTabs Registry Read error.";
    } else {
      std::vector<std::string> edge_dhps;
      if (SUCCEEDED(ImportObfuscatedHomePages(encrypted_data, data_size,
                                              edge_dhps)) &&
          !edge_dhps.empty()) {
        session_pref_type = importer::SessionStartupPrefType::URLS;
      }
      settings_group.startup_prefs_urls = edge_dhps;
    }
    if (encrypted_data) {
      free(encrypted_data);
      encrypted_data = nullptr;
    }
  } else {
    ImporterProfileMetrics::LogSpartanStartPageConfiguration(
        StartPageType::kLast);
  }
  settings_group.startup_prefs_type = session_pref_type;
}

bool EdgeImporter::ImportStartupPage() {
  ImporterSettingsType settings_group = {};
  settings_group.is_importer_startup_page = true;
  ImportDefaultHomePages(settings_group);
  bridge_->SetSettingsData(settings_group);
  return true;
}

bool EdgeImporter::ImportGeoLocationSettings(
    ImporterSettingsType& settings_group) {
  settings_group.geoloc_permissions_map.clear();
  std::unique_ptr<EdgeDatabaseTableEnumerator> enumerator =
      database_.OpenTableEnumerator(L"WebsitePermissions");
  if (!enumerator) {
    VLOG(1) << "Error opening database table " << database_.GetErrorMessage();
    return false;
  }
  if (!enumerator->Reset()) {
    bool status = (enumerator->last_error() == JET_errNoCurrentRecord);
    ImporterProfileMetrics::LogImporterErrorMetric(
        importer::TYPE_EDGE, importer::SETTINGS,
        status ? ImportErrorState::kEdgeEnumeratorNoRecords
               : ImportErrorState::kEdgeEnumeratorResetError);
    return status;
  }
  do {
    base::string16 url;
    if (!enumerator->RetrieveColumn(L"URL", &url)) {
      VLOG(1) << "URL column not found.";
      continue;
    }
    int geoloc_permission_value;
    if (!enumerator->RetrieveColumn(L"LocationPermissionStatus",
                                    &geoloc_permission_value)) {
      VLOG(1) << "LocationPermissionStatus column not found.";
      continue;
    }
    settings_group.geoloc_permissions_map.insert(
        std::pair<std::string, uint32_t>(base::WideToUTF8(url),
                                         geoloc_permission_value));
  } while (enumerator->Next() && !cancelled());
  return true;
}

void EdgeImporter::ImportWebsitePermissions(
    ImporterSettingsType& settings_group) {
  // Popup permissions
  ReadValuesFromRegistry(L"\\New Windows\\Allow",
                         settings_group.popups_urls_map, 1 /*default_value*/);

  // Notifications permissions
  ReadValuesFromRegistry(L"\\Notifications\\Domains",
                         settings_group.notifications_urls_map,
                         1 /*default_value*/);

  // Media permissions
  settings_group.media_permissions_map.clear();
  base::string16 settings_path(kEdgeSettingsRegPath);
  base::win::RegKey key(
      HKEY_CURRENT_USER,
      settings_path.append(L"\\MediaCapture\\AllowDomains").c_str(), KEY_READ);
  base::win::RegistryValueIterator reg_iterator(HKEY_CURRENT_USER,
                                                settings_path.c_str());
  while (reg_iterator.Valid() && !cancelled()) {
    if (!reg_iterator.Name()) {
      VLOG(1) << "media permissions setting not found";
    } else {
      DWORD value = 0;
      if (key.ReadValueDW(reg_iterator.Name(), &value) == ERROR_SUCCESS) {
        settings_group.media_permissions_map.insert(
            std::pair<std::string, uint32_t>(
                base::WideToUTF8(reg_iterator.Name()), value));
      } else {
        VLOG(1) << reg_iterator.Name() << " setting not found";
      }
    }
    ++reg_iterator;
  }

  // Geolocation permissions
  ImportGeoLocationSettings(settings_group);
}

void EdgeImporter::SetDefaultSearchEnginePref(
    const std::vector<importer::SearchEngineInfo>& edge_search_engines) {
  for (auto search_engine : edge_search_engines) {
    if (search_engine.is_default) {
      bridge_->SetDefaultSearchEnginePref(search_engine.url);
      break;
    }
  }
}

void EdgeImporter::MapHomepageURL(const base::string16& spartan_home_page,
                                  std::string& anaheim_home_page) {
  anaheim_home_page = (spartan_home_page.empty() ||
                       (spartan_home_page.compare(L"about:start") == 0) ||
                       (spartan_home_page.compare(L"about:tabs") == 0))
                          ? chrome::kChromeUINewTabURL
                          : base::WideToUTF8(spartan_home_page.data());
}

void EdgeImporter::StartSearchEngineMigration(uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeSettingsImport) &&
      (items & importer::SEARCH_ENGINES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::SEARCH_ENGINES);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = ImportSearchEngines();
    bridge_->NotifyItemEnded(importer::SEARCH_ENGINES, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartStartupPageMigration(uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeSettingsImportV2) &&
      (items & importer::STARTUP_PAGE) && !cancelled()) {
    ImporterSettingsType settings_group = {};
    settings_group.is_importer_startup_page = true;
    bridge_->NotifyItemStarted(importer::STARTUP_PAGE);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = ImportStartupPage();
    bridge_->NotifyItemEnded(importer::STARTUP_PAGE, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartFavoritesMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  if ((items & importer::FAVORITES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::FAVORITES);
    base::TimeTicks start_time = base::TimeTicks::Now();
    if (!is_fatal_error) {
      status = ImportFavorites();
      if (!status)
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::FAVORITES,
            ImportErrorState::kParseDbFailed);
    }
    bridge_->NotifyItemEnded(importer::FAVORITES, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartPaymentsMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport)) {
    if ((items & importer::PAYMENTS) && !cancelled()) {
      bridge_->NotifyItemStarted(importer::PAYMENTS);
      base::TimeTicks start_time = base::TimeTicks::Now();
      if (!is_fatal_error) {
        status = ImportCreditCards();
        if (!status)
          ImporterProfileMetrics::LogImporterErrorMetric(
              importer::TYPE_EDGE, importer::PAYMENTS,
              ImportErrorState::kParseDbFailed);
      }
      bridge_->NotifyItemEnded(importer::PAYMENTS, status,
                               base::TimeTicks::Now() - start_time);
    }
  }
}

void EdgeImporter::StartAutofillMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  if ((items & importer::AUTOFILL_FORM_DATA) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::AUTOFILL_FORM_DATA);
    base::TimeTicks start_time = base::TimeTicks::Now();
    if (!is_fatal_error) {
      status = ImportAutofillProfiles() & ImportAutofillFormData();
      if (!status)
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::AUTOFILL_FORM_DATA,
            ImportErrorState::kParseDbFailed);
    }
    bridge_->NotifyItemEnded(importer::AUTOFILL_FORM_DATA, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartReadingListMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  // Putting export of reading list under experimentation key
  // There is a scope of refatoring ImportReadingList code. Do refactor when
  // experiment key is removed.
  if (base::FeatureList::IsEnabled(features::edge::kEdgeImportReadingList) &&
      (items & importer::READING_LIST) && !cancelled()) {
    base::TimeTicks start_time = base::TimeTicks::Now();
    bridge_->NotifyItemStarted(importer::READING_LIST);
    if (!is_fatal_error) {
      status = ImportReadingList();
      if (!status)
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::READING_LIST,
            ImportErrorState::kParseDbFailed);
    }
    bridge_->NotifyItemEnded(importer::READING_LIST, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartWebNotesMigration() {
  // After collecting webnote paths from FAV and RL data, start migration.
  if (!webnote_spartan_paths_.empty())
    bridge_->MigrateWebnotes(webnote_spartan_paths_);
}

void EdgeImporter::StartTabSweepMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  if (base::FeatureList::IsEnabled(features::edge::kEdgeImportTabSweep) &&
      (items & importer::TAB_SWEEP) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::TAB_SWEEP);
    base::TimeTicks start_time = base::TimeTicks::Now();
    if (!is_fatal_error) {
      if (!OpenSpartanRecoveryStore(importer::TAB_SWEEP))
        VLOG(1) << "Error getting recovery store: " << importer_temp_dir_;
      // Not fatal if we couldn't open the recovery store. Will just lose
      // favicons. Proceed with tab sweep import regardless.
      status = ImportTabSweep();
    }
    bridge_->NotifyItemEnded(importer::TAB_SWEEP, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartTopSitesMigration(uint16_t items, bool is_fatal_error) {
  bool status = false;
  if (base::FeatureList::IsEnabled(
          features::edge::kEdgeSpartanTopSitesMigration) &&
      (items & importer::TOP_SITES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::TOP_SITES);
    base::TimeTicks start_time = base::TimeTicks::Now();
    if (!is_fatal_error) {
      status = ImportTopSites();
      if (!status)
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::TOP_SITES,
            ImportErrorState::kParseDbFailed);
    }
    bridge_->NotifyItemEnded(importer::TOP_SITES, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartSettingsMigration(uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeSettingsImportV2) &&
      (items & importer::SETTINGS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::SETTINGS);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = ImportSettings();
    bridge_->NotifyItemEnded(importer::SETTINGS, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartCookiesMigration(uint16_t items) {
  // Import Cookies. Cookies should always be imported before Open Tabs
  // import.
  if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport) &&
      (items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartOpenTabsMigration(uint16_t items) {
  // Import tab data.
  if (base::FeatureList::IsEnabled(features::edge::kEdgeImportOpenTabs) &&
      (items & importer::OPEN_TABS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::OPEN_TABS);
    base::TimeTicks start_time = base::TimeTicks::Now();

    bool status = false;
    if (OpenSpartanRecoveryStore(importer::OPEN_TABS)) {
      status = ImportTabs(importer::OPEN_TABS);
    }

    bridge_->NotifyItemEnded(importer::OPEN_TABS, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartPasswordsMigration(uint16_t items) {
  // Import passwords from CredVault.
  if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport) &&
      (items & importer::PASSWORDS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::PASSWORDS);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = importer::ImportPasswordsFromCredentials(this, bridge_,
                                                           importer::TYPE_EDGE);
    if (!status)
      ImporterProfileMetrics::LogImporterErrorMetric(
          importer::TYPE_EDGE, importer::PASSWORDS,
          ImportErrorState::kParseDbFailed);
    bridge_->NotifyItemEnded(importer::PASSWORDS, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartHomePageMigration(uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeSettingsImport) &&
      (items & importer::HOME_PAGE) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::HOME_PAGE);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = ImportHomePage();
    bridge_->NotifyItemEnded(importer::HOME_PAGE, status,
                             base::TimeTicks::Now() - start_time);
  }
}

void EdgeImporter::StartExtensionsMigration(
    const importer::SourceProfile& source_profile,
    uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeExtensionImport) &&
      (items & importer::EXTENSIONS) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::EXTENSIONS);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = false;
    if (EnsureTempDir(importer::EXTENSIONS)) {
      importer::extensions_importer::ExtensionImporter extension_importer;
      std::unique_ptr<importer::extensions_importer::ExtensionsList>
          extension_list(extension_importer.AllocateAndGetExtensionsList(
              source_profile, importer_temp_dir_));
      if (extension_list && extension_list->DoExtensionsAvailable()) {
        importer::extensions_importer::ExtensionImportGroup
            extension_import_group;
        extension_import_group.importer_type = source_profile.importer_type;
        extension_import_group.extensions = extension_list->GetExtensions();
        bridge_->SetExtensions(extension_import_group);
      }
      status = true;
    }
    base::TimeDelta run_time = base::TimeTicks::Now() - start_time;
    bridge_->NotifyItemEnded(importer::EXTENSIONS, status, run_time);
  }
}

void EdgeImporter::StartHistoryMigration(uint16_t items) {
  if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport) &&
      (items & importer::HISTORY)) {
    bridge_->NotifyItemStarted(importer::HISTORY);
    base::TimeTicks start_time = base::TimeTicks::Now();
    bool status = false;
    if (!OpenSpartanDatabase(true /*is_history_db*/)) {
      VLOG(1) << "StartImport - Error opening database "
              << source_path_.value().c_str() << " Error "
              << database_.GetErrorMessage();
    } else if (!cancelled()) {
      VLOG(1) << "StartImport - WebCacheV01.dat was opened successfully.";
      ImporterProfileMetrics::LogEdgeHistoryImportPerfMetric(
          true /*is_open_db_perf*/, base::TimeTicks::Now() - start_time);
      status = ImportHistory();
      if (!status)
        ImporterProfileMetrics::LogImporterErrorMetric(
            importer::TYPE_EDGE, importer::HISTORY,
            ImportErrorState::kParseDbFailed);
    }
    base::TimeDelta run_time = base::TimeTicks::Now() - start_time;
    ImporterProfileMetrics::LogEdgeHistoryImportPerfMetric(
        false /*is_open_db_perf*/, run_time);
    // NotifyItemEnded would show the Done dialog in import dialog.
    bridge_->NotifyItemEnded(importer::HISTORY, status, run_time);
    // Releasing the database before notify ended. So that the
    // temp files are not locked by this process when it is getting deleted.
    database_.ReleaseDatabase();
  }
}
