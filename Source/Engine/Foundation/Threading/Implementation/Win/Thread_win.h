#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

// Deactivate Doxygen document generation for the following block.
/// \cond

// Thread entry point used to launch xiiRunnable instances
DWORD __stdcall xiiThreadClassEntryPoint(LPVOID lpThreadParameter)
{
  XII_ASSERT_RELEASE(lpThreadParameter != nullptr, "thread parameter in thread entry point must not be nullptr!");

  xiiThread* pThread = reinterpret_cast<xiiThread*>(lpThreadParameter);

  return RunThread(pThread);
}


/// \endcond
