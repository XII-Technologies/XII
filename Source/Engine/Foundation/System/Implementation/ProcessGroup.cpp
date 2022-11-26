#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
// Include inline file
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/System/Implementation/Win/ProcessGroup_win.h>
#  else
#    include <Foundation/System/Implementation/other/ProcessGroup_other.h>
#  endif

const xiiHybridArray<xiiProcess, 8>& xiiProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_ProcessGroup);
