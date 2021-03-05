// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/web_ui_data_source_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
<<<<<<< HEAD
#include "base/command_line.h"
#include "base/logging.h"
=======
#include "base/check_op.h"
>>>>>>> f01bb3e32bb982557ec662dc13374950fe6d4c62
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/grit/content_resources.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/buildflags.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "ui/base/template_expressions.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

namespace content {

// static
WebUIDataSource* WebUIDataSource::Create(const std::string& source_name) {
  return new WebUIDataSourceImpl(source_name);
}

// static
void WebUIDataSource::Add(BrowserContext* browser_context,
                          WebUIDataSource* source) {
  URLDataManager::AddWebUIDataSource(browser_context, source);
}

// static
void WebUIDataSource::Update(BrowserContext* browser_context,
                             const std::string& source_name,
                             std::unique_ptr<base::DictionaryValue> update) {
  URLDataManager::UpdateWebUIDataSource(browser_context, source_name,
                                        std::move(update));
}

namespace {

std::string CleanUpPath(const std::string& path) {
  // Remove the query string for named resource lookups.
  return path.substr(0, path.find_first_of('?'));
}

scoped_refptr<base::RefCountedMemory> CreateNotFoundResponse() {
  const char kHttpNotFound[] = "HTTP/1.1 404 Not Found\n\n";
  return base::MakeRefCounted<base::RefCountedStaticMemory>(
      kHttpNotFound, strlen(kHttpNotFound));
}

}  // namespace

WebUIDataSourceImpl::PendingRequest::PendingRequest() = default;
WebUIDataSourceImpl::PendingRequest::PendingRequest(WebUIDataSourceImpl::PendingRequest&& other) = default;
WebUIDataSourceImpl::PendingRequest& WebUIDataSourceImpl::PendingRequest::operator=(WebUIDataSourceImpl::PendingRequest&& other) = default;

WebUIDataSourceImpl::PendingRequest::~PendingRequest() {
  if (callback)
    std::move(callback).Run(CreateNotFoundResponse());
}

// Internal class to hide the fact that WebUIDataSourceImpl implements
// URLDataSource.
class WebUIDataSourceImpl::InternalDataSource : public URLDataSource {
 public:
  explicit InternalDataSource(WebUIDataSourceImpl* parent) : parent_(parent) {}

  ~InternalDataSource() override {}

  // URLDataSource implementation.
  std::string GetSource() override { return parent_->GetSource(); }
  std::string GetMimeType(const std::string& path) override {
    return parent_->GetMimeType(path);
  }
  void StartDataRequest(const GURL& url,
                        const WebContents::Getter& wc_getter,
                        URLDataSource::GotDataCallback callback) override {
    return parent_->StartDataRequest(url, wc_getter, std::move(callback));
  }
  bool ShouldReplaceExistingSource() override {
    return parent_->replace_existing_source_;
  }
  bool AllowCaching() override { return false; }
  bool ShouldAddContentSecurityPolicy() override { return parent_->add_csp_; }
  std::string GetContentSecurityPolicyScriptSrc() override {
    if (parent_->script_src_set_)
      return parent_->script_src_;
    return URLDataSource::GetContentSecurityPolicyScriptSrc();
  }
  std::string GetContentSecurityPolicyObjectSrc() override {
    if (parent_->object_src_set_)
      return parent_->object_src_;
    return URLDataSource::GetContentSecurityPolicyObjectSrc();
  }
  std::string GetContentSecurityPolicyChildSrc() override {
    if (parent_->frame_src_set_)
      return parent_->frame_src_;
    return URLDataSource::GetContentSecurityPolicyChildSrc();
  }
  std::string GetContentSecurityPolicyWorkerSrc() override {
    if (parent_->worker_src_set_)
      return parent_->worker_src_;
    return URLDataSource::GetContentSecurityPolicyWorkerSrc();
  }
  std::string GetContentSecurityPolicyFrameAncestors() override {
    std::string frame_ancestors = "";
    if (parent_->frame_ancestors_.size() == 0)
      frame_ancestors += " 'none'";
    for (const GURL& frame_ancestor : parent_->frame_ancestors_) {
      frame_ancestors += " " + frame_ancestor.spec();
    }
    return "frame-ancestors" + frame_ancestors + ";";
  }
  bool ShouldDenyXFrameOptions() override {
    return parent_->deny_xframe_options_;
  }
  bool ShouldServeMimeTypeAsContentTypeHeader() override { return true; }
  const ui::TemplateReplacements* GetReplacements() override {
    return &parent_->replacements_;
  }
  bool ShouldReplaceI18nInJS() override {
    return parent_->ShouldReplaceI18nInJS();
  }

 private:
  WebUIDataSourceImpl* parent_;
};

WebUIDataSourceImpl::WebUIDataSourceImpl(const std::string& source_name)
    : URLDataSourceImpl(source_name,
                        std::make_unique<InternalDataSource>(this)),
      source_name_(source_name),
      default_resource_(-1) {}

WebUIDataSourceImpl::~WebUIDataSourceImpl() {
}

void WebUIDataSourceImpl::AddString(base::StringPiece name,
                                    const base::string16& value) {
  // TODO(dschuyler): Share only one copy of these strings.
  localized_strings_.SetKey(name, base::Value(value));
  replacements_[name.as_string()] = base::UTF16ToUTF8(value);
}

void WebUIDataSourceImpl::AddString(base::StringPiece name,
                                    const std::string& value) {
  localized_strings_.SetKey(name, base::Value(value));
  replacements_[name.as_string()] = value;
}

void WebUIDataSourceImpl::AddLocalizedString(base::StringPiece name, int ids) {
  std::string utf8_str =
      base::UTF16ToUTF8(GetContentClient()->GetLocalizedString(ids));
  localized_strings_.SetKey(name, base::Value(utf8_str));
  replacements_[name.as_string()] = utf8_str;
}

void WebUIDataSourceImpl::AddLocalizedStrings(
    const base::DictionaryValue& localized_strings) {
  localized_strings_.MergeDictionary(&localized_strings);
  ui::TemplateReplacementsFromDictionaryValue(localized_strings,
                                              &replacements_);
}

void WebUIDataSourceImpl::AddBoolean(base::StringPiece name, bool value) {
  localized_strings_.SetBoolean(name, value);
  // TODO(dschuyler): Change name of |localized_strings_| to |load_time_data_|
  // or similar. These values haven't been found as strings for
  // localization. The boolean values are not added to |replacements_|
  // for the same reason, that they are used as flags, rather than string
  // replacements.
}

void WebUIDataSourceImpl::AddInteger(base::StringPiece name, int32_t value) {
  localized_strings_.SetInteger(name, value);
}

void WebUIDataSourceImpl::UseStringsJs() {
  use_strings_js_ = true;
}

void WebUIDataSourceImpl::AddResourcePath(base::StringPiece path,
                                          int resource_id) {
  path_to_idr_map_[path.as_string()] = resource_id;
}

void WebUIDataSourceImpl::SetDefaultResource(int resource_id) {
  default_resource_ = resource_id;
}

void WebUIDataSourceImpl::SetSharedURLLoaderFactory(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = std::move(url_loader_factory);
}

void WebUIDataSourceImpl::SetRequestFilter(
    const ShouldHandleRequestCallback& should_handle_request_callback,
    const HandleRequestCallback& handle_request_callback) {
  CHECK(!should_handle_request_callback_);
  CHECK(!filter_callback_);
  should_handle_request_callback_ = should_handle_request_callback;
  filter_callback_ = handle_request_callback;
}

void WebUIDataSourceImpl::DisableReplaceExistingSource() {
  replace_existing_source_ = false;
}

bool WebUIDataSourceImpl::IsWebUIDataSourceImpl() const {
  return true;
}

void WebUIDataSourceImpl::DisableContentSecurityPolicy() {
  add_csp_ = false;
}

void WebUIDataSourceImpl::OverrideContentSecurityPolicyScriptSrc(
    const std::string& data) {
  script_src_set_ = true;
  script_src_ = data;
}

void WebUIDataSourceImpl::OverrideContentSecurityPolicyObjectSrc(
    const std::string& data) {
  object_src_set_ = true;
  object_src_ = data;
}

void WebUIDataSourceImpl::OverrideContentSecurityPolicyChildSrc(
    const std::string& data) {
  frame_src_set_ = true;
  frame_src_ = data;
}

void WebUIDataSourceImpl::OverrideContentSecurityPolicyWorkerSrc(
    const std::string& data) {
  worker_src_set_ = true;
  worker_src_ = data;
}

void WebUIDataSourceImpl::AddFrameAncestor(const GURL& frame_ancestor) {
  // Do not allow a wildcard to be a frame ancestor or it will allow any website
  // to embed the WebUI.
  CHECK(frame_ancestor.SchemeIs(kChromeUIScheme) ||
        frame_ancestor.SchemeIs(kChromeUIUntrustedScheme));
  frame_ancestors_.insert(frame_ancestor);
}

void WebUIDataSourceImpl::DisableDenyXFrameOptions() {
  deny_xframe_options_ = false;
}

void WebUIDataSourceImpl::EnableReplaceI18nInJS() {
  should_replace_i18n_in_js_ = true;
}

void WebUIDataSourceImpl::EnsureLoadTimeDataDefaultsAdded() {
  if (!add_load_time_data_defaults_)
    return;

  add_load_time_data_defaults_ = false;
  std::string locale = GetContentClient()->browser()->GetApplicationLocale();
  base::DictionaryValue defaults;
  webui::SetLoadTimeDataDefaults(locale, &defaults);
  AddLocalizedStrings(defaults);
}

std::string WebUIDataSourceImpl::GetSource() {
  return source_name_;
}

std::string WebUIDataSourceImpl::GetMimeType(const std::string& path) const {
  // Remove the query string for to determine the mime type.
  std::string file_path = path.substr(0, path.find_first_of('?'));

  if (base::EndsWith(file_path, ".css", base::CompareCase::INSENSITIVE_ASCII))
    return "text/css";

  if (base::EndsWith(file_path, ".js", base::CompareCase::INSENSITIVE_ASCII))
    return "application/javascript";

  if (base::EndsWith(file_path, ".json", base::CompareCase::INSENSITIVE_ASCII))
    return "application/json";

  if (base::EndsWith(file_path, ".pdf", base::CompareCase::INSENSITIVE_ASCII))
    return "application/pdf";

  if (base::EndsWith(file_path, ".svg", base::CompareCase::INSENSITIVE_ASCII))
    return "image/svg+xml";

  if (base::EndsWith(file_path, ".jpg", base::CompareCase::INSENSITIVE_ASCII))
    return "image/jpeg";

  if (base::EndsWith(file_path, ".png", base::CompareCase::INSENSITIVE_ASCII))
    return "image/png";

  if (base::EndsWith(file_path, ".mp4", base::CompareCase::INSENSITIVE_ASCII))
    return "video/mp4";

  return "text/html";
}

void WebUIDataSourceImpl::StartDataRequest(
    const GURL& url,
    const WebContents::Getter& wc_getter,
    URLDataSource::GotDataCallback callback) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  if (!should_handle_request_callback_.is_null() &&
      should_handle_request_callback_.Run(path)) {
    filter_callback_.Run(path, std::move(callback));
    return;
  }

  EnsureLoadTimeDataDefaultsAdded();

  if (use_strings_js_) {
    bool from_js_module = path == "strings.m.js";
    if (from_js_module || path == "strings.js") {
      SendLocalizedStringsAsJSON(std::move(callback), from_js_module);
      return;
    }
  }

#if BUILDFLAG(ENABLE_CUSTOM_WEBUI)
  // Serve from a custom location.
  std::string custom_webui_url =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(switches::kCustomWebUIUrl);

  if (!custom_webui_url.empty() && url_loader_factory_) {
    GURL url = GURL(custom_webui_url + "/" + path);
    DCHECK(url.is_valid());
    StartCustomDataRequest(url, std::move(callback));
    return;
  }
#endif
  
  int resource_id = PathToIdrOrDefault(CleanUpPath(path));
  DCHECK_NE(resource_id, -1) << " for " << path;
  scoped_refptr<base::RefCountedMemory> response(
      GetContentClient()->GetDataResourceBytes(resource_id));
  std::move(callback).Run(response.get());
}

void WebUIDataSourceImpl::SendLocalizedStringsAsJSON(
    URLDataSource::GotDataCallback callback,
    bool from_js_module) {
  std::string template_data;
  webui::AppendJsonJS(&localized_strings_, &template_data, from_js_module);
  std::move(callback).Run(base::RefCountedString::TakeString(&template_data));
}

const base::DictionaryValue* WebUIDataSourceImpl::GetLocalizedStrings() const {
  return &localized_strings_;
}

bool WebUIDataSourceImpl::ShouldReplaceI18nInJS() const {
  return should_replace_i18n_in_js_;
}

int WebUIDataSourceImpl::PathToIdrOrDefault(const std::string& path) const {
  auto it = path_to_idr_map_.find(path);
  return it == path_to_idr_map_.end() ? default_resource_ : it->second;
}

void WebUIDataSourceImpl::StartCustomDataRequest(
    const GURL& url,
    content::URLDataSource::GotDataCallback callback) {
  if (!url.is_valid()) {
    std::move(callback).Run(CreateNotFoundResponse());
    return;
  }

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("webui_free_data_source", R"(
        semantics {
          sender: "WebUI Remote Data Request"
          description:
            "This service fetches Chromium WebUI content files from the "
            "cloud for the developer inner loop scenario. This can only happen if "
            "a URL was passed on the commandline via flag "
            "'--custom-webui-url' and BUILDFLAG(ENABLE_CUSTOM_WEBUI) was set to "
            "true. This URL overrides the default fetching from the resource pack."
          trigger:
            "When command line flag --custom-webui-url is specified "
            "and a WebUI is opened."
          data: "None"
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented, this is only used during WebUI development and "
            "is behind both a build flag and a command line switch."
        })");

  StartNetworkRequest(url, traffic_annotation, net::LOAD_DISABLE_CACHE,
                      std::move(callback));
}

void WebUIDataSourceImpl::StartNetworkRequest(
    const GURL& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    int load_flags,
    GotDataCallback callback) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = load_flags;

  auto request_iter = pending_requests_.emplace(pending_requests_.begin());
  request_iter->callback = std::move(callback);
  request_iter->loader =
      network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
  request_iter->loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&WebUIDataSourceImpl::OnLoadComplete,
                     base::Unretained(this), request_iter));
}

void WebUIDataSourceImpl::OnLoadComplete(
    std::list<PendingRequest>::iterator request_iter,
    std::unique_ptr<std::string> response_body) {
  std::move(request_iter->callback)
      .Run(response_body
               ? base::RefCountedString::TakeString(response_body.get())
               : CreateNotFoundResponse());
  pending_requests_.erase(request_iter);
}

}  // namespace content
