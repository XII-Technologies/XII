/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSVULKAN_DLL xiiGALFramebufferVulkan final : public xiiGALFramebuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALFramebufferVulkan, xiiGALFramebuffer);

public:
  [[nodiscard]] XII_ALWAYS_INLINE vk::Framebuffer GetVulkanFramebuffer() const { return m_vkFramebuffer; }

protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  xiiGALFramebufferVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferVulkan();

  virtual xiiResult InitPlatform() override final;

  virtual void SetDebugNamePlatform(xiiStringView sName) const override final;

private:
  vk::Framebuffer m_vkFramebuffer;
};
