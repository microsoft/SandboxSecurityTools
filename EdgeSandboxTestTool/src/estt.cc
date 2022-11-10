#include "sandbox/renderer/renderer.h"

BOOL wmain(int argc, wchar_t *argv[], wchar_t *) {
  if (argc != 2) {
    printf("Usage: %ls [cmdline]\n", argv[0]);
    return FALSE;
  }

  RendererSandbox process;
  Sandbox *sandbox_process = &process;
  sandbox_process->create_child_process(argv);

  return TRUE;
}