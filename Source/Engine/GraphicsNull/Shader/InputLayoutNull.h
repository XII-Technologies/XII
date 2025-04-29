#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>

class XII_GRAPHICSNULL_DLL xiiGALInputLayoutNull final : public xiiGALInputLayout
{
public:
protected:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceNull;
  friend class xiiGALShaderNull;

  xiiGALInputLayoutNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALInputLayoutCreationDescription& creationDescription);

  virtual ~xiiGALInputLayoutNull();

  virtual xiiResult InitPlatform() override final;
};
