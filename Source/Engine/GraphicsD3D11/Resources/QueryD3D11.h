#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Query.h>

class XII_GRAPHICSD3D11_DLL xiiGALQueryD3D11 final : public xiiGALQuery
{
public:
  virtual bool GetData(void* pData, xiiUInt32 uiDataSize, bool bAutoInvalidate = true) override final;

  virtual void Invalidate() override final;

  Diligent::IQuery* GetQuery() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALQueryD3D11(const xiiGALQueryCreationDescription& creationDescription);

  virtual ~xiiGALQueryD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IQuery* m_pQuery = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/QueryD3D11_inl.h>
