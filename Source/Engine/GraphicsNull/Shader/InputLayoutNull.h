#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSNULL_DLL xiiGALInputLayoutNull final : public xiiGALInputLayout
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALInputLayoutNull(const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/Shader/Implementation/InputLayoutNull_inl.h>
