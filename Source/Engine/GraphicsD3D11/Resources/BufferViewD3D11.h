#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSD3D11_DLL xiiGALBufferViewD3D11 final : public xiiGALBufferView
{
public:
  Diligent::IBufferView* GetBufferView() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALBufferViewD3D11(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IBufferView* m_pBufferView = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/BufferViewD3D11_inl.h>
