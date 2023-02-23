
#include <ShaderCompiler/ShaderCompilerPCH.h>

#if VULKAN_SUPPORTED

#  include <DiligentCore/Graphics/GraphicsEngine/include/EngineMemory.h>
#  include <DiligentCore/Graphics/ShaderTools/include/DXBCUtils.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/DXCompiler.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/HLSLUtils.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/SPIRVShaderResources.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/SPIRVTools.hpp>
#  include <DiligentCore/Graphics/ShaderTools/include/ShaderToolsCommon.hpp>

#  include <ShaderCompiler/Implementation/Vulkan/ShaderCompilerVulkan.h>
#  include <ShaderCompiler/ShaderCompiler.h>
#  include <ShaderCompiler/ShaderMetadata.h>

#  if !DILIGENT_NO_HLSL
#    include <DiligentCore/Graphics/ShaderTools/include/SPIRVTools.hpp>
#  else
#    error Diligent Core must be built with HLSL support enabled.
#  endif
// #  include <spirv_cross/spirv_cross.hpp>
// #  include <spirv_cross/spirv_parser.hpp>

std::unique_ptr<Diligent::IDXCompiler> g_pDXCompiler = nullptr;

////////// Utility Functions //////////

Diligent::SHADER_TYPE GALToDiligentShaderStage(xiiGALShaderStage::Enum e)
{
  switch (e)
  {
    case xiiGALShaderStage::VertexShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_VERTEX;
    case xiiGALShaderStage::HullShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_HULL;
    case xiiGALShaderStage::DomainShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_DOMAIN;
    case xiiGALShaderStage::GeometryShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_GEOMETRY;
    case xiiGALShaderStage::PixelShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_PIXEL;
    case xiiGALShaderStage::ComputeShader:
      return Diligent::SHADER_TYPE::SHADER_TYPE_COMPUTE;

      XII_ASSERT_NOT_IMPLEMENTED;
  }
  return Diligent::SHADER_TYPE::SHADER_TYPE_UNKNOWN;
}

xiiResult xiiShaderCompilerVulkan::CompileShader(const char* szFile, const char* szSource, bool bDebug, xiiGALShaderStage::Enum Stage, const char* szProfile, const char* szEntryPoint, std::unique_ptr<Diligent::IDXCompiler>& pDXCompiler, xiiDynamicArray<xiiUInt8>& out_ByteCode, std::vector<xiiUInt32>& out_SpirvOutput)
{
  out_ByteCode.Clear();

  const char*      szCompileSource = szSource;
  xiiStringBuilder sDebugSource;

  xiiDynamicArray<const char*> args;

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//line ");
    szCompileSource = sDebugSource;

    args.PushBack("-Zi"); // Enable debug information.
    args.PushBack("-Od"); // Disable optimization
  }
  else
  {
    args.PushBack("-O3"); // Optimization Level 3
  }

  xiiComPtr<IDxcBlob>            pByteCode;
  xiiComPtr<Diligent::IDataBlob> pCompilerOutput;

  Diligent::ShaderCreateInfo ShaderCI;
  ShaderCI.EntryPoint      = szEntryPoint;
  ShaderCI.CompileFlags    = Diligent::SHADER_COMPILE_FLAG_NONE;
  ShaderCI.Source          = szCompileSource;
  ShaderCI.SourceLength    = (xiiUInt32)strlen(szCompileSource);
  ShaderCI.Desc.ShaderType = GALToDiligentShaderStage(Stage);
  ShaderCI.SourceLanguage  = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
  ShaderCI.HLSLVersion     = {6, 0};

  pDXCompiler->Compile(ShaderCI, ShaderCI.HLSLVersion, nullptr, pByteCode.RawDblPtr(), &out_SpirvOutput, pCompilerOutput.RawDblPtr());

  if (pByteCode == nullptr)
  {
    xiiLog::Error("No shader bytecode was generated.");
    return XII_FAILURE;
  }

  // SPIR-V bytecode generated from HLSL must be legalized into a valid Vulkan SPIR-V shader.
  std::vector<xiiUInt32> LegalizedSPIRV = Diligent::OptimizeSPIRV(out_SpirvOutput, spv_target_env::SPV_ENV_MAX, Diligent::SPIRV_OPTIMIZATION_FLAG_LEGALIZATION);
  if (!LegalizedSPIRV.empty())
  {
    out_SpirvOutput = std::move(LegalizedSPIRV);
  }
  else
  {
    xiiLog::Error("Failed to legalize SPIR-V shader generated from HLSL. This may result in undefined behavior.");
    return XII_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<xiiUInt32>(pByteCode->GetBufferSize()));

  xiiMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<xiiUInt8*>(pByteCode->GetBufferPointer()), out_ByteCode.GetCount());

  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerVulkan::ReflectShaderStage(xiiShaderProgramCompiler::xiiShaderProgramData& inout_Data, xiiGALShaderStage::Enum Stage, std::vector<xiiUInt32>& spirvOutput, std::unique_ptr<Diligent::IDXCompiler>& pDXCompiler, xiiMap<const char*, xiiGALVertexAttributeSemantic::Enum, CompareConstChar>& vertexInputMapping)
{
  XII_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();

  Diligent::ShaderDesc shaderDesc = {};
  shaderDesc.Name                 = "";
  shaderDesc.ShaderType           = GALToDiligentShaderStage(Stage);

  std::string sEntryPoint = "";

  auto& Allocator        = Diligent::GetRawAllocator();
  auto* pRawMem          = ALLOCATE(Allocator, "Memory for SPIRVShaderResources", Diligent::SPIRVShaderResources, 1);
  auto  LoadShaderInputs = Stage == xiiGALShaderStage::VertexShader;
  auto* pResources       = new (pRawMem) Diligent::SPIRVShaderResources //
    {
      Allocator,
      spirvOutput,
      shaderDesc,
      shaderDesc.UseCombinedTextureSamplers ? shaderDesc.CombinedSamplerSuffix : nullptr,
      LoadShaderInputs,
      true,
      sEntryPoint //
    };

  std::shared_ptr<Diligent::SPIRVShaderResources> pShaderResources;
  pShaderResources.reset(pResources, Diligent::STDDeleterRawMem<Diligent::SPIRVShaderResources>(Allocator));

  xiiUInt32 uiNumConstantBuffers = pShaderResources->GetNumUBs();
  for (xiiUInt32 i = 0; i < uiNumConstantBuffers; ++i)
  {
    const Diligent::ShaderCodeBufferDesc* constantBuffer = pShaderResources->GetUniformBufferDesc(i);
  }

  return XII_SUCCESS;
}

xiiShaderConstantBufferLayout* xiiShaderCompilerVulkan::ReflectConstantBufferLayout(xiiShaderStageBinary& pStageBinary, const char* szName)
{
  XII_LOG_BLOCK("Constant Buffer Layout", szName);

  return nullptr;
}

xiiResult xiiShaderCompilerVulkan::FillResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding)
{
  return XII_FAILURE;
}

xiiResult xiiShaderCompilerVulkan::FillSRVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding)
{
  return XII_FAILURE;
}

xiiResult xiiShaderCompilerVulkan::FillUAVResourceBinding(xiiShaderStageBinary& shaderBinary, xiiShaderResourceBinding& binding)
{
  return XII_FAILURE;
}

#endif
