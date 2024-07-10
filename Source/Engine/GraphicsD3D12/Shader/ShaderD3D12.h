#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSD3D12_DLL xiiGALShaderD3D12 final : public xiiGALShader
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALShaderD3D12, xiiGALShader);

public:
  const D3D12_SHADER_BYTECODE* GetD3D12ShaderByteCodeDescription() const;

protected:
  friend class xiiGALDeviceD3D12;
  friend class xiiMemoryUtils;

  xiiGALShaderD3D12(xiiGALDeviceD3D12* pDeviceD3D12, const xiiGALShaderCreationDescription& creationDescription);

  virtual ~xiiGALShaderD3D12();

  virtual xiiResult InitPlatform();

  virtual xiiResult DeInitPlatform();

protected:

  D3D12_SHADER_BYTECODE m_ShaderByteCodeD3D12;
};

#include <GraphicsD3D12/Shader/Implementation/ShaderD3D12_inl.h>
