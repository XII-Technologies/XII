/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Device/DeviceFactory.h>
#include <GraphicsFoundation/Resources/Framebuffer.h>
#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>
#include <GraphicsFoundation/States/PipelineState.h>

XII_CREATE_SIMPLE_TEST_GROUP(ShaderCompiler);

XII_CREATE_SIMPLE_TEST(ShaderCompiler, ShaderCompiler)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shader text sections")
  {
    const char* szShader = R"(
// leading source
[platforms]
ALL
[PERMUTATIONS]
QUALITY
MODE = MODE_FAST
[VERTEX_SHADER]
void VSMain() {}
[pixel_shader]
void PSMain() {}
)";

    xiiGALShaderTextSectionizer sections;
    xiiGALShaderSections::GetShaderSections(szShader, sections);

    xiiUInt32 uiFirstLine = 0U;
    xiiStringView content = sections.GetSectionContent(xiiGALShaderSections::Platforms, uiFirstLine);
    XII_TEST_INT(uiFirstLine, 3U);
    XII_TEST_BOOL(content.FindSubString("ALL") != nullptr);
    XII_TEST_BOOL(content.FindSubString("QUALITY") == nullptr);

    content = sections.GetSectionContent(xiiGALShaderSections::Permutations, uiFirstLine);
    XII_TEST_INT(uiFirstLine, 5U);
    XII_TEST_BOOL(content.FindSubString("QUALITY") != nullptr);
    XII_TEST_BOOL(content.FindSubString("MODE = MODE_FAST") != nullptr);
    XII_TEST_BOOL(content.FindSubString("VSMain") == nullptr);

    content = sections.GetSectionContent(xiiGALShaderSections::VertexShader, uiFirstLine);
    XII_TEST_INT(uiFirstLine, 8U);
    XII_TEST_BOOL(content.FindSubString("void VSMain()") != nullptr);

    content = sections.GetSectionContent(xiiGALShaderSections::ComputeShader, uiFirstLine);
    XII_TEST_INT(uiFirstLine, 0U);
    XII_TEST_BOOL(content.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Permutation section parsing")
  {
    xiiHybridArray<xiiHashedString, 16>       dynamicVariables;
    xiiHybridArray<xiiGALPermutationVariable, 16> fixedVariables;
    xiiGALShaderParser::ParsePermutationSection(
      "QUALITY\n"
      "// ignored comment\n"
      "MODE = MODE_FAST\n"
      "SKINNED\n",
      dynamicVariables, fixedVariables);

    XII_TEST_INT(dynamicVariables.GetCount(), 2U);
    XII_TEST_STRING(dynamicVariables[0].GetView(), "QUALITY");
    XII_TEST_STRING(dynamicVariables[1].GetView(), "SKINNED");
    XII_TEST_INT(fixedVariables.GetCount(), 1U);
    XII_TEST_STRING(fixedVariables[0].m_sName.GetView(), "MODE");
    XII_TEST_STRING(fixedVariables[0].m_sValue.GetView(), "MODE_FAST");

    xiiVariant                           defaultValue;
    xiiGALShaderParser::EnumDefinition definition;
    xiiGALShaderParser::ParsePermutationVariableConfiguration(" bool USE_SHADOWS = true ", defaultValue, definition);
    XII_TEST_STRING(definition.m_sName, "USE_SHADOWS");
    XII_TEST_BOOL(defaultValue.IsA<bool>());
    XII_TEST_BOOL(defaultValue.Get<bool>());

    defaultValue = xiiVariant();
    definition   = {};
    xiiGALShaderParser::ParsePermutationVariableConfiguration(
      "enum QUALITY { QUALITY_LOW = 2, default = 3, QUALITY_HIGH = 4 };", defaultValue, definition);
    XII_TEST_STRING(definition.m_sName, "QUALITY");
    XII_TEST_INT(definition.m_uiDefaultValue, 3U);
    XII_TEST_INT(defaultValue.Get<xiiUInt32>(), 3U);
    XII_TEST_INT(definition.m_Values.GetCount(), 2U);
    XII_TEST_STRING(definition.m_Values[0].m_sValueName.GetView(), "QUALITY_LOW");
    XII_TEST_INT(definition.m_Values[0].m_iValueValue, 2);
    XII_TEST_STRING(definition.m_Values[1].m_sValueName.GetView(), "QUALITY_HIGH");
    XII_TEST_INT(definition.m_Values[1].m_iValueValue, 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Permutation generation")
  {
    xiiHashedString quality, low, high, skinned, disabled, enabled;
    quality.Assign("QUALITY");
    low.Assign("LOW");
    high.Assign("HIGH");
    skinned.Assign("SKINNED");
    disabled.Assign("FALSE");
    enabled.Assign("TRUE");

    xiiGALPermutationGenerator generator;
    generator.AddPermutation(quality, low);
    generator.AddPermutation(quality, high);
    generator.AddPermutation(quality, high); // Duplicate values must not multiply the search space.
    generator.AddPermutation(skinned, disabled);
    generator.AddPermutation(skinned, enabled);
    XII_TEST_INT(generator.GetPermutationCount(), 4U);

    bool combinations[2][2] = {};
    for (xiiUInt32 uiPermutation = 0; uiPermutation < generator.GetPermutationCount(); ++uiPermutation)
    {
      xiiHybridArray<xiiGALPermutationVariable, 16> variables;
      generator.GetPermutation(uiPermutation, variables);
      XII_TEST_INT(variables.GetCount(), 2U);

      xiiUInt32 uiQuality = xiiInvalidIndex;
      xiiUInt32 uiSkinning = xiiInvalidIndex;
      for (const xiiGALPermutationVariable& variable : variables)
      {
        if (variable.m_sName == quality)
          uiQuality = variable.m_sValue == low ? 0U : 1U;
        else if (variable.m_sName == skinned)
          uiSkinning = variable.m_sValue == disabled ? 0U : 1U;
      }

      XII_TEST_BOOL(uiQuality < 2U);
      XII_TEST_BOOL(uiSkinning < 2U);
      XII_TEST_BOOL(!combinations[uiQuality][uiSkinning]);
      combinations[uiQuality][uiSkinning] = true;
    }

    generator.RemovePermutations(skinned);
    XII_TEST_INT(generator.GetPermutationCount(), 2U);
    generator.Clear();
    XII_TEST_INT(generator.GetPermutationCount(), 1U);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shader resource parsing and binding")
  {
    const char* szSource = R"(
Texture2D<float4> Albedo : register(t3, space2);
RWStructuredBuffer<float4> Output[4] : register(u5, space1);
SamplerState LinearSampler : register(sAUTO, space0);
cbuffer Globals : register(b1, space0)
{
  float4 Tint;
};
)";

    xiiDynamicArray<xiiGALShaderResourceDefinition> resources;
    xiiGALShaderParser::ParseShaderResources(szSource, resources);
    XII_TEST_INT(resources.GetCount(), 4U);

    XII_TEST_STRING(resources[0].m_ResourceDescription.m_sName.GetView(), "Albedo");
    XII_TEST_BOOL(resources[0].m_ResourceDescription.m_Type == xiiGALShaderResourceType::TextureSRV);
    XII_TEST_BOOL(resources[0].m_ResourceDescription.m_TextureType == xiiGALShaderTextureType::Texture2D);
    XII_TEST_INT(resources[0].m_ResourceDescription.m_uiArraySize, 1U);
    XII_TEST_INT(resources[0].m_ResourceDescription.m_uiBindIndex, 3U);
    XII_TEST_INT(resources[0].m_ResourceDescription.m_uiDescriptorSet, 2U);
    XII_TEST_STRING(resources[0].m_sDeclaration, "Texture2D<float4> Albedo");

    XII_TEST_STRING(resources[1].m_ResourceDescription.m_sName.GetView(), "Output");
    XII_TEST_BOOL(resources[1].m_ResourceDescription.m_Type == xiiGALShaderResourceType::BufferUAV);
    XII_TEST_INT(resources[1].m_ResourceDescription.m_uiArraySize, 4U);
    XII_TEST_INT(resources[1].m_ResourceDescription.m_uiBindIndex, 5U);
    XII_TEST_INT(resources[1].m_ResourceDescription.m_uiDescriptorSet, 1U);

    XII_TEST_STRING(resources[2].m_ResourceDescription.m_sName.GetView(), "LinearSampler");
    XII_TEST_BOOL(resources[2].m_ResourceDescription.m_Type == xiiGALShaderResourceType::Sampler);
    XII_TEST_INT(resources[2].m_ResourceDescription.m_uiArraySize, 1U);
    XII_TEST_INT(resources[2].m_ResourceDescription.m_uiBindIndex, xiiInvalidIndex);
    XII_TEST_INT(resources[2].m_ResourceDescription.m_uiDescriptorSet, 0U);

    XII_TEST_STRING(resources[3].m_ResourceDescription.m_sName.GetView(), "Globals");
    XII_TEST_BOOL(resources[3].m_ResourceDescription.m_Type == xiiGALShaderResourceType::ConstantBuffer);
    XII_TEST_INT(resources[3].m_ResourceDescription.m_uiArraySize, 1U);

    xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription> bindings;
    for (const xiiGALShaderResourceDefinition& resource : resources)
    {
      xiiGALShaderResourceDescription binding = resource.m_ResourceDescription;
      binding.m_uiBindIndex                     = bindings.GetCount() + 7U;
      binding.m_uiDescriptorSet                 = 4U;
      bindings.Insert(binding.m_sName, binding);
    }

    xiiStringBuilder patchedSource;
    xiiGALShaderParser::ApplyShaderResourceBindings(
      "TEST", szSource, resources, bindings,
      [](xiiStringView, xiiStringView sDeclaration, const xiiGALShaderResourceDescription& binding, xiiStringBuilder& out_sDeclaration) {
        out_sDeclaration.SetFormat("{} : register(x{}, space{})", sDeclaration, binding.m_uiBindIndex, binding.m_uiDescriptorSet);
      },
      patchedSource);

    XII_TEST_BOOL(patchedSource.FindSubString("Texture2D<float4> Albedo : register(x7, space4)") != nullptr);
    XII_TEST_BOOL(patchedSource.FindSubString("RWStructuredBuffer<float4> Output[4] : register(x8, space4)") != nullptr);
    XII_TEST_BOOL(patchedSource.FindSubString("register(t3, space2)") == nullptr);

    xiiMuteLog muteLog;
    xiiGALShaderResourceDescription validBinding;
    validBinding.m_sName.Assign("Valid");
    validBinding.m_uiBindIndex     = 0U;
    validBinding.m_uiDescriptorSet = 0U;
    xiiHashTable<xiiHashedString, xiiGALShaderResourceDescription> sanityBindings;
    sanityBindings.Insert(validBinding.m_sName, validBinding);
    XII_TEST_BOOL(xiiGALShaderParser::SanityCheckShaderResourceBindings(sanityBindings, &muteLog).Succeeded());

    sanityBindings.Find(validBinding.m_sName).Value().m_uiBindIndex = xiiInvalidIndex;
    XII_TEST_BOOL(xiiGALShaderParser::SanityCheckShaderResourceBindings(sanityBindings, &muteLog).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shader state parsing and serialization")
  {
    xiiGALShaderStateResourceDescriptor state;
    XII_TEST_BOOL(state.Parse(
      "AlphaToCoverage = true\n"
      "IndependentBlend = true\n"
      "BlendEnable0 = true\n"
      "SourceBlend0 = BlendFactor_SourceAlpha\n"
      "DestinationBlend0 = BlendFactor_InverseSourceAlpha\n"
      "ColorMask0 = 5\n"
      "FillMode = FillMode_Wireframe\n"
      "CullMode = CullMode_Front\n"
      "FrontCounterClockwise = true\n"
      "DepthBias = -7\n"
      "DepthBiasClamp = 1.25\n"
      "SlopeScaledDepthBias = 2.5\n"
      "DepthEnable = true\n"
      "DepthWriteEnable = false\n"
      "ComparisonDepthFunction = ComparisonFunction_GreaterEqual\n"
      "StencilEnable = true\n"
      "StencilReadMask = 63\n"
      "StencilWriteMask = 31\n"
      "FrontFaceStencilPassOperation = StencilOperation_Replace\n")
                    .Succeeded());

    XII_TEST_BOOL(state.m_BlendDescription.m_bAlphaToCoverage);
    XII_TEST_BOOL(state.m_BlendDescription.m_bIndependentBlend);
    XII_TEST_INT(state.m_BlendDescription.m_RenderTargets.GetCount(), 8U);
    XII_TEST_BOOL(state.m_BlendDescription.m_RenderTargets[0].m_bBlendEnable);
    XII_TEST_BOOL(state.m_BlendDescription.m_RenderTargets[0].m_SourceBlend == xiiGALBlendFactor::SourceAlpha);
    XII_TEST_BOOL(state.m_BlendDescription.m_RenderTargets[0].m_DestinationBlend == xiiGALBlendFactor::InverseSourceAlpha);
    XII_TEST_INT(state.m_BlendDescription.m_RenderTargets[0].m_ColorMask.GetValue(), 5U);
    XII_TEST_BOOL(state.m_RasterizerDescription.m_FillMode == xiiGALFillMode::Wireframe);
    XII_TEST_BOOL(state.m_RasterizerDescription.m_CullMode == xiiGALCullMode::Front);
    XII_TEST_BOOL(state.m_RasterizerDescription.m_bFrontCounterClockwise);
    XII_TEST_INT(state.m_RasterizerDescription.m_iDepthBias, -7);
    XII_TEST_FLOAT(state.m_RasterizerDescription.m_fDepthBiasClamp, 1.25f, 0.0001f);
    XII_TEST_FLOAT(state.m_RasterizerDescription.m_fSlopeScaledDepthBias, 2.5f, 0.0001f);
    XII_TEST_BOOL(state.m_DepthStencilDescription.m_bDepthEnable);
    XII_TEST_BOOL(!state.m_DepthStencilDescription.m_bDepthWriteEnable);
    XII_TEST_BOOL(state.m_DepthStencilDescription.m_ComparisonDepthFunction == xiiGALComparisonFunction::GreaterEqual);
    XII_TEST_BOOL(state.m_DepthStencilDescription.m_bStencilEnable);
    XII_TEST_INT(state.m_DepthStencilDescription.m_uiStencilReadMask, 63U);
    XII_TEST_INT(state.m_DepthStencilDescription.m_uiStencilWriteMask, 31U);
    XII_TEST_BOOL(state.m_DepthStencilDescription.m_FrontFace.m_StencilPassOperation == xiiGALStencilOperation::Replace);

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    state.Save(writer);

    xiiGALShaderStateResourceDescriptor loadedState;
    xiiMemoryStreamReader                reader(&storage);
    loadedState.Load(reader);
    XII_TEST_BOOL(loadedState.m_BlendDescription == state.m_BlendDescription);
    XII_TEST_BOOL(loadedState.m_RasterizerDescription == state.m_RasterizerDescription);
    XII_TEST_BOOL(loadedState.m_DepthStencilDescription == state.m_DepthStencilDescription);
    XII_TEST_INT(loadedState.CalculateHash(), state.CalculateHash());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Permutation binary round trip")
  {
    xiiGALShaderPermutationBinary source;
    source.m_ShaderStageHashes.Insert(xiiGALShaderType::Vertex, 0x12345678U);
    source.m_ShaderStageHashes.Insert(xiiGALShaderType::Pixel, 0x9ABCDEF0U);
    source.m_StateDescriptor.m_RasterizerDescription.m_CullMode = xiiGALCullMode::Front;
    source.m_StateDescriptor.m_DepthStencilDescription.m_bDepthEnable = true;

    xiiGALPermutationVariable& quality = source.m_PermutationVariables.ExpandAndGetRef();
    quality.m_sName.Assign("QUALITY");
    quality.m_sValue.Assign("QUALITY_HIGH");
    xiiGALPermutationVariable& skinned = source.m_PermutationVariables.ExpandAndGetRef();
    skinned.m_sName.Assign("SKINNED");
    skinned.m_sValue.Assign("TRUE");

    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    XII_TEST_BOOL(source.Write(writer).Succeeded());
    XII_TEST_BOOL(storage.GetStorageSize64() > 0U);

    xiiGALShaderPermutationBinary loaded;
    xiiMemoryStreamReader         reader(&storage);
    bool                          bOldVersion = true;
    XII_TEST_BOOL(loaded.Read(reader, bOldVersion).Succeeded());
    XII_TEST_BOOL(!bOldVersion);
    XII_TEST_INT(loaded.m_ShaderStageHashes.GetCount(), 2U);
    XII_TEST_INT(loaded.m_ShaderStageHashes.Find(xiiGALShaderType::Vertex).Value(), 0x12345678U);
    XII_TEST_INT(loaded.m_ShaderStageHashes.Find(xiiGALShaderType::Pixel).Value(), 0x9ABCDEF0U);
    XII_TEST_BOOL(loaded.m_StateDescriptor.m_RasterizerDescription == source.m_StateDescriptor.m_RasterizerDescription);
    XII_TEST_BOOL(loaded.m_StateDescriptor.m_DepthStencilDescription == source.m_StateDescriptor.m_DepthStencilDescription);
    XII_TEST_INT(loaded.m_PermutationVariables.GetCount(), 2U);
    XII_TEST_STRING(loaded.m_PermutationVariables[0].m_sName.GetView(), "QUALITY");
    XII_TEST_STRING(loaded.m_PermutationVariables[0].m_sValue.GetView(), "QUALITY_HIGH");
    XII_TEST_STRING(loaded.m_PermutationVariables[1].m_sName.GetView(), "SKINNED");
    XII_TEST_STRING(loaded.m_PermutationVariables[1].m_sValue.GetView(), "TRUE");

  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compile and reuse the cache for every selected backend")
  {
    XII_TEST_BOOL(xiiConfigureGPUTestDataDirectories().Succeeded());

    for (xiiUInt32 uiImplementation = 0; uiImplementation < xiiGetGPUTestingEnvironmentCount(); ++uiImplementation)
    {
      const xiiStringView sImplementation = xiiGetGPUTestingEnvironmentName(uiImplementation);
      xiiStringView       sShaderModel;
      xiiStringView       sShaderCompiler;
      xiiGALDeviceFactory::GetShaderModelAndCompiler(sImplementation, sShaderModel, sShaderCompiler);
      XII_TEST_BOOL(!sShaderModel.IsEmpty());
      XII_TEST_BOOL(!sShaderCompiler.IsEmpty());
      if (sShaderModel.IsEmpty() || sShaderCompiler.IsEmpty())
        continue;

      XII_TEST_BOOL(xiiPlugin::LoadPlugin(sShaderCompiler).Succeeded());
      xiiGALShaderManager::Configure(sShaderModel, true, ":shadercache/GraphicsFoundationTest");
      XII_TEST_STRING(xiiGALShaderManager::GetActivePlatform(), sShaderModel);
      XII_TEST_BOOL(xiiGALShaderManager::IsRuntimeCompilationEnabled());

      const xiiStringView sShaderFile = "Shaders/Minimal.xiiShader";
      xiiFileReader       shaderFile;
      const xiiResult     shaderOpenResult = shaderFile.Open(sShaderFile);
      XII_TEST_BOOL(shaderOpenResult.Succeeded());
      if (shaderOpenResult.Failed())
      {
        xiiPlugin::UnloadAllPlugins();
        continue;
      }
      shaderFile.Close();
      xiiGALShaderCompiler compiler;
      const xiiResult firstCompileResult = compiler.CompileShaderPermutationForPlatforms(sShaderFile, {}, xiiLog::GetThreadLocalLogSystem(), sShaderModel);
      XII_TEST_BOOL(firstCompileResult.Succeeded());
      if (firstCompileResult.Failed())
      {
        xiiPlugin::UnloadAllPlugins();
        continue;
      }
      XII_TEST_BOOL(compiler.CompileShaderPermutationForPlatforms(sShaderFile, {}, xiiLog::GetThreadLocalLogSystem(), sShaderModel).Succeeded());

      xiiStringBuilder sPermutationFile = xiiGALShaderManager::GetCacheDirectory();
      sPermutationFile.AppendPath(sShaderModel);
      sPermutationFile.AppendPath(sShaderFile);
      sPermutationFile.ChangeFileExtension("");
      if (sPermutationFile.EndsWith("."))
        sPermutationFile.Shrink(0U, 1U);
      sPermutationFile.AppendFormat("_{0}.xiiPermutation", xiiArgU(xiiGALPermutationVariable::CalculateHash({}), 8, true, 16, true));

      xiiFileReader permutationFile;
      const xiiResult permutationOpenResult = permutationFile.Open(sPermutationFile);
      XII_TEST_BOOL(permutationOpenResult.Succeeded());
      if (permutationOpenResult.Failed())
      {
        xiiPlugin::UnloadAllPlugins();
        continue;
      }
      xiiGALShaderPermutationBinary permutation;
      bool                          bOldVersion = true;
      XII_TEST_BOOL(permutation.Read(permutationFile, bOldVersion).Succeeded());
      XII_TEST_BOOL(!bOldVersion);

      auto vertexHash = permutation.m_ShaderStageHashes.Find(xiiGALShaderType::Vertex);
      auto pixelHash  = permutation.m_ShaderStageHashes.Find(xiiGALShaderType::Pixel);
      XII_TEST_BOOL(vertexHash.IsValid());
      XII_TEST_BOOL(pixelHash.IsValid());
      if (!vertexHash.IsValid() || !pixelHash.IsValid())
      {
        xiiPlugin::UnloadAllPlugins();
        continue;
      }

      xiiGALShaderStageBinary* pVertexBinary = xiiGALShaderStageBinary::LoadStageBinary(xiiGALShaderType::Vertex, vertexHash.Value(), sShaderModel);
      xiiGALShaderStageBinary* pPixelBinary  = xiiGALShaderStageBinary::LoadStageBinary(xiiGALShaderType::Pixel, pixelHash.Value(), sShaderModel);
      XII_TEST_BOOL(pVertexBinary != nullptr);
      XII_TEST_BOOL(pPixelBinary != nullptr);
      if (pVertexBinary == nullptr || pPixelBinary == nullptr)
      {
        xiiPlugin::UnloadAllPlugins();
        continue;
      }

      xiiGPUTestingEnvironment environment(sImplementation);
      XII_TEST_BOOL(environment.Initialize().Succeeded());
      if (environment.GetDevice() == nullptr)
        continue;

      xiiGALDevice* pDevice = environment.GetDevice();
      xiiGALShaderCreationDescription vertexShaderDescription;
      vertexShaderDescription.m_ShaderType = xiiGALShaderType::Vertex;
      vertexShaderDescription.m_ByteCode   = const_cast<xiiGALShaderByteCode*>(pVertexBinary->GetByteCode().Borrow());
      xiiSharedPtr<xiiGALShader> pVertexShader = pDevice->CreateShader(vertexShaderDescription);
      XII_TEST_BOOL(pVertexShader != nullptr);

      xiiGALShaderCreationDescription pixelShaderDescription;
      pixelShaderDescription.m_ShaderType = xiiGALShaderType::Pixel;
      pixelShaderDescription.m_ByteCode   = const_cast<xiiGALShaderByteCode*>(pPixelBinary->GetByteCode().Borrow());
      xiiSharedPtr<xiiGALShader> pPixelShader = pDevice->CreateShader(pixelShaderDescription);
      XII_TEST_BOOL(pPixelShader != nullptr);
      if (pVertexShader == nullptr || pPixelShader == nullptr)
        continue;

      pVertexShader->SetDebugName("Compiled Unit Test Vertex Shader");
      pPixelShader->SetDebugName("Compiled Unit Test Pixel Shader");
      XII_TEST_STRING(pVertexShader->GetDebugName(), "Compiled Unit Test Vertex Shader");
      XII_TEST_STRING(pPixelShader->GetDebugName(), "Compiled Unit Test Pixel Shader");

      xiiGALInputLayoutCreationDescription inputLayoutDescription;
      xiiSharedPtr<xiiGALInputLayout> pInputLayout = pVertexShader->CreateInputLayout(inputLayoutDescription);
      XII_TEST_BOOL(pInputLayout != nullptr);

      xiiGALPipelineResourceSignatureCreationDescription signatureDescription;
      xiiSharedPtr<xiiGALPipelineResourceSignature> pSignature = pDevice->CreatePipelineResourceSignature(signatureDescription);
      XII_TEST_BOOL(pSignature != nullptr);

      xiiGALBlendStateCreationDescription blendDescription;
      blendDescription.m_RenderTargets.ExpandAndGetRef();
      xiiSharedPtr<xiiGALBlendState> pBlendState = pDevice->CreateBlendState(blendDescription);
      xiiSharedPtr<xiiGALRasterizerState> pRasterizerState = pDevice->CreateRasterizerState(permutation.m_StateDescriptor.m_RasterizerDescription);
      XII_TEST_BOOL(pBlendState != nullptr);
      XII_TEST_BOOL(pRasterizerState != nullptr);

      xiiGALRenderPassCreationDescription renderPassDescription;
      auto& attachment                = renderPassDescription.m_Attachments.ExpandAndGetRef();
      attachment.m_Format            = xiiGALResourceFormat::RGBA8UNormalized;
      attachment.m_uiSampleCount     = 1U;
      attachment.m_LoadOperation     = xiiGALAttachmentLoadOperation::Clear;
      attachment.m_StoreOperation    = xiiGALAttachmentStoreOperation::Store;
      attachment.m_InitialStateFlags = xiiGALResourceStateFlags::Undefined;
      attachment.m_FinalStateFlags   = xiiGALResourceStateFlags::RenderTarget;
      renderPassDescription.m_SubPasses.ExpandAndGetRef().m_RenderTargetAttachments.PushBack({0U, xiiGALResourceStateFlags::RenderTarget});
      xiiSharedPtr<xiiGALRenderPass> pRenderPass = pDevice->CreateRenderPass(renderPassDescription);
      XII_TEST_BOOL(pRenderPass != nullptr);

      if (pInputLayout != nullptr && pSignature != nullptr && pBlendState != nullptr && pRasterizerState != nullptr && pRenderPass != nullptr)
      {
        xiiGALGraphicsPipelineStateCreationDescription pipelineDescription;
        pipelineDescription.m_pPipelineResourceSignature         = pSignature;
        pipelineDescription.m_pVertexShader                      = pVertexShader;
        pipelineDescription.m_pPixelShader                       = pPixelShader;
        pipelineDescription.m_GraphicsPipeline.m_pInputLayout    = pInputLayout;
        pipelineDescription.m_GraphicsPipeline.m_pBlendState     = pBlendState;
        pipelineDescription.m_GraphicsPipeline.m_pRasterizerState = pRasterizerState;
        pipelineDescription.m_GraphicsPipeline.m_pRenderPass     = pRenderPass;
        xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline = pDevice->CreateGraphicsPipelineState(pipelineDescription);
        XII_TEST_BOOL(pPipeline != nullptr);
        if (pPipeline != nullptr)
        {
          pPipeline->SetDebugName("Compiled Unit Test Graphics Pipeline");
          XII_TEST_STRING(pPipeline->GetDebugName(), "Compiled Unit Test Graphics Pipeline");

          xiiGALTextureCreationDescription renderTargetDescription;
          renderTargetDescription.m_Type               = xiiGALResourceDimension::Texture2D;
          renderTargetDescription.m_Size               = xiiSizeU32(16U, 16U);
          renderTargetDescription.m_uiArraySizeOrDepth = 1U;
          renderTargetDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
          renderTargetDescription.m_uiMipLevels        = 1U;
          renderTargetDescription.m_BindFlags          = xiiGALBindFlags::RenderTarget;
          renderTargetDescription.m_Usage              = xiiGALResourceUsage::Mutable;
          xiiSharedPtr<xiiGALTexture> pRenderTarget = pDevice->CreateTexture(renderTargetDescription);
          XII_TEST_BOOL(pRenderTarget != nullptr);

          xiiGALFramebufferCreationDescription framebufferDescription;
          framebufferDescription.m_pRenderPass       = pRenderPass;
          framebufferDescription.m_FramebufferSize   = renderTargetDescription.m_Size;
          framebufferDescription.m_uiArraySliceCount = 1U;
          if (pRenderTarget != nullptr)
            framebufferDescription.m_Attachments.PushBack(pRenderTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget));
          xiiSharedPtr<xiiGALFramebuffer> pFramebuffer = pDevice->CreateFramebuffer(framebufferDescription);
          XII_TEST_BOOL(pFramebuffer != nullptr);

          xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
          XII_TEST_BOOL(pQueue != nullptr);
          if (pFramebuffer != nullptr && pQueue != nullptr)
          {
            xiiGALCommandListCreationDescription commandListDescription;
            commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
            xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
            XII_TEST_BOOL(pCommandList != nullptr);
            if (pCommandList != nullptr)
            {
              xiiGALOptimizedClearValue clearValue;
              clearValue.m_ResourceFormat = xiiGALResourceFormat::RGBA8UNormalized;
              clearValue.m_ClearColour    = xiiColor::RebeccaPurple;

              pCommandList->Begin();
              pCommandList->BeginRenderPass({pRenderPass.Borrow(), pFramebuffer.Borrow(), xiiMakeArrayPtr(&clearValue, 1U)});
              pCommandList->SetViewport(xiiRectFloat(0.0f, 0.0f, 16.0f, 16.0f));
              pCommandList->SetPipelineState(pPipeline.Borrow());
              XII_TEST_BOOL(pCommandList->CommitShaderResources().Succeeded());
              pCommandList->Draw({3U});
              pCommandList->EndRenderPass();
              pCommandList->End();

              XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiBeginRenderPass, 1U);
              XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiSetPipelineState, 1U);
              XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiCommitShaderResources, 1U);
              XII_TEST_INT(pCommandList->GetStatistics().m_CommandListCounters.m_uiDraw, 1U);
              XII_TEST_INT(pCommandList->GetStatistics().m_PrimitiveCounters[xiiGALPrimitiveTopology::TriangleList], 1U);

              const xiiUInt64 uiFenceValue = pQueue->Submit(pCommandList.Borrow());
              XII_TEST_BOOL(uiFenceValue > 0U);
              pQueue->WaitForFenceValue(uiFenceValue);
            }
          }
        }
      }
    }
  }
}
