/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_CLANG("-Wunused-function")

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

XII_WARNING_POP()

xiiHashStreamWriter32::xiiHashStreamWriter32(xiiUInt32 uiSeed)
{
  m_pState = XXH32_createState();
  XII_VERIFY(XXH_OK == XXH32_reset((XXH32_state_t*)m_pState, uiSeed), "");
}

xiiHashStreamWriter32::~xiiHashStreamWriter32()
{
  XXH32_freeState((XXH32_state_t*)m_pState);
}

xiiResult xiiHashStreamWriter32::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite == 0)
    return XII_SUCCESS;

  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return XII_FAILURE;

  if (XXH_OK == XXH32_update((XXH32_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return XII_SUCCESS;

  return XII_FAILURE;
}

xiiUInt32 xiiHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest((XXH32_state_t*)m_pState);
}

//////////////////////////////////////////////////////////////////////////

xiiHashStreamWriter64::xiiHashStreamWriter64(xiiUInt64 uiSeed)
{
  m_pState = XXH64_createState();
  XII_VERIFY(XXH_OK == XXH64_reset((XXH64_state_t*)m_pState, uiSeed), "");
}

xiiHashStreamWriter64::~xiiHashStreamWriter64()
{
  XXH64_freeState((XXH64_state_t*)m_pState);
}

xiiResult xiiHashStreamWriter64::WriteBytes(const void* pWriteBuffer, xiiUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite == 0)
    return XII_SUCCESS;

  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return XII_FAILURE;

  if (XXH_OK == XXH64_update((XXH64_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return XII_SUCCESS;

  return XII_FAILURE;
}

xiiUInt64 xiiHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest((XXH64_state_t*)m_pState);
}

XII_STATICLINK_FILE(Foundation, Foundation_Algorithm_Implementation_HashStream);
