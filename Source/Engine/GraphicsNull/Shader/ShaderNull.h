#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSNULL_DLL xiiGALShaderNull final : public xiiGALShader
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALShaderNull(const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/Shader/Implementation/ShaderNull_inl.h>
