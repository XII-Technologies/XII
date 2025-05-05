#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

class XII_GRAPHICSNULL_DLL xiiGALRenderPassNull final : public xiiGALRenderPass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALRenderPassNull, xiiGALRenderPass);

public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALRenderPassNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALRenderPassCreationDescription& creationDescription);

  virtual ~xiiGALRenderPassNull();

  virtual xiiResult InitPlatform() override final;
};
