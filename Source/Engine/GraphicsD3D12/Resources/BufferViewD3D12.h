#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/BufferView.h>

class XII_GRAPHICSD3D12_DLL xiiGALBufferViewD3D12 final : public xiiGALBufferView
{
public:
  Diligent::IBufferView* GetBufferView() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBufferViewD3D12(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription);

  virtual ~xiiGALBufferViewD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IBufferView> m_pBufferView;
};

#include <GraphicsD3D12/Resources/Implementation/BufferViewD3D12_inl.h>
