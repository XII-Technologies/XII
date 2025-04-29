#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFramebufferVulkan final : public xiiGALFramebuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALFramebufferVulkan, xiiGALFramebuffer);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::Framebuffer GetVulkanFramebuffer() const { return m_vkFramebuffer; }

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;

  xiiGALFramebufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  vk::Framebuffer m_vkFramebuffer;
};
