/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Resources/Framebuffer.h>

class XII_GRAPHICSD3D12_DLL xiiGALFramebufferD3D12 final : public xiiGALFramebuffer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALFramebufferD3D12, xiiGALFramebuffer);

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALFramebufferD3D12(xiiSharedPtr<xiiGALDeviceD3D12> pDeviceD3D12, const xiiGALFramebufferCreationDescription& creationDescription);

  virtual ~xiiGALFramebufferD3D12();

  virtual xiiResult InitPlatform() override final;
};
