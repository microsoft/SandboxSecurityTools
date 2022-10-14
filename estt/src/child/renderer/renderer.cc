#include "renderer_process.h"

#include <stdio.h>

// This is a custom function which researchers can edit to make their exploit
void custom() {
  // replace while and write your code here
  while (1)
    ;
}

int main() {
  // Set renderer process like mitigations.
  RendererProcess renderer_process;
  if (!renderer_process.set_mitigation_policies()) {
    printf("set_mitigation_policies failed.\n");
    return 0;
  }

  // Revert the process to use the original token
  if (!RevertToSelf()) {
    printf("RevertToSelf failed (%ld).\n", GetLastError());
    return 0;
  }

  custom();

  return 1;
}