// Deactivate Doxygen document generation for the following block.
/// \cond

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

#include <Foundation/Strings/String.h>

// Helper function to detect a 64-bit Windows
bool Is64BitWindows()
{
#if defined(_WIN64)
  return true; // 64-bit programs run only on Win64 (although if we get to Win128 this will be wrong probably)
#elif defined(_WIN32)
  // 32-bit programs run on both 32-bit and 64-bit Windows
  // Note that we used IsWow64Process before which is not available on UWP.
  SYSTEM_INFO info;
  GetNativeSystemInfo(&info);
  // According to documentation: "The processor architecture of the installed operating system."
  return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#else
  return false; // Win64 does not support Win16
#endif
}

/// \endcond

bool xiiSystemInformation::IsDebuggerAttached()
{
  return ::IsDebuggerPresent();
}

void xiiSystemInformation::Initialize()
{
  if (s_SystemInformation.m_bIsInitialized)
    return;

  s_SystemInformation.m_CpuFeatures.Detect();

  s_SystemInformation.m_sHostName[0] = '\0';

  // Retrieve system information via various APIs
  SYSTEM_INFO sysInfo;
  ZeroMemory(&sysInfo, sizeof(sysInfo));
  GetNativeSystemInfo(&sysInfo);

  s_SystemInformation.m_uiCPUCoreCount   = sysInfo.dwNumberOfProcessors;
  s_SystemInformation.m_uiMemoryPageSize = sysInfo.dwPageSize;

  MEMORYSTATUSEX memStatus;
  ZeroMemory(&memStatus, sizeof(memStatus));
  memStatus.dwLength = sizeof(memStatus);
  GlobalMemoryStatusEx(&memStatus);

  s_SystemInformation.m_uiInstalledMainMemory = memStatus.ullTotalPhys;
  s_SystemInformation.m_bB64BitOS             = Is64BitWindows();
  s_SystemInformation.m_szPlatformName        = "Windows - Desktop";

#if defined BUILDSYSTEM_BUILDTYPE
  s_SystemInformation.m_szBuildConfiguration = BUILDSYSTEM_BUILDTYPE;
#else
  s_SystemInformation.m_szBuildConfiguration = "Undefined";
#endif

  // Retrieve host name
  DWORD bufCharCount = sizeof(s_SystemInformation.m_sHostName);
  GetComputerNameA(s_SystemInformation.m_sHostName, &bufCharCount);

  s_SystemInformation.m_bIsInitialized = true;
}

xiiUInt64 xiiSystemInformation::GetAvailableMainMemory() const
{
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);
  GlobalMemoryStatusEx(&statex);

  return statex.ullAvailPhys;
}

float xiiSystemInformation::GetCPUUtilization() const
{
  LARGE_INTEGER kernel, user, idle;
  GetSystemTimes((FILETIME*)&idle, (FILETIME*)&kernel, (FILETIME*)&user);

  static thread_local uint64_t lastKernel = 0u, lastIdle = 0u, lastUser = 0u;

  auto kernelTime = kernel.QuadPart - lastKernel;
  auto idleTime   = idle.QuadPart - lastIdle;
  auto userTime   = user.QuadPart - lastUser;

  lastKernel = kernel.QuadPart;
  lastUser   = user.QuadPart;
  lastIdle   = idle.QuadPart;

  auto util = static_cast<float>(kernelTime + userTime - idleTime) / (kernelTime + userTime);

  return xiiMath::Clamp(util, 0.f, 1.f) * 100.f;
}
