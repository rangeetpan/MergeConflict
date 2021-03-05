// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the definitions of the installer functions that build
// the WorkItemList used to install the application.

#include "chrome/installer/setup/install_worker.h"

#include <windows.h>
#include <winioctl.h>

#include <atlsecurity.h>
#include <oaidl.h>
#include <sddl.h>
#include <shlobj.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>
#include <wrl/client.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "chrome/install_static/buildflags.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
<<<<<<< HEAD
#include "chrome/installer/setup/edge_install_worker.h"
#include "chrome/installer/setup/install.h"
=======
#include "chrome/installer/setup/install_params.h"
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/setup/update_active_setup_version_work_item.h"
#include "chrome/installer/util/callback_work_item.h"
#include "chrome/installer/util/conditional_work_item_list.h"
#include "chrome/installer/util/create_reg_key_work_item.h"
#include "chrome/installer/util/firewall_manager_win.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/install_service_work_item.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/set_reg_value_work_item.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/wof_util.h"
#include "chrome/installer/util/work_item_list.h"

using base::ASCIIToUTF16;
using base::win::RegKey;

namespace installer {

namespace {

constexpr wchar_t kChromeInstallFilesCapabilitySid[] =
    L"S-1-15-3-1024-3424233489-972189580-2057154623-747635277-1604371224-"
    L"316187997-3786583170-1043257646";
constexpr wchar_t kLpacChromeInstallFilesCapabilitySid[] =
    L"S-1-15-3-1024-2302894289-466761758-1166120688-1039016420-2430351297-"
    L"4240214049-4028510897-3317428798";

void AddInstallerCopyTasks(const InstallParams& install_params,
                           WorkItemList* install_list) {
  DCHECK(install_list);

  const InstallerState& installer_state = install_params.installer_state;
  const base::FilePath& setup_path = install_params.setup_path;
  const base::FilePath& archive_path = install_params.archive_path;
  const base::FilePath& temp_path = install_params.temp_path;
  const base::Version& new_version = install_params.new_version;

  base::FilePath installer_dir(
      installer_state.GetInstallerDirectory(new_version));
  install_list->AddCreateDirWorkItem(installer_dir);

  base::FilePath exe_dst(installer_dir.Append(setup_path.BaseName()));

  if (exe_dst != setup_path) {
    install_list->AddCopyTreeWorkItem(setup_path.value(), exe_dst.value(),
                                      temp_path.value(), WorkItem::ALWAYS);
  }

  if (installer_state.RequiresActiveSetup() &&
      (base::win::GetVersion() < base::win::Version::VISTA)) {
    // For Windows versions before Vista we make a copy of setup.exe with a
    // different name so that Active Setup doesn't require an admin to run.
    // This is not necessary on Vista or later versions of Windows.
    base::FilePath active_setup_exe(installer_dir.Append(kActiveSetupExe));
    install_list->AddCopyTreeWorkItem(
        setup_path.value(), active_setup_exe.value(), temp_path.value(),
        WorkItem::ALWAYS);
  }

  base::FilePath archive_dst(installer_dir.Append(archive_path.BaseName()));
  if (archive_path != archive_dst) {
    // In the past, we copied rather than moved for system level installs so
    // that the permissions of %ProgramFiles% would be picked up.  Now that
    // |temp_path| is in %ProgramFiles% for system level installs (and in
    // %LOCALAPPDATA% otherwise), there is no need to do this for the archive.
    // Setup.exe, on the other hand, is created elsewhere so it must always be
    // copied.
    if (temp_path.IsParent(archive_path)) {
      install_list->AddMoveTreeWorkItem(archive_path.value(),
                                        archive_dst.value(),
                                        temp_path.value(),
                                        WorkItem::ALWAYS_MOVE);
    } else {
      // This may occur when setup is run out of an existing installation
      // directory. We cannot remove the system-level archive.
      install_list->AddCopyTreeWorkItem(archive_path.value(),
                                        archive_dst.value(),
                                        temp_path.value(),
                                        WorkItem::ALWAYS);
    }
  }
}

void AddWofCompressionTasks(const InstallerState& installer_state,
                            const bool is_new_install,
                            const base::FilePath& path_to_compress,
                            WorkItemList* install_list) {
  // Do not WOF compress for the initial install. The user is waiting for the
  // installation to finish or the installation is running on the boot critical
  // path to logon. It is more important to finish quick than compress.
  if (is_new_install) {
    VLOG(1) << "AddWofCompressionTasks: Skipping for initial install."
               " Finishing user blocking installs faster is more important.";
    return;
  }

  // Do not WOF compress if the product was installed through windows_update
  // until the browser has at least updated once. This is important to let the
  // first critical update complete as fast as possible.
  if (GoogleUpdateSettings::IsInstallSourceWindowsUpdate() &&
      GoogleUpdateSettings::GetLastUpdateTime().is_null()) {
    VLOG(1) << "AddWofCompressionTasks: Skipping for windows_update install."
               " The browser has not been updated at least once yet.";
    return;
  }

  ULONG algorithm;
  WofShouldCompressResult wof_result =
      WinWofShouldCompressBinaries(path_to_compress, &algorithm);

  UMA_HISTOGRAM_ENUMERATION("Microsoft.Setup.Install.WofShouldCompress",
                            wof_result);

  if (wof_result == WofShouldCompressResult::NotSupported) {
    VLOG(1) << "WinWofShouldCompressBinaries: WOF not supported.";
    return;
  } else if (wof_result == WofShouldCompressResult::NotRecommended) {
    VLOG(1) << "WinWofShouldCompressBinaries: WOF not recommended.";
    return;
  }

  DCHECK(wof_result == WofShouldCompressResult::Recommended);

  VLOG(1) << "WinWofShouldCompressBinaries. Algorithm: " << algorithm;
  UMA_HISTOGRAM_ENUMERATION("Microsoft.Setup.Install.WofCompressAlgorithm",
                            algorithm, FILE_PROVIDER_COMPRESSION_MAXIMUM);

  WorkItem* wof_compress_work = install_list->AddCallbackWorkItem(
      base::BindOnce(
          [](const base::FilePath& path_to_compress, ULONG algorithm,
             bool is_background, const CallbackWorkItem& work_item) {
            const base::TimeTicks wof_start = base::TimeTicks::Now();

            WofCompressCounts wof_counts{};
            TryWinWofCompressTree(path_to_compress, algorithm, &wof_counts);

            const base::TimeDelta wof_time =
                (base::TimeTicks::Now() - wof_start);

            if (is_background) {
              UMA_HISTOGRAM_LONG_TIMES(
                  "Microsoft.Setup.Install.WofCompressTotalTime.background",
                  wof_time);
            } else {
              UMA_HISTOGRAM_LONG_TIMES(
                  "Microsoft.Setup.Install.WofCompressTotalTime", wof_time);
            }

            VLOG(1) << "Milliseconds spent on WOF compression: "
                    << wof_time.InMilliseconds()
                    << ". Is background: " << is_background;

            return true;
          },
          path_to_compress, algorithm, installer_state.is_background_mode()),
      base::DoNothing());
  wof_compress_work->set_best_effort(true);
  wof_compress_work->set_rollback_enabled(false);
}

// A callback invoked by |work_item| that adds firewall rules for Chrome. Rules
// are left in-place on rollback unless |remove_on_rollback| is true. This is
// the case for new installs only. Updates and overinstalls leave the rule
// in-place on rollback since a previous install of Chrome will be used in that
// case.
bool AddFirewallRulesCallback(bool system_level,
                              const base::FilePath& chrome_path,
                              const CallbackWorkItem& work_item) {
  std::unique_ptr<FirewallManager> manager =
      FirewallManager::Create(chrome_path);
  if (!manager) {
    LOG(ERROR) << "Failed creating a FirewallManager. Continuing with install.";
    return true;
  }

  // Adding the firewall rule is expected to fail for user-level installs on
  // Vista+. Try anyway in case the installer is running elevated.
  if (!manager->AddFirewallRules() && system_level) {
    LOG(ERROR) << "Failed creating firewall rules (which is unexpected for "
    "this system-level install). Continuing with install.";
  }

  // Don't abort installation if the firewall rule couldn't be added.
  return true;
}

// A callback invoked by |work_item| that removes firewall rules on rollback
// if this is a new install.
void RemoveFirewallRulesCallback(const base::FilePath& chrome_path,
                                 const CallbackWorkItem& work_item) {
  std::unique_ptr<FirewallManager> manager =
      FirewallManager::Create(chrome_path);
  if (!manager) {
    LOG(ERROR) << "Failed creating a FirewallManager. Continuing rollback.";
    return;
  }

  manager->RemoveFirewallRules();
}

// Adds work item to |list| to create firewall rule.
void AddFirewallRuleWorkItem(const InstallerState& installer_state,
                             bool is_new_install,
                             WorkItemList* list,
                             const base::FilePath& exe_path) {
  VLOG(1) << "AddFireWallRuleWorkItem " << exe_path;

  WorkItem* item = list->AddCallbackWorkItem(
      base::BindOnce(&AddFirewallRulesCallback,
                     installer_state.system_install(), exe_path),
      base::BindOnce(&RemoveFirewallRulesCallback, exe_path));
  item->set_rollback_enabled(is_new_install);
}

// Adds work items to |list| to create firewall rules.
void AddFirewallRulesWorkItems(const InstallerState& installer_state,
                               bool is_new_install,
                               WorkItemList* list,
                               const base::Version& version) {
  base::FilePath target_path = installer_state.target_path();

  bool is_webview_preview = false;
  if (installer_state.is_browser()) {
    const base::FilePath browser = target_path.Append(kChromeExe);
    AddFirewallRuleWorkItem(installer_state, is_new_install, list, browser);

    is_webview_preview = !install_static::IsBrowserStableChannel();
  }

  if (is_webview_preview || installer_state.is_webview()) {
    const base::FilePath& target_versioned_path =
        target_path.AppendASCII(version.GetString());
    const base::FilePath webview = target_versioned_path.Append(kWebViewExe);
    AddFirewallRuleWorkItem(installer_state, is_new_install, list, webview);
  }
}

// Probes COM machinery to get an instance of notification_helper.exe's
// NotificationActivator class.
//
// This is required so that COM purges its cache of the path to the binary,
// which changes on updates.
bool ProbeNotificationActivatorCallback(const CLSID& toast_activator_clsid,
                                        const CallbackWorkItem& work_item) {
  DCHECK(toast_activator_clsid != CLSID_NULL);

  Microsoft::WRL::ComPtr<IUnknown> notification_activator;
  HRESULT hr =
      ::CoCreateInstance(toast_activator_clsid, nullptr, CLSCTX_LOCAL_SERVER,
                         IID_PPV_ARGS(&notification_activator));

  if (hr != REGDB_E_CLASSNOTREG) {
    LOG(ERROR) << "Unexpected result creating NotificationActivator; hr=0x"
               << std::hex << hr;
    return false;
  }

  return true;
}

// This is called when an MSI installation is run. It may be that a user is
// attempting to install the MSI on top of a non-MSI managed installation. If
// so, try and remove any existing "Add/Remove Programs" entry, as we want the
// uninstall to be managed entirely by the MSI machinery (accessible via the
// Add/Remove programs dialog).
void AddDeleteUninstallEntryForMSIWorkItems(
    const InstallerState& installer_state,
    WorkItemList* work_item_list) {
  DCHECK(installer_state.is_msi())
      << "This must only be called for MSI installations!";

  HKEY reg_root = installer_state.root_key();
  base::string16 uninstall_reg = install_static::GetUninstallRegistryPath();

  WorkItem* delete_reg_key = work_item_list->AddDeleteRegKeyWorkItem(
      reg_root, uninstall_reg, KEY_WOW64_32KEY);
  delete_reg_key->set_best_effort(true);
}

void AddDeleteOldArchiveWorkItem(const InstallerState& installer_state,
                                 const base::Version& current_version,
                                 const base::FilePath& temp_path,
                                 const base::FilePath& new_archive_path,
                                 WorkItemList* install_list) {
  base::FilePath old_installer_dir(
      installer_state.GetInstallerDirectory(current_version));
  base::FilePath old_archive(
        old_installer_dir.Append(installer::kChromeArchive));

  // Don't delete the archive that we are actually installing from.
  if (new_archive_path == old_archive) {
    return;
  }

  auto* work_item = install_list->AddDeleteTreeWorkItem(old_archive, temp_path);

  // Disable rollback for this item otherwise the file is just moved and defeats
  // the purpose of opportunistically deleting the old archive to make space for
  // the current install.
  work_item->set_best_effort(true);
  work_item->set_rollback_enabled(false);

  // Add a workitem to try to set the -full AP hint since once the previous
  // workitem is executed we will no longer have an archive to use as baseline
  // during a retry. We must do this because the installer might not reach the
  // end of the install attempt if the machine is shutdown or a crash occurs.
  // If the installation completes to the end, the installer will set the hint
  // as part of the final call to GoogleUpdateSettings::UpdateInstallStatus.
  auto* fullhint_workitem = install_list->AddCallbackWorkItem(
      base::BindOnce([](const CallbackWorkItem& work_item) {
        GoogleUpdateSettings::SetAPFullHint(install_static::IsSystemInstall(),
                                            install_static::GetAppGuid(),
                                            true /* full_value */);
        return true;
      }),
      base::DoNothing());
  fullhint_workitem->set_best_effort(true);
  fullhint_workitem->set_rollback_enabled(false);
}

// Adds Chrome specific install work items to |install_list|.
void AddChromeWorkItems(const InstallParams& install_params,
                        WorkItemList* install_list) {
<<<<<<< HEAD
=======
  const InstallerState& installer_state = install_params.installer_state;
  const base::FilePath& archive_path = install_params.archive_path;
  const base::FilePath& src_path = install_params.src_path;
  const base::FilePath& temp_path = install_params.temp_path;
  const base::Version* current_version = install_params.current_version;
  const base::Version& new_version = install_params.new_version;

  const base::FilePath& target_path = installer_state.target_path();

>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
  if (current_version) {
    // Delete the archive from an existing install to save some disk space.
    AddDeleteOldArchiveWorkItem(installer_state, *current_version, temp_path,
                                archive_path, install_list);
  }

  const base::FilePath& target_path = installer_state.target_path();
  const base::FilePath& target_versioned_path =
      target_path.AppendASCII(new_version.GetString());

  // Move or copy all the files that go into the new versioned folder first.
  // We should not touch anything in the parent application folder until the
  // versioned folder contents are settled.

  // In the past, we copied rather than moved for system level installs so that
  // the permissions of %ProgramFiles% would be picked up.  Now that |temp_path|
  // is in %ProgramFiles% for system level installs (and in %LOCALAPPDATA%
  // otherwise), there is no need to do this.
  // Note that we pass true for check_duplicates to avoid failing on in-use
  // repair runs if the current_version is the same as the new_version.
  const bool check_for_duplicates = (current_version &&
                                    *current_version == new_version);
  install_list->AddMoveTreeWorkItem(
      src_path.AppendASCII(new_version.GetString()).value(),
      target_versioned_path.value(),
      temp_path.value(),
      check_for_duplicates ? WorkItem::CHECK_DUPLICATES :
                             WorkItem::ALWAYS_MOVE);

  // Copy or modify files in the application folder now that the versioned
  // folder contents are settled.

  if (!installer_state.is_browser()) {
    return;
  }

  // Create |kVisualElementsManifest| in |target_path| (if required).
  install_list->AddCallbackWorkItem(
    base::BindOnce(
        [](const base::FilePath& src_path,
          const base::FilePath& target_path,
          const base::Version& new_version,
          const CallbackWorkItem& work_item) {
          return CreateVisualElementsManifest(src_path,
                                              target_path,
                                              new_version);
        },
        target_path, target_path, new_version),
        base::DoNothing());

  // TODO(grt): Touch the Start Menu shortcut after putting the manifest in
  // place to force the Start Menu to refresh Edge's tile.

  // Delete any new_msedge.exe if present (we will end up creating a new one
  // if required) and then copy msedge.exe.
  base::FilePath new_msedge_exe(target_path.Append(installer::kChromeNewExe));
  install_list->AddDeleteTreeWorkItem(new_msedge_exe, temp_path);

  // Make a hard link of msedge.exe from the version folder to the Application
  // folder, if not possible then make a copy. If the file is in use we will
  // create msedge_new.exe.
  constexpr bool prefer_hardlink = true;
  install_list->AddCopyTreeWorkItem(
      target_versioned_path.Append(installer::kChromeExe).value(),
      target_path.Append(installer::kChromeExe).value(), temp_path.value(),
      WorkItem::NEW_NAME_IF_IN_USE, prefer_hardlink, new_msedge_exe.value());

  // Delete any old_msedge.exe if present (ignore failure if it's in use).
  base::FilePath old_msedge_exe(target_path.Append(installer::kChromeOldExe));
  auto* delete_old_workitem =
      install_list->AddDeleteTreeWorkItem(old_msedge_exe, temp_path);
  delete_old_workitem->set_best_effort(true);
}

// Adds an ACE from a trustee SID, access mask and flags to an existing DACL.
// If the exact ACE already exists then the DACL is not modified and true is
// returned.
bool AddAceToDacl(const ATL::CSid& trustee,
                  ACCESS_MASK access_mask,
                  BYTE ace_flags,
                  ATL::CDacl* dacl) {
  // Check if the requested access already exists and return if so.
  for (UINT i = 0; i < dacl->GetAceCount(); ++i) {
    ATL::CSid sid;
    ACCESS_MASK mask = 0;
    BYTE type = 0;
    BYTE flags = 0;
    dacl->GetAclEntry(i, &sid, &mask, &type, &flags);
    if (sid == trustee && type == ACCESS_ALLOWED_ACE_TYPE &&
        (flags & ace_flags) == ace_flags &&
        (mask & access_mask) == access_mask) {
      return true;
    }
  }

  // Add the new access to the DACL.
  return dacl->AddAllowedAce(trustee, access_mask, ace_flags);
}

// Add to the ACL of an object on disk. This follows the method from MSDN:
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa379283.aspx
// This is done using explicit flags rather than the "security string" format
// because strings do not necessarily read what is written which makes it
// difficult to de-dup. Working with the binary format is always exact and the
// system libraries will properly ignore duplicate ACL entries.
bool AddAclToPath(const base::FilePath& path,
                  const std::vector<ATL::CSid>& trustees,
                  ACCESS_MASK access_mask,
                  BYTE ace_flags) {
  DCHECK(!path.empty());

  // Get the existing DACL.
  ATL::CDacl dacl;
  if (!ATL::AtlGetDacl(path.value().c_str(), SE_FILE_OBJECT, &dacl)) {
    DPLOG(ERROR) << "Failed getting DACL for path \"" << path.value() << "\"";
    return false;
  }

  for (const auto& trustee : trustees) {
    DCHECK(trustee.IsValid());
    if (!AddAceToDacl(trustee, access_mask, ace_flags, &dacl)) {
      DPLOG(ERROR) << "Failed adding ACE to DACL for trustee " << trustee.Sid();
      return false;
    }
  }

  // Attach the updated ACL as the object's DACL.
  if (!ATL::AtlSetDacl(path.value().c_str(), SE_FILE_OBJECT, dacl)) {
    DPLOG(ERROR) << "Failed setting DACL for path \"" << path.value() << "\"";
    return false;
  }

  return true;
}

bool AddAclToPath(const base::FilePath& path,
                  const CSid& trustee,
                  ACCESS_MASK access_mask,
                  BYTE ace_flags) {
  std::vector<ATL::CSid> trustees = {trustee};
  return AddAclToPath(path, trustees, access_mask, ace_flags);
}

}  // namespace

bool AddAclToPath(const base::FilePath& path,
                  const std::vector<const wchar_t*>& trustees,
                  ACCESS_MASK access_mask,
                  BYTE ace_flags) {
  std::vector<ATL::CSid> converted_trustees;
  for (const wchar_t* trustee : trustees) {
    PSID sid;
    if (!::ConvertStringSidToSid(trustee, &sid)) {
      DPLOG(ERROR) << "Failed to convert SID \"" << trustee << "\"";
      return false;
    }
    converted_trustees.emplace_back(static_cast<SID*>(sid));
    ::LocalFree(sid);
  }

  return AddAclToPath(path, converted_trustees, access_mask, ace_flags);
}

// Sets the right permissions so AppContainer processes can read and execute
// from the product binary folders.
void AddAppContainerFoldersAclWorkItems(const base::FilePath& target_path,
                                        const base::FilePath& temp_path,
                                        WorkItemList* install_list) {
  WorkItem* add_ac_acl_to_install = install_list->AddCallbackWorkItem(
      base::BindOnce(
          [](const base::FilePath& target_path, const base::FilePath& temp_path,
             const CallbackWorkItem& work_item) {
            std::vector<const wchar_t*> sids = {
                kChromeInstallFilesCapabilitySid,
                kLpacChromeInstallFilesCapabilitySid};
            bool success_target = AddAclToPath(
                target_path, sids, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
            bool success_temp = AddAclToPath(
                temp_path, sids, FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);

            bool success = (success_target && success_temp);
            base::UmaHistogramBoolean("Setup.Install.AddAppContainerAce",
                                      success);
            return success;
          },
          target_path, temp_path),
      base::DoNothing());
  add_ac_acl_to_install->set_best_effort(true);
  add_ac_acl_to_install->set_rollback_enabled(false);
}

namespace {

  // Creates the directory in which persistent metrics will be stored.
void AddHistogramsDirectoryWorkItems(const InstallerState& installer_state,
                                     const base::FilePath& target_path,
                                     WorkItemList* install_list) {
  const base::FilePath histogram_storage_dir(
      target_path.AppendASCII(kSetupHistogramAllocatorName));
  install_list->AddCreateDirWorkItem(histogram_storage_dir);

  if (installer_state.system_install()) {
    WorkItem* add_acl_to_histogram_storage_dir_work_item =
        install_list->AddCallbackWorkItem(
            base::BindOnce(
                [](const base::FilePath& histogram_storage_dir,
                   const CallbackWorkItem& work_item) {
                  return AddAclToPath(
                      histogram_storage_dir, ATL::Sids::AuthenticatedUser(),
                      FILE_GENERIC_READ | FILE_DELETE_CHILD,
                      CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE);
                },
                histogram_storage_dir),
            base::DoNothing());
    add_acl_to_histogram_storage_dir_work_item->set_best_effort(true);
    add_acl_to_histogram_storage_dir_work_item->set_rollback_enabled(false);
  }
}

#if defined(MICROSOFT_EDGE_BUILD)
// Adds work items to register the Elevation Service with Windows. Only for
// system level installs.
void AddElevationServiceWorkItems(const base::FilePath& elevation_service_path,
                                  WorkItemList* list) {
  DCHECK(::IsUserAnAdmin());

  if (elevation_service_path.empty()) {
    LOG(DFATAL) << "The path to elevation_service.exe is invalid.";
    return;
  }

  WorkItem* install_service_work_item = new InstallServiceWorkItem(
      install_static::GetElevationServiceName(),
      install_static::GetElevationServiceDisplayName(),
      InstallUtil::GetElevationServiceDescription(),
      base::CommandLine(elevation_service_path),
      install_static::GetElevatorClsid(), install_static::GetElevatorIid());
  install_service_work_item->set_best_effort(true);
  list->AddWorkItem(install_service_work_item);
}
#endif  // defined(MICROSOFT_EDGE_BUILD

#if EXCLUDED_FROM_EDGE
// Adds work items to add or remove the "store-dmtoken" command to Chrome's
// version key. This method is a no-op if this is anything other than
// system-level Chrome. The command is used when enrolling Chrome browser
// instances into enterprise management. |new_version| is the version currently
// being installed -- can be empty on uninstall.
void AddEnterpriseEnrollmentWorkItems(const InstallerState& installer_state,
                                      const base::FilePath& setup_path,
                                      const base::Version& new_version,
                                      WorkItemList* install_list) {
  if (!installer_state.system_install())
    return;

  const HKEY root_key = installer_state.root_key();
  const base::string16 cmd_key(GetCommandKey(kCmdStoreDMToken));

  if (installer_state.operation() == InstallerState::UNINSTALL) {
    install_list->AddDeleteRegKeyWorkItem(root_key, cmd_key, KEY_WOW64_32KEY)
        ->set_log_message("Removing store DM token command");
  } else {
    // Register a command to allow Chrome to request Google Update to run
    // setup.exe --store-dmtoken=<token>, which will store the specified token
    // in the registry.
    base::CommandLine cmd_line(
        installer_state.GetInstallerDirectory(new_version)
            .Append(setup_path.BaseName()));
    cmd_line.AppendSwitchASCII(switches::kStoreDMToken, "%1");
    cmd_line.AppendSwitch(switches::kSystemLevel);
    cmd_line.AppendSwitch(switches::kVerboseLogging);
    InstallUtil::AppendModeSwitch(&cmd_line);

    AppCommand cmd(cmd_line.GetCommandLineString());
    // TODO(alito): For now setting this command as web accessible is required
    // by Google Update.  Could revisit this should Google Update change the
    // way permissions are handled for commands.
    cmd.set_is_web_accessible(true);
    cmd.AddWorkItems(root_key, cmd_key, install_list);
  }
}
#endif // EXCLUDED_FROM_EDGE

}  // namespace

// This method adds work items to create (or update) Chrome uninstall entry in
// either the Control Panel->Add/Remove Programs list or in the Omaha client
// state key if running under an MSI installer.
void AddUninstallShortcutWorkItems(const InstallParams& install_params,
                                   WorkItemList* install_list) {
  const InstallerState& installer_state = install_params.installer_state;
  const base::FilePath& setup_path = install_params.setup_path;
  const base::Version& new_version = install_params.new_version;

  HKEY reg_root = installer_state.root_key();

  // When we are installed via an MSI, we need to store our uninstall strings
  // in the Google Update client state key. We do this even for non-MSI
  // managed installs to avoid breaking the edge case whereby an MSI-managed
  // install is updated by a non-msi installer (which would confuse the MSI
  // machinery if these strings were not also updated). The UninstallString
  // value placed in the client state key is also used by the mini_installer to
  // locate the setup.exe instance used for binary patching.
  // Do not quote the command line for the MSI invocation.
  base::FilePath install_path(installer_state.target_path());
  base::FilePath installer_path(
      installer_state.GetInstallerDirectory(new_version));
  installer_path = installer_path.Append(setup_path.BaseName());

  base::CommandLine install_arguments(base::CommandLine::NO_PROGRAM);
  AppendInstallCommandLineFlags(installer_state, &install_arguments);

  base::CommandLine uninstall_arguments(base::CommandLine::NO_PROGRAM);
  AppendUninstallCommandLineFlags(installer_state, &uninstall_arguments);

  base::string16 update_state_key(install_static::GetClientStateKeyPath());
  install_list->AddCreateRegKeyWorkItem(
      reg_root, update_state_key, KEY_WOW64_32KEY);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       update_state_key,
                                       KEY_WOW64_32KEY,
                                       installer::kUninstallStringField,
                                       installer_path.value(),
                                       true);
  install_list->AddSetRegValueWorkItem(
      reg_root,
      update_state_key,
      KEY_WOW64_32KEY,
      installer::kUninstallArgumentsField,
      uninstall_arguments.GetCommandLineString(),
      true);

  // MSI installations will manage their own uninstall shortcuts.
  if (!installer_state.is_msi()) {
    base::string16 uninstall_reg = install_static::GetUninstallRegistryPath();
    install_list->AddCreateRegKeyWorkItem(
        reg_root, uninstall_reg, KEY_WOW64_32KEY);
    install_list->AddSetRegValueWorkItem(reg_root, uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         installer::kUninstallDisplayNameField,
                                         InstallUtil::GetDisplayName(), true);

    const bool is_sticky_install = ShellUtil::IsStickyInstall();

    // We need to quote the command line for the Add/Remove Programs dialog.
    base::CommandLine quoted_install_cmd(installer_path);
    DCHECK_EQ(quoted_install_cmd.GetCommandLineString()[0], '"');
    quoted_install_cmd.AppendArguments(install_arguments, false);
    install_list->AddSetRegValueWorkItem(
        reg_root,
        uninstall_reg,
        KEY_WOW64_32KEY,
        installer::kModifyPathField,
        quoted_install_cmd.GetCommandLineString(),
        true);

    if (is_sticky_install) {
      install_list->AddDeleteRegValueWorkItem(
          reg_root,
          uninstall_reg,
          KEY_WOW64_32KEY,
          installer::kUninstallStringField);
    } else {
      // We need to quote the command line for the Add/Remove Programs dialog.
      base::CommandLine quoted_uninstall_cmd(installer_path);
      DCHECK_EQ(quoted_uninstall_cmd.GetCommandLineString()[0], '"');
      quoted_uninstall_cmd.AppendArguments(uninstall_arguments, false);

      install_list->AddSetRegValueWorkItem(
          reg_root,
          uninstall_reg,
          KEY_WOW64_32KEY,
          installer::kUninstallStringField,
          quoted_uninstall_cmd.GetCommandLineString(),
          true);
    }
    install_list->AddSetRegValueWorkItem(reg_root,
                                         uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         L"InstallLocation",
                                         install_path.value(),
                                         true);

    base::FilePath icon_path = install_path;
    if (installer_state.is_browser()) {
      icon_path = icon_path.Append(kChromeExe);
    }
    if (installer_state.is_webview()) {
      icon_path = icon_path
                      .AppendASCII(new_version.GetString())
                      .Append(kWebViewExe);
    }

    base::string16 chrome_icon = ShellUtil::FormatIconLocation(
        icon_path,
        install_static::GetIconResourceIndex(
            install_static::GetHTMLProgIdIndex()));
    install_list->AddSetRegValueWorkItem(reg_root,
                                         uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         L"DisplayIcon",
                                         chrome_icon,
                                         true);
    install_list->AddDeleteRegValueWorkItem(reg_root,
                                            uninstall_reg,
                                            KEY_WOW64_32KEY,
                                            L"NoModify");
    if (is_sticky_install)
    {
      install_list->AddSetRegValueWorkItem(reg_root,
                                           uninstall_reg,
                                           KEY_WOW64_32KEY,
                                           L"NoRemove",
                                           1UL,
                                           true);
    } else {
      install_list->AddDeleteRegValueWorkItem(
          reg_root,
          uninstall_reg,
          KEY_WOW64_32KEY,
          L"NoRemove");
    }
    install_list->AddSetRegValueWorkItem(reg_root,
                                         uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         L"NoRepair",
                                         1UL,
                                         true);

    install_list->AddSetRegValueWorkItem(reg_root, uninstall_reg,
                                         KEY_WOW64_32KEY, L"Publisher",
                                         InstallUtil::GetPublisherName(), true);
    install_list->AddSetRegValueWorkItem(reg_root,
                                         uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         L"Version",
                                         ASCIIToUTF16(new_version.GetString()),
                                         true);
    install_list->AddSetRegValueWorkItem(reg_root,
                                         uninstall_reg,
                                         KEY_WOW64_32KEY,
                                         L"DisplayVersion",
                                         ASCIIToUTF16(new_version.GetString()),
                                         true);
    // TODO(wfh): Ensure that this value is preserved in the 64-bit hive when
    // 64-bit installs place the uninstall information into the 64-bit registry.
    install_list->AddSetRegValueWorkItem(reg_root, uninstall_reg,
                                         KEY_WOW64_32KEY, L"InstallDate",
                                         InstallUtil::GetCurrentDate(), true);

    const std::vector<uint32_t>& version_components = new_version.components();
    if (version_components.size() == 4) {
      // Our version should be in major.minor.build.rev.
      install_list->AddSetRegValueWorkItem(
          reg_root,
          uninstall_reg,
          KEY_WOW64_32KEY,
          L"VersionMajor",
          static_cast<DWORD>(version_components[2]),
          true);
      install_list->AddSetRegValueWorkItem(
          reg_root,
          uninstall_reg,
          KEY_WOW64_32KEY,
          L"VersionMinor",
          static_cast<DWORD>(version_components[3]),
          true);
    }
  }
}

// Create Version key for a product (if not already present) and sets the new
// product version as the last step.
<<<<<<< HEAD
void AddVersionKeyWorkItems(HKEY root,
                            const InstallerState& installer_state,
                            const base::Version& new_version,
                            bool add_language_identifier,
=======
void AddVersionKeyWorkItems(const InstallParams& install_params,
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
                            WorkItemList* list) {
  const InstallerState& installer_state = install_params.installer_state;
  const HKEY root = installer_state.root_key();

  // Only set "lang" for user-level installs since for system-level, the install
  // language may not be related to a given user's runtime language.
  const bool add_language_identifier = !installer_state.system_install();

  const base::string16 clients_key = install_static::GetClientsKeyPath();
  list->AddCreateRegKeyWorkItem(root, clients_key, KEY_WOW64_32KEY);

  list->AddSetRegValueWorkItem(root, clients_key, KEY_WOW64_32KEY,
                               google_update::kRegNameField,
                               InstallUtil::GetDisplayName(),
                               true);  // overwrite name also
  list->AddSetRegValueWorkItem(root, clients_key, KEY_WOW64_32KEY,
                               google_update::kRegOopcrashesField,
                               1UL,
                               false);  // set during first install
  if (add_language_identifier) {
    // Write the language identifier of the current translation.  Omaha's set of
    // languages is a superset of Chrome's set of translations with this one
    // exception: what Chrome calls "en-us", Omaha calls "en".  sigh.
    base::string16 language(GetCurrentTranslation());
    if (base::LowerCaseEqualsASCII(language, "en-us"))
      language.resize(2);
    list->AddSetRegValueWorkItem(root, clients_key, KEY_WOW64_32KEY,
                                 google_update::kRegLangField, language,
                                 false);  // do not overwrite language
  }
<<<<<<< HEAD
  base::FilePath install_path(installer_state.target_path());
  list->AddSetRegValueWorkItem(root, clients_key, KEY_WOW64_32KEY,
                               google_update::kRegLocationField,
                               install_path.value(),
                               true);  // overwrite location
  list->AddSetRegValueWorkItem(root, clients_key, KEY_WOW64_32KEY,
                               google_update::kRegVersionField,
                               ASCIIToUTF16(new_version.GetString()),
                               true);  // overwrite version
=======
  list->AddSetRegValueWorkItem(
      root, clients_key, KEY_WOW64_32KEY, google_update::kRegVersionField,
      ASCIIToUTF16(install_params.new_version.GetString()),
      true);  // overwrite version
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
}

void AddUpdateBrandCodeWorkItem(const InstallerState& installer_state,
                                WorkItemList* install_list) {
  // Only update specific brand codes needed for enterprise.
  base::string16 brand;
  if (!GoogleUpdateSettings::GetBrand(&brand))
    return;

  base::string16 new_brand = GetUpdatedBrandCode(brand);
  if (new_brand.empty())
    return;

  // Only update if this machine is:
  // - domain joined, or
  // - registered with MDM and is not windows home edition
  bool is_enterprise_version =
      base::win::OSInfo::GetInstance()->version_type() != base::win::SUITE_HOME;
  if (!(base::win::IsEnrolledToDomain() ||
      (base::win::IsDeviceRegisteredWithManagement() &&
       is_enterprise_version))) {
    return;
  }

  install_list->AddSetRegValueWorkItem(
      installer_state.root_key(), install_static::GetClientStateKeyPath(),
      KEY_WOW64_32KEY, google_update::kRegRLZBrandField, new_brand, true);
}

base::string16 GetUpdatedBrandCode(const base::string16& brand_code) {
  // Brand codes to be remapped on enterprise installs.
  static constexpr struct EnterpriseBrandRemapping {
    const wchar_t* old_brand;
    const wchar_t* new_brand;
  } kEnterpriseBrandRemapping[] = {
      {L"GGLS", L"GCEU"},
      {L"GGRV", L"GCEV"},
  };

  for (auto mapping : kEnterpriseBrandRemapping) {
    if (brand_code == mapping.old_brand)
      return mapping.new_brand;
  }
  return base::string16();
}

<<<<<<< HEAD


bool AppendPostInstallTasks(const InstallerState& installer_state,
                            const base::FilePath& setup_path,
                            const base::FilePath& src_path,
                            const base::FilePath& temp_path,
                            const base::Version* current_version,
                            const base::Version& new_version,
=======
bool AppendPostInstallTasks(const InstallParams& install_params,
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
                            WorkItemList* post_install_task_list) {
  DCHECK(post_install_task_list);
  DCHECK(installer_state.is_browser());
  constexpr bool prefer_hardlink = true;

  const InstallerState& installer_state = install_params.installer_state;
  const base::FilePath& setup_path = install_params.setup_path;
  const base::FilePath& src_path = install_params.src_path;
  const base::FilePath& temp_path = install_params.temp_path;
  const base::Version* current_version = install_params.current_version;
  const base::Version& new_version = install_params.new_version;

  HKEY root = installer_state.root_key();
  const base::FilePath& target_path = installer_state.target_path();
  base::FilePath new_chrome_exe(target_path.Append(kChromeNewExe));
  
  const base::FilePath& target_versioned_path =
      target_path.AppendASCII(new_version.GetString());

  // Append work items that will only be executed if this was an update.
  // We update the 'opv' value with the current version that is active,
  // the 'cpv' value with the critical update version (if present), and the
  // 'cmd' value with the rename command to run.
  {
    std::unique_ptr<WorkItemList> in_use_update_work_items(
        WorkItem::CreateConditionalWorkItemList(
            new ConditionRunIfFileExists(new_chrome_exe)));
    in_use_update_work_items->set_log_message("InUseUpdateWorkItemList");

    // |critical_version| will be valid only if this in-use update includes a
    // version considered critical relative to the version being updated.
    base::Version critical_version(installer_state.DetermineCriticalVersion(
        current_version, new_version));
    base::FilePath installer_path(
        installer_state.GetInstallerDirectory(new_version).Append(
            setup_path.BaseName()));

    const base::string16 clients_key(install_static::GetClientsKeyPath());

    if (current_version) {
      in_use_update_work_items->AddSetRegValueWorkItem(
          root, clients_key, KEY_WOW64_32KEY,
          google_update::kRegOldVersionField,
          ASCIIToUTF16(current_version->GetString()), true);
    }
    if (critical_version.IsValid()) {
      in_use_update_work_items->AddSetRegValueWorkItem(
          root, clients_key, KEY_WOW64_32KEY,
          google_update::kRegCriticalVersionField,
          ASCIIToUTF16(critical_version.GetString()), true);
    } else {
      in_use_update_work_items->AddDeleteRegValueWorkItem(
          root, clients_key, KEY_WOW64_32KEY,
          google_update::kRegCriticalVersionField);
    }

    // Form the mode-specific rename command.
    base::CommandLine product_rename_cmd(installer_path);
    product_rename_cmd.AppendSwitch(switches::kRenameChromeExe);
    if (installer_state.system_install())
      product_rename_cmd.AppendSwitch(switches::kSystemLevel);
    if (installer_state.verbose_logging())
      product_rename_cmd.AppendSwitch(switches::kVerboseLogging);
    InstallUtil::AppendModeSwitch(&product_rename_cmd);
    in_use_update_work_items->AddSetRegValueWorkItem(
        root, clients_key, KEY_WOW64_32KEY, google_update::kRegRenameCmdField,
        product_rename_cmd.GetCommandLineString(), true);

    // Delay deploying the new chrome_proxy while chrome is running.
    in_use_update_work_items->AddCopyTreeWorkItem(
        target_versioned_path.Append(kChromeProxyExe).value(),
        target_path.Append(kChromeProxyNewExe).value(), temp_path.value(),
        WorkItem::ALWAYS, prefer_hardlink);

    // Delay deploying the new pwahelper while chrome is running.
    in_use_update_work_items->AddCopyTreeWorkItem(
        target_versioned_path.Append(kPwaHelperExe).value(),
        target_path.Append(kPwaHelperNewExe).value(), temp_path.value(),
        WorkItem::ALWAYS, prefer_hardlink);

    post_install_task_list->AddWorkItem(in_use_update_work_items.release());
  }

  // Append work items that will be executed if this was NOT an in-use update.
  {
    std::unique_ptr<WorkItemList> regular_update_work_items(
        WorkItem::CreateConditionalWorkItemList(
            new Not(new ConditionRunIfFileExists(new_chrome_exe))));
    regular_update_work_items->set_log_message("RegularUpdateWorkItemList");

    // Since this was not an in-use-update, delete 'opv', 'cpv', and 'cmd' keys.
    const base::string16 clients_key(install_static::GetClientsKeyPath());
    regular_update_work_items->AddDeleteRegValueWorkItem(
        root, clients_key, KEY_WOW64_32KEY, google_update::kRegOldVersionField);
    regular_update_work_items->AddDeleteRegValueWorkItem(
        root, clients_key, KEY_WOW64_32KEY,
        google_update::kRegCriticalVersionField);

    regular_update_work_items->AddDeleteRegValueWorkItem(
        root, clients_key, KEY_WOW64_32KEY,
        google_update::kRegRenameCmdField);

    // Only copy chrome_proxy.exe directly when chrome.exe isn't in use to avoid
    // different versions getting mixed up between the two binaries.
    regular_update_work_items->AddCopyTreeWorkItem(
        target_versioned_path.Append(kChromeProxyExe).value(),
        target_path.Append(kChromeProxyExe).value(), temp_path.value(),
        WorkItem::ALWAYS, prefer_hardlink);

    // The valid pwahelper.exe will help to locate the pwahelper.dll,
    // which is under version folder.
    regular_update_work_items->AddCopyTreeWorkItem(
        target_versioned_path.Append(kPwaHelperExe).value(),
        target_path.Append(kPwaHelperExe).value(), temp_path.value(),
        WorkItem::ALWAYS, prefer_hardlink);

    post_install_task_list->AddWorkItem(regular_update_work_items.release());
  }

  // Add a best-effort item to create the ClientStateMedium key for system-level
  // installs. This is ordinarily done by Google Update prior to running
  // Chrome's installer. Do it here as well so that the key exists for manual
  // installs.
  if (install_static::IsEnabledGoogleUpdateIntegration()) {
    if (installer_state.system_install()) {
      const base::string16 path = install_static::GetClientStateMediumKeyPath();
      post_install_task_list
          ->AddCreateRegKeyWorkItem(HKEY_LOCAL_MACHINE, path, KEY_WOW64_32KEY)
          ->set_best_effort(true);
    }
  }

  return true;
}

void AddInstallWorkItems(const InstallParams& install_params,
                         WorkItemList* install_list) {
  DCHECK(install_list);
<<<<<<< HEAD
  DCHECK(installer_state.is_browser());
  const bool is_new_install = (current_version == nullptr);
=======

  const InstallerState& installer_state = install_params.installer_state;
  const base::FilePath& setup_path = install_params.setup_path;
  const base::FilePath& temp_path = install_params.temp_path;
  const base::Version* current_version = install_params.current_version;
  const base::Version& new_version = install_params.new_version;

>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
  const base::FilePath& target_path = installer_state.target_path();

  // A temp directory that work items need and the actual install directory.
  install_list->AddCreateDirWorkItem(temp_path);
  install_list->AddCreateDirWorkItem(target_path);

  // Set permissions early on both temp and target, since moved files may not
  // inherit permissions.
  AddAppContainerFoldersAclWorkItems(target_path, temp_path, install_list);

  // Create the directory in which persistent metrics will be stored.
  AddHistogramsDirectoryWorkItems(installer_state, target_path, install_list);

<<<<<<< HEAD
  // Copy the product binaries.
  AddChromeWorkItems(original_state, installer_state, setup_path, archive_path,
                     src_path, temp_path, current_version, new_version,
                     install_list);

  // Copy installer in install directory
  AddInstallerCopyTasks(installer_state, setup_path, archive_path, temp_path,
                        new_version, install_list);

  AddFirewallRulesWorkItems(installer_state, is_new_install, install_list,
                            new_version);

  const HKEY root = installer_state.root_key();
  // Only set "lang" for user-level installs since for system-level, the install
  // language may not be related to a given user's runtime language.
  const bool add_language_identifier = !installer_state.system_install();
=======
  AddChromeWorkItems(install_params, install_list);

  // Copy installer in install directory
  AddInstallerCopyTasks(install_params, install_list);
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368

  AddUninstallShortcutWorkItems(install_params, install_list);

<<<<<<< HEAD
  AddVersionKeyWorkItems(root, installer_state, new_version, add_language_identifier,
                         install_list);
=======
  AddVersionKeyWorkItems(install_params, install_list);
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368

#if EXCLUDED_FROM_EDGE
  AddCleanupDeprecatedPerUserRegistrationsWorkItems(install_list);
#endif  // EXCLUDED_FROM_EDGE

  AddActiveSetupWorkItems(installer_state, new_version, install_list);

  AddOsUpgradeWorkItems(installer_state, setup_path, new_version, install_list);

// Exclude Chrome Device Managment workitems from Edge
#if EXCLUDED_FROM_EDGE
  AddEnterpriseEnrollmentWorkItems(installer_state, setup_path, new_version,
                                   install_list);
#endif  // EXCLUDED_FROM_EDGE

  // We don't have a version check for Win10+ here so that Windows upgrades
  // work.
  AddNativeNotificationWorkItems(
      installer_state.root_key(),
      GetNotificationHelperPath(target_path, new_version), install_list);

#if defined(MICROSOFT_EDGE_BUILD)
  if (installer_state.system_install()) {
    AddElevationServiceWorkItems(
        GetElevationServicePath(target_path, new_version), install_list);
  }
#endif  // defined(MICROSOFT_EDGE_BUILD

  InstallUtil::AddUpdateDowngradeVersionItem(
      installer_state.root_key(), current_version, new_version, install_list);

  AddUpdateBrandCodeWorkItem(installer_state, install_list);

#if defined(MICROSOFT_EDGE_BUILD)
  AddNeedEdgeWorkItems(installer_state, install_list);
  AddWebViewBridgeWorkItems(installer_state, new_version, install_list);
  if (installer_state.system_install()) {
    AddInternetExplorerEdgeIntegrationWorkItems(installer_state, install_list);
  }

  AddEdgeUWPReplacementWorkItems(installer_state, setup_path,
                                  new_version, install_list);

#if defined(NDEBUG)
  // Add Diagnostic Data Viewer Decoder Host Package ACLs to telclient.dll.
  AddDecoderHostPackageAclsToTelClientWorkItem(installer_state, new_version,
                                               install_list);
  AddEdgeSpartanAclForCookieExporterWorkItem(installer_state, new_version,
                                             install_list);
#endif  // defined(NDEBUG)

  if (installer_state.system_install() &&
      !install_static::IsBrowserStableChannel()) {
    // Only allowed to add SACL if process is running elevated or as SYSTEM.
    AddSaclToIcuDatFileWorkItem(installer_state, new_version, install_list);
  }

#endif  // defined(MICROSOFT_EDGE_BUILD)

  // Append the tasks that run after the installation.
<<<<<<< HEAD
  // Post install tasks encapsulates what should happen if the browser is in
  // use while the installer task list is running versus not.
  AppendPostInstallTasks(installer_state, setup_path, src_path, temp_path,
                         current_version, new_version, install_list);

  // If we're told that we're an MSI install, make sure to set the marker
  // in the client state key so that future updates do the right thing.
  if (installer_state.is_msi()) {
    AddSetMsiMarkerWorkItem(installer_state, true, install_list);

    // We want MSI installs to take over the Add/Remove Programs entry. Make a
    // best-effort attempt to delete any entry left over from previous non-MSI
    // installations for the same type of install (system or per user).
    AddDeleteUninstallEntryForMSIWorkItems(installer_state, install_list);
  }

  // Add WOF compression tasks after all install and post install tasks.
  // The install or update is complete at this point.
  // If the machine shuts down while this is happening, there is no impact to
  // the update.
  const base::FilePath path_to_compress =
      installer_state.target_path().AppendASCII(new_version.GetString());
  AddWofCompressionTasks(installer_state, is_new_install, path_to_compress,
                         install_list);
}

void AddWebViewInstallWorkItems(const InstallationState& original_state,
                                const InstallerState& installer_state,
                                const base::FilePath& setup_path,
                                const base::FilePath& archive_path,
                                const base::FilePath& src_path,
                                const base::FilePath& temp_path,
                                const base::Version* current_version,
                                const base::Version& new_version,
                                WorkItemList* install_list) {
  DCHECK(installer_state.is_webview());
  const bool is_new_install = (current_version == nullptr);
  const base::FilePath& target_path = installer_state.target_path();

  // A temp directory that work items need and the actual install directory.
  install_list->AddCreateDirWorkItem(temp_path);
  install_list->AddCreateDirWorkItem(target_path);

  // Set permissions early on both temp and target, since moved files may not
  // inherit permissions.
  AddAppContainerFoldersAclWorkItems(target_path, temp_path, install_list);

  // Create the directory in which persistent metrics will be stored.
  AddHistogramsDirectoryWorkItems(installer_state, target_path, install_list);

  // Copy the product binaries.
  AddChromeWorkItems(original_state, installer_state, setup_path, archive_path,
                     src_path, temp_path, current_version, new_version,
                     install_list);

  // Copy installer in install directory
  AddInstallerCopyTasks(installer_state, setup_path, archive_path, temp_path,
                        new_version, install_list);

  AddFirewallRulesWorkItems(installer_state, is_new_install, install_list,
                            new_version);

  AddUninstallShortcutWorkItems(installer_state, setup_path, new_version,
                                install_list);

  const HKEY root = installer_state.root_key();
  // Only set "lang" for user-level installs since for system-level, the install
  // language may not be related to a given user's runtime language.
  const bool add_language_id = !installer_state.system_install();
  AddVersionKeyWorkItems(root, installer_state, new_version, add_language_id,
                         install_list);

  AddWebViewBridgeWorkItems(installer_state, new_version, install_list);

  // Add WOF compression tasks after all install and post install tasks.
  // The install or update is complete at this point.
  // If the machine shuts down while this is happening, there is no impact to
  // the update.
  const base::FilePath path_to_compress =
      installer_state.target_path().AppendASCII(new_version.GetString());
  AddWofCompressionTasks(installer_state, is_new_install, path_to_compress,
                         install_list);
=======
  AppendPostInstallTasks(install_params, install_list);
>>>>>>> cf1a6b6aacf15509b6eae50701dd5abab24ef368
}

void AddNativeNotificationWorkItems(
    HKEY root,
    const base::FilePath& notification_helper_path,
    WorkItemList* list) {
  if (notification_helper_path.empty()) {
    LOG(DFATAL) << "The path to notification_helper.exe is invalid.";
    return;
  }

  base::string16 toast_activator_reg_path =
      InstallUtil::GetToastActivatorRegistryPath();

  if (toast_activator_reg_path.empty()) {
    LOG(DFATAL) << "Cannot retrieve the toast activator registry path";
    return;
  }

  // Delete the old registration before adding in the new key to ensure that the
  // COM probe/flush below does its job. Delete both 64-bit and 32-bit keys to
  // handle 32-bit -> 64-bit or 64-bit -> 32-bit migration.
  list->AddDeleteRegKeyWorkItem(root, toast_activator_reg_path,
                                KEY_WOW64_32KEY);

  list->AddDeleteRegKeyWorkItem(root, toast_activator_reg_path,
                                KEY_WOW64_64KEY);

  // Also delete the old registration's AppID key.
  base::string16 edge_toast_activator_app_id_path =
      InstallUtil::EdgeGetToastActivatorAppIdRegistryPath();

  if (edge_toast_activator_app_id_path.empty()) {
    LOG(DFATAL) << "Cannot retrieve the toast activator AppID registry path";
    return;
  }

  list->AddDeleteRegKeyWorkItem(root, edge_toast_activator_app_id_path,
                                KEY_WOW64_32KEY);

  list->AddDeleteRegKeyWorkItem(root, edge_toast_activator_app_id_path,
                                KEY_WOW64_64KEY);

  // Force COM to flush its cache containing the path to the old handler.
  WorkItem* item = list->AddCallbackWorkItem(
      base::BindOnce(&ProbeNotificationActivatorCallback,
                     install_static::GetToastActivatorClsid()),
      base::BindOnce(base::IgnoreResult(&ProbeNotificationActivatorCallback),
                     install_static::GetToastActivatorClsid()));
  item->set_best_effort(true);

  base::string16 toast_activator_server_path =
      toast_activator_reg_path + L"\\LocalServer32";

  // Command-line featuring the quoted path to the exe.
  base::string16 command(1, L'"');
  command.append(notification_helper_path.value()).append(1, L'"');

  list->AddCreateRegKeyWorkItem(root, toast_activator_server_path,
                                WorkItem::kWow64Default);

  list->AddSetRegValueWorkItem(root, toast_activator_server_path,
                               WorkItem::kWow64Default, L"", command, true);

  list->AddSetRegValueWorkItem(root, toast_activator_server_path,
                               WorkItem::kWow64Default, L"ServerExecutable",
                               notification_helper_path.value(), true);

  // Re-use the toast activator's CLSID as its AppId by setting the following
  // REG_SZ value:
  //
  // AppId = "{toast activator clsid guid}"
  //
  // https://docs.microsoft.com/en-us/windows/win32/com/appid-clsid
  const base::string16 app_id_value =
      base::win::String16FromGUID(install_static::GetToastActivatorClsid());

  list->AddSetRegValueWorkItem(root, toast_activator_reg_path,
                               WorkItem::kWow64Default, L"AppId", app_id_value,
                               /*overwrite=*/true);

  // Create the toast activator's AppId registry key and then set the following
  // REG_DWORD value:
  //
  // LoadUserSettings = 1
  //
  // https://docs.microsoft.com/en-us/windows/win32/com/loadusersettings
  //
  // Without LoadUserSettings, background task COM activations load the system's
  // environment.  For example:
  //
  // LOCALAPPDATA=C:\WINDOWS\system32\config\systemprofile\AppData\Local
  //
  // With LoadUserSettings, background task COM activations load the current
  // user's environment, which is required by Chromium utilities like
  // crashpad and UMA telemetry.
  list->AddCreateRegKeyWorkItem(root, edge_toast_activator_app_id_path,
                                WorkItem::kWow64Default);

  const DWORD load_user_settings_value = 1u;
  list->AddSetRegValueWorkItem(root, edge_toast_activator_app_id_path,
                               WorkItem::kWow64Default, L"LoadUserSettings",
                               load_user_settings_value, /*overwrite=*/true);
}

void AddSetMsiMarkerWorkItem(const InstallerState& installer_state,
                             bool set,
                             WorkItemList* work_item_list) {
  DCHECK(work_item_list);
  DWORD msi_value = set ? 1 : 0;
  WorkItem* set_msi_work_item = work_item_list->AddSetRegValueWorkItem(
      installer_state.root_key(), install_static::GetClientStateKeyPath(),
      KEY_WOW64_32KEY, google_update::kRegMSIField, msi_value, true);
  DCHECK(set_msi_work_item);
  set_msi_work_item->set_best_effort(true);
  set_msi_work_item->set_log_message("Could not write MSI marker!");
}

void AddCleanupDeprecatedPerUserRegistrationsWorkItems(WorkItemList* list) {
  // This cleanup was added in M49. There are still enough active users on M48
  // and earlier today (M55 timeframe) to justify keeping this cleanup in-place.
  // Remove this when that population stops shrinking.
  VLOG(1) << "Adding unregistration items for per-user Metro keys.";
  list->AddDeleteRegKeyWorkItem(HKEY_CURRENT_USER,
                                install_static::GetRegistryPath() + L"\\Metro",
                                KEY_WOW64_32KEY);
  list->AddDeleteRegKeyWorkItem(HKEY_CURRENT_USER,
                                install_static::GetRegistryPath() + L"\\Metro",
                                KEY_WOW64_64KEY);
}

void AddActiveSetupWorkItems(const InstallerState& installer_state,
                             const base::Version& new_version,
                             WorkItemList* list) {
  DCHECK(installer_state.operation() != InstallerState::UNINSTALL);

  if (!installer_state.system_install()) {
    VLOG(1) << "No Active Setup processing to do for user-level Edge";
    return;
  }
  DCHECK(installer_state.RequiresActiveSetup());

  const HKEY root = HKEY_LOCAL_MACHINE;
  const base::string16 active_setup_path(install_static::GetActiveSetupPath());

  VLOG(1) << "Adding registration items for Active Setup.";
  list->AddCreateRegKeyWorkItem(
      root, active_setup_path, WorkItem::kWow64Default);
  list->AddSetRegValueWorkItem(root, active_setup_path, WorkItem::kWow64Default,
                               L"", InstallUtil::GetDisplayName(), true);

  // For Windows versions before Vista we use a copy of setup.exe with a
  // different name so that Active Setup doesn't require an admin to run.
  // This is not necessary on Vista or later versions of Windows.
  base::FilePath active_setup_exe(installer_state.GetInstallerDirectory(
                                  new_version));
  if (base::win::GetVersion() < base::win::Version::VISTA) {
    active_setup_exe = active_setup_exe.Append(kActiveSetupExe);
  } else {
    active_setup_exe = active_setup_exe.Append(kSetupExe);
  }

  base::CommandLine cmd(active_setup_exe);
  cmd.AppendSwitch(installer::switches::kConfigureUserSettings);
  cmd.AppendSwitch(installer::switches::kVerboseLogging);
  cmd.AppendSwitch(installer::switches::kSystemLevel);
  InstallUtil::AppendModeSwitch(&cmd);
  list->AddSetRegValueWorkItem(root,
                               active_setup_path,
                               WorkItem::kWow64Default,
                               L"StubPath",
                               cmd.GetCommandLineString(),
                               true);

  // TODO(grt): http://crbug.com/75152 Write a reference to a localized
  // resource.
  list->AddSetRegValueWorkItem(root, active_setup_path, WorkItem::kWow64Default,
                               L"Localized Name", InstallUtil::GetDisplayName(),
                               true);

  list->AddSetRegValueWorkItem(root,
                               active_setup_path,
                               WorkItem::kWow64Default,
                               L"IsInstalled",
                               1UL,
                               true);

  list->AddWorkItem(new UpdateActiveSetupVersionWorkItem(
      active_setup_path, UpdateActiveSetupVersionWorkItem::UPDATE));
}

void AppendInstallCommandLineFlags(const InstallerState& installer_state,
                                   base::CommandLine* install_cmd) {
  DCHECK(install_cmd);

  InstallUtil::AppendModeSwitch(install_cmd);
  if (installer_state.is_msi())
    install_cmd->AppendSwitch(installer::switches::kMsi);
  if (installer_state.system_install())
    install_cmd->AppendSwitch(installer::switches::kSystemLevel);
  if (installer_state.verbose_logging())
    install_cmd->AppendSwitch(installer::switches::kVerboseLogging);
}

void AppendUninstallCommandLineFlags(const InstallerState& installer_state,
                                     base::CommandLine* uninstall_cmd) {
  DCHECK(uninstall_cmd);

  uninstall_cmd->AppendSwitch(installer::switches::kUninstall);

  AppendInstallCommandLineFlags(installer_state, uninstall_cmd);
}

void AddOsUpgradeWorkItems(const InstallerState& installer_state,
                           const base::FilePath& setup_path,
                           const base::Version& new_version,
                           WorkItemList* install_list) {
  const HKEY root_key = installer_state.root_key();
  const base::string16 cmd_key(GetCommandKey(kCmdOnOsUpgrade));

  if (installer_state.operation() == InstallerState::UNINSTALL) {
    install_list->AddDeleteRegKeyWorkItem(root_key, cmd_key, KEY_WOW64_32KEY)
        ->set_log_message("Removing OS upgrade command");
  } else {
    // Register with Google Update to have setup.exe --on-os-upgrade called on
    // OS upgrade.
    base::CommandLine cmd_line(
        installer_state.GetInstallerDirectory(new_version)
            .Append(setup_path.BaseName()));
    // Add the main option to indicate OS upgrade flow.
    cmd_line.AppendSwitch(installer::switches::kOnOsUpgrade);
    InstallUtil::AppendModeSwitch(&cmd_line);
    if (installer_state.system_install())
      cmd_line.AppendSwitch(installer::switches::kSystemLevel);
    // Log everything for now.
    cmd_line.AppendSwitch(installer::switches::kVerboseLogging);

    AppCommand cmd(cmd_line.GetCommandLineString());
    cmd.set_is_auto_run_on_os_upgrade(true);
    cmd.AddWorkItems(installer_state.root_key(), cmd_key, install_list);
  }
}

}  // namespace installer
