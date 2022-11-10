// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See License.md in the project root for license information.

#pragma comment(lib, "advapi32.lib")

#include <windows.h>

class Child {
public:
  // Set all the mitigations for the child process. It is to set the same
  // mitigation policies as it can be seen in PowerShell Get-ProcessMitigation
  // function.
  virtual BOOL set_mitigation_policies() = 0;
};