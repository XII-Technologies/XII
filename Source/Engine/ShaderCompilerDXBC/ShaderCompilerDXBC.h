#pragma once

#include <ShaderCompilerDXBC/ShaderCompilerDXBCDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>

template <typename T>
struct xiiComPtr;

struct ID3D11ShaderReflectionConstantBuffer;

struct _D3D11_SHADER_INPUT_BIND_DESC;
typedef struct _D3D11_SHADER_INPUT_BIND_DESC D3D11_SHADER_INPUT_BIND_DESC;

struct ID3D11ShaderReflection;
struct ID3D11ShaderReflectionConstantBuffer;

class XII_SHADERCOMPILERDXBC_DLL xiiShaderCompilerDXBC : xiiGALShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerDXBC, xiiGALShaderProgramCompiler);

public:
  virtual void      GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) override;
  virtual xiiResult ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) override;
  virtual xiiResult Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) override;

private:
  xiiResult     Initialize();
  xiiStringView GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage);
  xiiResult     CompileDXBCShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode);

  xiiResult ReflectShaderStage(xiiGALShaderProgramData& inout_Data, xiiEnum<xiiGALShaderType> stage);
  xiiResult ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  xiiResult FillResourceBinding(xiiGALShaderResourceDescription& binding, xiiComPtr<ID3D11ShaderReflection>& pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);

private:
  xiiMap<xiiStringView, xiiEnum<xiiGALInputLayoutSemantic>> m_InputLayoutMapping;
};
