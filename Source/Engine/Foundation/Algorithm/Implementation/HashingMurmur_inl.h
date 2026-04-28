/// Copyright (c) Theophilus Eriata. All Rights Reserved.

namespace xiiInternal
{
  constexpr xiiUInt32 MURMUR_M = 0x5bd1e995;
  constexpr xiiUInt32 MURMUR_R = 24;

  template <size_t N, size_t Loop>
  struct CompileTimeMurmurHash
  {
    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return CompileTimeMurmurHash<N, Loop - 4>()(CompileTimeMurmurHash<N, 4>()(uiHash, str, i), str, i + 4);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 4>
  {
    static constexpr XII_ALWAYS_INLINE xiiUInt32 helper(xiiUInt32 k) { return (k ^ (k >> MURMUR_R)) * MURMUR_M; }

    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      // In C++11 constexpr local variables are not allowed. Need to express the following without "xiiUInt32 k"
      // (this restriction is lifted in C++14's generalized constexpr)
      // xiiUInt32 k = ((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24));
      // k *= MURMUR_M;
      // k ^= (k >> MURMUR_R);
      // k *= MURMUR_M;
      // return (hash * MURMUR_M) ^ k;

      return (uiHash * MURMUR_M) ^ helper(((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24)) * MURMUR_M);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 3>
  {
    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 2] << 16) ^ (str[i + 1] << 8) ^ (str[i + 0])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 2>
  {
    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 1] << 8) ^ (str[i])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 1>
  {
    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const { return (uiHash ^ (str[i])) * MURMUR_M; }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 0>
  {
    constexpr XII_ALWAYS_INLINE xiiUInt32 operator()(xiiUInt32 uiHash, const char (&str)[N], size_t i) const { return uiHash; }
  };

  constexpr xiiUInt32 rightShift_and_xorWithPrevSelf(xiiUInt32 h, xiiUInt32 uiShift) { return h ^ (h >> uiShift); }
} // namespace xiiInternal

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::MurmurHash32String(const char (&str)[N], xiiUInt32 uiSeed)
{
  // In C++11 constexpr local variables are not allowed. Need to express the following without "xiiUInt32 h"
  // (this restriction is lifted in C++14's generalized constexpr)
  // const xiiUInt32 uiStrlen = (xiiUInt32)(N - 1);
  // xiiUInt32 h = xiiInternal::CompileTimeMurmurHash<N - 1>(uiSeed ^ uiStrlen, str, 0);
  // h ^= h >> 13;
  // h *= xiiInternal::MURMUR_M;
  // h ^= h >> 15;
  // return h;

  return xiiInternal::rightShift_and_xorWithPrevSelf(xiiInternal::rightShift_and_xorWithPrevSelf(xiiInternal::CompileTimeMurmurHash<N, N - 1>()(uiSeed ^ static_cast<xiiUInt32>(N - 1), str, 0), 13) * xiiInternal::MURMUR_M, 15);
}

XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::MurmurHash32String(xiiStringView sStr, xiiUInt32 uiSeed)
{
  return MurmurHash32(sStr.GetStartPointer(), sStr.GetElementCount(), uiSeed);
}
