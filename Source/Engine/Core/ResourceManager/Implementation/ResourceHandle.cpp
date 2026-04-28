/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>

xiiTypelessResourceHandle::xiiTypelessResourceHandle(xiiResource* pResource)
{
  m_pResource = pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(m_pResource, this);
  }
}

void xiiTypelessResourceHandle::Invalidate()
{
  if (m_pResource)
  {
    DecreaseResourceRefCount(m_pResource, this);
  }

  m_pResource = nullptr;
}

xiiUInt64 xiiTypelessResourceHandle::GetResourceIDHash() const
{
  return IsValid() ? m_pResource->GetResourceIDHash() : 0;
}

xiiStringView xiiTypelessResourceHandle::GetResourceID() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceID();
  }

  return {};
}

xiiStringView xiiTypelessResourceHandle::GetResourceIdOrDescription() const
{
  if (IsValid())
  {
    return m_pResource->GetResourceIdOrDescription();
  }

  return {};
}

const xiiRTTI* xiiTypelessResourceHandle::GetResourceType() const
{
  return IsValid() ? m_pResource->GetDynamicRTTI() : nullptr;
}

void xiiTypelessResourceHandle::operator=(const xiiTypelessResourceHandle& rhs)
{
  XII_ASSERT_DEBUG(this != &rhs, "Cannot assign a resource handle to itself! This would invalidate the handle.");

  Invalidate();

  m_pResource = rhs.m_pResource;

  if (m_pResource)
  {
    IncreaseResourceRefCount(reinterpret_cast<xiiResource*>(m_pResource), this);
  }
}

void xiiTypelessResourceHandle::operator=(xiiTypelessResourceHandle&& rhs)
{
  Invalidate();

  m_pResource     = rhs.m_pResource;
  rhs.m_pResource = nullptr;

  if (m_pResource)
  {
    MigrateResourceRefCount(m_pResource, &rhs, this);
  }
}

// static
void xiiResourceHandleStreamOperations::WriteHandle(xiiStreamWriter& Stream, const xiiResource* pResource)
{
  if (pResource != nullptr)
  {
    Stream << pResource->GetDynamicRTTI()->GetTypeName();
    Stream << pResource->GetResourceID();
  }
  else
  {
    const char* szEmpty = "";
    Stream << szEmpty;
  }
}

// static
void xiiResourceHandleStreamOperations::ReadHandle(xiiStreamReader& Stream, xiiTypelessResourceHandle& ResourceHandle)
{
  xiiStringBuilder sTemp;

  Stream >> sTemp;
  if (sTemp.IsEmpty())
  {
    ResourceHandle.Invalidate();
    return;
  }

  const xiiRTTI* pRtti = xiiResourceManager::FindResourceForAssetType(sTemp);

  if (pRtti == nullptr)
  {
    pRtti = xiiRTTI::FindTypeByName(sTemp);
  }

  if (pRtti == nullptr)
  {
    xiiLog::Error("Unknown resource type '{0}'", sTemp);
    ResourceHandle.Invalidate();
  }

  // read unique ID for restoring the resource (from file)
  Stream >> sTemp;

  if (pRtti != nullptr)
  {
    ResourceHandle = xiiResourceManager::LoadResourceByType(pRtti, sTemp);
  }
}

XII_STATICLINK_FILE(Core, Core_ResourceManager_Implementation_ResourceHandle);
