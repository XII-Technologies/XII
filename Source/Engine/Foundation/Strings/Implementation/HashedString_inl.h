#pragma once

#include <Foundation/Algorithm/HashingUtils.h>

inline xiiHashedString::xiiHashedString(const xiiHashedString& rhs)
{
  m_Data = rhs.m_Data;

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  // the string has a refcount of at least one (rhs holds a reference), thus it will definitely not get deleted on some other thread
  // therefore we can simply increase the refcount without locking
  m_Data.Value().m_iRefCount.Increment();
#endif
}

XII_FORCE_INLINE xiiHashedString::xiiHashedString(xiiHashedString&& rhs)
{
  m_Data     = rhs.m_Data;
  rhs.m_Data = HashedType(); // This leaves the string in an invalid state, all operations will fail except the destructor
}

inline xiiHashedString::~xiiHashedString()
{
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  // Explicit check if data is still valid. It can be invalid if this string has been moved.
  if (m_Data.IsValid())
  {
    // just decrease the refcount of the object that we are set to, it might reach refcount zero, but we don't care about that here
    m_Data.Value().m_iRefCount.Decrement();
  }
#endif
}

inline void xiiHashedString::operator=(const xiiHashedString& rhs)
{
  // first increase the other refcount, then decrease ours
  HashedType tmp = rhs.m_Data;

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Increment();

  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data = tmp;
}

XII_FORCE_INLINE void xiiHashedString::operator=(xiiHashedString&& rhs)
{
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Decrement();
#endif

  m_Data     = rhs.m_Data;
  rhs.m_Data = HashedType();
}

template <size_t N>
XII_FORCE_INLINE void xiiHashedString::Assign(const char (&string)[N])
{
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(string, xiiHashingUtils::StringHash(string));

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

XII_FORCE_INLINE void xiiHashedString::Assign(xiiStringView sString)
{
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  HashedType tmp = m_Data;
#endif
  // this function will already increase the refcount as needed
  m_Data = AddHashedString(sString, xiiHashingUtils::StringHash(sString));

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  tmp.Value().m_iRefCount.Decrement();
#endif
}

inline bool xiiHashedString::operator==(const xiiHashedString& rhs) const
{
  return m_Data == rhs.m_Data;
}

inline bool xiiHashedString::operator!=(const xiiHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool xiiHashedString::operator==(const xiiTempHashedString& rhs) const
{
  return m_Data.Key() == rhs.m_uiHash;
}

inline bool xiiHashedString::operator!=(const xiiTempHashedString& rhs) const
{
  return !(*this == rhs);
}

inline bool xiiHashedString::operator<(const xiiHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_Data.Key();
}

inline bool xiiHashedString::operator<(const xiiTempHashedString& rhs) const
{
  return m_Data.Key() < rhs.m_uiHash;
}

XII_ALWAYS_INLINE const xiiString& xiiHashedString::GetString() const
{
  return m_Data.Value().m_sString;
}

XII_ALWAYS_INLINE const char* xiiHashedString::GetData() const
{
  return m_Data.Value().m_sString.GetData();
}

XII_ALWAYS_INLINE xiiUInt64 xiiHashedString::GetHash() const
{
  return m_Data.Key();
}

template <size_t N>
XII_FORCE_INLINE xiiHashedString xiiMakeHashedString(const char (&string)[N])
{
  xiiHashedString sResult;
  sResult.Assign(string);
  return sResult;
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString()
{
  constexpr xiiUInt64 uiEmptyHash = xiiHashingUtils::StringHash("");
  m_uiHash                        = uiEmptyHash;
}

template <size_t N>
XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString(const char (&string)[N])
{
  m_uiHash = xiiHashingUtils::StringHash<N>(string);
}

XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString(xiiStringView sString)
{
  m_uiHash = xiiHashingUtils::StringHash(sString);
}

XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString(const xiiTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString(const xiiHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

XII_ALWAYS_INLINE xiiTempHashedString::xiiTempHashedString(xiiUInt64 uiHash)
{
  m_uiHash = uiHash;
}

template <size_t N>
XII_ALWAYS_INLINE void xiiTempHashedString::operator=(const char (&string)[N])
{
  m_uiHash = xiiHashingUtils::StringHash<N>(string);
}

XII_ALWAYS_INLINE void xiiTempHashedString::operator=(xiiStringView sString)
{
  m_uiHash = xiiHashingUtils::StringHash(sString);
}

XII_ALWAYS_INLINE void xiiTempHashedString::operator=(const xiiTempHashedString& rhs)
{
  m_uiHash = rhs.m_uiHash;
}

XII_ALWAYS_INLINE void xiiTempHashedString::operator=(const xiiHashedString& rhs)
{
  m_uiHash = rhs.GetHash();
}

XII_ALWAYS_INLINE bool xiiTempHashedString::operator==(const xiiTempHashedString& rhs) const
{
  return m_uiHash == rhs.m_uiHash;
}

XII_ALWAYS_INLINE bool xiiTempHashedString::operator!=(const xiiTempHashedString& rhs) const
{
  return !(m_uiHash == rhs.m_uiHash);
}

XII_ALWAYS_INLINE bool xiiTempHashedString::operator<(const xiiTempHashedString& rhs) const
{
  return m_uiHash < rhs.m_uiHash;
}

XII_ALWAYS_INLINE bool xiiTempHashedString::IsEmpty() const
{
  constexpr xiiUInt64 uiEmptyHash = xiiHashingUtils::StringHash("");
  return m_uiHash == uiEmptyHash;
}

XII_ALWAYS_INLINE void xiiTempHashedString::Clear()
{
  *this = xiiTempHashedString();
}

XII_ALWAYS_INLINE xiiUInt64 xiiTempHashedString::GetHash() const
{
  return m_uiHash;
}

//////////////////////////////////////////////////////////////////////////

template <>
struct xiiHashHelper<xiiHashedString>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiHashedString& value)
  {
    return xiiHashingUtils::StringHashTo32(value.GetHash());
  }

  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiTempHashedString& value)
  {
    return xiiHashingUtils::StringHashTo32(value.GetHash());
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiHashedString& a, const xiiHashedString& b) { return a == b; }

  XII_ALWAYS_INLINE static bool Equal(const xiiHashedString& a, const xiiTempHashedString& b) { return a == b; }
};

template <>
struct xiiHashHelper<xiiTempHashedString>
{
  XII_ALWAYS_INLINE static xiiUInt32 Hash(const xiiTempHashedString& value)
  {
    return xiiHashingUtils::StringHashTo32(value.GetHash());
  }

  XII_ALWAYS_INLINE static bool Equal(const xiiTempHashedString& a, const xiiTempHashedString& b) { return a == b; }
};
