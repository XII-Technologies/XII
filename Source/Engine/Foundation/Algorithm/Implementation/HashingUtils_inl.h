/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Strings/Implementation/StringBase.h>

namespace xiiInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl
  {
    static xiiUInt32 Hash(const T& value);
  };

  template <typename T>
  struct HashHelperImpl<T, true>
  {
    XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiStringView sString)
    {
      return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sString));
    }
  };

  template <typename T, bool isString>
  XII_ALWAYS_INLINE xiiUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    static_assert(isString, "xiiHashHelper is not implemented for the given type.");
    return 0;
  }
} // namespace xiiInternal

template <typename T>
template <typename U>
XII_ALWAYS_INLINE xiiUInt32 xiiHashHelper<T>::Hash(const U& value)
{
  return xiiInternal::HashHelperImpl<T, XII_IS_DERIVED_FROM_STATIC(xiiThisIsAString, T)>::Hash(value);
}

template <typename T>
template <typename U>
XII_ALWAYS_INLINE bool xiiHashHelper<T>::Equal(const T& a, const U& b)
{
  return a == b;
}



template <>
struct xiiHashHelper<xiiUInt32>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiUInt32 value)
  {
    // Knuth: multiplication by the golden ratio will minimize gaps in the hash space.
    // 2654435761U: prime close to 2^32/phi with phi = golden ratio (sqrt(5) - 1) / 2
    return value * 2654435761U;
  }

  XII_ALWAYS_INLINE static bool Equal(xiiUInt32 a, xiiUInt32 b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiInt32>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiInt32 value) { return xiiHashHelper<xiiUInt32>::Hash(xiiUInt32(value)); }

  XII_ALWAYS_INLINE static bool Equal(xiiInt32 a, xiiInt32 b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiUInt64>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiUInt64 value)
  {
    // boost::hash_combine.
    xiiUInt32 a = xiiUInt32(value >> 32);
    xiiUInt32 b = xiiUInt32(value);
    return a ^ (b + 0x9e3779b9 + (a << 6) + (b >> 2));
  }

  XII_ALWAYS_INLINE static bool Equal(xiiUInt64 a, xiiUInt64 b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiInt64>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiInt64 value) { return xiiHashHelper<xiiUInt64>::Hash(xiiUInt64(value)); }

  XII_ALWAYS_INLINE static bool Equal(xiiInt64 a, xiiInt64 b) { return a == b; }
};

template <>
struct xiiHashHelper<const char*>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiStringView sValue)
  {
    return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sValue));
  }

  XII_ALWAYS_INLINE static bool Equal(const char* a, const char* b) { return xiiStringUtils::IsEqual(a, b); }
};

template <>
struct xiiHashHelper<xiiStringView>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiStringView sValue)
  {
    return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(sValue));
  }

  XII_ALWAYS_INLINE static bool Equal(xiiStringView a, xiiStringView b) { return a == b; }
};

template <typename T>
struct xiiHashHelper<T*>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(T* value)
  {
#if XII_ENABLED(XII_PLATFORM_64BIT)
    return xiiHashHelper<xiiUInt64>::Hash(reinterpret_cast<xiiUInt64>(value) >> 4);
#else
    return xiiHashHelper<xiiUInt32>::Hash(reinterpret_cast<xiiUInt32>(value) >> 4);
#endif
  }

  XII_ALWAYS_INLINE static bool Equal(T* a, T* b)
  {
    return a == b;
  }
};

template <size_t N>
constexpr XII_ALWAYS_INLINE xiiUInt64 xiiHashingUtils::StringHash(const char (&str)[N], xiiUInt64 uiSeed)
{
  return xxHash64String(str, uiSeed);
}

XII_ALWAYS_INLINE xiiUInt64 xiiHashingUtils::StringHash(xiiStringView sStr, xiiUInt64 uiSeed)
{
  return xxHash64String(sStr, uiSeed);
}

constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::StringHashTo32(xiiUInt64 uiHash)
{
  // just throw away the upper bits
  return static_cast<xiiUInt32>(uiHash);
}

constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::CombineHashValues32(xiiUInt32 ui0, xiiUInt32 ui1)
{
  // See boost::hash_combine
  return ui0 ^ (ui1 + 0x9e3779b9 + (ui0 << 6) + (ui1 >> 2));
}
