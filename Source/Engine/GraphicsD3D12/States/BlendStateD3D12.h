#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/BlendState.h>

class XII_GRAPHICSD3D12_DLL xiiGALBlendStateD3D12 final : public xiiGALBlendState
{
public:
  const Diligent::BlendStateDesc* GetBlendState() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALBlendStateD3D12(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::BlendStateDesc m_BlendState = {};
};

#include <GraphicsD3D12/States/Implementation/BlendStateD3D12_inl.h>
