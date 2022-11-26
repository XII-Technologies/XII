#ifdef XII_MUTEX_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define XII_MUTEX_WIN_INL_H_INCLUDED

#if XII_ENABLED(XII_COMPILER_MSVC) && XII_ENABLED(XII_PLATFORM_ARCH_X86)

extern "C"
{
  // The main purpose of this little hack here is to have Mutex::Lock and Mutex::Unlock inline-able without including windows.h
  // The hack however does only work on the MSVC compiler. See fall back code below.

  // First define two functions which are binary compatible with EnterCriticalSection and LeaveCriticalSection
  __declspec(dllimport) void __stdcall xiiWinEnterCriticalSection(xiiMutexHandle* handle);
  __declspec(dllimport) void __stdcall xiiWinLeaveCriticalSection(xiiMutexHandle* handle);
  __declspec(dllimport) xiiMinWindows::BOOL __stdcall xiiWinTryEnterCriticalSection(xiiMutexHandle* handle);

  // Now redirect them through linker flags to the correct implementation
#  if XII_ENABLED(XII_PLATFORM_32BIT)
#    pragma comment(linker, "/alternatename:__imp__xiiWinEnterCriticalSection@4=__imp__EnterCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__xiiWinLeaveCriticalSection@4=__imp__LeaveCriticalSection@4")
#    pragma comment(linker, "/alternatename:__imp__xiiWinTryEnterCriticalSection@4=__imp__TryEnterCriticalSection@4")
#  else
#    pragma comment(linker, "/alternatename:__imp_xiiWinEnterCriticalSection=__imp_EnterCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_xiiWinLeaveCriticalSection=__imp_LeaveCriticalSection")
#    pragma comment(linker, "/alternatename:__imp_xiiWinTryEnterCriticalSection=__imp_TryEnterCriticalSection")
#  endif
}

inline void xiiMutex::Lock()
{
  xiiWinEnterCriticalSection(&m_hHandle);
  ++m_iLockCount;
}

inline void xiiMutex::Unlock()
{
  --m_iLockCount;
  xiiWinLeaveCriticalSection(&m_hHandle);
}

inline xiiResult xiiMutex::TryLock()
{
  if (xiiWinTryEnterCriticalSection(&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

#else

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

inline void xiiMutex::Lock()
{
  EnterCriticalSection((CRITICAL_SECTION*)&m_hHandle);
  ++m_iLockCount;
}

inline void xiiMutex::Unlock()
{
  --m_iLockCount;
  LeaveCriticalSection((CRITICAL_SECTION*)&m_hHandle);
}

inline xiiResult xiiMutex::TryLock()
{
  if (TryEnterCriticalSection((CRITICAL_SECTION*)&m_hHandle) != 0)
  {
    ++m_iLockCount;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

#endif
