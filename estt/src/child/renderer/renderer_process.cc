#include "renderer_process.h"

#include <stdio.h>

BOOL RendererProcess::set_mitigation_policies() {
  BOOL success = TRUE;

  {
    // By default it is already set for the processes
    PROCESS_MITIGATION_ASLR_POLICY policy = {};
    policy.EnableForceRelocateImages = true;
    policy.DisallowStrippedImages = true;
    success = success && SetProcessMitigationPolicy(ProcessASLRPolicy, &policy,
                                                    sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessASLRPolicy failed (%ld).\n",
             GetLastError());
    }
    // Ignoring Access denied
    if (GetLastError() == 5) {
      success = TRUE;
    }
  }

  {
    PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY policy = {};
    policy.DisallowWin32kSystemCalls = true;
    success =
        success && SetProcessMitigationPolicy(ProcessSystemCallDisablePolicy,
                                              &policy, sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessSystemCallDisablePolicy failed "
             "(%ld).\n",
             GetLastError());
    }
  }

  {
    PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY policy = {};
    policy.DisableExtensionPoints = true;
    success = success &&
              SetProcessMitigationPolicy(ProcessExtensionPointDisablePolicy,
                                         &policy, sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessExtensionPointDisablePolicy "
             "failed "
             "(%ld).\n",
             GetLastError());
    }
  }

  {
    PROCESS_MITIGATION_FONT_DISABLE_POLICY policy = {};
    policy.DisableNonSystemFonts = true;
    success = success && SetProcessMitigationPolicy(ProcessFontDisablePolicy,
                                                    &policy, sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessFontDisablePolicy failed "
             "(%ld).\n",
             GetLastError());
    }
  }

  {
    PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY policy = {};
    policy.MicrosoftSignedOnly = true;
    success = success && SetProcessMitigationPolicy(ProcessSignaturePolicy,
                                                    &policy, sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessSignaturePolicy failed "
             "(%ld).\n",
             GetLastError());
    }
  }

  {
    PROCESS_MITIGATION_IMAGE_LOAD_POLICY policy = {};
    policy.NoRemoteImages = true;
    policy.NoLowMandatoryLabelImages = true;
    success = success && SetProcessMitigationPolicy(ProcessImageLoadPolicy,
                                                    &policy, sizeof(policy));
    if (!success) {
      printf("SetProcessMitigationPolicy ProcessImageLoadPolicy failed "
             "(%ld).\n",
             GetLastError());
    }
  }

  return success;
}