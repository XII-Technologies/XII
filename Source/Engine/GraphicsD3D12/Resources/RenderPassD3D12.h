#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSD3D12_DLL xiiGALRenderPassD3D12 final : public xiiGALRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRenderPassD3D12, xiiGALRenderPass);

public:

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALRenderPassD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassD3D12();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};

#include <GraphicsD3D12/Resources/Implementation/RenderPassD3D12_inl.h>
