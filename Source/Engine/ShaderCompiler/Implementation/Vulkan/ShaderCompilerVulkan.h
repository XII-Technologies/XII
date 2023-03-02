#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#if VULKAN_SUPPORTED

template <typename T>
struct xiiComPtr;

struct IDxcBlob;

struct SpvReflectDescriptorBinding;

class xiiShaderCompilerProgram;

class XII_SHADERCOMPILER_DLL xiiShaderCompilerVulkan
{
public:
  xiiResult CompileShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode, xiiComPtr<IDxcBlob>& out_pOutputBlob);

private:
  friend xiiShaderCompilerProgram;

  xiiResult                      ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, xiiComPtr<IDxcBlob>& pShaderBlob, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName, const SpvReflectDescriptorBinding& constantBufferReflection);
  xiiResult                      FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  xiiResult                      FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  xiiResult                      FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
};

#endif
