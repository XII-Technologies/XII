/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ShaderCompilerDXIL/ShaderCompilerDXILDLL.h>

#include <GraphicsFoundation/Shader/InputLayout.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>

struct _D3D12_SHADER_INPUT_BIND_DESC;
typedef struct _D3D12_SHADER_INPUT_BIND_DESC D3D12_SHADER_INPUT_BIND_DESC;

struct ID3D12ShaderReflection;
struct ID3D12ShaderReflectionConstantBuffer;

class XII_SHADERCOMPILERDXIL_DLL xiiShaderCompilerDXIL : xiiGALShaderProgramCompiler
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderCompilerDXIL, xiiGALShaderProgramCompiler);

public:
  virtual void      GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& out_platforms) override;
  virtual xiiResult ModifyShaderSource(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) override;
  virtual xiiResult Compile(xiiGALShaderProgramData& inout_data, xiiLogInterface* pLog) override;

protected:
  virtual bool PermitCombinedImageSamplers() const { return true; }

private:
  /// Sets fixed set / slot bindings to each resource.
  /// The end result will have these properties:
  /// 1. Every binding name has a unique set / slot.
  /// 2. Bindings that already had a fixed set or slot (e.g. != -1) should not have these changed.
  /// 2. Set / slots can only be the same for two bindings if they have been changed to xiiGALShaderResourceType::TextureAndSampler.
  xiiResult DefineShaderResourceBindings(const xiiGALShaderProgramData& data, xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription>& inout_resourceBinding, xiiLogInterface* pLog);

  void CreateNewShaderResourceDeclaration(xiiStringView sPlatform, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration);

  xiiResult Initialize();
  xiiString GetProfileName(xiiStringView sPlatform, xiiEnum<xiiGALShaderType> stage);
  xiiResult CompileDXILShader(xiiStringView sFile, xiiStringView sSource, bool bDebug, xiiStringView sProfile, xiiStringView sEntryPoint, xiiDynamicArray<xiiUInt8>& out_ByteCode);

  xiiResult ReflectShaderStage(xiiGALShaderProgramData& inout_Data, xiiEnum<xiiGALShaderType> stage);
  xiiResult FillResourceBinding(xiiGALShaderResourceDescription& binding, ID3D12ShaderReflection* pReflector, const D3D12_SHADER_INPUT_BIND_DESC& info);
  xiiResult ReflectConstantBufferLayout(xiiGALShaderResourceDescription& binding, ID3D12ShaderReflectionConstantBuffer* pConstantBufferReflection);
  xiiResult FillSRVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D12_SHADER_INPUT_BIND_DESC& info);
  xiiResult FillUAVResourceBinding(xiiGALShaderResourceDescription& binding, const D3D12_SHADER_INPUT_BIND_DESC& info);

private:
  xiiMap<xiiStringView, xiiEnum<xiiGALInputLayoutSemantic>> m_InputLayoutMapping;
};
