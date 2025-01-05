#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

#include <GraphicsD3D11/Resources/DisjointQueryPool.h>

class XII_GRAPHICSD3D11_DLL xiiGALQueryD3D11 final : public xiiGALQuery
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALQueryD3D11, xiiGALQuery);

public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate) override final;

  XII_ALWAYS_INLINE virtual void Invalidate() override final
  {
    m_DisjointQuery.Clear();

    xiiGALQuery::Invalidate();
  }

  XII_ALWAYS_INLINE ID3D11Query* GetQuery(xiiUInt32 uiQueryID) const
  {
    XII_ASSERT_DEV(uiQueryID == 0 || (m_Description.m_Type == xiiGALQueryType::Duration && uiQueryID == 1), "");

    return m_pQueryD3D11[uiQueryID];
  }

  XII_ALWAYS_INLINE void SetDisjointQuery(xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> disjointQuery)
  {
    m_DisjointQuery = disjointQuery;
  }

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALQueryD3D11(xiiGALDeviceD3D11* pDeviceD3D11, const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryD3D11();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  ID3D11Query* m_pQueryD3D11[2] = {nullptr, nullptr};

  xiiSharedPtr<xiiDisjointQueryPool::DisjointQueryWrapper> m_DisjointQuery;
};
