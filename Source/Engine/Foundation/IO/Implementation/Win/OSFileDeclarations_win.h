#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Avoid conflicts with windows.h
#ifdef DeleteFile
#  undef DeleteFile
#endif

#ifdef CopyFile
#  undef CopyFile
#endif

#if XII_DISABLED(XII_USE_POSIX_FILE_API)

#  include <Foundation/Basics/Platform/Windows/MinWindows.h>

struct xiiOSFileData
{
  xiiOSFileData() { m_pFileHandle = XII_WINDOWS_INVALID_HANDLE_VALUE; }

  xiiMinWindows::HANDLE m_pFileHandle;
};

struct xiiFileIterationData
{
  xiiHybridArray<xiiMinWindows::HANDLE, 16> m_Handles;
};

#endif

/// \endcond
