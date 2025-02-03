#pragma once

#include <ShaderCompilerFXC/ShaderCompilerFXCDLL.h>

#include <GraphicsCore/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/Shader/InputLayout.h>

struct ID3D11ShaderReflectionConstantBuffer;

struct _D3D11_SHADER_INPUT_BIND_DESC;
typedef struct _D3D11_SHADER_INPUT_BIND_DESC D3D11_SHADER_INPUT_BIND_DESC;

struct ID3D11ShaderReflection;
struct ID3D11ShaderReflectionConstantBuffer;

class XII_SHADERCOMPILERFXC_DLL xiiShaderCompilerFXC : public xiiShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerFXC, xiiShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) override
  {
    //out_platforms.PushBack("D3D_SM40_93");
    //out_platforms.PushBack("D3D_SM40");
    //out_platforms.PushBack("D3D_SM41");
    //out_platforms.PushBack("D3D_SM50");
  }

  virtual xiiResult ModifyShaderSource(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) override;
  virtual xiiResult Compile(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) override;

private:
  void Initialize();

  xiiResult DefineShaderResourceBindings(const xiiShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog);
  void      CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration);

  xiiResult ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiBitflags<xiiGALShaderType> Stage);
  xiiResult FillResourceBinding(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflection* pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);

private:
  xiiMap<xiiStringView, xiiGALInputLayoutSemantic::Enum> m_InputLayoutMapping;
};
