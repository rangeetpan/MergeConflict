// Copyright 2015 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/importer_progress_observer.h"
#include "chrome/browser/importer/importer_unittest_utils.h"
#include "chrome/browser/importer/profile_writer.h"
#include "chrome/browser/metrics/subprocess_metrics_provider.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/edge_cookie_import_feature.h"
#include "chrome/common/edge_onramp_features.h"
#include "chrome/common/importer/edge_importer_profile_metrics.h"
#include "chrome/common/importer/edge_importer_utils_win.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_bridge.h"
#include "chrome/common/importer/importer_data_types.h"
#include "chrome/common/importer/importer_test_registry_overrider_win.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/favicon_base/favicon_usage_data.h"
<<<<<<< HEAD
#include "content/public/browser/storage_partition.h"
#include "testing/gmock/include/gmock/gmock.h"
=======
#include "content/public/test/browser_test.h"
>>>>>>> eaac5b5035fe189b6706e1637122e37134206059
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/zlib/google/compression_utils.h"

// Edge includes
#include "chrome/common/importer/test_support/edge_importer_test_util.h"

namespace {

struct FaviconGroup {
  const base::char16* favicon_url;
  const base::char16* site_url;
};

class TestObserver : public ProfileWriter,
                     public importer::ImporterProgressObserver {
 public:
  explicit TestObserver(
      base::OnceClosure quit_closure,
      const std::vector<BookmarkInfo>& expected_bookmark_entries,
      const std::vector<FaviconGroup>& expected_favicon_groups,
      const std::vector<net::CanonicalCookie>& expected_cookies,
      bool should_show_fav_on_ntp = true)
      : ProfileWriter(nullptr),
        bookmark_count_(0),
        expected_bookmark_entries_(expected_bookmark_entries),
        expected_favicon_groups_(expected_favicon_groups),
        favicon_count_(0),
        expected_cookies_(expected_cookies),
        cookie_count_(0),
        quit_closure_(std::move(quit_closure)),
        should_show_fav_on_ntp_(should_show_fav_on_ntp) {}

  // importer::ImporterProgressObserver:
  void ImportStarted() override {}
  MOCK_METHOD1(ImportItemStarted, void(importer::ImportItem item));
  MOCK_METHOD1(ImportItemEnded, void(importer::ImportItem item));
  void ImportEnded() override {
    EXPECT_EQ(expected_bookmark_entries_.size(), bookmark_count_);
    EXPECT_EQ(expected_favicon_groups_.size(), favicon_count_);
    if (expected_cookies_.size() > 0 && writer_ != nullptr) {
      TestCookiesFromCookieManager();
    } else {
      std::move(quit_closure_).Run();
    }
  }

  void ImportItemErrorStatus(
      std::unique_ptr<base::ListValue> failed_import_items) override {}

  // ProfileWriter:
  bool BookmarkModelIsLoaded() const override {
    // Profile is ready for writing.
    return true;
  }

  bool TemplateURLServiceIsLoaded() const override { return true; }

  void AddBookmarks(const std::vector<ImportedBookmarkEntry>& bookmarks,
                    const base::string16& top_level_folder_name,
                    bool should_show_fav_bar_on_ntp) override {
    EXPECT_EQ(should_show_fav_on_ntp_, should_show_fav_bar_on_ntp);
    ASSERT_EQ(expected_bookmark_entries_.size(), bookmarks.size());
    for (size_t i = 0; i < bookmarks.size(); ++i) {
      // Test contents of bookmarks retrieved from ChromeImporter
      // is same as the bookmarks expected.
      ASSERT_EQ(base::WideToUTF16(expected_bookmark_entries_[i].title),
                bookmarks[i].title);
      ASSERT_EQ(expected_bookmark_entries_[i].in_toolbar,
                bookmarks[i].in_toolbar)
          << bookmarks[i].title;
      ASSERT_EQ(expected_bookmark_entries_[i].url, bookmarks[i].url.spec())
          << bookmarks[i].title;
      if (base::FeatureList::IsEnabled(features::edge::kEdgeMergeBookmarks) &&
          bookmarks[i].in_toolbar) {
        // There will be one path entry less as the top level folder is not
        // considered when merging bookmarks as the top level folder name is
        // Favorites bar/ Links when |in_toolbar| is true.
        ASSERT_EQ(expected_bookmark_entries_[i].path_size > 0
                      ? expected_bookmark_entries_[i].path_size - 1
                      : 0,
                  bookmarks[i].path.size())
            << bookmarks[i].title;
        for (size_t path_count = 1; path_count < bookmarks[i].path.size();
             ++path_count) {
          ASSERT_EQ(base::ASCIIToUTF16(
                        expected_bookmark_entries_[i].path[path_count]),
                    bookmarks[i].path[path_count])
              << bookmarks[i].title;
        }
      } else {
        ASSERT_EQ(expected_bookmark_entries_[i].path_size,
                  bookmarks[i].path.size())
            << bookmarks[i].title;
        for (size_t path_count = 0;
             path_count < expected_bookmark_entries_[i].path_size;
             ++path_count) {
          ASSERT_EQ(base::ASCIIToUTF16(
                        expected_bookmark_entries_[i].path[path_count]),
                    bookmarks[i].path[path_count])
              << bookmarks[i].title;
        }
      }
      ++bookmark_count_;
    }
  }

  void AddFavicons(const favicon_base::FaviconUsageDataList& usage) override {
    // Importer should group the favicon information for each favicon URL.
    ASSERT_EQ(expected_favicon_groups_.size(), usage.size());
    for (size_t i = 0; i < expected_favicon_groups_.size(); ++i) {
      GURL favicon_url(expected_favicon_groups_[i].favicon_url);
      std::set<GURL> urls;
      urls.insert(GURL(expected_favicon_groups_[i].site_url));

      bool expected_favicon_url_found = false;
      for (size_t j = 0; j < usage.size(); ++j) {
        if (usage[j].favicon_url == favicon_url) {
          EXPECT_EQ(urls, usage[j].urls);
          expected_favicon_url_found = true;
          break;
        }
      }
      EXPECT_TRUE(expected_favicon_url_found);
    }
    favicon_count_ += usage.size();
  }

  void AddCookies(const std::vector<net::CanonicalCookie>& cookies) override {
    if (base::FeatureList::IsEnabled(
            features::edge::kEnableVirtualCookieImportForTesting)) {
      ASSERT_EQ(expected_cookies_.size(), cookies.size());
      for (size_t i = 0; i < cookies.size(); ++i) {
        EXPECT_TRUE(CookieEqualityForTesting(cookies[i], expected_cookies_[i]));
        ++cookie_count_;
      }
    } else {
      for (size_t i = 0; i < expected_cookies_.size(); ++i) {
        auto it = std::find_if(cookies.begin(), cookies.end(),
            [this, i](const net::CanonicalCookie& cookie) {
                                 return CookieEqualityForTesting(
                                     cookie, expected_cookies_[i]);
            });
        EXPECT_NE(it, cookies.end());
      }
      cookie_count_ = cookies.size();
    }

    if (writer_ != nullptr) {
      writer_->AddCookies(std::move(cookies));
    }
  }

  void SetProfileWriter(ProfileWriter* writer) { writer_ = writer; }
  void SetStoragePartition(content::StoragePartition* partition) {
    partition_ = partition;
  }

 private:
  ~TestObserver() override {}
  void TestCookiesFromCookieManager() {
    network::mojom::CookieManager* cookie_manager =
        partition_->GetCookieManagerForBrowserProcess();
    cookie_manager->GetAllCookies(base::BindOnce(
        &TestObserver::OnCookieListReceived, base::Unretained(this)));
  }

  void OnCookieListReceived(const std::vector<net::CanonicalCookie>& cookies) {
    EXPECT_EQ(cookie_count_, cookies.size());
    for (size_t i = 0; i < expected_cookies_.size(); ++i) {
      auto it = std::find_if(cookies.begin(), cookies.end(),
          [this, i](const net::CanonicalCookie& cookie) {
                               return CookieEqualityForTesting(
                                   cookie, expected_cookies_[i]);
          });
      EXPECT_NE(it, cookies.end());
    }
    std::move(quit_closure_).Run();
  }

  // This is the count of bookmark entries observed during the test.
  size_t bookmark_count_;
  // This is the expected list of bookmark entries to observe during the test.
  std::vector<BookmarkInfo> expected_bookmark_entries_;
  // This is the expected list of favicon groups to observe during the test.
  std::vector<FaviconGroup> expected_favicon_groups_;
  // This is the count of favicon groups observed during the test.
  size_t favicon_count_;
  // This is the expected list of cookies to observe during the test.
  std::vector<net::CanonicalCookie> expected_cookies_;
  // This is the count of cookies observed during the test.
  size_t cookie_count_;

  ProfileWriter* writer_ = nullptr;
  content::StoragePartition* partition_;
  base::OnceClosure quit_closure_;
  bool should_show_fav_on_ntp_;
};

bool DecompressDatabase(const base::FilePath& data_path) {
  // Code for import from spartan looks for |LogFiles| directory, in case it
  // needs recovery files.
  if (!CreateDirectory(
          data_path.Append(
              L"DataStore\\Data\\nouser1\\120712-0049\\DBStore\\LogFiles")))
    return false;

  base::FilePath output_file = data_path.Append(
      L"DataStore\\Data\\nouser1\\120712-0049\\DBStore\\spartan.edb");
  base::FilePath gzip_file = output_file.AddExtension(L".gz");
  std::string gzip_data;
  if (!base::ReadFileToString(gzip_file, &gzip_data))
    return false;
  if (!compression::GzipUncompress(gzip_data, &gzip_data))
    return false;
  return base::WriteFile(output_file, gzip_data.c_str(), gzip_data.size()) >= 0;
}

const char kDummyFaviconImageData[] =
    "\x42\x4D"           // Magic signature 'BM'
    "\x1E\x00\x00\x00"   // File size
    "\x00\x00\x00\x00"   // Reserved
    "\x1A\x00\x00\x00"   // Offset of the pixel data
    "\x0C\x00\x00\x00"   // Header Size
    "\x01\x00\x01\x00"   // Size: 1x1
    "\x01\x00"           // Reserved
    "\x18\x00"           // 24-bits
    "\x00\xFF\x00\x00";  // The pixel

}  // namespace

// These tests need to be browser tests in order to be able to run the OOP
// import (via ExternalProcessImporterHost) which launches a utility process.
class EdgeImporterBrowserTest : public InProcessBrowserTest {
 public:
  EdgeImporterBrowserTest() {
    feature_list_.InitAndEnableFeature(
        features::edge::kEnableVirtualCookieImportForTesting);
  }

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // This will launch the browser test and thus needs to happen last.
    InProcessBrowserTest::SetUp();
  }

  base::ScopedTempDir temp_dir_;
  base::test::ScopedFeatureList feature_list_;
  // Overrides the default registry key for Edge tests.
  ImporterTestRegistryOverrider test_registry_overrider_;
};

IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, EdgeImporter) {
  // Only verified to work with ESE library on Windows 8.1 and above.
  if (base::win::GetVersion() < base::win::Version::WIN8_1)
    return;

  const BookmarkInfo kEdgeBookmarks[] = {
      {true,
       2,
       {"Favorites bar", "SubFolderOfLinks"},
       L"SubLink",
       "http://www.links-sublink.com/"},
      {true, 1, {"Favorites bar"}, L"TheLink", "http://www.links-thelink.com/"},
      {false, 0, {}, L"MyLinks", ""},
      {false, 0, {}, L"Google Home Page", "http://www.google.com/"},
      {false, 0, {}, L"TheLink", "http://www.links-thelink.com/"},
      {false, 1, {"SubFolder"}, L"Title", "http://www.link.com/"},
      {false, 0, {}, L"WithPortAndQuery", "http://host:8080/cgi?q=query"},
      {false, 1, {"a"}, L"\x4E2D\x6587", "http://chinese-title-favorite/"},
      {false, 0, {}, L"SubFolder", "http://www.subfolder.com/"},
      {false, 0, {}, L"InvalidFavicon", "http://www.invalid-favicon.com/"},
  };
  std::vector<BookmarkInfo> bookmark_entries(
      kEdgeBookmarks, kEdgeBookmarks + base::size(kEdgeBookmarks));

  const FaviconGroup kEdgeFaviconGroup[] = {
      {L"http://www.links-sublink.com/favicon.ico",
       L"http://www.links-sublink.com"},
      {L"http://www.links-thelink.com/favicon.ico",
       L"http://www.links-thelink.com"},
      {L"http://www.google.com/favicon.ico", L"http://www.google.com"},
      {L"http://www.links-thelink.com/favicon.ico",
       L"http://www.links-thelink.com"},
      {L"http://www.link.com/favicon.ico", L"http://www.link.com"},
      {L"http://host:8080/favicon.ico", L"http://host:8080/cgi?q=query"},
      {L"http://chinese-title-favorite/favicon.ico",
       L"http://chinese-title-favorite"},
      {L"http://www.subfolder.com/favicon.ico", L"http://www.subfolder.com"},
  };

  std::vector<FaviconGroup> favicon_groups(
      kEdgeFaviconGroup, kEdgeFaviconGroup + base::size(kEdgeFaviconGroup));

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  // The fixed item id in edge_profile for |__FAVORITES_BAR__| is not valid
  // and the spartan.edb seems to be manually created from upstream.
  // Adding a spartan.edb taken from spartan and added the entries similar
  // to the upstream db for the tests.
  if (base::FeatureList::IsEnabled(features::edge::kEdgeOnRampImport)) {
    data_path = data_path.AppendASCII("import").AppendASCII("edge").
                    AppendASCII("edge_profile");
  } else {
    data_path = data_path.AppendASCII("edge_profile");
  }

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
    ASSERT_TRUE(DecompressDatabase(temp_path.AppendASCII("edge_profile")));
  }

  base::string16 key_path(importer::GetEdgeSettingsKey());
  base::win::RegKey key;
  ASSERT_EQ(ERROR_SUCCESS,
            key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE));
  key.WriteValue(L"FavoritesESEEnabled", 1);
  ASSERT_FALSE(importer::IsEdgeFavoritesLegacyMode());

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), bookmark_entries, favicon_groups,
                       std::vector<net::CanonicalCookie>()));
  EXPECT_CALL(*observer, ImportItemStarted(importer::FAVORITES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::FAVORITES)).Times(1);
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII("edge_profile");

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::FAVORITES,
                            observer.get());
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, EdgeImporterLegacyFallback) {
  // We only do legacy fallback on versions < Version::WIN10_TH2.
  if (base::win::GetVersion() >= base::win::Version::WIN10_TH2)
    return;

  const BookmarkInfo kEdgeBookmarks[] = {
      {true, 0, {}, L"Google", "http://www.google.com/"}};
  std::vector<BookmarkInfo> bookmark_entries(
      kEdgeBookmarks, kEdgeBookmarks + base::size(kEdgeBookmarks));
  const FaviconGroup kEdgeFaviconGroup[] = {
      {L"http://www.google.com/favicon.ico", L"http://www.google.com/"}};
  std::vector<FaviconGroup> favicon_groups(
      kEdgeFaviconGroup, kEdgeFaviconGroup + base::size(kEdgeFaviconGroup));

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("edge_profile");

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_dir_.GetPath(), true));
    ASSERT_TRUE(importer::IsEdgeFavoritesLegacyMode());
  }

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(new TestObserver(
      loop.QuitClosure(), bookmark_entries, favicon_groups,
      std::vector<net::CanonicalCookie>(), /*should_show_fav_on_ntp=*/false));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  base::FilePath source_path = temp_dir_.GetPath().AppendASCII("edge_profile");
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_NE(
        -1, base::WriteFile(
                source_path.AppendASCII("Favorites\\Google.url:favicon:$DATA"),
                kDummyFaviconImageData, sizeof(kDummyFaviconImageData)));
  }
  source_profile.source_path = source_path;

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::FAVORITES, observer.get());
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, EdgeImporterNoDatabase) {
  // Only verified to work with ESE library on Windows 8.1 and above.
  if (base::win::GetVersion() < base::win::Version::WIN8_1)
    return;

  std::vector<BookmarkInfo> bookmark_entries;
  std::vector<FaviconGroup> favicon_groups;

  base::string16 key_path(importer::GetEdgeSettingsKey());
  base::win::RegKey key;
  ASSERT_EQ(ERROR_SUCCESS,
            key.Create(HKEY_CURRENT_USER, key_path.c_str(), KEY_WRITE));
  key.WriteValue(L"FavoritesESEEnabled", 1);
  ASSERT_FALSE(importer::IsEdgeFavoritesLegacyMode());

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), bookmark_entries, favicon_groups,
                       std::vector<net::CanonicalCookie>()));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_dir_.GetPath();

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::FAVORITES, observer.get());
  loop.Run();
}

// Test importing cookies of varying properties
IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, CookieImportTest) {
  // Disable test below Windows 10 as Spartan is only supported on Windows 10
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    return;
  }

  const base::Time test_time = base::Time::FromJsTime(1546902716000);
  const base::Time expiry_time = base::Time::FromJsTime(3751161424154);
  const net::CanonicalCookie kEdgeCookies[] = {
      {
          std::string("tomcat"),
          std::string("tomcat"),
          std::string(".foo.com"),
          std::string("/"),
          test_time,
          expiry_time,
          test_time,
          false,
          false,
          net::CookieSameSite::STRICT_MODE,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("foo"),
          std::string("bar"),
          std::string(".foo.com"),
          std::string("/1"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::NO_RESTRICTION,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("foo"),
          std::string("bar"),
          std::string(".foo.com"),
          std::string("/2"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::NO_RESTRICTION,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("A1"),
          std::string("B1"),
          std::string(".foo.com"),
          std::string("/edgelegacycookie"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::NO_RESTRICTION,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("A2"),
          std::string("B2"),
          std::string(".foo.com"),
          std::string("/creation"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::NO_RESTRICTION,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("A3"),
          std::string("B3"),
          std::string(".foo.com"),
          std::string("/last_access"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::NO_RESTRICTION,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      }};

  std::vector<net::CanonicalCookie> cookies(std::begin(kEdgeCookies),
                                            std::end(kEdgeCookies));
  cookies[1].SetEdgeLegacyCookie(true);
  cookies[2].SetEdgeLegacyCookie(true);

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("cookies").AppendASCII("edge_profile");

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
  }

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), std::vector<BookmarkInfo>(),
                       std::vector<FaviconGroup>(), cookies));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII("edge_profile");

  EXPECT_CALL(*observer, ImportItemStarted(importer::COOKIES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::COOKIES)).Times(1);

  observer->SetProfileWriter(new ProfileWriter(browser()->profile()));
  observer->SetStoragePartition(
      content::BrowserContext::GetDefaultStoragePartition(
          browser()->profile()));

  base::HistogramTester hist_tester;
  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::COOKIES, observer.get());
  loop.Run();

  hist_tester.ExpectUniqueSample(
      "Microsoft.ImportProfile.Cookies.ImportCookieInclusionStatus", 0,
      cookies.size());
  const int kCookieImporterPreFilterCookieCount = 8;
  hist_tester.ExpectUniqueSample("Microsoft.ImportProfile.Cookies.CookiesCount",
                                 kCookieImporterPreFilterCookieCount, 1);
  hist_tester.ExpectUniqueSample(
      "Microsoft.ImportProfile.Cookies.FilteredCookiesCount", cookies.size(),
      1);
}

class EdgeCookieImporterEndToEndBrowserTest : public EdgeImporterBrowserTest {
 public:
  EdgeCookieImporterEndToEndBrowserTest() { feature_list_.Reset(); }

 protected:
  void SetCookiesInSpartan(const base::FilePath& cookie_file,
                           const base::Time& time,
                           bool delete_cookies) {
    base::CommandLine cmd_line(base::FilePath(L"cookie_setter.exe"));
    cmd_line.AppendSwitchPath("path", cookie_file);
    if (delete_cookies) {
      cmd_line.AppendSwitch("delete");
    } else {
      cmd_line.AppendSwitchASCII("expiration",
                                 base::NumberToString(time.ToJavaTime()));
    }
    base::LaunchOptions options;
    options.wait = true;
    options.start_hidden = true;
    base::Process process = LaunchProcess(cmd_line, std::move(options));
    int exit_code;
    process.WaitForExitWithTimeout(base::TimeDelta::FromSeconds(5), &exit_code);
    ASSERT_EQ(exit_code, 0);
  }
};

IN_PROC_BROWSER_TEST_F(EdgeCookieImporterEndToEndBrowserTest,
                       EDGE_DISABLED_TRIAGE(CookieImportEndToEnd, 26191804)) {
  // Disable test below Windows 10 as Spartan is only supported on Windows 10
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    return;
  }

  importer::ApplyACLPermissions(true);

  const base::Time test_time = base::Time::FromJsTime(1546902716000);

  // Set the expiration of the Edge Spartan cookies to 5 minutes from now since
  // we want them to delete as soon as possible. We convert to Java time and
  // back to ensure the times match since Java time (which is an easily
  // convertible format) is what we send to the cookie_setter.exe process.
  const base::Time future_time =
      base::Time::Now() + base::TimeDelta::FromMinutes(5);
  const base::Time expiry_time =
      base::Time::FromJavaTime(future_time.ToJavaTime());

  const net::CanonicalCookie kEdgeCookies[] = {
      {
          std::string("name1"),
          std::string("value1"),
          std::string(".cookie-importer.test"),
          std::string("/"),
          test_time,
          expiry_time,
          test_time,
          false,
          false,
          net::CookieSameSite::UNSPECIFIED,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("name2"),
          std::string("value2"),
          std::string(".cookie-importer.test"),
          std::string("/1"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::LAX_MODE,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      },
      {
          std::string("name3"),
          std::string("value3"),
          std::string("cookie-importer.test"),
          std::string("/"),
          test_time,
          expiry_time,
          test_time,
          true,
          true,
          net::CookieSameSite::STRICT_MODE,
          net::CookiePriority::COOKIE_PRIORITY_DEFAULT,
      }};

  std::vector<net::CanonicalCookie> cookies(std::begin(kEdgeCookies),
                                            std::end(kEdgeCookies));
  cookies[1].SetEdgeLegacyCookie(true);

  base::FilePath data_path;
  std::string cookie_folder_name = "roundtrip_end_to_end";
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("cookies").AppendASCII(cookie_folder_name);

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
  }

  SetCookiesInSpartan(temp_path.AppendASCII(cookie_folder_name), future_time,
                      false);

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), std::vector<BookmarkInfo>(),
                       std::vector<FaviconGroup>(), cookies));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII(cookie_folder_name);

  observer->SetProfileWriter(new ProfileWriter(browser()->profile()));
  observer->SetStoragePartition(
      content::BrowserContext::GetDefaultStoragePartition(
          browser()->profile()));

  EXPECT_CALL(*observer, ImportItemStarted(importer::COOKIES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::COOKIES)).Times(1);

  base::HistogramTester hist_tester;
  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::COOKIES, observer.get());
  loop.Run();

  SubprocessMetricsProvider::MergeHistogramDeltasForTesting();
  importer::LogCookieHistogramErrors(hist_tester);

  SetCookiesInSpartan(temp_path.AppendASCII(cookie_folder_name), future_time,
                      true);
}

// Test importing a large number (100) of large cookies (4kb - cookie max
// size)
IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, CookieImportStressTest) {
  // Disable test below Windows 10 as Spartan is only supported on Windows 10
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    return;
  }

  const base::Time test_time = base::Time::FromJsTime(1546902716000);
  const base::Time expiry_time = base::Time::FromJsTime(3751161424154);
  std::vector<net::CanonicalCookie> cookies(100);
  std::fill(cookies.begin(), cookies.end(),
            net::CanonicalCookie(std::string("foo"), std::string(4090, 'a'),
                                 std::string(".foo.com"), std::string("/"),
                                 test_time, expiry_time, test_time, false,
                                 false, net::CookieSameSite::NO_RESTRICTION,
                                 net::CookiePriority::COOKIE_PRIORITY_DEFAULT));

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("cookies").AppendASCII("stress");

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
  }

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), std::vector<BookmarkInfo>(),
                       std::vector<FaviconGroup>(), std::move(cookies)));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII("stress");

  EXPECT_CALL(*observer, ImportItemStarted(importer::COOKIES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::COOKIES)).Times(1);

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::COOKIES, observer.get());
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, CookieImportMalformedJSON) {
  // Disable test below Windows 10 as Spartan is only supported on Windows 10
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    return;
  }

  std::vector<net::CanonicalCookie> cookies;

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("cookies").AppendASCII("bad_json");

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
  }

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), std::vector<BookmarkInfo>(),
                       std::vector<FaviconGroup>(), std::move(cookies)));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII("bad_json");

  EXPECT_CALL(*observer, ImportItemStarted(importer::COOKIES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::COOKIES)).Times(1);

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::COOKIES, observer.get());
  loop.Run();
}

IN_PROC_BROWSER_TEST_F(EdgeImporterBrowserTest, CookieImportMissingJSON) {
  // Disable test below Windows 10 as Spartan is only supported on Windows 10
  if (base::win::GetVersion() < base::win::Version::WIN10) {
    return;
  }

  std::vector<net::CanonicalCookie> cookies;

  base::FilePath data_path;
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &data_path));
  data_path = data_path.AppendASCII("cookies").AppendASCII("no_json");

  base::FilePath temp_path = temp_dir_.GetPath();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::CopyDirectory(data_path, temp_path, true));
  }

  // Starts to import the above settings.
  // Deletes itself.
  ExternalProcessImporterHost* host = new ExternalProcessImporterHost(
      ImportType::kManualImport /*is_auto_import*/, false /*retry_on_error*/,
      base::TimeTicks::Now());
  base::RunLoop loop;
  scoped_refptr<TestObserver> observer(
      new TestObserver(loop.QuitClosure(), std::vector<BookmarkInfo>(),
                       std::vector<FaviconGroup>(), std::move(cookies)));
  host->set_observer(observer.get());

  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_EDGE;
  source_profile.source_path = temp_path.AppendASCII("no_json");

  EXPECT_CALL(*observer, ImportItemStarted(importer::COOKIES)).Times(1);
  EXPECT_CALL(*observer, ImportItemEnded(importer::COOKIES)).Times(1);

  host->StartImportSettings(source_profile, browser()->profile(),
                            importer::COOKIES, observer.get());
  loop.Run();
}
