#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSVULKAN_DLL xiiGALRenderPassVulkan final : public xiiGALRenderPass
{
public:
  XII_ALWAYS_INLINE const Diligent::IRenderPass* GetRenderPass() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRenderPassVulkan(const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IRenderPass> m_pRenderPass;
};

#include <GraphicsVulkan/Resources/Implementation/RenderPassVulkan_inl.h>
