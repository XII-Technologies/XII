/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
// Include inline file
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/Platform/Implementation/Windows/ProcessGroup_win.h>
#  else
#    include <Foundation/Platform/Implementation/NoImpl/ProcessGroup_NoImpl.h>
#  endif

const xiiHybridArray<xiiProcess, 8>& xiiProcessGroup::GetProcesses() const
{
  return m_Processes;
}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_ProcessGroup);
