#pragma once

#include <GraphicsNull/GraphicsNullDLL.h>

#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

class XII_GRAPHICSNULL_DLL xiiGALShaderResourceVariableNull final : public xiiGALShaderResourceVariable
{
public:
protected:
  friend class xiiGALDeviceNull;
  friend class xiiMemoryUtils;

  xiiGALShaderResourceVariableNull();

  virtual ~xiiGALShaderResourceVariableNull();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsNull/Shader/Implementation/ShaderResourceVariableNull_inl.h>
