// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/browser/media/edge_autoplay_config.h"
#include "chrome/browser/policy/policy_test_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
<<<<<<< HEAD
#include "content/public/common/web_preferences.h"
=======
#include "content/public/test/browser_test.h"
>>>>>>> eaac5b5035fe189b6706e1637122e37134206059
#include "content/public/test/test_navigation_observer.h"
#include "media/base/media_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace policy {

namespace {
const char kAutoplayTestPageURL[] = "/media/autoplay_iframe.html";
}

class AutoplayPolicyTest : public PolicyTest {
 public:
  AutoplayPolicyTest() {
    // Start two embedded test servers on different ports. This will ensure
    // the test works correctly with cross origin iframes and site-per-process.
    embedded_test_server2()->AddDefaultHandlers(GetChromeTestDataDir());
    EXPECT_TRUE(embedded_test_server()->Start());
    EXPECT_TRUE(embedded_test_server2()->Start());
  }

  void NavigateToTestPage() {
    GURL origin = embedded_test_server()->GetURL(kAutoplayTestPageURL);
    ui_test_utils::NavigateToURL(browser(), origin);

    // Navigate the subframe to the test page but on the second origin.
    GURL origin2 = embedded_test_server2()->GetURL(kAutoplayTestPageURL);
    std::string script = base::StringPrintf(
        "setTimeout(\""
        "document.getElementById('subframe').src='%s';"
        "\",0)",
        origin2.spec().c_str());
    content::TestNavigationObserver load_observer(GetWebContents());
    EXPECT_TRUE(ExecuteScriptWithoutUserGesture(GetWebContents(), script));
    load_observer.Wait();
  }

  void NavigateToTestPageAndValidate(const bool should_autoplay) {
    NavigateToTestPage();
    EXPECT_EQ(TryAutoplay(GetMainFrame()), should_autoplay);
    EXPECT_EQ(TryAutoplay(GetChildFrame()), should_autoplay);
  }

  net::EmbeddedTestServer* embedded_test_server2() {
    return &embedded_test_server2_;
  }

  bool TryAutoplay(content::RenderFrameHost* rfh) {
    bool result = false;

    EXPECT_TRUE(content::ExecuteScriptWithoutUserGestureAndExtractBool(
        rfh, "tryPlayback();", &result));

    return result;
  }

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderFrameHost* GetMainFrame() {
    return GetWebContents()->GetMainFrame();
  }

  content::RenderFrameHost* GetChildFrame() {
    return GetWebContents()->GetAllFrames()[1];
  }

  void SetAutoplayBehaviorPref(const AutoplayBehavior behavior_value) {
    GetPrefs()->SetInteger(prefs::kEdgeAutoplayBehavior,
                           static_cast<int>(behavior_value));
    GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  }

  void SetAutoplayAllowedManagedPolicy(const bool allowed) {
    PolicyMap policies;
    SetPolicy(&policies, key::kAutoplayAllowed,
              std::make_unique<base::Value>(allowed));
    UpdateProviderPolicy(policies);

    GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
    content::WebPreferences web_prefs =
        GetWebContents()->GetRenderViewHost()->GetWebkitPreferences();

    EXPECT_EQ(GetAutoplayPrefValue(),
              static_cast<int>(allowed ? AutoplayBehavior::Allow
                                       : AutoplayBehavior::Block));

    EXPECT_EQ(web_prefs.autoplay_policy,
              allowed ? content::AutoplayPolicy::kNoUserGestureRequired
                      : content::AutoplayPolicy::kUserGestureRequired);
  }

  void SetAutoplayPolicyCommandLine(const std::string command_line_value) {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kAutoplayPolicy, command_line_value);
  }

 private:
  int GetAutoplayPrefValue() {
    return GetPrefs()->GetInteger(prefs::kEdgeAutoplayBehavior);
  }

  PrefService* GetPrefs() { return browser()->profile()->GetPrefs(); }

  // Second instance of embedded test server to provide a second test origin.
  net::EmbeddedTestServer embedded_test_server2_;
};

// The reason why we have so many of these small tests is that autoplay behavior
// is depedent on the state of the session. Navigating multiple times using the
// same test server could affect the autoplay behavior. Starting the test
// servers multiple times per test would be inappropriate.

// The following tests verify that the kAutoplayAllowed managed policy overrides
// the kAutoplayBehavior pref.

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefAllows_ThenPolicyAllows) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Allow);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenPrefAllows) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Allow);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefLimits_ThenPolicyAllows) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Limit);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenPrefLimits) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Limit);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefBlocks_ThenPolicyAllows) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Block);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenPrefBlocks) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Block);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefAllows_ThenPolicyBlocks) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Allow);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenPrefAllows) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Allow);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefLimits_ThenPolicyBlocks) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Limit);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenPrefLimits) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Limit);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PrefBlocks_ThenPolicyBlocks) {
  // Set the pref first, then the policy.
  SetAutoplayBehaviorPref(AutoplayBehavior::Block);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenPrefBlocks) {
  // Set the policy first, then the pref.
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayBehaviorPref(AutoplayBehavior::Block);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

// The following tests verify that the kAutoplayAllowed managed policy overrides
// the kAutoplayPolicy command line.

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineAllows_ThenPolicyAllows) {
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kNoUserGestureRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenCommandLineAllows) {
  SetAutoplayAllowedManagedPolicy(true /* allow by policy*/);
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kNoUserGestureRequiredPolicy);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineLimits_ThenPolicyAllows) {
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kDocumentUserActivationRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenCommandLineLimits) {
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kDocumentUserActivationRequiredPolicy);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineBlocks_ThenPolicyAllows) {
  SetAutoplayPolicyCommandLine(switches::autoplay::kUserGestureRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyAllows_ThenCommandLineBlocks) {
  SetAutoplayPolicyCommandLine(switches::autoplay::kUserGestureRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(true /* allow by policy */);
  NavigateToTestPageAndValidate(true /* should autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineAllows_ThenPolicyBlocks) {
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kNoUserGestureRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenCommandLineAllows) {
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kNoUserGestureRequiredPolicy);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineLimits_ThenPolicyBlocks) {
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kDocumentUserActivationRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenCommandLineLimits) {
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayPolicyCommandLine(
      switches::autoplay::kDocumentUserActivationRequiredPolicy);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, CommandLineBlocks_ThenPolicyBlocks) {
  SetAutoplayPolicyCommandLine(switches::autoplay::kUserGestureRequiredPolicy);
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest, PolicyBlocks_ThenCommandLineBlocks) {
  SetAutoplayAllowedManagedPolicy(false /* block by policy */);
  SetAutoplayPolicyCommandLine(switches::autoplay::kUserGestureRequiredPolicy);
  NavigateToTestPageAndValidate(false /* should not autoplay */);
}

// ADO 23251279: we are missing tests for changing the policy on the fly due to
// media session propagation.

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayAllowedByPolicy,
                                               20492501)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Update policy to allow autoplay.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayAllowed,
            std::make_unique<base::Value>(true));
  UpdateProviderPolicy(policies);

  // Check that autoplay was allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_TRUE(TryAutoplay(GetMainFrame()));
  EXPECT_TRUE(TryAutoplay(GetChildFrame()));
}

// The following tests are disabled until autoplay allow and deny lists are
// implemented (ADO 23251107). We will add additional tests for deny lists.

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayWhitelist_Allowed,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with our origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value(embedded_test_server()->GetURL("/").spec()));

  // Update policy to allow autoplay for our test origin.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_TRUE(TryAutoplay(GetMainFrame()));
  EXPECT_TRUE(TryAutoplay(GetChildFrame()));
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayWhitelist_PatternAllowed,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with our origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value("127.0.0.1:*"));

  // Update policy to allow autoplay for our test origin.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_TRUE(TryAutoplay(GetMainFrame()));
  EXPECT_TRUE(TryAutoplay(GetChildFrame()));
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayWhitelist_Missing,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with a random origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value("https://www.example.com"));

  // Update policy to allow autoplay for a random origin.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was not allowed.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayDeniedByPolicy,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Update policy to forbid autoplay.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayAllowed,
            std::make_unique<base::Value>(false));
  UpdateProviderPolicy(policies);

  // Check that autoplay was not allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with a random origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value("https://www.example.com"));

  // Update policy to allow autoplay for a random origin.
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was not allowed.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayDeniedAllowedWithURL,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Update policy to forbid autoplay.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayAllowed,
            std::make_unique<base::Value>(false));
  UpdateProviderPolicy(policies);

  // Check that autoplay was not allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with our test origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value(embedded_test_server()->GetURL("/").spec()));

  // Update policy to allow autoplay for our test origin.
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_TRUE(TryAutoplay(GetMainFrame()));
  EXPECT_TRUE(TryAutoplay(GetChildFrame()));
}

IN_PROC_BROWSER_TEST_F(AutoplayPolicyTest,
                       EDGE_DISABLED_PERMANENT(AutoplayAllowedGlobalAndURL,
                                               23251107)) {
  NavigateToTestPage();

  // Check that autoplay was not allowed.
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Update policy to forbid autoplay.
  PolicyMap policies;
  SetPolicy(&policies, key::kAutoplayAllowed,
            std::make_unique<base::Value>(false));
  UpdateProviderPolicy(policies);

  // Check that autoplay was not allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_FALSE(TryAutoplay(GetMainFrame()));
  EXPECT_FALSE(TryAutoplay(GetChildFrame()));

  // Create a test whitelist with our test origin.
  std::vector<base::Value> whitelist;
  whitelist.push_back(base::Value(embedded_test_server()->GetURL("/").spec()));

  // Update policy to allow autoplay for our test origin.
  SetPolicy(&policies, key::kAutoplayWhitelist,
            std::make_unique<base::ListValue>(whitelist));
  UpdateProviderPolicy(policies);

  // Check that autoplay was allowed by policy.
  GetWebContents()->GetRenderViewHost()->OnWebkitPreferencesChanged();
  EXPECT_TRUE(TryAutoplay(GetMainFrame()));
  EXPECT_TRUE(TryAutoplay(GetChildFrame()));
}

}  // namespace policy
