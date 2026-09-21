/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>

/// Helper class to acquire and release a resource safely.
///
/// The constructor calls xiiResourceManager::BeginAcquireResource, the destructor makes sure to call xiiResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
///
/// Whether the acquisition succeeded or returned a loading fallback, missing fallback or even no result, at all,
/// can be retrieved through GetAcquireResult().
/// \note If a resource is missing, but no missing fallback is specified for the resource type, the code will fail with an assertion,
/// unless you used xiiResourceAcquireMode::BlockTillLoaded_NeverFail. Only then will the error be silently ignored and the acquire result
/// will be xiiResourceAcquireResult::None.
///
/// \sa xiiResourceManager::BeginAcquireResource()
/// \sa xiiResourceAcquireMode
/// \sa xiiResourceAcquireResult
template <class RESOURCE_TYPE>
class xiiResourceLock
{
public:
  XII_ALWAYS_INLINE xiiResourceLock(const xiiTypedResourceHandle<RESOURCE_TYPE>& hResource, xiiResourceAcquireMode mode, const xiiTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = xiiTypedResourceHandle<RESOURCE_TYPE>())
  {
    m_pResource = xiiResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, &m_AcquireResult);
  }

  xiiResourceLock(const xiiResourceLock&) = delete;

  xiiResourceLock(xiiResourceLock&& other) :
    m_AcquireResult(other.m_AcquireResult), m_pResource(other.m_pResource)
  {
    other.m_pResource     = nullptr;
    other.m_AcquireResult = xiiResourceAcquireResult::None;
  }

  XII_ALWAYS_INLINE ~xiiResourceLock()
  {
    if (m_pResource)
    {
      xiiResourceManager::EndAcquireResource(m_pResource);
    }
  }

  XII_ALWAYS_INLINE RESOURCE_TYPE*       operator->() { return m_pResource; }
  XII_ALWAYS_INLINE const RESOURCE_TYPE* operator->() const { return m_pResource; }

  XII_ALWAYS_INLINE bool     IsValid() const { return m_pResource != nullptr; }
  XII_ALWAYS_INLINE explicit operator bool() const { return m_pResource != nullptr; }

  XII_ALWAYS_INLINE xiiResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

  XII_ALWAYS_INLINE const RESOURCE_TYPE* GetPointer() const { return m_pResource; }
  XII_ALWAYS_INLINE RESOURCE_TYPE*       GetPointerNonConst() const { return m_pResource; }

private:
  xiiResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE*           m_pResource;
};
