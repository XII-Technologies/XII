#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/ShaderResourceVariable.h>

class XII_GRAPHICSD3D12_DLL xiiGALShaderResourceVariableD3D12 final : public xiiGALShaderResourceVariable
{
public:
protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderResourceVariableD3D12();

  virtual ~xiiGALShaderResourceVariableD3D12();

  virtual xiiResult InitPlatform(xiiGALDevice* pDevice);

  virtual xiiResult DeInitPlatform(xiiGALDevice* pDevice);
};

#include <GraphicsD3D12/Shader/Implementation/ShaderResourceVariableD3D12_inl.h>
