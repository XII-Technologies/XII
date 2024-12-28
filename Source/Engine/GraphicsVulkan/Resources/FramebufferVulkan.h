#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFramebufferVulkan final : public xiiGALFramebuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALFramebufferVulkan, xiiGALFramebuffer);

public:
protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFramebufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  vk::Framebuffer m_vkFramebuffer;
};
