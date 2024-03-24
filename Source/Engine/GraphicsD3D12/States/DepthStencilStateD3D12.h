#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSD3D12_DLL xiiGALDepthStencilStateD3D12 final : public xiiGALDepthStencilState
{
public:
  const Diligent::DepthStencilStateDesc* GetDepthStencilState() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateD3D12(const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::DepthStencilStateDesc m_DepthStencilState = {};
};

#include <GraphicsD3D12/States/Implementation/DepthStencilStateD3D12_inl.h>
