#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSNULL_DLL xiiGALRenderPassNull final : public xiiGALRenderPass
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALRenderPassNull(const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/Resources/Implementation/RenderPassNull_inl.h>
