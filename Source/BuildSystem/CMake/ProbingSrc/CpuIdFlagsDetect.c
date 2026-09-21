/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <stdint.h>
#include <stdio.h>
#if defined(__APPLE__)
#  include <sys/sysctl.h>
#  include <sys/types.h>
#endif

#if defined(__linux__)
#  include <fcntl.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <sys/auxv.h>
#  include <unistd.h>
#endif
#if defined(_WIN32)
#  include <windows.h>
#endif

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  include <intrin.h>
// Wrapper functions for MSVC on x86/x64.
#  define HAVE_X86_CPUID 1
static void cpuid(int pCpuInfo[4], int iFunctionID)
{
  __cpuid(pCpuInfo, iFunctionID);
}
static void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  __cpuidex(pCpuInfo, iFunctionID, iSubFunctionID);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#  include <cpuid.h>
// Wrapper functions for GCC/Clang on x86/x64.
#  define HAVE_X86_CPUID 1
static void cpuid(int pCpuInfo[4], int iFunctionID)
{
  __cpuid(iFunctionID, pCpuInfo[0], pCpuInfo[1], pCpuInfo[2], pCpuInfo[3]);
}
static void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  __cpuid_count(iFunctionID, iSubFunctionID, pCpuInfo[0], pCpuInfo[1], pCpuInfo[2], pCpuInfo[3]);
}
#else
// Non-x86 platforms: provide safe stubs so the probe compiles and runs (will report no x86 features).
static void cpuid(int pCpuInfo[4], int iFunctionID)
{
  (void)iFunctionID;
  pCpuInfo[0] = 0;
  pCpuInfo[1] = 0;
  pCpuInfo[2] = 0;
  pCpuInfo[3] = 0;
}
static void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  (void)iFunctionID;
  (void)iSubFunctionID;
  pCpuInfo[0] = 0;
  pCpuInfo[1] = 0;
  pCpuInfo[2] = 0;
  pCpuInfo[3] = 0;
}
#endif

/// Helper: on Apple platforms, query sysctl boolean values for CPU features.
#if defined(__APPLE__)
static int sysctl_bool(const char* szName)
{
  int    iValue   = 0;
  size_t uiLength = sizeof(iValue);
  if (sysctlbyname(szName, &iValue, &uiLength, NULL, 0) == 0)
    return iValue != 0;
  return 0;
}

static void print_apple_arm_features(void)
{
  // Check common Apple/ARM feature keys. Some keys differ across OS versions; try several.
  int bHasNeon = sysctl_bool("hw.optional.neon") || sysctl_bool("hw.optional.asimd");
  int bHasAes  = sysctl_bool("hw.optional.aes") || sysctl_bool("hw.optional.arm.FEAT_AES");
  int bHasSha1 = sysctl_bool("hw.optional.sha1") || sysctl_bool("hw.optional.arm.FEAT_SHA1");
  int bHasSha2 = sysctl_bool("hw.optional.sha2") || sysctl_bool("hw.optional.arm.FEAT_SHA2");
  int bHasCrc  = sysctl_bool("hw.optional.crc32") || sysctl_bool("hw.optional.arm.FEAT_CRC32");

  if (bHasNeon)
    printf("NEON\n");
  if (bHasAes)
    printf("AES\n");
  if (bHasSha1)
    printf("SHA1\n");
  if (bHasSha2)
    printf("SHA2\n");
  if (bHasCrc)
    printf("CRC32\n");
}
#endif

/// Generic ARM detection for Linux and Windows.
static void print_generic_arm_features(void)
{
#if defined(__linux__)
  unsigned long uiHwCap = 0;
#  ifdef AT_HWCAP
  uiHwCap = getauxval(AT_HWCAP);
#  endif

#  if defined(HWCAP_ASIMD) || defined(HWCAP_NEON)
  // NEON/ASIMD
#    if defined(HWCAP_ASIMD)
  if (uiHwCap & HWCAP_ASIMD)
#    elif defined(HWCAP_NEON)
  if (uiHwCap & HWCAP_NEON)
#    endif
    printf("NEON\n");
#  endif

#  if defined(HWCAP_AES)
  if (uiHwCap & HWCAP_AES) printf("AES\n");
#  endif
#  if defined(HWCAP_SHA1)
  if (uiHwCap & HWCAP_SHA1) printf("SHA1\n");
#  endif
#  if defined(HWCAP_SHA2)
  if (uiHwCap & HWCAP_SHA2) printf("SHA2\n");
#  endif
#  if defined(HWCAP_CRC32)
  if (uiHwCap & HWCAP_CRC32) printf("CRC32\n");
#  endif

  // Fallback: parse /proc/cpuinfo for features if getauxval didn't provide them
  FILE* f = fopen("/proc/cpuinfo", "r");
  if (f)
  {
    char line[512];
    while (fgets(line, sizeof(line), f))
    {
      if (strstr(line, "Features") || strstr(line, "flags"))
      {
        if (strstr(line, "asimd") || strstr(line, "neon")) printf("NEON\n");
        if (strstr(line, "aes")) printf("AES\n");
        if (strstr(line, "sha1")) printf("SHA1\n");
        if (strstr(line, "sha2")) printf("SHA2\n");
        if (strstr(line, "crc32")) printf("CRC32\n");
        break;
      }
    }
    fclose(f);
  }
#elif defined(_WIN32)
  // Windows on ARM: attempt to use IsProcessorFeaturePresent if those flags are available at compile time.
  // Only use the API if PF_ARM_NEON_INSTRUCTIONS_AVAILABLE is defined by the platform headers.
#  ifdef PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
  if (IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE)) printf("NEON\n");
#  endif
#  ifdef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
  if (IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE)) printf("AES\n");
#  endif
  // If runtime detection isn't available, we intentionally do not guess.
#else
  (void)0; // Other platforms: nothing to report.
#endif
}
#if defined(HAVE_X86_CPUID)
/// Extract x86/x64 feature detection and printing into its own function.
static void print_x86_features(void)
{
  int cpuInfo[4];

  // Highest supported standard CPUID function.
  cpuid(cpuInfo, 0);
  int iHighestStandardFunc = cpuInfo[0];

  // SIMD flags from CPUID function 1.
  cpuid(cpuInfo, 1);
  int iEdx = cpuInfo[3];
  int iEcx = cpuInfo[2];

  int bMmx  = (iEdx >> 23) & 1;
  int bSse  = (iEdx >> 25) & 1;
  int bSse2 = (iEdx >> 26) & 1;

  int bSse3  = (iEcx >> 0) & 1;
  int bSsse3 = (iEcx >> 9) & 1;
  int bSse41 = (iEcx >> 19) & 1;
  int bSse42 = (iEcx >> 20) & 1;
  int bFma3  = (iEcx >> 12) & 1;
  int bAvx   = (iEcx >> 28) & 1;

  if (bMmx) printf("MMX\n");
  if (bSse) printf("SSE\n");
  if (bSse2) printf("SSE2\n");
  if (bSse3) printf("SSE3\n");
  if (bSsse3) printf("SSSE3\n");
  if (bSse41) printf("SSE4.1\n");
  if (bSse42) printf("SSE4.2\n");
  if (bFma3) printf("FMA3\n");
  if (bAvx) printf("AVX\n");

  if (iHighestStandardFunc >= 7)
  {
    int cpuInfo7[4];
    cpuidex(cpuInfo7, 7, 0);
    int iEbx = cpuInfo7[1];

    if ((iEbx >> 5) & 1) printf("AVX2\n");
    if ((iEbx >> 16) & 1) printf("AVX512F\n");
    if ((iEbx >> 17) & 1) printf("AVX512DQ\n");
    if ((iEbx >> 21) & 1) printf("AVX512IFMA\n");
    if ((iEbx >> 28) & 1) printf("AVX512CD\n");
    if ((iEbx >> 30) & 1) printf("AVX512BW\n");
    if ((iEbx >> 31) & 1) printf("AVX512VL\n");
  }
}
#endif

int main(void)
{
#if defined(HAVE_X86_CPUID)
  print_x86_features();
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
  print_apple_arm_features();
#elif defined(__aarch64__) || defined(__arm64__)
  print_generic_arm_features();
#else
  /// No CPU feature detection available on this platform.
#endif
  return 0;
}
