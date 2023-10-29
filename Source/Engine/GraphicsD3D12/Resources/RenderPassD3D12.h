#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSD3D12_DLL xiiGALRenderPassD3D12 final : public xiiGALRenderPass
{
public:
  XII_ALWAYS_INLINE Diligent::IRenderPass* GetRenderPass() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALRenderPassD3D12(const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IRenderPass> m_pRenderPass;
};

#include <GraphicsD3D12/Resources/Implementation/RenderPassD3D12_inl.h>
