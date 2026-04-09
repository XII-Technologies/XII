#pragma once

namespace xiiInternal
{
  constexpr xiiUInt32 PRIME32_1 = 0x9E3779B1U;
  constexpr xiiUInt32 PRIME32_2 = 0x85EBCA77U;
  constexpr xiiUInt32 PRIME32_3 = 0xC2B2AE3DU;
  constexpr xiiUInt32 PRIME32_4 = 0x27D4EB2FU;
  constexpr xiiUInt32 PRIME32_5 = 0x165667B1U;

  constexpr xiiUInt64 PRIME64_1 = 0x9E3779B185EBCA87ULL;
  constexpr xiiUInt64 PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
  constexpr xiiUInt64 PRIME64_3 = 0x165667B19E3779F9ULL;
  constexpr xiiUInt64 PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
  constexpr xiiUInt64 PRIME64_5 = 0x27D4EB2F165667C5ULL;

  constexpr xiiUInt32 xiiRotLeft(xiiUInt32 value, xiiUInt32 uiAmount) { return (value << uiAmount) | (value >> (32 - uiAmount)); }
  constexpr xiiUInt64 xiiRotLeft(xiiUInt64 value, xiiUInt64 uiAmount) { return (value << uiAmount) | (value >> (64 - uiAmount)); }

  template <size_t N>
  constexpr xiiUInt32 CompileTimeXxHash32(const char (&str)[N], xiiUInt32 uiSeed)
  {
    // Note: N will contain the trailing 0 of a string literal. This needs to be ignored.
    constexpr xiiUInt32 uiLength = static_cast<xiiUInt32>(N - 1);
    if constexpr (uiLength == 0)
    {
      return 46947589u;
    }
    else
    {
      xiiUInt32 uiAccumulator = 0;
      xiiUInt32 uiIndex       = 0;
      // Perform simple initialization if N < 16
      if constexpr (uiLength < 16)
      {
        uiAccumulator = uiSeed + PRIME32_5;
      }
      else
      {
        xiiUInt32 pAccumulators[4] = {uiSeed + PRIME32_1 + PRIME32_2, uiSeed + PRIME32_2, uiSeed, uiSeed - PRIME32_1};
        for (; uiLength - uiIndex >= 16; uiIndex += 16)
        {
          for (xiiInt32 i = 0; i < 4; ++i)
          {
            xiiUInt32 uiLaneN = (static_cast<xiiUInt32>(str[uiIndex + i * 4 + 0]) << 0) | (static_cast<xiiUInt32>(str[uiIndex + i * 4 + 1]) << 8) |
              (static_cast<xiiUInt32>(str[uiIndex + i * 4 + 2]) << 16) | (static_cast<xiiUInt32>(str[uiIndex + i * 4 + 3]) << 24);
            pAccumulators[i] = pAccumulators[i] + (uiLaneN * PRIME32_2);
            pAccumulators[i] = xiiRotLeft(pAccumulators[i], 13);
            pAccumulators[i] = pAccumulators[i] * PRIME32_1;
          }
        }
        uiAccumulator = xiiRotLeft(pAccumulators[0], 1) + xiiRotLeft(pAccumulators[1], 7) + xiiRotLeft(pAccumulators[2], 12) + xiiRotLeft(pAccumulators[3], 18);
      }

      // Step 4
      uiAccumulator = uiAccumulator + uiLength;

      // Step 5
      for (; uiLength - uiIndex >= 4; uiIndex += 4)
      {
        xiiUInt32 uiLane = (static_cast<xiiUInt32>(str[uiIndex + 0]) << 0) | (static_cast<xiiUInt32>(str[uiIndex + 1]) << 8) |
          (static_cast<xiiUInt32>(str[uiIndex + 2]) << 16) | (static_cast<xiiUInt32>(str[uiIndex + 3]) << 24);
        uiAccumulator = uiAccumulator + uiLane * PRIME32_3;
        uiAccumulator = xiiRotLeft(uiAccumulator, 17) * PRIME32_4;
      }

      for (; uiLength - uiIndex >= 1; ++uiIndex)
      {
        xiiUInt32 uiLane = static_cast<xiiUInt32>(str[uiIndex]);
        uiAccumulator    = uiAccumulator + uiLane * PRIME32_5;
        uiAccumulator    = xiiRotLeft(uiAccumulator, 11) * PRIME32_1;
      }

      // Step 6
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 15);
      uiAccumulator = uiAccumulator * PRIME32_2;
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 13);
      uiAccumulator = uiAccumulator * PRIME32_3;
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 16);

      return uiAccumulator;
    }
  }

  template <size_t N>
  constexpr xiiUInt64 CompileTimeXxHash64(const char (&str)[N], xiiUInt64 uiSeed)
  {
    // Note: N will contain the trailing 0 of a string literal. This needs to be ignored.
    constexpr xiiUInt32 uiLength = static_cast<xiiUInt32>(N - 1);
    if constexpr (uiLength == 0)
    {
      return 17241709254077376921llu;
    }
    else
    {
      xiiUInt64 uiAccumulator = 0;
      xiiUInt32 uiIndex       = 0;

      // Step 1
      if constexpr (uiLength < 32)
      {
        // simple initialization
        uiAccumulator = uiSeed + PRIME64_5;
      }
      else
      {
        xiiUInt64 pAccumulators[] = {uiSeed + PRIME64_1 + PRIME64_2, uiSeed + PRIME64_2, uiSeed + 0, uiSeed - PRIME64_1};
        // Step 2
        for (; uiLength - uiIndex >= 32; uiIndex += 32)
        {
          for (xiiInt32 i = 0; i < 4; ++i)
          {
            xiiUInt64 uiLaneN = (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 0]) << 0) | (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 1]) << 8) |
              (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 2]) << 16) | (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 3]) << 24) |
              (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 4]) << 32) | (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 5]) << 40) |
              (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 6]) << 48) | (static_cast<xiiUInt64>(str[uiIndex + i * 8 + 7]) << 56);
            pAccumulators[i] = pAccumulators[i] + (uiLaneN * PRIME64_2);
            pAccumulators[i] = xiiRotLeft(pAccumulators[i], 31ULL);
            pAccumulators[i] = pAccumulators[i] * PRIME64_1;
          }
        }

        // Step 3
        uiAccumulator = xiiRotLeft(pAccumulators[0], 1ULL) + xiiRotLeft(pAccumulators[1], 7ULL) + xiiRotLeft(pAccumulators[2], 12ULL) + xiiRotLeft(pAccumulators[3], 18ULL);
        for (xiiInt32 i = 0; i < 4; ++i)
        {
          uiAccumulator = (uiAccumulator ^ (xiiRotLeft(pAccumulators[i] * PRIME64_2, 31ULL) * PRIME64_1)) * PRIME64_1 + PRIME64_4;
        }
      }
      // Step 4
      uiAccumulator += uiLength;

      // Step 5
      for (; uiLength - uiIndex >= 8; uiIndex += 8)
      {
        xiiUInt64 uiLane = (static_cast<xiiUInt64>(str[uiIndex + 0]) << 0) | (static_cast<xiiUInt64>(str[uiIndex + 1]) << 8) |
          (static_cast<xiiUInt64>(str[uiIndex + 2]) << 16) | (static_cast<xiiUInt64>(str[uiIndex + 3]) << 24) |
          (static_cast<xiiUInt64>(str[uiIndex + 4]) << 32) | (static_cast<xiiUInt64>(str[uiIndex + 5]) << 40) |
          (static_cast<xiiUInt64>(str[uiIndex + 6]) << 48) | (static_cast<xiiUInt64>(str[uiIndex + 7]) << 56);
        uiAccumulator = uiAccumulator ^ (xiiRotLeft(uiLane * PRIME64_2, 31ULL) * PRIME64_1);
        uiAccumulator = xiiRotLeft(uiAccumulator, 27ULL) * PRIME64_1;
        uiAccumulator += PRIME64_4;
      }

      for (; uiLength - uiIndex >= 4; uiIndex += 4)
      {
        xiiUInt64 uiLane = (static_cast<xiiUInt64>(str[uiIndex + 0]) << 0) | (static_cast<xiiUInt64>(str[uiIndex + 1]) << 8) |
          (static_cast<xiiUInt64>(str[uiIndex + 2]) << 16) | (static_cast<xiiUInt64>(str[uiIndex + 3]) << 24);
        uiAccumulator = uiAccumulator ^ (uiLane * PRIME64_1);
        uiAccumulator = xiiRotLeft(uiAccumulator, 23ULL) * PRIME64_2;
        uiAccumulator += PRIME64_3;
      }

      for (; uiLength - uiIndex >= 1; ++uiIndex)
      {
        xiiUInt64 uiLane = static_cast<xiiUInt64>(str[uiIndex]);
        uiAccumulator    = uiAccumulator ^ (uiLane * PRIME64_5);
        uiAccumulator    = xiiRotLeft(uiAccumulator, 11ULL) * PRIME64_1;
      }

      // Step 6
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 33);
      uiAccumulator = uiAccumulator * PRIME64_2;
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 29);
      uiAccumulator = uiAccumulator * PRIME64_3;
      uiAccumulator = uiAccumulator ^ (uiAccumulator >> 32);

      return uiAccumulator;
    }
  }
} // namespace xiiInternal

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::xxHash32String(const char (&str)[N], xiiUInt32 uiSeed)
{
  return xiiInternal::CompileTimeXxHash32(str, uiSeed);
}

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiUInt64 xiiHashingUtils::xxHash64String(const char (&str)[N], xiiUInt64 uiSeed)
{
  return xiiInternal::CompileTimeXxHash64(str, uiSeed);
}

XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::xxHash32String(xiiStringView sStr, xiiUInt32 uiSeed)
{
  return xxHash32(sStr.GetStartPointer(), sStr.GetElementCount(), uiSeed);
}

XII_ALWAYS_INLINE xiiUInt64 xiiHashingUtils::xxHash64String(xiiStringView sStr, xiiUInt64 uiSeed)
{
  return xxHash64(sStr.GetStartPointer(), sStr.GetElementCount(), uiSeed);
}
