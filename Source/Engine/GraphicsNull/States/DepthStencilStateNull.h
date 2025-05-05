#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/DepthStencilState.h>

class XII_GRAPHICSNULL_DLL xiiGALDepthStencilStateNull final : public xiiGALDepthStencilState
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALDepthStencilStateNull, xiiGALDepthStencilState);

public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALDepthStencilStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALDepthStencilStateCreationDescription& creationDescription);

  virtual ~xiiGALDepthStencilStateNull();

  virtual xiiResult InitPlatform() override final;
};
