#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/States/RasterizerState.h>

class XII_GRAPHICSNULL_DLL xiiGALRasterizerStateNull final : public xiiGALRasterizerState
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALRasterizerStateNull(const xiiGALRasterizerStateCreationDescription& creationDescription);

  virtual ~xiiGALRasterizerStateNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/States/Implementation/RasterizerStateNull_inl.h>
