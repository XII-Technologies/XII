#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct xiiOSFileData
{
  xiiOSFileData() { m_pFileHandle = nullptr; }

  FILE* m_pFileHandle;
};

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

struct xiiFileIterationData
{
  // This is storing DIR*, which we can't forward declare
  xiiHybridArray<void*, 16> m_Handles;
  xiiString                 m_wildcardSearch;
};

#endif

/// \endcond
