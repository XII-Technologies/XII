#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#if BUILDSYSTEM_ENABLE_D3D11_SUPPORT

template <typename T>
struct xiiComPtr;

struct ID3D11ShaderReflectionConstantBuffer;

struct _D3D11_SHADER_INPUT_BIND_DESC;
typedef struct _D3D11_SHADER_INPUT_BIND_DESC D3D11_SHADER_INPUT_BIND_DESC;

struct ID3D11ShaderReflection;
struct ID3D11ShaderReflectionConstantBuffer;

class xiiShaderCompilerProgram;

class XII_SHADERCOMPILER_DLL xiiShaderCompilerD3D11
{
public:
  xiiResult CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode);

private:
  friend xiiShaderCompilerProgram;

  xiiResult                      ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
  xiiResult                      FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, xiiComPtr<ID3D11ShaderReflection>& pReflector, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult                      FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);
  xiiResult                      FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D11_SHADER_INPUT_BIND_DESC& info);
};

#endif
