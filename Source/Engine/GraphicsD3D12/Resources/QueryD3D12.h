#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSD3D12_DLL xiiGALQueryD3D12 final : public xiiGALQuery
{
public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override;

  virtual void Invalidate() override;

  XII_ALWAYS_INLINE Diligent::IQuery* GetQuery() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALQueryD3D12(const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IQuery> m_pQuery;
};

#include <GraphicsD3D12/Resources/Implementation/QueryD3D12_inl.h>
