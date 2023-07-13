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
    constexpr xiiUInt32 length = static_cast<xiiUInt32>(N - 1);
    if constexpr (length == 0)
    {
      return 46947589u;
    }
    else
    {
      xiiUInt32 acc   = 0;
      xiiUInt32 index = 0;
      // Perform simple initialization if N < 16
      if constexpr (length < 16)
      {
        acc = uiSeed + PRIME32_5;
      }
      else
      {
        xiiUInt32 accs[4] = {uiSeed + PRIME32_1 + PRIME32_2, uiSeed + PRIME32_2, uiSeed, uiSeed - PRIME32_1};
        for (; length - index >= 16; index += 16)
        {
          for (xiiInt32 i = 0; i < 4; i++)
          {
            xiiUInt32 laneN = (static_cast<xiiUInt32>(str[index + i * 4 + 0]) << 0) | (static_cast<xiiUInt32>(str[index + i * 4 + 1]) << 8) |
              (static_cast<xiiUInt32>(str[index + i * 4 + 2]) << 16) | (static_cast<xiiUInt32>(str[index + i * 4 + 3]) << 24);
            accs[i] = accs[i] + (laneN * PRIME32_2);
            accs[i] = xiiRotLeft(accs[i], 13);
            accs[i] = accs[i] * PRIME32_1;
          }
        }
        acc = xiiRotLeft(accs[0], 1) + xiiRotLeft(accs[1], 7) + xiiRotLeft(accs[2], 12) + xiiRotLeft(accs[3], 18);
      }

      // Step 4
      acc = acc + length;

      // Step 5
      for (; length - index >= 4; index += 4)
      {
        xiiUInt32 lane = (static_cast<xiiUInt32>(str[index + 0]) << 0) | (static_cast<xiiUInt32>(str[index + 1]) << 8) |
          (static_cast<xiiUInt32>(str[index + 2]) << 16) | (static_cast<xiiUInt32>(str[index + 3]) << 24);
        acc = acc + lane * PRIME32_3;
        acc = xiiRotLeft(acc, 17) * PRIME32_4;
      }

      for (; length - index >= 1; index++)
      {
        xiiUInt32 lane = static_cast<xiiUInt32>(str[index]);
        acc            = acc + lane * PRIME32_5;
        acc            = xiiRotLeft(acc, 11) * PRIME32_1;
      }

      // Step 6
      acc = acc ^ (acc >> 15);
      acc = acc * PRIME32_2;
      acc = acc ^ (acc >> 13);
      acc = acc * PRIME32_3;
      acc = acc ^ (acc >> 16);

      return acc;
    }
  }

  template <size_t N>
  constexpr xiiUInt64 CompileTimeXxHash64(const char (&str)[N], xiiUInt64 uiSeed)
  {
    // Note: N will contain the trailing 0 of a string literal. This needs to be ignored.
    constexpr xiiUInt32 length = static_cast<xiiUInt32>(N - 1);
    if constexpr (length == 0)
    {
      return 17241709254077376921llu;
    }
    else
    {
      xiiUInt64 acc   = 0;
      xiiUInt32 index = 0;

      // Step 1
      if constexpr (length < 32)
      {
        // simple initialization
        acc = uiSeed + PRIME64_5;
      }
      else
      {
        xiiUInt64 accs[] = {uiSeed + PRIME64_1 + PRIME64_2, uiSeed + PRIME64_2, uiSeed + 0, uiSeed - PRIME64_1};
        // Step 2
        for (; length - index >= 32; index += 32)
        {
          for (xiiInt32 i = 0; i < 4; i++)
          {
            xiiUInt64 laneN = (static_cast<xiiUInt64>(str[index + i * 8 + 0]) << 0) | (static_cast<xiiUInt64>(str[index + i * 8 + 1]) << 8) |
              (static_cast<xiiUInt64>(str[index + i * 8 + 2]) << 16) | (static_cast<xiiUInt64>(str[index + i * 8 + 3]) << 24) |
              (static_cast<xiiUInt64>(str[index + i * 8 + 4]) << 32) | (static_cast<xiiUInt64>(str[index + i * 8 + 5]) << 40) |
              (static_cast<xiiUInt64>(str[index + i * 8 + 6]) << 48) | (static_cast<xiiUInt64>(str[index + i * 8 + 7]) << 56);
            accs[i] = accs[i] + (laneN * PRIME64_2);
            accs[i] = xiiRotLeft(accs[i], 31ULL);
            accs[i] = accs[i] * PRIME64_1;
          }
        }

        // Step 3
        acc = xiiRotLeft(accs[0], 1ULL) + xiiRotLeft(accs[1], 7ULL) + xiiRotLeft(accs[2], 12ULL) + xiiRotLeft(accs[3], 18ULL);
        for (xiiInt32 i = 0; i < 4; i++)
        {
          acc = (acc ^ (xiiRotLeft(accs[i] * PRIME64_2, 31ULL) * PRIME64_1)) * PRIME64_1 + PRIME64_4;
        }
      }
      // Step 4
      acc += length;

      // Step 5
      for (; length - index >= 8; index += 8)
      {
        xiiUInt64 lane = (static_cast<xiiUInt64>(str[index + 0]) << 0) | (static_cast<xiiUInt64>(str[index + 1]) << 8) |
          (static_cast<xiiUInt64>(str[index + 2]) << 16) | (static_cast<xiiUInt64>(str[index + 3]) << 24) |
          (static_cast<xiiUInt64>(str[index + 4]) << 32) | (static_cast<xiiUInt64>(str[index + 5]) << 40) |
          (static_cast<xiiUInt64>(str[index + 6]) << 48) | (static_cast<xiiUInt64>(str[index + 7]) << 56);
        acc = acc ^ (xiiRotLeft(lane * PRIME64_2, 31ULL) * PRIME64_1);
        acc = xiiRotLeft(acc, 27ULL) * PRIME64_1;
        acc += PRIME64_4;
      }

      for (; length - index >= 4; index += 4)
      {
        xiiUInt64 lane = (static_cast<xiiUInt64>(str[index + 0]) << 0) | (static_cast<xiiUInt64>(str[index + 1]) << 8) |
          (static_cast<xiiUInt64>(str[index + 2]) << 16) | (static_cast<xiiUInt64>(str[index + 3]) << 24);
        acc = acc ^ (lane * PRIME64_1);
        acc = xiiRotLeft(acc, 23ULL) * PRIME64_2;
        acc += PRIME64_3;
      }

      for (; length - index >= 1; index++)
      {
        xiiUInt64 lane = static_cast<xiiUInt64>(str[index]);
        acc            = acc ^ (lane * PRIME64_5);
        acc            = xiiRotLeft(acc, 11ULL) * PRIME64_1;
      }

      // Step 6
      acc = acc ^ (acc >> 33);
      acc = acc * PRIME64_2;
      acc = acc ^ (acc >> 29);
      acc = acc * PRIME64_3;
      acc = acc ^ (acc >> 32);

      return acc;
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
