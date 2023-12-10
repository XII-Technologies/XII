#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/BlendState.h>

class XII_GRAPHICSNULL_DLL xiiGALBlendStateNull final : public xiiGALBlendState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALBlendStateNull(const xiiGALBlendStateCreationDescription& creationDescription);

  virtual ~xiiGALBlendStateNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/States/Implementation/BlendStateNull_inl.h>
