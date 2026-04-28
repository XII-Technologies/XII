/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch xiiRunnable instances
void* xiiThreadClassEntryPoint(void* pThreadParameter)
{
  XII_ASSERT_RELEASE(pThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  xiiThread* pThread = reinterpret_cast<xiiThread*>(pThreadParameter);

  RunThread(pThread);

  return nullptr;
}

/// \endcond
