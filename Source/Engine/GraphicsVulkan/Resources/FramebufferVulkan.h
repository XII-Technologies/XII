#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFramebufferVulkan final : public xiiGALFramebuffer
{
public:
  Diligent::IFramebuffer* GetFramebuffer() const;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFramebufferVulkan(const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferVulkan();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::IFramebuffer* m_pFramebuffer = nullptr;
};

#include <GraphicsVulkan/Resources/Implementation/FramebufferVulkan_inl.h>
