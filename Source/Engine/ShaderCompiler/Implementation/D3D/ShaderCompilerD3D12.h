#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#if BUILDSYSTEM_ENABLE_D3D12_SUPPORT

template <typename T>
struct xiiComPtr;

struct IDxcBlob;

struct _D3D12_SHADER_INPUT_BIND_DESC;
typedef struct _D3D12_SHADER_INPUT_BIND_DESC D3D12_SHADER_INPUT_BIND_DESC;

struct ID3D12ShaderReflection;
struct ID3D12ShaderReflectionConstantBuffer;

class xiiShaderCompilerProgram;

class XII_SHADERCOMPILER_DLL xiiShaderCompilerD3D12
{
public:
  xiiResult CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode, xiiComPtr<IDxcBlob>& out_pOutputBlob);

private:
  friend xiiShaderCompilerProgram;

  xiiResult                      ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiComPtr<IDxcBlob>& pShaderBlob, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, ID3D12ShaderReflectionConstantBuffer* pConstantBufferReflection);
  xiiResult                      FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, xiiComPtr<ID3D12ShaderReflection>& pReflector, const D3D12_SHADER_INPUT_BIND_DESC& info);
  xiiResult                      FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D12_SHADER_INPUT_BIND_DESC& info);
  xiiResult                      FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const D3D12_SHADER_INPUT_BIND_DESC& info);
};

#endif
