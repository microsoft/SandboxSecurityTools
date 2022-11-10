#pragma comment(lib, "advapi32.lib")

#include <vector>
#include <windows.h>

#include "../sandbox.h"

class RendererSandbox : public Sandbox {
public:
  BOOL create_restricted_sid_token(WELL_KNOWN_SID_TYPE sid_type);
  BOOL set_process_token();
  BOOL set_impersonation_process_token();
  BOOL create_child_process(wchar_t *argv[]);
};