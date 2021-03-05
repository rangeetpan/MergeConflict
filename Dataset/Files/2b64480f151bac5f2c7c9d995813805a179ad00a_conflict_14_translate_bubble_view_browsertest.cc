// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(MICROSOFT_EDGE_BUILD)
#include "chrome/browser/ui/views/translate/edge_translate_bubble_view.h"
#else
#include "chrome/browser/ui/views/translate/translate_bubble_view.h"
#endif

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/browser/translate/translate_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_container.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_controller.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/translate/core/browser/translate_manager.h"
<<<<<<< HEAD
#include "components/translate/core/common/edge_translate_features.h"
=======
#include "content/public/test/browser_test.h"
>>>>>>> eaac5b5035fe189b6706e1637122e37134206059
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/menu_button.h"

namespace translate {

class TranslateBubbleViewBrowserTest : public InProcessBrowserTest {
 public:
  TranslateBubbleViewBrowserTest() {}
  ~TranslateBubbleViewBrowserTest() override {}

  void SetUp() override {
#if defined(MICROSOFT_EDGE_BUILD)
    // Enable edge specific reading view key
    scoped_feature_list_.InitAndEnableFeature(features::edge::kEdgeTranslate);
#endif
    set_open_about_blank_on_browser_launch(true);
    TranslateManager::SetIgnoreMissingKeyForTesting(true);
    InProcessBrowserTest::SetUp();
  }

 protected:
  void NavigateAndWaitForLanguageDetection(const GURL& url,
                                           const std::string& expected_lang) {
    ui_test_utils::NavigateToURL(browser(), url);

    while (expected_lang !=
           ChromeTranslateClient::FromWebContents(
               browser()->tab_strip_model()->GetActiveWebContents())
               ->GetLanguageState()
               .original_language()) {
      CreateTranslateWaiter(
          browser()->tab_strip_model()->GetActiveWebContents(),
          TranslateWaiter::WaitEvent::kLanguageDetermined)
          ->Wait();
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TranslateBubbleViewBrowserTest);
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseBrowserWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Show a French page and wait until the bubble is shown.
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());

  // Close the window without translating. Spin the runloop to allow
  // asynchronous window closure to happen.
  chrome::CloseWindow(browser());
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());
}

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseLastTabWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Show a French page and wait until the bubble is shown.
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());

  // Close the tab without translating. Spin the runloop to allow asynchronous
  // window closure to happen.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  chrome::CloseWebContents(
      browser(), browser()->tab_strip_model()->GetActiveWebContents(), false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());
}

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       CloseAnotherTabWithoutTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  int active_index = browser()->tab_strip_model()->active_index();

  // Open another tab to load a French page on background.
  int french_index = active_index + 1;
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  chrome::AddTabAt(browser(), french_url, french_index, false);
  EXPECT_EQ(active_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetWebContentsAt(french_index);

  // The bubble is not shown because the tab is not activated.
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Close the French page tab immediately.
  chrome::CloseWebContents(browser(), web_contents, false);
  EXPECT_EQ(active_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Close the last tab.
  chrome::CloseWebContents(browser(),
                           browser()->tab_strip_model()->GetActiveWebContents(),
                           false);
}

#if defined(MICROSOFT_EDGE_BUILD)
IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
    OnPageActionIconClick) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  // Show a French page and wait until the bubble is shown.
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());

  // Get the page action icon for translate
  BrowserWindow* browser_window = browser()->window();
  EXPECT_TRUE(browser_window);
  BrowserView* browser_view = static_cast<BrowserView*>(browser_window);
  EXPECT_TRUE(browser_view);

  PageActionIconView* translate_icon = browser_view
      ->toolbar_button_provider()
      ->GetPageActionIconView(PageActionIconType::kTranslate);

  browser_view->UpdateTranslateIcon(
      translate::TranslateStep::TRANSLATE_STEP_BEFORE_TRANSLATE);
  EXPECT_NE(translate_icon->state(), views::Button::STATE_DISABLED);

  browser_view->UpdateTranslateIcon(
      translate::TranslateStep::TRANSLATE_STEP_TRANSLATING);
  EXPECT_EQ(translate_icon->state(), views::Button::STATE_DISABLED);

  browser_view->UpdateTranslateIcon(
      translate::TranslateStep::TRANSLATE_STEP_AFTER_TRANSLATE);
  EXPECT_NE(translate_icon->state(), views::Button::STATE_DISABLED);

  // Close the French page tab immediately.
  chrome::CloseWebContents(browser(), web_contents, false);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());
}

IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       SwitchTabsWhileTranslating) {
  EXPECT_FALSE(TranslateBubbleView::GetCurrentBubble());

  GURL french_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath(FILE_PATH_LITERAL("french_page.html")));

  int first_french_index = browser()->tab_strip_model()->active_index();

  // Show a French page and wait until the bubble is shown.
  NavigateAndWaitForLanguageDetection(french_url, "fr");
  EXPECT_TRUE(TranslateBubbleView::GetCurrentBubble());

  BrowserWindow* browser_window = browser()->window();
  EXPECT_TRUE(browser_window);
  BrowserView* browser_view = static_cast<BrowserView*>(browser_window);
  EXPECT_TRUE(browser_view);

  PageActionIconView* translate_icon = browser_view
      ->toolbar_button_provider()
      ->GetPageActionIconView(PageActionIconType::kTranslate);

  // Open French page in another tab.
  int second_french_index = first_french_index + 1;
  chrome::AddTabAt(browser(), french_url, second_french_index, false);
  NavigateAndWaitForLanguageDetection(french_url, "fr");

  EXPECT_EQ(first_french_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // Switch to second tab
  browser()->tab_strip_model()->ActivateTabAt(
      second_french_index, TabStripModel::GestureType::kMouse);

  // Update the icon in second french tab by
  // setting the step to TRANSLATING
  EXPECT_NE(translate_icon->state(), views::Button::STATE_DISABLED);
  browser_view->UpdateTranslateIcon(
      translate::TranslateStep::TRANSLATE_STEP_TRANSLATING);
  EXPECT_EQ(translate_icon->state(), views::Button::STATE_DISABLED);

  // Switch again to first tab
  browser()->tab_strip_model()->ActivateTabAt(
      first_french_index, TabStripModel::GestureType::kMouse);
  EXPECT_NE(translate_icon->state(), views::Button::STATE_DISABLED);

  // Close the tabs.
  chrome::CloseWebContents(browser(),
      browser()->tab_strip_model()->GetActiveWebContents(),
      false);
}

// Tests previous translation step is set correctly
IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       PreviousTranslationStep) {
  ChromeTranslateClient* client = ChromeTranslateClient::FromWebContents(
      browser()->tab_strip_model()->GetActiveWebContents());

  client->ShowTranslateUI(
      translate::TranslateStep::TRANSLATE_STEP_BEFORE_TRANSLATE, "fr", "en",
      translate::TranslateErrors::NONE, false);
  EXPECT_EQ(client->GetPreviousTranslateStep(),
            translate::TranslateStep::TRANSLATE_STEP_BEFORE_TRANSLATE);

  client->ShowTranslateUI(translate::TranslateStep::TRANSLATE_STEP_TRANSLATING,
                          "fr", "en", translate::TranslateErrors::NONE, false);
  EXPECT_EQ(client->GetPreviousTranslateStep(),
            translate::TranslateStep::TRANSLATE_STEP_TRANSLATING);

  client->ShowTranslateUI(
      translate::TranslateStep::TRANSLATE_STEP_AFTER_TRANSLATE, "fr", "en",
      translate::TranslateErrors::NONE, false);
  EXPECT_EQ(client->GetPreviousTranslateStep(),
            translate::TranslateStep::TRANSLATE_STEP_AFTER_TRANSLATE);

  client->ShowTranslateUI(translate::TranslateStep::TRANSLATE_STEP_TRANSLATING,
                          "fr", "en",
                          translate::TranslateErrors::TRANSLATION_ERROR, false);
  EXPECT_EQ(client->GetPreviousTranslateStep(),
            translate::TranslateStep::TRANSLATE_STEP_TRANSLATE_ERROR);
}

// Tests translateEnabled is set true after ShowTranslateUI is called
IN_PROC_BROWSER_TEST_F(TranslateBubbleViewBrowserTest,
                       TranslateEnabledAfterShowTranslateUI) {
  ChromeTranslateClient* client = ChromeTranslateClient::FromWebContents(
      browser()->tab_strip_model()->GetActiveWebContents());

  client->GetLanguageState().SetTranslateEnabled(false);

  client->ShowTranslateUI(
      translate::TranslateStep::TRANSLATE_STEP_TRANSLATING, "fr", "en",
      translate::TranslateErrors::NONE, false);

  // After ShowTranslateUI is called then translate_enabled should be true.
  EXPECT_TRUE(client->GetLanguageState().translate_enabled());
}

#endif

}  // namespace translate
