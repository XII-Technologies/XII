#include <stdint.h>
#include <stdio.h>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  include <intrin.h>
// Wrapper functions for MSVC on x86/x64.
void cpuid(int pCpuInfo[4], int iFunctionID)
{
  __cpuid(pCpuInfo, iFunctionID);
}
void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  __cpuidex(pCpuInfo, iFunctionID, iSubFunctionID);
}
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#  include <cpuid.h>
// Wrapper functions for GCC/Clang on x86/x64.
void cpuid(int pCpuInfo[4], int iFunctionID)
{
  __cpuid(iFunctionID, pCpuInfo[0], pCpuInfo[1], pCpuInfo[2], pCpuInfo[3]);
}
void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  __cpuid_count(iFunctionID, iSubFunctionID, pCpuInfo[0], pCpuInfo[1], pCpuInfo[2], pCpuInfo[3]);
}
#else
// Non-x86 platforms: provide safe stubs so the probe compiles and runs (will report no x86 features).
void cpuid(int pCpuInfo[4], int iFunctionID)
{
  (void)iFunctionID;
  pCpuInfo[0] = 0;
  pCpuInfo[1] = 0;
  pCpuInfo[2] = 0;
  pCpuInfo[3] = 0;
}
void cpuidex(int pCpuInfo[4], int iFunctionID, int iSubFunctionID)
{
  (void)iFunctionID;
  (void)iSubFunctionID;
  pCpuInfo[0] = 0;
  pCpuInfo[1] = 0;
  pCpuInfo[2] = 0;
  pCpuInfo[3] = 0;
}
#endif

int main(void)
{
  // Array to hold CPUID data
  int cpuInfo[4];

  // Get highest supported standard CPUID function.
  cpuid(cpuInfo, 0);
  int iHighestStandardFunc = cpuInfo[0];

  // Get SIMD flags from CPUID function 1.
  cpuid(cpuInfo, 1);
  int iEdx = cpuInfo[3]; // Feature bits in EDX.
  int iEcx = cpuInfo[2]; // Feature bits in ECX.

  // Check for SIMD features in EDX.
  int bMmx  = (iEdx >> 23) & 1; // MMX: bit 23 of EDX.
  int bSse  = (iEdx >> 25) & 1; // SSE: bit 25 of EDX.
  int bSse2 = (iEdx >> 26) & 1; // SSE2: bit 26 of EDX.

  // Check for SIMD features in ECX.
  int bSse3  = (iEcx >> 0) & 1;  // SSE3: bit 0 of ECX.
  int bSsse3 = (iEcx >> 9) & 1;  // SSSE3: bit 9 of ECX.
  int bSse41 = (iEcx >> 19) & 1; // SSE4.1: bit 19 of ECX.
  int bSse42 = (iEcx >> 20) & 1; // SSE4.2: bit 20 of ECX.
  int bFma3  = (iEcx >> 12) & 1; // FMA3: bit 12 of ECX.
  int bAvx   = (iEcx >> 28) & 1; // AVX: bit 28 of ECX.

  // Print flags from CPUID function 1.
  if (bMmx)
  {
    printf("MMX\n");
  }
  if (bSse)
  {
    printf("SSE\n");
  }
  if (bSse2)
  {
    printf("SSE2\n");
  }
  if (bSse3)
  {
    printf("SSE3\n");
  }
  if (bSsse3)
  {
    printf("SSSE3\n");
  }
  if (bSse41)
  {
    printf("SSE4.1\n");
  }
  if (bSse42)
  {
    printf("SSE4.2\n");
  }
  if (bFma3)
  {
    printf("FMA3\n");
  }
  if (bAvx)
  {
    printf("AVX\n");
  }

  // If the CPU supports CPUID function 7, check for extended SIMD features.
  if (iHighestStandardFunc >= 7)
  {
    int cpuInfo7[4];
    cpuidex(cpuInfo7, 7, 0);
    int iEbx = cpuInfo7[1]; // Extended feature bits reside in EBX.

    int bAvx2       = (iEbx >> 5) & 1;  // AVX2: bit 5 of EBX.
    int bAvx512F    = (iEbx >> 16) & 1; // AVX-512 Foundation: bit 16 of EBX.
    int bAvx512DQ   = (iEbx >> 17) & 1; // AVX-512 DQ: bit 17 of EBX.
    int bAvx512IFMA = (iEbx >> 21) & 1; // AVX-512 IFMA: bit 21 of EBX.
    int bAvx512CD   = (iEbx >> 28) & 1; // AVX-512 Conflict Detection: bit 28 of EBX.
    int bAvx512BW   = (iEbx >> 30) & 1; // AVX-512 BW: bit 30 of EBX.
    int bAvx512VL   = (iEbx >> 31) & 1; // AVX-512 VL: bit 31 of EBX.

    if (bAvx2)
    {
      printf("AVX2\n");
    }
    if (bAvx512F)
    {
      printf("AVX512F\n");
    }
    if (bAvx512DQ)
    {
      printf("AVX512DQ\n");
    }
    if (bAvx512IFMA)
    {
      printf("AVX512IFMA\n");
    }
    if (bAvx512CD)
    {
      printf("AVX512CD\n");
    }
    if (bAvx512BW)
    {
      printf("AVX512BW\n");
    }
    if (bAvx512VL)
    {
      printf("AVX512VL\n");
    }
  }

  return 0;
}
