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
    XII_ALWAYS_INLINE static xiiUInt32 Hash(xiiStringView string)
    {
      return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(string));
    }
  };

  template <typename T, bool isString>
  XII_ALWAYS_INLINE xiiUInt32 HashHelperImpl<T, isString>::Hash(const T& value)
  {
    XII_CHECK_AT_COMPILETIME_MSG(isString, "xiiHashHelper is not implemented for the given type.");
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
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const char* szValue)
  {
    return xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(szValue));
  }

  XII_ALWAYS_INLINE static bool Equal(const char* a, const char* b) { return xiiStringUtils::IsEqual(a, b); }
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

XII_ALWAYS_INLINE xiiUInt64 xiiHashingUtils::StringHash(xiiStringView str, xiiUInt64 uiSeed)
{
  return xxHash64String(str, uiSeed);
}

constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::StringHashTo32(xiiUInt64 hash)
{
  // just throw away the upper bits
  return static_cast<xiiUInt32>(hash);
}

constexpr XII_ALWAYS_INLINE xiiUInt32 xiiHashingUtils::CombineHashValues32(xiiUInt32 h0, xiiUInt32 h1)
{
  // See boost::hash_combine
  return h0 ^ (h1 + 0x9e3779b9 + (h0 << 6) + (h1 >> 2));
}
