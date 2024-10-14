#pragma once

// Deactivate Doxygen document generation for the following block.
/// \cond

#include <Foundation/Basics/Platform/Windows/MinWindows.h>

#if XII_ENABLED(XII_PLATFORM_32BIT)
struct alignas(4) xiiMutexHandle
{
  xiiUInt8 data[24];
};
#else
struct alignas(8) xiiMutexHandle
{
  xiiUInt8 data[40];
};
#endif


#if XII_ENABLED(XII_PLATFORM_32BIT)
struct alignas(4) xiiConditionVariableHandle
{
  xiiUInt8 data[4];
};
#else
struct alignas(8) xiiConditionVariableHandle
{
  xiiUInt8 data[8];
};
#endif



using xiiThreadHandle       = xiiMinWindows::HANDLE;
using xiiThreadID           = xiiMinWindows::DWORD;
using xiiOSThreadEntryPoint = xiiMinWindows::DWORD(__stdcall*)(void* lpThreadParameter);
using xiiSemaphoreHandle    = xiiMinWindows::HANDLE;

#define XII_THREAD_CLASS_ENTRY_POINT xiiMinWindows::DWORD __stdcall xiiThreadClassEntryPoint(void* lpThreadParameter);

struct xiiConditionVariableData
{
  xiiConditionVariableHandle m_ConditionVariable;
};

/// \endcond
