// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/net/variations_http_headers.h"

#include <stddef.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
<<<<<<< HEAD
#include "components/microsoft/core/common/url_utils.h"
#include "components/variations/edge_features.h"
=======
#include "components/google/core/common/google_util.h"
#include "components/variations/net/omnibox_http_headers.h"
>>>>>>> 3bdb5af13d03b6eaee24494aa820b90a1a4fabd9
#include "components/variations/variations_http_header_provider.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace variations {

// The name string for the header for variations information.
// Note that prior to M33 this header was named X-Chrome-Variations.
const char kClientDataHeader[] = "X-Client-Data";

namespace {

using namespace components::msft_util;

const char* kMsnVariationHeaderHosts[] = {
    "ntp.msn.com",
    "ntp.msn.cn",
    "www.msn.com",
    "www.msn.cn",
    "microsoftnews.msn.com",
    "microsoftnews.msn.cn",
    "www.microsoftnews.com",
    "www.microsoftnews.cn",
    "api.msn.com",
    "web.vortex.data.msn.com",
    "web.vortex.data.msn.cn",
    "web.vortex-sandbox.data.msn.com",
    "web.vortex-sandbox.data.msn.cn",
    "events-sandbox.data.msn.com",
    "events-sandbox.data.msn.cn",
    "browser.events.data.msn.com",
    "browser.events.data.msn.cn"
};

bool IsUrlOfMsnVariationHeaderHosts(const GURL& url) {
  for (auto* host : kMsnVariationHeaderHosts) {
    if (url.host() == host)
      return true;
  }
  return false;
}

bool IsGaplessExperimentationUrl(const GURL& url) {
  return IsBingUrl(url, ALLOW_SUBDOMAIN, ALLOW_NON_STANDARD_PORTS) ||
         IsUrlOfMsnVariationHeaderHosts(url);
}

// The result of checking if a URL should have variations headers appended.
// This enum is used to record UMA histogram values, and should not be
// reordered.
enum URLValidationResult {
  INVALID_URL,
  NOT_HTTPS,
  NOT_MICROSOFT_DOMAIN,
  SHOULD_APPEND,
  NEITHER_HTTP_HTTPS,
  IS_MICROSOFT_NOT_HTTPS,
  URL_VALIDATION_RESULT_SIZE,
  IS_MICROSOFT_URL_NOT_APPEND,
  FEATURE_NOT_ON
};

void LogUrlValidationHistogram(URLValidationResult result) {
  UMA_HISTOGRAM_ENUMERATION("Variations.Headers.URLValidationResult", result,
                            URL_VALIDATION_RESULT_SIZE);
}

bool ShouldAppendVariationsHeader(const GURL& url) {
  if (!url.is_valid()) {
    LogUrlValidationHistogram(INVALID_URL);
    return false;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
    LogUrlValidationHistogram(NEITHER_HTTP_HTTPS);
    return false;
  }

  bool is_microsoft_url =
      IsMicrosoftUrl(url, ALLOW_SUBDOMAIN, ALLOW_NON_STANDARD_PORTS);
  if (!is_microsoft_url) {
    LogUrlValidationHistogram(NOT_MICROSOFT_DOMAIN);
    return false;
  }

  // URL should be Microsoft from here on.

  // We check https here, rather than before the IsMicrosoftUrl() check, to know
  // how many Microsoft domains are being rejected by the change to https only.
  if (!url.SchemeIs("https")) {
    LogUrlValidationHistogram(IS_MICROSOFT_NOT_HTTPS);
    return false;
  }

  bool is_mesh_url = IsEdgeMeshUrl(url);
  if (is_mesh_url && !base::FeatureList::IsEnabled(
                         features::kSendClientDataHeaderToEdgeServices)) {
    // Mesh url, but feature off.
    LogUrlValidationHistogram(FEATURE_NOT_ON);
    return false;
  }

  bool is_gapless_url = IsGaplessExperimentationUrl(url);
  if (is_gapless_url &&
      !base::FeatureList::IsEnabled(features::kSendClientDataHeader)) {
    // Gapless url, but feature off.
    LogUrlValidationHistogram(FEATURE_NOT_ON);
    return false;
  }

  if (!(is_gapless_url || is_mesh_url)) {
    // If it's neither gapless nor mesh, but still Microsoft url.
    LogUrlValidationHistogram(IS_MICROSOFT_URL_NOT_APPEND);
    return false;
  }

  // Make sure that somehow future code edits doesn't break this.
  DCHECK(is_microsoft_url);
  LogUrlValidationHistogram(SHOULD_APPEND);
  return true;
}

class VariationsHeaderHelper {
 public:
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  VariationsHeaderHelper(network::ResourceRequest* request,
                         SignedIn signed_in = SignedIn::kNo)
      : VariationsHeaderHelper(
            request,
            CreateVariationsHeader(signed_in, request->url)) {}
  VariationsHeaderHelper(network::ResourceRequest* resource_request,
                         std::string variations_header)
      : resource_request_(resource_request) {
    DCHECK(resource_request_);
    variations_header_ = std::move(variations_header);
  }

  bool AppendHeaderIfNeeded(const GURL& url, InIncognito incognito) {
    AppendOmniboxOnDeviceSuggestionsHeaderIfNeeded(url, resource_request_);

    // Note the criteria for attaching client experiment headers:
    // 1. We only transmit to Microsoft owned domains which can evaluate
    // experiments.
    //    1a. A specific list of msn domains.
    //    1b. Bing domains.
    //    1c. Edge Mesh domains if the feature flag is on.
    // 2. Only transmit for non-Incognito profiles.
    // 3. For the X-Client-Data header, the format will depend on the service
    //    and the data format they support, but we will only send the header
    //    when it isn't empty.

    // The variations header gets sent to Microsoft Edge services to allow
    // experiment coordination.
    if ((incognito == InIncognito::kYes) ||
        !ShouldAppendVariationsHeader(url) || variations_header_.empty())
      return false;

    // Set the variations header to cors_exempt_headers rather than headers
    // to be exempted from CORS checks.
    resource_request_->cors_exempt_headers.SetHeaderIfMissing(
        kClientDataHeader, variations_header_);
    return true;
  }

 private:
  static std::string CreateVariationsHeader(SignedIn signed_in,
                                            const GURL& url) {
    return VariationsHttpHeaderProvider::GetInstance()->GetClientDataHeader(
        signed_in == SignedIn::kYes, IsEdgeMeshUrl(url) /* use_proto */);
  }

  network::ResourceRequest* resource_request_;
  std::string variations_header_;

  DISALLOW_COPY_AND_ASSIGN(VariationsHeaderHelper);
};

}  // namespace

bool AppendVariationsHeader(const GURL& url,
                            InIncognito incognito,
                            SignedIn signed_in,
                            network::ResourceRequest* request) {
  return VariationsHeaderHelper(request, signed_in)
      .AppendHeaderIfNeeded(url, incognito);
}

bool AppendVariationsHeaderWithCustomValue(const GURL& url,
                                           InIncognito incognito,
                                           const std::string& variations_header,
                                           network::ResourceRequest* request) {
  return VariationsHeaderHelper(request, variations_header)
      .AppendHeaderIfNeeded(url, incognito);
}

bool AppendVariationsHeaderUnknownSignedIn(const GURL& url,
                                           InIncognito incognito,
                                           network::ResourceRequest* request) {
  return VariationsHeaderHelper(request).AppendHeaderIfNeeded(url, incognito);
}

void RemoveVariationsHeaderIfNeeded(
    const net::RedirectInfo& redirect_info,
    const network::mojom::URLResponseHead& response_head,
    std::vector<std::string>* to_be_removed_headers) {
  if (!ShouldAppendVariationsHeader(redirect_info.new_url)) {
    to_be_removed_headers->push_back(kClientDataHeader);
  }
}

std::unique_ptr<network::SimpleURLLoader>
CreateSimpleURLLoaderWithVariationsHeader(
    std::unique_ptr<network::ResourceRequest> request,
    InIncognito incognito,
    SignedIn signed_in,
    const net::NetworkTrafficAnnotationTag& annotation_tag) {
  bool variation_headers_added =
      AppendVariationsHeader(request->url, incognito, signed_in, request.get());
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader =
      network::SimpleURLLoader::Create(std::move(request), annotation_tag);
  if (variation_headers_added) {
    simple_url_loader->SetOnRedirectCallback(
        base::BindRepeating(&RemoveVariationsHeaderIfNeeded));
  }
  return simple_url_loader;
}

std::unique_ptr<network::SimpleURLLoader>
CreateSimpleURLLoaderWithVariationsHeaderUnknownSignedIn(
    std::unique_ptr<network::ResourceRequest> request,
    InIncognito incognito,
    const net::NetworkTrafficAnnotationTag& annotation_tag) {
  return CreateSimpleURLLoaderWithVariationsHeader(
      std::move(request), incognito, SignedIn::kNo, annotation_tag);
}

std::unique_ptr<network::SimpleURLLoader>
CreateSimpleURLLoaderWithVariationsHeaderAndTriggerUsage(
    std::unique_ptr<network::ResourceRequest> request,
    InIncognito incognito,
    SignedIn signed_in,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    const base::FeatureTrigger& trigger_flag) {
  if (incognito == InIncognito::kNo) {
    base::FeatureList::TriggerUsage(trigger_flag);
  }
  return CreateSimpleURLLoaderWithVariationsHeader(
      std::move(request), incognito, signed_in, annotation_tag);
}

std::unique_ptr<network::SimpleURLLoader>
CreateCircuitBreakerSimpleURLLoaderWithVariationsHeader(
    std::unique_ptr<network::ResourceRequest> request,
    InIncognito incognito,
    SignedIn signed_in,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    PrefService* local_state) {
  bool variation_headers_added =
      AppendVariationsHeader(request->url, incognito, signed_in, request.get());
  std::unique_ptr<network::SimpleURLLoader> simple_url_loader =
      network::CircuitBreakerSimpleURLLoader::Create(
          std::move(request), annotation_tag, local_state);
  if (variation_headers_added) {
    simple_url_loader->SetOnRedirectCallback(
        base::BindRepeating(&RemoveVariationsHeaderIfNeeded));
  }
  return simple_url_loader;
}

std::unique_ptr<network::SimpleURLLoader>
CreateCircuitBreakerSimpleURLLoaderWithVariationsHeaderAndTriggerUsage(
    std::unique_ptr<network::ResourceRequest> request,
    InIncognito incognito,
    SignedIn signed_in,
    const net::NetworkTrafficAnnotationTag& annotation_tag,
    const base::FeatureTrigger& trigger_flag,
    PrefService* local_state) {
  if (incognito == InIncognito::kNo) {
    base::FeatureList::TriggerUsage(trigger_flag);
  }
  return CreateCircuitBreakerSimpleURLLoaderWithVariationsHeader(
      std::move(request), incognito, signed_in, annotation_tag, local_state);
}

bool IsVariationsHeader(const std::string& header_name) {
  return header_name == kClientDataHeader ||
         header_name == kOmniboxOnDeviceSuggestionsHeader;
}

bool HasVariationsHeader(const network::ResourceRequest& request) {
  // Note: kOmniboxOnDeviceSuggestionsHeader is not listed because this function
  // is only used for testing.
  return request.cors_exempt_headers.HasHeader(kClientDataHeader);
}

bool ShouldAppendVariationsHeaderForTesting(const GURL& url) {
  return ShouldAppendVariationsHeader(url);
}

void UpdateCorsExemptHeaderForVariations(
    network::mojom::NetworkContextParams* params) {
  params->cors_exempt_header_list.push_back(kClientDataHeader);

  if (base::FeatureList::IsEnabled(kReportOmniboxOnDeviceSuggestionsHeader)) {
    params->cors_exempt_header_list.push_back(
        kOmniboxOnDeviceSuggestionsHeader);
  }
}

}  // namespace variations
