#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSD3D11_DLL xiiGALRenderPassD3D11 final : public xiiGALRenderPass
{
public:
protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALRenderPassD3D11(const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
};

#include <GraphicsD3D11/Resources/Implementation/RenderPassD3D11_inl.h>
