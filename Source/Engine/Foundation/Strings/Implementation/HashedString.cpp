#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

struct HashedStringData
{
  xiiMutex                       m_Mutex;
  xiiHashedString::StringStorage m_Storage;
  xiiHashedString::HashedType    m_Empty;
};

static HashedStringData* s_pHSData;

XII_MSVC_ANALYSIS_WARNING_PUSH
XII_MSVC_ANALYSIS_WARNING_DISABLE(6011) // Disable warning for null pointer dereference as InitHashedString() will ensure that s_pHSData is set

// static
xiiHashedString::HashedType xiiHashedString::AddHashedString(xiiStringView sString, xiiUInt64 uiHash)
{
  if (s_pHSData == nullptr)
    InitHashedString();

  XII_LOCK(s_pHSData->m_Mutex);

  // try to find the existing string
  bool bExisted = false;
  auto ret      = s_pHSData->m_Storage.FindOrAdd(uiHash, &bExisted);

  // if it already exists, just increase the refcount
  if (bExisted)
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (ret.Value().m_sString != sString)
    {
      // TODO: I think this should be a more serious issue
      xiiLog::Error("Hash collision encountered: Strings \"{}\" and \"{}\" both hash to {}.", xiiArgSensitive(ret.Value().m_sString), xiiArgSensitive(sString), uiHash);
    }
#endif

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
    ret.Value().m_iRefCount.Increment();
#endif
  }
  else
  {
    xiiHashedString::HashedData& d = ret.Value();
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
    d.m_iRefCount = 1;
#endif
    d.m_sString = sString;
  }

  return ret;
}

XII_MSVC_ANALYSIS_WARNING_POP

// static
void xiiHashedString::InitHashedString()
{
  if (s_pHSData != nullptr)
    return;

  alignas(XII_ALIGNMENT_OF(HashedStringData)) static xiiUInt8 HashedStringDataBuffer[sizeof(HashedStringData)];
  s_pHSData = new (HashedStringDataBuffer) HashedStringData();

  // makes sure the empty string exists for the default constructor to use
  s_pHSData->m_Empty = AddHashedString("", xiiHashingUtils::StringHash(""));

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  // this one should never get deleted, so make sure its refcount is 2
  s_pHSData->m_Empty.Value().m_iRefCount.Increment();
#endif
}

#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
xiiUInt32 xiiHashedString::ClearUnusedStrings()
{
  XII_LOCK(s_pHSData->m_Mutex);

  xiiUInt32 uiDeleted = 0;

  for (auto it = s_pHSData->m_Storage.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_iRefCount == 0)
    {
      it = s_pHSData->m_Storage.Remove(it);
      ++uiDeleted;
    }
    else
      ++it;
  }

  return uiDeleted;
}
#endif

XII_MSVC_ANALYSIS_WARNING_PUSH
XII_MSVC_ANALYSIS_WARNING_DISABLE(6011) // Disable warning for null pointer dereference as InitHashedString() will ensure that s_pHSData is set

xiiHashedString::xiiHashedString()
{
  static_assert(sizeof(m_Data) == sizeof(void*), "The hashed string data should only be as large as one pointer.");
  static_assert(sizeof(*this) == sizeof(void*), "The hashed string data should only be as large as one pointer.");

  // only insert the empty string once, after that, we can just use it without the need for the mutex
  if (s_pHSData == nullptr)
    InitHashedString();

  m_Data = s_pHSData->m_Empty;
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  m_Data.Value().m_iRefCount.Increment();
#endif
}

XII_MSVC_ANALYSIS_WARNING_POP

bool xiiHashedString::IsEmpty() const
{
  return m_Data == s_pHSData->m_Empty;
}

void xiiHashedString::Clear()
{
#if XII_ENABLED(XII_HASHED_STRING_REF_COUNTING)
  if (m_Data != s_pHSData->m_Empty)
  {
    HashedType tmp = m_Data;

    m_Data = s_pHSData->m_Empty;
    m_Data.Value().m_iRefCount.Increment();

    tmp.Value().m_iRefCount.Decrement();
  }
#else
  m_Data = s_pHSData->m_Empty;
#endif
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_HashedString);
