// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/omnibox_metrics_provider.h"

#include <vector>
#include <locale>
#include <codecvt>

#include "base/bind.h"
<<<<<<< HEAD
#include "base/logging.h"
#include "base/time/time.h"
=======
>>>>>>> 9e19386f02d111ced11d2cc89294f0b3d747775b
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/metrics/metrics_log.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/omnibox_log.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/microsoft/core/common/url_utils.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "third_party/metrics_proto/omnibox_input_type.pb.h"

using metrics::OmniboxEventProto;

OmniboxMetricsProvider::OmniboxMetricsProvider(
    const base::Callback<bool(void)>& is_off_the_record_callback)
    : is_off_the_record_callback_(is_off_the_record_callback) {}

OmniboxMetricsProvider::~OmniboxMetricsProvider() {
}

void OmniboxMetricsProvider::OnRecordingEnabled() {
  open_url_subscription_ =
      OmniboxEventGlobalTracker::GetInstance()->RegisterUrlOpenedCallback(
      base::Bind(&OmniboxMetricsProvider::OnURLOpenedFromOmnibox,
                 base::Unretained(this)));

  popup_changed_subscription_ =
      OmniboxEventGlobalTracker::GetInstance()->RegisterPopupDataChangedCallback(
      base::Bind(&OmniboxMetricsProvider::OnPopupDataChanged,
                 base::Unretained(this)));
}

void OmniboxMetricsProvider::OnRecordingDisabled() {
  open_url_subscription_.reset();
  popup_changed_subscription_.reset();
}

void OmniboxMetricsProvider::ProvideCurrentSessionData(
    metrics::ChromeUserMetricsExtension* uma_proto) {
  uma_proto->mutable_omnibox_event()->Swap(
      omnibox_events_cache.mutable_omnibox_event());
}

void OmniboxMetricsProvider::OnURLOpenedFromOmnibox(OmniboxLog* log) {
  if (!is_off_the_record_callback_.Run())
    RecordOmniboxLog(MSEdgeExt::OPEN_MATCH, *log);
}

void OmniboxMetricsProvider::OnPopupDataChanged(OmniboxLog* log) {
  // Do not log events to UMA if the embedder reports that the user is in an
  // off-the-record context.
  if (!is_off_the_record_callback_.Run())
    RecordOmniboxLog(MSEdgeExt::IMPRESSION, *log);
}

void OmniboxMetricsProvider::RecordOmniboxLog(
  MSEdgeExtEventType event_type,
  const OmniboxLog& log) {
  std::vector<base::StringPiece16> terms = base::SplitStringPiece(
      log.text, base::kWhitespaceUTF16,
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  OmniboxEventProto* omnibox_event = omnibox_events_cache.add_omnibox_event();
  MSEdgeExt* bing_extensions = omnibox_event->mutable_msedge_bing_extensions();
  bing_extensions->set_event_type(event_type);
  omnibox_event->set_time_sec(metrics::MetricsLog::GetCurrentTime());
  if (log.tab_id.is_valid()) {
    // If we know what tab the autocomplete URL was opened in, log it.
    omnibox_event->set_tab_id(log.tab_id.id());
  }
  omnibox_event->set_typed_length(log.text.length());
  omnibox_event->set_just_deleted_text(log.just_deleted_text);
  omnibox_event->set_num_typed_terms(static_cast<int>(terms.size()));
  omnibox_event->set_selected_index(log.selected_index);
  omnibox_event->set_selected_tab_match(log.disposition ==
                                        WindowOpenDisposition::SWITCH_TO_TAB);
  if (log.completed_length != base::string16::npos)
    omnibox_event->set_completed_length(log.completed_length);
  const base::TimeDelta default_time_delta =
      base::TimeDelta::FromMilliseconds(-1);
  if (log.elapsed_time_since_user_first_modified_omnibox !=
      default_time_delta) {
    // Only upload the typing duration if it is set/valid.
    omnibox_event->set_typing_duration_ms(
        log.elapsed_time_since_user_first_modified_omnibox.InMilliseconds());
  }
  if (log.elapsed_time_since_last_change_to_default_match !=
      default_time_delta) {
    omnibox_event->set_duration_since_last_default_match_update_ms(
        log.elapsed_time_since_last_change_to_default_match.InMilliseconds());
  }
  omnibox_event->set_current_page_classification(
      log.current_page_classification);
  omnibox_event->set_input_type(log.input_type);
  // We consider a paste-and-search/paste-and-go action to have a closed popup
  // (as explained in omnibox_event.proto) even if it was not, because such
  // actions ignore the contents of the popup so it doesn't matter that it was
  // open.
  omnibox_event->set_is_popup_open(log.is_popup_open && !log.is_paste_and_go);
  omnibox_event->set_is_paste_and_go(log.is_paste_and_go);

  if (!log.search_conversation_id.empty())
    bing_extensions->set_search_conversation_id(log.search_conversation_id);

  if (!log.search_impression_id.empty())
    bing_extensions->set_search_impression_id(log.search_impression_id);

  if (base::FeatureList::IsEnabled(omnibox::kAddDelayToProtobufEvent)) {
    bing_extensions->set_event_time(
        (base::Time::Now() - base::TimeDelta::FromSeconds(10)).ToJavaTime());
  }
  else {
    bing_extensions->set_event_time(base::Time::Now().ToJavaTime());
  }

  for (auto i(log.result.begin()); i != log.result.end(); ++i) {
    OmniboxEventProto::Suggestion* suggestion = omnibox_event->add_suggestion();
        metrics::OmniboxEventProto_ProviderType provider_type;
    if (i->provider) {
      provider_type = i->provider->AsOmniboxEventProviderType();
    } else {
      // Provider can be null in tests.
      provider_type = metrics::OmniboxEventProto::UNKNOWN_PROVIDER;
    }
    suggestion->set_provider(provider_type);
    suggestion->set_result_type(i->AsOmniboxEventResultType());
    suggestion->set_relevance(i->relevance);
    if (i->typed_count != -1)
      suggestion->set_typed_count(i->typed_count);
    if (i->subtype_identifier > 0)
      suggestion->set_result_subtype_identifier(i->subtype_identifier);
    if (i->answer)
      suggestion->set_msedge_answer(i->AsOmniboxAnswerType());
    suggestion->set_has_tab_match(i->has_tab_match);
    suggestion->set_is_keyword_suggestion(i->from_keyword);
    if (provider_type == metrics::OmniboxEventProto::SEARCH &&
        components::msft_util::IsBingSearchUrl(i->destination_url) &&
        i->type != AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED &&
        i->type != AutocompleteMatchType::SEARCH_HISTORY) {
      suggestion->set_msedge_query(base::UTF16ToUTF8(i->contents));
    }
  }
  for (auto i(log.providers_info.begin()); i != log.providers_info.end(); ++i) {
    OmniboxEventProto::ProviderInfo* provider_info =
        omnibox_event->add_provider_info();
    provider_info->CopyFrom(*i);
  }
  omnibox_event->set_in_keyword_mode(log.in_keyword_mode);
  if (log.in_keyword_mode)
    omnibox_event->set_keyword_mode_entry_method(log.keyword_mode_entry_method);
}
