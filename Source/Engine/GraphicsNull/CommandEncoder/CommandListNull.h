#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>

class XII_GRAPHICSNULL_DLL xiiGALCommandListNull final : public xiiGALCommandList
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALCommandListNull(const xiiGALCommandListCreationDescription& creationDescription);

  virtual ~xiiGALCommandListNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice) override final;

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice) override final;
};

#include <GraphicsNull/CommandEncoder/Implementation/CommandListNull_inl.h>
