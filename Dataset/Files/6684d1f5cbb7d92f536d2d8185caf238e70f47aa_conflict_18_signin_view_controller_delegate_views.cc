// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/profiles/signin_view_controller_delegate_views.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/signin/reauth_result.h"
#include "chrome/browser/signin/reauth_tab_helper.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/signin_view_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/webui/signin/sync_confirmation_ui.h"
#include "chrome/common/url_constants.h"
<<<<<<< HEAD
#include "chrome/grit/generated_resources.h"
=======
#include "chrome/common/webui_url_constants.h"
>>>>>>> 32ea0b0591779cbd45618dc7e02bb9bd635354e9
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "google_apis/gaia/core_account_id.h"
#include "google_apis/gaia/gaia_urls.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_types.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"

// Edge only headers.
#include "chrome/browser/ui/webui/edge_signin/edge_personalized_data_collection_request_ui.h"
#include "chrome/browser/ui/webui/edge_signin/edge_profile_reuse_warning_ui.h"
#include "chrome/browser/ui/webui/edge_signin/edge_signin_error_ui.h"
#include "chrome/browser/ui/webui/edge_signin/edge_sync_confirmation_ui.h"
#include "components/signin/core/browser/edge_identity_features.h"

namespace {

const int kModalDialogWidth = 400;
// Keeping chromium only constants to avoid upstream conflicts.
const int kSigninErrorDialogHeight = 210;
const int kSyncConfirmationDialogWidth = 512;
<<<<<<< HEAD

const int kEdgeSyncConfirmationDialogWidth = 400;
const int kEdgeSyncConfirmationDialogHeight = 298;

const int kReauthDialogWidth = 625;
const int kReauthDialogHeight = 625;
=======
const int kSyncConfirmationDialogHeight = 487;
const int kSigninErrorDialogHeight = 164;
const int kReauthDialogWidth = 540;
const int kReauthDialogHeight = 520;
>>>>>>> 32ea0b0591779cbd45618dc7e02bb9bd635354e9

int GetSyncConfirmationDialogPreferredHeight(Profile* profile) {
  return kEdgeSyncConfirmationDialogHeight;
}

void CompleteReauthFlow(Browser* browser,
                        base::OnceCallback<void(signin::ReauthResult)> callback,
                        signin::ReauthResult result) {
  browser->signin_view_controller()->CloseModalSignin();
  std::move(callback).Run(result);
}
<<<<<<< HEAD

// The view displaying a fake modal reauth dialog. The fake dialog has OK and
// CANCEL buttons. The class invokes a callback when the user clicks one of the
// buttons or dismisses the dialog.
// TODO(crbug.com/1045515): remove this class once the real implementation is
// done.
class SigninViewControllerFakeReauthDelegateView
    : public views::DialogDelegateView,
      public SigninViewControllerDelegate {
 public:
  SigninViewControllerFakeReauthDelegateView(
      Browser* browser,
      base::OnceCallback<void(signin::ReauthResult)> reauth_callback);

  // SigninViewControllerDelegate:
  void CloseModalSignin() override;
  void ResizeNativeView(int height) override;
  void ResizeNativeView(base::Optional<int> height,
                        base::Optional<int> width) override;
  content::WebContents* GetWebContents() override;
  void OverrideAccessibleWindowTitle(
      const base::string16& accessible_title) override;

  // views::DialogDelegateView:
  void DeleteDelegate() override;
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;

 private:
  void DisplayModal();

  void OnAccept();
  void OnCancel();
  void OnClose();

  Browser* browser_;
  base::OnceCallback<void(signin::ReauthResult)> reauth_callback_;
  views::Widget* widget_ = nullptr;
};

SigninViewControllerFakeReauthDelegateView::
    SigninViewControllerFakeReauthDelegateView(
        Browser* browser,
        base::OnceCallback<void(signin::ReauthResult)> reauth_callback)
    : browser_(browser), reauth_callback_(std::move(reauth_callback)) {
  SetButtons(ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL);
  SetAcceptCallback(
      base::BindOnce(&SigninViewControllerFakeReauthDelegateView::OnAccept,
                     base::Unretained(this)));
  SetCancelCallback(
      base::BindOnce(&SigninViewControllerFakeReauthDelegateView::OnCancel,
                     base::Unretained(this)));
  SetCloseCallback(
      base::BindOnce(&SigninViewControllerFakeReauthDelegateView::OnClose,
                     base::Unretained(this)));
  DisplayModal();
}

void SigninViewControllerFakeReauthDelegateView::CloseModalSignin() {
  NotifyModalSigninClosed();
  if (widget_)
    widget_->Close();
}

void SigninViewControllerFakeReauthDelegateView::ResizeNativeView(int height) {}

void SigninViewControllerFakeReauthDelegateView::ResizeNativeView(
    base::Optional<int> height, base::Optional<int> width) {}

content::WebContents*
SigninViewControllerFakeReauthDelegateView::GetWebContents() {
  return nullptr;
}

void SigninViewControllerFakeReauthDelegateView::OverrideAccessibleWindowTitle(
    const base::string16& title) {}

void SigninViewControllerFakeReauthDelegateView::DeleteDelegate() {
  NotifyModalSigninClosed();
  delete this;
}

ui::ModalType SigninViewControllerFakeReauthDelegateView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 SigninViewControllerFakeReauthDelegateView::GetWindowTitle()
    const {
  return base::ASCIIToUTF16("Reauth fake dialog");
}

void SigninViewControllerFakeReauthDelegateView::DisplayModal() {
  content::WebContents* host_web_contents =
      browser_->tab_strip_model()->GetActiveWebContents();

  if (!host_web_contents) {
    OnClose();
    return;
  }

  widget_ = constrained_window::CreateBrowserModalDialogViews(
      this, host_web_contents->GetTopLevelNativeWindow());
  widget_->Show();
}

void SigninViewControllerFakeReauthDelegateView::OnAccept() {
  if (reauth_callback_)
    std::move(reauth_callback_).Run(signin::ReauthResult::kSuccess);
}

void SigninViewControllerFakeReauthDelegateView::OnCancel() {
  if (reauth_callback_)
    std::move(reauth_callback_).Run(signin::ReauthResult::kCancelled);
}

void SigninViewControllerFakeReauthDelegateView::OnClose() {
  if (reauth_callback_)
    std::move(reauth_callback_).Run(signin::ReauthResult::kCancelled);
}

=======
>>>>>>> 32ea0b0591779cbd45618dc7e02bb9bd635354e9
}  // namespace

// static
std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateSyncConfirmationWebView(
    Browser* browser) {
  return CreateDialogWebView(
      browser, chrome::kChromeUISyncConfirmationURL,
      GetSyncConfirmationDialogPreferredHeight(browser->profile()),
      kSyncConfirmationDialogWidth);
}

// static
std::unique_ptr<views::WebView>
<<<<<<< HEAD
SigninViewControllerDelegateViews::CreateReauthWebView(
=======
SigninViewControllerDelegateViews::CreateSigninErrorWebView(Browser* browser) {
  return CreateDialogWebView(browser, chrome::kChromeUISigninErrorURL,
                             kSigninErrorDialogHeight, base::nullopt);
}

// static
std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateReauthConfirmationWebView(
    Browser* browser) {
  return CreateDialogWebView(browser, chrome::kChromeUISigninReauthURL,
                             kReauthDialogHeight, kReauthDialogWidth);
}

// static
std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateGaiaReauthWebView(
>>>>>>> 32ea0b0591779cbd45618dc7e02bb9bd635354e9
    Browser* browser,
    base::OnceCallback<void(signin::ReauthResult)> reauth_callback) {
  auto web_view = std::make_unique<views::WebView>(browser->profile());
  const GURL& reauth_url = GaiaUrls::GetInstance()->reauth_url();
  web_view->LoadInitialURL(reauth_url);

  gfx::Size max_size = browser->window()
                           ->GetWebContentsModalDialogHost()
                           ->GetMaximumDialogSize();
  web_view->SetPreferredSize(
      gfx::Size(std::min(kReauthDialogWidth, max_size.width()),
                std::min(kReauthDialogHeight, max_size.height())));

  content::WebContents* web_contents = web_view->GetWebContents();

  signin::ReauthTabHelper::CreateForWebContents(
      web_contents, reauth_url, true,
      base::BindOnce(&CompleteReauthFlow, base::Unretained(browser),
                     std::move(reauth_callback)));

  return web_view;
}

views::View* SigninViewControllerDelegateViews::GetContentsView() {
  return content_view_;
}

views::Widget* SigninViewControllerDelegateViews::GetWidget() {
  return content_view_->GetWidget();
}

const views::Widget* SigninViewControllerDelegateViews::GetWidget() const {
  return content_view_->GetWidget();
}

void SigninViewControllerDelegateViews::DeleteDelegate() {
  NotifyModalSigninClosed();
  delete this;
}

ui::ModalType SigninViewControllerDelegateViews::GetModalType() const {
  return dialog_modal_type_;
}

bool SigninViewControllerDelegateViews::ShouldShowCloseButton() const {
  return should_show_close_button_;
}

base::string16 SigninViewControllerDelegateViews::GetAccessibleWindowTitle() const {
  if (accessible_title_.empty())
    return views::DialogDelegateView::GetAccessibleWindowTitle();

  return accessible_title_;
}

void SigninViewControllerDelegateViews::OverrideAccessibleWindowTitle(
    const base::string16& accessible_title) {
  accessible_title_ = accessible_title;
}

void SigninViewControllerDelegateViews::CloseModalSignin() {
  signin::ReauthTabHelper* reauth_tab_helper =
      signin::ReauthTabHelper::FromWebContents(web_contents_);
  if (reauth_tab_helper) {
    reauth_tab_helper->CompleteReauth(signin::ReauthResult::kCancelled);
  }

  NotifyModalSigninClosed();
  if (modal_signin_widget_)
    modal_signin_widget_->Close();
}

void SigninViewControllerDelegateViews::ResizeNativeView(int height) {
  ResizeNativeView(height, {});
}

void SigninViewControllerDelegateViews::ResizeNativeView(
    base::Optional<int> height,
    base::Optional<int> width) {
  auto max_size = browser()
                      ->window()
                      ->GetWebContentsModalDialogHost()
                      ->GetMaximumDialogSize();
  auto current_size = content_view_->GetPreferredSize();
  content_view_->SetPreferredSize(gfx::Size(
      std::min(width.value_or(current_size.width()), max_size.width()),
      std::min(height.value_or(current_size.height()), max_size.height())));

  if (!modal_signin_widget_) {
    // The modal wasn't displayed yet so just show it with the already resized
    // view.
    DisplayModal();
  }
}

content::WebContents* SigninViewControllerDelegateViews::GetWebContents() {
  return web_contents_;
}

bool SigninViewControllerDelegateViews::HandleContextMenu(
    content::RenderFrameHost* render_frame_host,
    const content::ContextMenuParams& params) {
  // Discard the context menu
  return true;
}

bool SigninViewControllerDelegateViews::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // If this is a MODAL_TYPE_CHILD, then GetFocusManager() will return the focus
  // manager of the parent window, which has registered accelerators, and the
  // accelerators will fire. If this is a MODAL_TYPE_WINDOW, then this will have
  // no effect, since no accelerators have been registered for this standalone
  // window.
  return unhandled_keyboard_event_handler_.HandleKeyboardEvent(
      event, GetFocusManager());
}

web_modal::WebContentsModalDialogHost*
SigninViewControllerDelegateViews::GetWebContentsModalDialogHost() {
  return browser()->window()->GetWebContentsModalDialogHost();
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateConfirmationWebView(
    Browser* browser,
    bool show_sync_confirmation,
    bool show_signin_confirmation,
    bool is_sync_disabled_by_admin) {
  return CreateConfirmationDialogWebView(
      browser, chrome::kChromeUISyncConfirmationURL,
      GetSyncConfirmationDialogPreferredHeight(browser->profile()),
      kEdgeSyncConfirmationDialogWidth,
      show_sync_confirmation, show_signin_confirmation, is_sync_disabled_by_admin);
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateSigninErrorWebView(Browser* browser) {
  return CreateDialogWebView(browser, chrome::kChromeUISigninErrorURL,
                             kSigninErrorDialogHeight, base::nullopt);
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateEdgeProfileReuseWarningWebView(
    Browser* browser,
    const std::string& last_user_name,
    const std::string& current_user_name,
    ProfileReuseCallback response_callback) {
  std::unique_ptr<views::WebView> web_view =
      std::make_unique<views::WebView>(browser->profile());
  web_view->LoadInitialURL(GURL(chrome::kChromeUIProfileReuseWarningURL));

  // To record metrics using javascript, extensions are needed.
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_view->GetWebContents());

  // We can cast to our EdgeProfileReuseWarningUI here.
  EdgeProfileReuseWarningUI* web_dialog_ui =
      static_cast<EdgeProfileReuseWarningUI*>(
          web_view->GetWebContents()->GetWebUI()->GetController());
  web_dialog_ui->InitializeMessageHandler(
      browser, last_user_name, current_user_name, std::move(response_callback));

  const int max_height = browser->window()
                             ->GetWebContentsModalDialogHost()
                             ->GetMaximumDialogSize()
                             .height();
  web_view->SetPreferredSize(gfx::Size(
      kModalDialogWidth, std::min(kSigninErrorDialogHeight, max_height)));
  return web_view;
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateEdgeSigninErrorWebView(
    Browser* browser,
    const EdgeSigninErrorUIParams& ui_params) {
  std::unique_ptr<views::WebView> web_view =
      std::make_unique<views::WebView>(browser->profile());
  web_view->LoadInitialURL(GURL(chrome::kChromeUISigninErrorURL));
  // To record metrics using javascript, extensions are needed.
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_view->GetWebContents());

  // We can cast to our EdgeSigninErrorUI here.
  EdgeSigninErrorUI* web_dialog_ui = static_cast<EdgeSigninErrorUI*>(
      web_view->GetWebContents()->GetWebUI()->GetController());
  web_dialog_ui->InitializeMessageHandler(browser, ui_params);

  const int max_height = browser->window()
                       ->GetWebContentsModalDialogHost()
                       ->GetMaximumDialogSize()
                       .height();
  web_view->SetPreferredSize(gfx::Size(
      kModalDialogWidth, std::min(kSigninErrorDialogHeight, max_height)));
  return web_view;
}

SigninViewControllerDelegateViews::SigninViewControllerDelegateViews(
    std::unique_ptr<views::WebView> content_view,
    Browser* browser,
    ui::ModalType dialog_modal_type,
    bool wait_for_size,
    bool should_show_close_button)
    : web_contents_(content_view->GetWebContents()),
      browser_(browser),
      content_view_(content_view.release()),
      modal_signin_widget_(nullptr),
      dialog_modal_type_(dialog_modal_type),
      should_show_close_button_(should_show_close_button) {
  DCHECK(web_contents_);
  DCHECK(browser_);
  DCHECK(browser_->tab_strip_model()->GetActiveWebContents())
      << "A tab must be active to present the sign-in modal dialog.";

  SetButtons(ui::DIALOG_BUTTON_NONE);

  web_contents_->SetDelegate(this);

  DCHECK(dialog_modal_type == ui::MODAL_TYPE_CHILD ||
         dialog_modal_type == ui::MODAL_TYPE_WINDOW)
      << "Unsupported dialog modal type " << dialog_modal_type;

  if (!wait_for_size)
    DisplayModal();
}

SigninViewControllerDelegateViews::~SigninViewControllerDelegateViews() =
    default;

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreatePersonalizedDataRequestWebView(
    Browser* browser) {
  return CreatePersonalizedDataRequestDialogWebView(
      browser, chrome::kEdgeUIPersonalizedDataRequestURL,
      GetSyncConfirmationDialogPreferredHeight(browser->profile()),
      kEdgeSyncConfirmationDialogWidth);
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateDialogWebView(
    Browser* browser,
    const std::string& url,
    int dialog_height,
    base::Optional<int> opt_width) {
  int dialog_width = opt_width.value_or(kModalDialogWidth);
  views::WebView* web_view = new views::WebView(browser->profile());
  web_view->LoadInitialURL(GURL(url));
  // To record metrics using javascript, extensions are needed.
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_view->GetWebContents());

  SigninWebDialogUI* web_dialog_ui = static_cast<SigninWebDialogUI*>(
      web_view->GetWebContents()->GetWebUI()->GetController());
  web_dialog_ui->InitializeMessageHandlerWithBrowser(browser);

  int max_height = browser->window()
                       ->GetWebContentsModalDialogHost()
                       ->GetMaximumDialogSize()
                       .height();
  web_view->SetPreferredSize(
      gfx::Size(dialog_width, std::min(dialog_height, max_height)));

  return std::unique_ptr<views::WebView>(web_view);
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreateConfirmationDialogWebView(
    Browser* browser,
    const std::string& url,
    int dialog_height,
    base::Optional<int> opt_width,
    bool show_sync_confirmation,
    bool show_signin_confirmation,
    bool is_sync_turned_off_by_admin) {
  int dialog_width = opt_width.value_or(kModalDialogWidth);
  std::unique_ptr<views::WebView> web_view =
      std::make_unique<views::WebView>(browser->profile());
  const GURL dialog_url(url);
  web_view->LoadInitialURL(dialog_url);

  // Reset the zoom level to 0 for this webview
  content::WebContents* dialog_contents = web_view->GetWebContents();
  content::HostZoomMap::Get(dialog_contents->GetSiteInstance())
      ->SetZoomLevelForHostAndScheme(dialog_url.scheme(), dialog_url.host(), 0);

  // To record metrics using javascript, extensions are needed.
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_view->GetWebContents());

  SigninWebDialogUI* web_dialog_ui = static_cast<SigninWebDialogUI*>(
      web_view->GetWebContents()->GetWebUI()->GetController());

  EdgeSyncConfirmationUI* confirmation_dialog =
      static_cast<EdgeSyncConfirmationUI*>(web_dialog_ui);
  if (nullptr != confirmation_dialog) {
    confirmation_dialog->SetConfirmationOptions(show_sync_confirmation,
                                                show_signin_confirmation,
                                                is_sync_turned_off_by_admin);
  }

  web_dialog_ui->InitializeMessageHandlerWithBrowser(browser);

  int max_height = browser->window()
                       ->GetWebContentsModalDialogHost()
                       ->GetMaximumDialogSize()
                       .height();
  web_view->SetPreferredSize(
      gfx::Size(dialog_width, std::min(dialog_height, max_height)));

  return web_view;
}

std::unique_ptr<views::WebView>
SigninViewControllerDelegateViews::CreatePersonalizedDataRequestDialogWebView(
    Browser* browser,
    const std::string& url,
    int dialog_height,
    base::Optional<int> opt_width) {
  int dialog_width = opt_width.value_or(kModalDialogWidth);
  std::unique_ptr<views::WebView> web_view =
      std::make_unique<views::WebView>(browser->profile());
  const GURL dialog_url(url);
  web_view->LoadInitialURL(dialog_url);

  // Reset the zoom level to 0 for this webview
  content::WebContents* dialog_contents = web_view->GetWebContents();
  content::HostZoomMap::Get(dialog_contents->GetSiteInstance())
      ->SetZoomLevelForHostAndScheme(dialog_url.scheme(), dialog_url.host(), 0);

  // To record metrics using javascript, extensions are needed.
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_view->GetWebContents());

  SigninWebDialogUI* web_dialog_ui = static_cast<SigninWebDialogUI*>(
      web_view->GetWebContents()->GetWebUI()->GetController());

  web_dialog_ui->InitializeMessageHandlerWithBrowser(browser);

  int max_height = browser->window()
                       ->GetWebContentsModalDialogHost()
                       ->GetMaximumDialogSize()
                       .height();
  web_view->SetPreferredSize(
      gfx::Size(dialog_width, std::min(dialog_height, max_height)));

  return web_view;
}

void SigninViewControllerDelegateViews::DisplayModal() {
  DCHECK(!modal_signin_widget_);

  content::WebContents* host_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Avoid displaying the sign-in modal view if there are no active web
  // contents. This happens if the user closes the browser window before this
  // dialog has a chance to be displayed.
  if (!host_web_contents)
    return;

  gfx::NativeWindow window = host_web_contents->GetTopLevelNativeWindow();
  switch (dialog_modal_type_) {
    case ui::MODAL_TYPE_WINDOW:
      modal_signin_widget_ =
          constrained_window::CreateBrowserModalDialogViews(this, window);
      modal_signin_widget_->Show();
      break;
    case ui::MODAL_TYPE_CHILD:
      modal_signin_widget_ = constrained_window::ShowWebModalDialogViews(
          this, browser()->tab_strip_model()->GetActiveWebContents());
      break;
    default:
      NOTREACHED() << "Unsupported dialog modal type " << dialog_modal_type_;
  }
  if (should_show_close_button_) {
    GetBubbleFrameView()->SetBubbleBorder(std::make_unique<views::BubbleBorder>(
        views::BubbleBorder::NONE, views::BubbleBorder::BIG_SHADOW,
        SK_ColorWHITE));
  }

  content_view_->RequestFocus();
}

// --------------------------------------------------------------------
// SigninViewControllerDelegate static methods
// --------------------------------------------------------------------

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateConfirmationDelegate(
    Browser* browser,
    bool show_sync_confirmation,
    bool show_signin_confirmation,
    bool is_sync_disabled_by_admin) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateConfirmationWebView(
          browser, show_sync_confirmation, show_signin_confirmation, is_sync_disabled_by_admin),
      browser, ui::MODAL_TYPE_WINDOW, true, false);
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateSigninErrorDelegate(Browser* browser) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateSigninErrorWebView(browser),
      browser, ui::MODAL_TYPE_WINDOW, true, false);
}

SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateEdgeSigninErrorDelegate(
    Browser* browser,
    const EdgeSigninErrorUIParams& ui_params) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateEdgeSigninErrorWebView(
          browser, ui_params),
      browser, ui::MODAL_TYPE_WINDOW, true, false);
}

SigninViewControllerDelegate*
SigninViewControllerDelegate::CreatePersonalizedDataRequestDelegate(
    Browser* browser) {
  SigninViewControllerDelegate* delegate = new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreatePersonalizedDataRequestWebView(
          browser),
      browser, ui::MODAL_TYPE_WINDOW, true, false);
  delegate->OverrideAccessibleWindowTitle(l10n_util::GetStringUTF16(
      IDS_EDGE_PERSONALIZED_DATA_COLLECTION_DIALOG_NAME));
  return delegate;
}

SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateEdgeProfileReuseWarningDelegate(
    Browser* browser,
    const std::string& last_user_name,
    const std::string& current_user_name,
    ProfileReuseCallback response_callback) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateEdgeProfileReuseWarningWebView(
          browser, last_user_name, current_user_name,
          std::move(response_callback)),
      browser, ui::MODAL_TYPE_WINDOW, true, false);
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateGaiaReauthDelegate(
    Browser* browser,
    const CoreAccountId& account_id,
    base::OnceCallback<void(signin::ReauthResult)> reauth_callback) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateGaiaReauthWebView(
          browser, std::move(reauth_callback)),
      browser, ui::MODAL_TYPE_CHILD, false, true);
}

// static
SigninViewControllerDelegate*
SigninViewControllerDelegate::CreateReauthConfirmationDelegate(
    Browser* browser,
    const CoreAccountId& account_id) {
  return new SigninViewControllerDelegateViews(
      SigninViewControllerDelegateViews::CreateReauthConfirmationWebView(
          browser),
      browser, ui::MODAL_TYPE_CHILD, false, true);
}
