#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/Blob.h>

#include <Foundation/Memory/Allocator.h>

xiiBlob::xiiBlob() = default;

xiiBlob::xiiBlob(xiiBlob&& other)
{
  m_pStorage = other.m_pStorage;
  m_uiSize   = other.m_uiSize;

  other.m_pStorage = nullptr;
  other.m_uiSize   = 0;
}

void xiiBlob::operator=(xiiBlob&& rhs)
{
  Clear();

  m_pStorage = rhs.m_pStorage;
  m_uiSize   = rhs.m_uiSize;

  rhs.m_pStorage = nullptr;
  rhs.m_uiSize   = 0;
}

xiiBlob::~xiiBlob()
{
  Clear();
}

void xiiBlob::SetFrom(void* pSource, xiiUInt64 uiSize)
{
  SetCountUninitialized(uiSize);
  xiiMemoryUtils::Copy(static_cast<xiiUInt8*>(m_pStorage), static_cast<xiiUInt8*>(pSource), static_cast<size_t>(uiSize));
}

void xiiBlob::Clear()
{
  if (m_pStorage)
  {
    xiiFoundation::GetAlignedAllocator()->Deallocate(m_pStorage);
    m_pStorage = nullptr;
    m_uiSize   = 0;
  }
}

void xiiBlob::SetCountUninitialized(xiiUInt64 uiCount)
{
  if (m_uiSize != uiCount)
  {
    Clear();

    m_pStorage = xiiFoundation::GetAlignedAllocator()->Allocate(xiiMath::SafeConvertToSizeT(uiCount), 64u);
    m_uiSize   = uiCount;
  }
}

void xiiBlob::ZeroFill()
{
  if (m_pStorage)
  {
    xiiMemoryUtils::ZeroFill(static_cast<xiiUInt8*>(m_pStorage), static_cast<size_t>(m_uiSize));
  }
}


XII_STATICLINK_FILE(Foundation, Foundation_Containers_Implementation_Blob);
