// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See License.md in the project root for license information.

#pragma comment(lib, "advapi32.lib")

#include <vector>
#include <windows.h>

class Sandbox {
public:
  // This is verification function if integrity level is set correctly to
  // Untrusted IL or not. This function is for debugging purposes
  void verify_integrity_level_sid(HANDLE process, WELL_KNOWN_SID_TYPE sid_type);

  // To set an integrity level for a token
  BOOL set_token_integrity_level(HANDLE *token, WELL_KNOWN_SID_TYPE sid_type);

  BYTE *get_token_information_helper(HANDLE token,
                                     TOKEN_INFORMATION_CLASS info_type);
  std::vector<SID_AND_ATTRIBUTES> get_restricted_sids_list(HANDLE token);

  // creating a new restricted token
  BOOL create_restricted_token(HANDLE *token, HANDLE *new_token,
                               std::vector<SID_AND_ATTRIBUTES> &deny_sids,
                               std::vector<SID_AND_ATTRIBUTES> &restrict_sids,
                               WELL_KNOWN_SID_TYPE sid_type);
  // This functions needs to be overriden to create a restricted token as per
  // the requirement
  virtual BOOL create_restricted_sid_token(WELL_KNOWN_SID_TYPE sid_type) = 0;

  // With the token set for the process and temporarily elevation token, this
  // function can be called to create the child process from
  // create_child_process

  BOOL create_process(wchar_t *exe_file);
  // This function needs to overrriden by child class to set proper attributes
  // for the token as needed
  virtual BOOL create_child_process(wchar_t *argv[]) = 0;

protected:
  HANDLE cur_process_token_, impersonation_process_token_, restricted_token_;
  DWORD flags_ = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT |
                 DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB;
};