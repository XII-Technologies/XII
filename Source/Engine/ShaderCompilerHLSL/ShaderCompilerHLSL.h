#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;

class XII_SHADERCOMPILERHLSL_DLL xiiShaderCompilerHLSL : public xiiShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerHLSL, xiiShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& Platforms) override
  {
    Platforms.PushBack("DX11_SM40_93");
    Platforms.PushBack("DX11_SM40");
    Platforms.PushBack("DX11_SM41");
    Platforms.PushBack("DX11_SM50");
  }

  virtual xiiResult Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog) override;

private:
  void                           ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
};
