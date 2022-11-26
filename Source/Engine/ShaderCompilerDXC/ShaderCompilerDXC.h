#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;

class XII_SHADERCOMPILERDXC_DLL xiiShaderCompilerDXC : public xiiShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerDXC, xiiShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& Platforms) override { Platforms.PushBack("VULKAN"); }

  virtual xiiResult Compile(xiiShaderProgramData& inout_Data, xiiLogInterface* pLog) override;

private:
  xiiResult                      ReflectShaderStage(xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage);
  xiiShaderConstantBufferLayout* ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& pConstantBufferReflection);
  xiiResult                      FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  xiiResult                      FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  xiiResult                      FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);

  xiiResult Initialize();

private:
  xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
