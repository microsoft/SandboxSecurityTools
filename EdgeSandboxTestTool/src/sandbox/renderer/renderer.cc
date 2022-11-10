// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See License.md in the project root for license information.

#include "renderer.h"

#include <stdio.h>

BOOL RendererSandbox::set_process_token() {
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS,
                        &cur_process_token_)) {
    printf("OpenProcessToken failed (%ld).\n", GetLastError());
    return FALSE;
  }
  return TRUE;
}

BOOL RendererSandbox::create_restricted_sid_token(
    WELL_KNOWN_SID_TYPE sid_type) {
  PSID psd = NULL;
  DWORD cbSid = SECURITY_MAX_SID_SIZE;
  psd = malloc(SECURITY_MAX_SID_SIZE);

  if (!CreateWellKnownSid(sid_type, NULL, psd, &cbSid)) {
    printf("CreateWellKnownSid failed (%ld).\n", GetLastError());
    return FALSE;
  }

  SID_AND_ATTRIBUTES sid_to_restrict;
  sid_to_restrict.Attributes = 0;
  sid_to_restrict.Sid = psd;

  std::vector<SID_AND_ATTRIBUTES> restricted_sids;
  restricted_sids.push_back(sid_to_restrict);

  std::vector<SID_AND_ATTRIBUTES> deny_sids =
      get_restricted_sids_list(cur_process_token_);

  if (!create_restricted_token(&cur_process_token_, &restricted_token_,
                               deny_sids, restricted_sids,
                               WinUntrustedLabelSid)) {
    return FALSE;
  }

  return TRUE;
}

BOOL RendererSandbox::set_impersonation_process_token() {
  HANDLE imp_restrict_token;
  std::vector<SID_AND_ATTRIBUTES> deny_sids;

  std::vector<SID_AND_ATTRIBUTES> restricted_sids =
      get_restricted_sids_list(cur_process_token_);

  if (!create_restricted_token(&cur_process_token_, &imp_restrict_token,
                               deny_sids, restricted_sids,
                               WinUntrustedLabelSid)) {
    return FALSE;
  }
  if (!DuplicateTokenEx(imp_restrict_token, TOKEN_ALL_ACCESS, NULL,
                        SecurityImpersonation, TokenImpersonation,
                        &impersonation_process_token_)) {
    printf("DuplicateTokenEx TokenImpersonation failed (%ld).\n",
           GetLastError());
    return FALSE;
  }
  return TRUE;
}

BOOL RendererSandbox::create_child_process(wchar_t *argv[]) {
  if (!(set_process_token() && set_impersonation_process_token())) {
    return FALSE;
  }

  // create null sid restricted token
  create_restricted_sid_token(WinNullSid);

  return create_process(argv[1]);
}