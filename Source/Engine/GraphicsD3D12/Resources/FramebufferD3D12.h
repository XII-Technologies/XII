#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSD3D12_DLL xiiGALFramebufferD3D12 final : public xiiGALFramebuffer
{
public:
  XII_ALWAYS_INLINE Diligent::IFramebuffer* GetFramebuffer() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALFramebufferD3D12(const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override;

protected:
  Diligent::RefCntAutoPtr<Diligent::IFramebuffer> m_pFramebuffer;
};

#include <GraphicsD3D12/Resources/Implementation/FramebufferD3D12_inl.h>
