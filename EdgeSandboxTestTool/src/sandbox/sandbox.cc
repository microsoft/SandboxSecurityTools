// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See License.md in the project root for license information.

#include "sandbox.h"

#include <stdio.h>

void Sandbox::verify_integrity_level_sid(HANDLE process,
                                         WELL_KNOWN_SID_TYPE sid_type) {
  HANDLE token;
  if (!OpenProcessToken(process, TOKEN_QUERY, &token)) {
    printf("OpenProcessToken failed (%ld).\n", GetLastError());
  }

  BYTE *buffer = get_token_information_helper(token, TokenIntegrityLevel);

  if (IsWellKnownSid(((TOKEN_MANDATORY_LABEL *)buffer)->Label.Sid, sid_type)) {
    printf("Untrusted SID IL\n");
  } else {
    printf("Other SID IL\n");
  }
}

BOOL Sandbox::set_token_integrity_level(HANDLE *token,
                                        WELL_KNOWN_SID_TYPE sid_type) {
  PSID psd = NULL;
  DWORD cbSid = SECURITY_MAX_SID_SIZE;
  psd = malloc(SECURITY_MAX_SID_SIZE);

  if (!CreateWellKnownSid(sid_type, NULL, psd, &cbSid)) {
    printf("CreateWellKnownSid failed (%ld).\n", GetLastError());
    return FALSE;
  }

  TOKEN_MANDATORY_LABEL tml;
  ZeroMemory(&tml, sizeof(tml));
  tml.Label.Attributes = SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED;
  tml.Label.Sid = psd;

  if (!SetTokenInformation(*token, TokenIntegrityLevel, &tml,
                           sizeof(tml) + GetLengthSid(psd))) {
    printf("SetTokenInformation TokenIntegrityLevel failed (%ld).\n",
           GetLastError());
    return FALSE;
  }
  free(psd);

  return TRUE;
}

BYTE *Sandbox::get_token_information_helper(HANDLE token,
                                            TOKEN_INFORMATION_CLASS info_type) {
  // Get the size required for token information.
  DWORD size = 0;
  if (GetTokenInformation(token, info_type, nullptr, 0, &size) || !size) {
    printf("GetTokenInformationHelper failed to get data size %ld\n",
           GetLastError());
    return {};
  }

  BYTE *buffer = (BYTE *)malloc(size);
  if (buffer == nullptr)
    return nullptr;

  memset(buffer, 0, size);

  // Get the token information.
  if (!GetTokenInformation(token, info_type, buffer, size, &size)) {
    printf("GetTokenInformationHelper failed to get data %ld\n",
           GetLastError());
    return {};
  }

  return buffer;
}

std::vector<SID_AND_ATTRIBUTES>
Sandbox::get_restricted_sids_list(HANDLE token) {
  std::vector<SID_AND_ATTRIBUTES> restricted_sids;

  // Get the token user information.
  BYTE *token_user_data = get_token_information_helper(token, TokenUser);
  TOKEN_USER *token_user = (TOKEN_USER *)token_user_data;

  // Add the current user's SID to the list.
  restricted_sids.push_back({token_user->User.Sid, 0});

  // Get the list of token groups.
  BYTE *token_group_data = get_token_information_helper(token, TokenGroups);

  // Loop and build the restricted sids group list.
  TOKEN_GROUPS *groups = (TOKEN_GROUPS *)token_group_data;
  for (DWORD i = 0; i < groups->GroupCount; i++) {
    // Skip integrity sids.
    if ((groups->Groups[i].Attributes & SE_GROUP_INTEGRITY) != 0 &&
        (groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) != 0)
      continue;

    restricted_sids.push_back({groups->Groups[i].Sid, 0});
  }

  return restricted_sids;
}

BOOL Sandbox::create_restricted_token(
    HANDLE *token, HANDLE *new_token, std::vector<SID_AND_ATTRIBUTES> &denySids,
    std::vector<SID_AND_ATTRIBUTES> &restrictSids,
    WELL_KNOWN_SID_TYPE sid_type) {
  HANDLE restricted_token;
  if (!CreateRestrictedToken(*token, DISABLE_MAX_PRIVILEGE, denySids.size(),
                             denySids.data(), 0, NULL, restrictSids.size(),
                             restrictSids.data(), &restricted_token)) {
    printf("CreateRestrictedToken failed (%ld).\n", GetLastError());
    return FALSE;
  }

  if (!set_token_integrity_level(&restricted_token, sid_type)) {
    return FALSE;
  }

  if (!DuplicateHandle(::GetCurrentProcess(), restricted_token,
                       ::GetCurrentProcess(), new_token, TOKEN_ALL_ACCESS,
                       false, // Don't inherit.
                       0)) {
    printf("DuplicateHandle failed (%ld).\n", GetLastError());
    return FALSE;
  }

  return TRUE;
}

BOOL Sandbox::create_process(wchar_t *exe_file) {
  STARTUPINFOW startup_info;
  PROCESS_INFORMATION process_info;

  ZeroMemory(&startup_info, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  ZeroMemory(&process_info, sizeof(process_info));

  wprintf(L"Creating child process for %s\n", exe_file);
  if (!CreateProcessAsUserW(restricted_token_, exe_file, NULL, NULL, NULL, TRUE,
                            flags_, NULL, NULL, &startup_info, &process_info)) {
    printf("CreateProcess failed (%ld).\n", GetLastError());
    return FALSE;
  }
  printf("Process created with ID: %ld\n", process_info.dwProcessId);

  verify_integrity_level_sid(process_info.hProcess, WinUntrustedLabelSid);

  // Temporarily elevating priveleges of child process
  if (!SetThreadToken(&(process_info.hThread), impersonation_process_token_)) {
    printf("SetThreadToken failed (%ld).\n", GetLastError());
    return FALSE;
  }

  // Resuming thread execution
  if (!ResumeThread(process_info.hThread)) {
    printf("ResumeThread failed (%ld).\n", GetLastError());
    return FALSE;
  }

  // Wait until child process exits.
  WaitForSingleObject(process_info.hProcess, INFINITE);

  DWORD exit_code;
  if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
    printf("GetExitCodeProcess failed (%ld).\n", GetLastError());
  }
  printf("ExitCodeProcess (%ld).\n", exit_code);

  // Close process and thread handles.
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
  CloseHandle(cur_process_token_);

  return TRUE;
}