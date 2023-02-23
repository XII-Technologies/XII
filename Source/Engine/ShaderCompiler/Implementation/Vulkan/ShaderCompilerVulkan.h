#pragma once

#include <ShaderCompiler/ShaderCompilerDLL.h>

#if VULKAN_SUPPORTED

template <typename T>
struct xiiComPtr;

struct IDxcBlob;

class xiiShaderCompilerProgram;

class XII_SHADERCOMPILER_DLL xiiShaderCompilerVulkan
{
public:
  xiiResult CompileShader(const char* szFile, const char* szSource, bool bDebug, xiiGALShaderStage::Enum Stage, const char* szProfile, const char* szEntryPoint, std::unique_ptr<Diligent::IDXCompiler>& pDXCompiler, xiiDynamicArray<xiiUInt8>& out_ByteCode, std::vector<xiiUInt32>& out_SpirvOutput);

private:
  friend xiiShaderCompilerProgram;

  xiiResult                      ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, std::vector<xiiUInt32>& spirvOutput, std::unique_ptr<Diligent::IDXCompiler>& pDXCompiler, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName);
  xiiResult                      FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding);
  xiiResult                      FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding);
  xiiResult                      FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding);
};

#endif
