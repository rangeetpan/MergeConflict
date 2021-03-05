// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/about_flags.h"

#include <stddef.h>

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/metrics/histogram_enum_reader.h"
#include "build/build_config.h"
<<<<<<< HEAD
#include "chrome/browser/edge_included_flags_list.h"
=======
#include "chrome/common/chrome_version.h"
>>>>>>> f6bb6b2b711b1b3597a07545b26838a6faa829f9
#include "components/flags_ui/feature_entry.h"
#include "components/flags_ui/flags_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace about_flags {

namespace {

using Sample = base::HistogramBase::Sample;
using SwitchToIdMap = std::map<std::string, Sample>;

<<<<<<< HEAD
const char MicrosoftLoginCustomFlagsEnum[] = "Microsoft.LoginCustomFlags";

// it's a helper function which inserts a switch into the output set if one of
// the two conditions is met:
//    1. it's the microsoft edge enum and microsoft edge switch
//    2. it's not the microsoft edge enum and not the microsft edge swich
void InsertSwitch(bool is_edge_enum,
                  const std::string& command_line_switch,
                  std::set<std::string>* out) {
  const std::set<std::string>& EdgeAboutFlagsSwitches =
      testing::GetEdgeAboutFlagsSwitchesForTesting();
  bool is_edge_switch = EdgeAboutFlagsSwitches.find(command_line_switch) !=
                        EdgeAboutFlagsSwitches.end();
  if ((is_edge_enum && is_edge_switch) || (!is_edge_enum && !is_edge_switch)) {
    out->insert(command_line_switch);
  }
}

// It's a helper function which inserts a feature into the output set if one of
// the two conditions is met:
//    1. it's the microsoft edge enum and microsoft edge feature
//    2. it's not the microsoft edge enum and not the microsoft edge feature
void InsertFeature(bool is_edge_enum,
                   const std::string& feature_name,
                   std::set<std::string>* out) {
  bool is_edge_feature = feature_name.find("ms") == 0;
  if ((is_edge_enum && is_edge_feature) ||
      (!is_edge_enum && !is_edge_feature)) {
    out->insert(feature_name);
  }
}

// Get all associated switches and features corresponding to defined
// about_flags.cc and edge_about_flags.cc entries, given their histograms enum
std::set<std::string> GetAllSwitchesAndFeaturesForTesting(
    const std::string enum_name) {
=======
// Get all associated switches corresponding to defined about_flags.cc entries.
std::set<std::string> GetAllPublicSwitchesAndFeaturesForTesting() {
>>>>>>> f6bb6b2b711b1b3597a07545b26838a6faa829f9
  std::set<std::string> result;
  bool is_edge_enum = (enum_name == std::string(MicrosoftLoginCustomFlagsEnum));

  size_t num_entries = 0;
  const flags_ui::FeatureEntry* entries =
      testing::GetFeatureEntries(&num_entries);

  for (size_t i = 0; i < num_entries; ++i) {
    const flags_ui::FeatureEntry& entry = entries[i];

    // Skip over flags that are part of the flags system itself - they don't
    // have any of the usual metadata or histogram entries for flags, since they
    // are synthesized during the build process.
    // TODO(https://crbug.com/1068258): Remove the need for this by generating
    // histogram entries automatically.
    if (entry.supported_platforms & flags_ui::kFlagInfrastructure)
      continue;

    switch (entry.type) {
      case flags_ui::FeatureEntry::SINGLE_VALUE:
      case flags_ui::FeatureEntry::SINGLE_DISABLE_VALUE:
        InsertSwitch(is_edge_enum, entry.command_line_switch, &result);
        break;
      case flags_ui::FeatureEntry::ORIGIN_LIST_VALUE:
        // Do nothing, origin list values are not added as feature flags.
        break;
      case flags_ui::FeatureEntry::MULTI_VALUE:
        for (int j = 0; j < entry.num_options; ++j) {
          InsertSwitch(is_edge_enum,
                       entry.ChoiceForOption(j).command_line_switch, &result);
        }
        break;
      case flags_ui::FeatureEntry::ENABLE_DISABLE_VALUE:
        InsertSwitch(is_edge_enum, entry.command_line_switch, &result);
        InsertSwitch(is_edge_enum, entry.disable_command_line_switch, &result);
        break;
      case flags_ui::FeatureEntry::FEATURE_VALUE:
      case flags_ui::FeatureEntry::FEATURE_WITH_PARAMS_VALUE:
        InsertFeature(is_edge_enum,
                      std::string(entry.feature->name) + ":enabled", &result);
        InsertFeature(is_edge_enum,
                      std::string(entry.feature->name) + ":disabled", &result);
        break;
    }
  }
  return result;
}

}  // anonymous namespace

// Makes sure there are no separators in any of the entry names.
TEST(AboutFlagsTest, NoSeparators) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
  for (size_t i = 0; i < count; ++i) {
    std::string name = entries[i].internal_name;
    EXPECT_EQ(std::string::npos, name.find(flags_ui::testing::kMultiSeparator))
        << i;
  }
}

// Makes sure that every flag has an owner and an expiry entry in
// flag-metadata.json.
TEST(AboutFlagsTest, EveryFlagHasMetadata) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
<<<<<<< HEAD
  flags_ui::testing::EnsureEveryFlagHasMetadata(flags_ui::testing::FlagFile::kFlagMetadata, entries, count);
}

TEST(AboutFlagsTest, EveryFlagHasNonEmptyOwners) {
  flags_ui::testing::EnsureEveryFlagHasNonEmptyOwners(flags_ui::testing::FlagFile::kFlagMetadata);
=======
  flags_ui::testing::EnsureEveryFlagHasMetadata(
      base::make_span(entries, count));
>>>>>>> f6bb6b2b711b1b3597a07545b26838a6faa829f9
}

TEST(AboutFlagsTest, OnlyPermittedFlagsNeverExpire) {
  flags_ui::testing::EnsureOnlyPermittedFlagsNeverExpire(flags_ui::testing::FlagFile::kFlagMetadata);
}

// Makes sure that every flag has an owner and an expiry entry in
// edge_flag_metadata.json.
TEST(AboutFlagsTest, EveryEdgeFlagHasMetadata) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
  flags_ui::testing::EnsureEveryFlagHasMetadata(flags_ui::testing::FlagFile::kEdgeFlagMetadata, entries, count);
}

TEST(AboutFlagsTest, EveryEdgeFlagHasNonEmptyOwners) {
  flags_ui::testing::EnsureEveryFlagHasNonEmptyOwners(flags_ui::testing::FlagFile::kEdgeFlagMetadata);
}

TEST(AboutFlagsTest, OnlyPermittedEdgeFlagsNeverExpire) {
  flags_ui::testing::EnsureOnlyPermittedFlagsNeverExpire(flags_ui::testing::FlagFile::kEdgeFlagMetadata);
}

TEST(AboutFlagsTest, EdgeFlagsHaveNoBrandingIssues) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
  const std::set<std::string>& includedFlags = flags::GetIncludedFlags();
  flags_ui::testing::EnsureEdgeFlagsHaveNoBrandingIssues(entries, count, includedFlags);
}

TEST(AboutFlagsTest, EveryChromiumFlagHasEdgeMetadata) {
  flags_ui::testing::EnsureEveryChromiumFlagHasEdgeMetadata();
}


// DO NOT DISABLE!!!
// If the test is failing, please, follow instructions on
// 'https://aka.ms/edgefx/afwiki/fix-new-chromium-flags-detected-test' to fix it
// instead.
TEST(AboutFlagsTest, NewChromiumFlagsDetected) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
  flags_ui::testing::EnsureNoNewChromiumFlagsDetected(entries, count);
}

TEST(AboutFlagsTest, EdgeAboutFlagEntriesHaveEdgePrefix) {
  flags_ui::testing::EnsureAboutFlagEntriesHaveEdgePrefix();
}

TEST(AboutFlagsTest, EdgeOwnersLookValid) {
  flags_ui::testing::EnsureOwnersLookValid(flags_ui::testing::FlagFile::kEdgeFlagMetadata);
}

TEST(AboutFlagsTest, OwnersLookValid) {
  flags_ui::testing::EnsureOwnersLookValid(flags_ui::testing::FlagFile::kFlagMetadata);
}

// For some bizarre reason, far too many people see a file filled with
// alphabetically-ordered items and think "hey, let me drop this new item into a
// random location!" Prohibit such behavior in the flags files.
TEST(AboutFlagsTest, FlagsListedInAlphabeticalOrder) {
  flags_ui::testing::EnsureFlagsAreListedInAlphabeticalOrder();
}

TEST(AboutFlagsTest, RecentUnexpireFlagsArePresent) {
  size_t count;
  const flags_ui::FeatureEntry* entries = testing::GetFeatureEntries(&count);
  flags_ui::testing::EnsureRecentUnexpireFlagsArePresent(
      base::make_span(entries, count), CHROME_VERSION_MAJOR);
}

class AboutFlagsHistogramTest : public ::testing::Test {
 protected:
  // This is a helper function to check that all IDs in enums LoginCustomFlags,
  // Microsoft.LoginCustomFlags in histograms.xml are unique.
  void SetSwitchToHistogramIdMapping(const std::string& enum_name,
                                     const std::string& switch_name,
                                     const Sample switch_histogram_id,
                                     std::map<std::string, Sample>* out_map) {
    const std::pair<std::map<std::string, Sample>::iterator, bool> status =
        out_map->insert(std::make_pair(switch_name, switch_histogram_id));
    if (!status.second) {
      EXPECT_TRUE(status.first->second == switch_histogram_id)
          << "Duplicate switch '" << switch_name << "' found in enum "
          << enum_name << " in "
          << (enum_name == MicrosoftLoginCustomFlagsEnum ? "edge_enums.xml"
                                                         : "enums.xml")
          << ".";
    }
  }

  // This method generates a hint for the user for what string should be added
  // to the enum to make it consistent.
  std::string GetHistogramEnumEntryText(const std::string& switch_name,
                                        Sample value) {
    return base::StringPrintf(
        "<int value=\"%d\" label=\"%s\"/>", value, switch_name.c_str());
  }

  void CheckHistogram(const std::string enum_name) {
    bool is_edge_enum = (enum_name == MicrosoftLoginCustomFlagsEnum);
    std::string enum_file_name = (is_edge_enum ? "edge_enums.xml" : "enums.xml");

    base::Optional<base::HistogramEnumEntryMap> login_custom_flags =
        (is_edge_enum ? base::ReadEnumFromEdgeEnumsXml(enum_name)
                      : base::ReadEnumFromEnumsXml(enum_name));
    ASSERT_TRUE(login_custom_flags)
        << "Error reading enum " << enum_name << " from " << enum_file_name << ".";

    // Build reverse map {switch_name => id} from login_custom_flags.
    SwitchToIdMap histograms_xml_switches_ids;

    EXPECT_TRUE(login_custom_flags->count(testing::kBadSwitchFormatHistogramId))
        << "Entry for UMA ID of incorrect command-line flag is not found in "
        << enum_file_name << " enum " << enum_name
        << ". Consider adding entry:\n"
        << "  " << GetHistogramEnumEntryText("BAD_FLAG_FORMAT", 0);
    // Check that all 'enum_name' entries have correct values.
    for (const auto& entry : *login_custom_flags) {
      if (entry.first == testing::kBadSwitchFormatHistogramId) {
        // Add error value with empty name.
        SetSwitchToHistogramIdMapping(enum_name, std::string(), entry.first,
                                      &histograms_xml_switches_ids);
        continue;
      }
      const Sample uma_id = GetSwitchUMAId(entry.second);
      EXPECT_EQ(uma_id, entry.first)
          << enum_file_name << " enum " << enum_name << " entry '" << entry.second
          << "' has incorrect value=" << entry.first << ", but " << uma_id
          << " is expected. Consider changing entry to:\n"
          << "  " << GetHistogramEnumEntryText(entry.second, uma_id);
      SetSwitchToHistogramIdMapping(enum_name, entry.second, entry.first,
                                    &histograms_xml_switches_ids);
      // check for 'ms' prefix
      const std::set<std::string>& EdgeAboutFlagsSwitches =
        testing::GetEdgeAboutFlagsSwitchesForTesting();
      if (is_edge_enum) {
        EXPECT_TRUE(entry.second.find("ms") == 0 ||
                    EdgeAboutFlagsSwitches.find(entry.second) !=
                        EdgeAboutFlagsSwitches.end())
            << "edge_enums.xml enum " << enum_name << " entry " << entry.second
            << " has 'ms' prefix missing. Consider adding it.";
      } else {
        EXPECT_FALSE(entry.second.find("ms") == 0)
            << "enums.xml enum " << enum_name << " entry " << entry.second
            << " has 'ms' prefix. Consider adding this entry to edge_enums.xml enum "
            << MicrosoftLoginCustomFlagsEnum;
      }
    }

<<<<<<< HEAD
    // Check that all flags in the about_flags.cc that belog to 'enum_name' have
    // entries in 'enum_name'
    std::set<std::string> all_flags =
        GetAllSwitchesAndFeaturesForTesting(enum_name);
    for (const std::string& flag : all_flags) {
      // Skip empty placeholders.
      if (flag.empty())
        continue;
      const Sample uma_id = GetSwitchUMAId(flag);
      EXPECT_NE(testing::kBadSwitchFormatHistogramId, uma_id)
          << "Command-line switch '" << flag << "' from "
          << (is_edge_enum ? "edge_about_flags.cc" : "about_flags.cc")
          << "has UMA ID equal to reserved value "
             "kBadSwitchFormatHistogramId="
          << testing::kBadSwitchFormatHistogramId
          << ". Please modify switch name.";
      auto enum_entry = histograms_xml_switches_ids.lower_bound(flag);

      // Ignore case here when switch ID is incorrect - it has already been
      // reported in the previous loop.
      EXPECT_TRUE(enum_entry != histograms_xml_switches_ids.end() &&
                  enum_entry->first == flag)
          << enum_file_name << " enum " << enum_name << " doesn't contain switch '"
          << flag << "' (value=" << uma_id
          << " expected). Consider adding entry:\n"
          << "  " << GetHistogramEnumEntryText(flag, uma_id);
    }
=======
  // Check that all flags in about_flags.cc have entries in login_custom_flags.
  std::set<std::string> all_flags = GetAllPublicSwitchesAndFeaturesForTesting();
  for (const std::string& flag : all_flags) {
    // Skip empty placeholders.
    if (flag.empty())
      continue;
    const Sample uma_id = GetSwitchUMAId(flag);
    EXPECT_NE(testing::kBadSwitchFormatHistogramId, uma_id)
        << "Command-line switch '" << flag
        << "' from about_flags.cc has UMA ID equal to reserved value "
           "kBadSwitchFormatHistogramId="
        << testing::kBadSwitchFormatHistogramId
        << ". Please modify switch name.";
    auto enum_entry = histograms_xml_switches_ids.lower_bound(flag);

    // Ignore case here when switch ID is incorrect - it has already been
    // reported in the previous loop.
    EXPECT_TRUE(enum_entry != histograms_xml_switches_ids.end() &&
                enum_entry->first == flag)
        << "enums.xml enum LoginCustomFlags doesn't contain switch '" << flag
        << "' (value=" << uma_id << " expected). Consider adding entry:\n"
        << "  " << GetHistogramEnumEntryText(flag, uma_id);
>>>>>>> f6bb6b2b711b1b3597a07545b26838a6faa829f9
  }
};

TEST_F(AboutFlagsHistogramTest, CheckHistograms) {
  CheckHistogram("LoginCustomFlags");
}

TEST_F(AboutFlagsHistogramTest, CheckEdgeHistograms) {
  CheckHistogram(MicrosoftLoginCustomFlagsEnum);
}

}  // namespace about_flags
