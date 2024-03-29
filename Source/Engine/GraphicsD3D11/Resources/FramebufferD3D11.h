#pragma once

#include <GraphicsD3D11/GraphicsD3D11DLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSD3D11_DLL xiiGALFramebufferD3D11 final : public xiiGALFramebuffer
{
public:
  Diligent::IFramebuffer* GetFramebuffer() const;

protected:
  friend class xiiGALDeviceD3D11;
  friend class xiiMemoryUtils;

  xiiGALFramebufferD3D11(const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferD3D11();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;

protected:
  Diligent::IFramebuffer* m_pFramebuffer = nullptr;
};

#include <GraphicsD3D11/Resources/Implementation/FramebufferD3D11_inl.h>
