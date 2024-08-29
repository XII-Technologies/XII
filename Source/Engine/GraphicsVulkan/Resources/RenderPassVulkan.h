#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSVULKAN_DLL xiiGALRenderPassVulkan final : public xiiGALRenderPass
{
public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALRenderPassVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

protected:
};
