#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Application/Implementation/Uwp/ApplicationEntryPoint_uwp.h>
#  include <roapi.h>

namespace xiiApplicationDetails
{
  xiiResult InitializeWinrt()
  {
    HRESULT result = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(result))
    {
      xiiLog::Printf("Failed to init WinRT: %i", result);
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  void UninitializeWinrt() { RoUninitialize(); }
} // namespace xiiApplicationDetails
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_uwp_ApplicationEntryPoint_uwp);
