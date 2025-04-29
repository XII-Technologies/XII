#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSNULL_DLL xiiGALDepthStencilStateNull final : public xiiGALDepthStencilState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateNull();

  virtual xiiResult InitPlatform() override final;

  virtual xiiResult DeInitPlatform() override final;
};
