
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class XII_RENDERERFOUNDATION_DLL xiiGALResourceBase : public xiiRefCounted
{
public:
  virtual const xiiGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class xiiGALDevice;

  inline ~xiiGALResourceBase()
  {
    XII_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
    XII_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

    XII_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
    XII_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
    XII_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  mutable xiiHashedString m_sDebugName;
#endif

  xiiGALResourceViewHandle     m_hDefaultResourceView;
  xiiGALRenderTargetViewHandle m_hDefaultRenderTargetView;

  xiiHashTable<xiiUInt32, xiiGALResourceViewHandle>        m_ResourceViews;
  xiiHashTable<xiiUInt32, xiiGALRenderTargetViewHandle>    m_RenderTargetViews;
  xiiHashTable<xiiUInt32, xiiGALUnorderedAccessViewHandle> m_UnorderedAccessViews;
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class xiiGALResource : public xiiGALResourceBase
{
public:
  XII_ALWAYS_INLINE xiiGALResource(const CreationDescription& description) :
    m_Description(description)
  {
  }

  XII_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};
