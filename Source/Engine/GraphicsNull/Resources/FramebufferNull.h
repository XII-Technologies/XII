#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSNULL_DLL xiiGALFramebufferNull final : public xiiGALFramebuffer
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALFramebufferNull(const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/Resources/Implementation/FramebufferNull_inl.h>
