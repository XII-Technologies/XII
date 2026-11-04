/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/System/Process.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <fcntl.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALExternalMemoryDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGALExternalMemoryDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_MEMBER_PROPERTY("Type", xiiGALExternalMemoryKind, m_Type),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiGALExternalMemoryFlags, m_Flags),
    XII_MEMBER_PROPERTY("MemoryHandle", m_uiNativeHandle),
    XII_MEMBER_PROPERTY("SemaphoreHandle", m_uiNativeSemaphoreHandle),
    XII_MEMBER_PROPERTY("Size", m_uiSize),
    XII_MEMBER_PROPERTY("ProcessID", m_uiProcessId),
    XII_MEMBER_PROPERTY("MemoryTypeIndex", m_uiMemoryTypeIndex),
    XII_MEMBER_PROPERTY("DeviceUUID", m_DeviceUUID),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiGALExternalMemoryDescription::DuplicateHandles(xiiGALExternalMemoryDescription& out_description) const
{
  xiiGALExternalMemoryDescription duplicate = *this;
  duplicate.m_uiNativeHandle                = 0;
  duplicate.m_uiNativeSemaphoreHandle       = 0;
  duplicate.m_uiProcessId                   = xiiProcess::GetCurrentProcessID();
  bool success                              = false;
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  HANDLE process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, static_cast<DWORD>(m_uiProcessId));
  if (process)
  {
    HANDLE memory    = nullptr;
    HANDLE semaphore = nullptr;
    success          = DuplicateHandle(process, reinterpret_cast<HANDLE>(m_uiNativeHandle), GetCurrentProcess(), &memory, 0, FALSE, DUPLICATE_SAME_ACCESS) &&
      DuplicateHandle(process, reinterpret_cast<HANDLE>(m_uiNativeSemaphoreHandle), GetCurrentProcess(), &semaphore, 0, FALSE, DUPLICATE_SAME_ACCESS);
    duplicate.m_uiNativeHandle          = reinterpret_cast<xiiUInt64>(memory);
    duplicate.m_uiNativeSemaphoreHandle = reinterpret_cast<xiiUInt64>(semaphore);
    CloseHandle(process);
  }
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  int memory    = -1;
  int semaphore = -1;
  if (m_uiProcessId == xiiProcess::GetCurrentProcessID())
  {
    memory    = fcntl(static_cast<int>(m_uiNativeHandle), F_DUPFD_CLOEXEC, 3);
    semaphore = fcntl(static_cast<int>(m_uiNativeSemaphoreHandle), F_DUPFD_CLOEXEC, 3);
  }
#  if defined(SYS_pidfd_open) && defined(SYS_pidfd_getfd)
  else
  {
    const int process = static_cast<int>(syscall(SYS_pidfd_open, static_cast<pid_t>(m_uiProcessId), 0));
    if (process >= 0)
    {
      memory    = static_cast<int>(syscall(SYS_pidfd_getfd, process, static_cast<int>(m_uiNativeHandle), 0));
      semaphore = static_cast<int>(syscall(SYS_pidfd_getfd, process, static_cast<int>(m_uiNativeSemaphoreHandle), 0));
      close(process);
    }
  }
#  endif
  success = memory >= 0 && semaphore >= 0;
  if (memory >= 0)
    duplicate.m_uiNativeHandle = memory;
  if (semaphore >= 0)
    duplicate.m_uiNativeSemaphoreHandle = semaphore;
#endif
  if (!success)
  {
    duplicate.CloseHandles();
    return XII_FAILURE;
  }
  out_description = duplicate;
  return XII_SUCCESS;
}

void xiiGALExternalMemoryDescription::CloseHandles()
{
  XII_ASSERT_DEV(m_uiProcessId == 0 || m_uiProcessId == xiiProcess::GetCurrentProcessID(), "Only owned local handles may be closed.");
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  if (m_uiNativeHandle)
    CloseHandle(reinterpret_cast<HANDLE>(m_uiNativeHandle));
  if (m_uiNativeSemaphoreHandle)
    CloseHandle(reinterpret_cast<HANDLE>(m_uiNativeSemaphoreHandle));
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  if (m_uiNativeHandle)
    close(static_cast<int>(m_uiNativeHandle));
  if (m_uiNativeSemaphoreHandle)
    close(static_cast<int>(m_uiNativeSemaphoreHandle));
#endif
  m_uiNativeHandle          = 0;
  m_uiNativeSemaphoreHandle = 0;
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Declarations_Implementation_ExternalMemory);
