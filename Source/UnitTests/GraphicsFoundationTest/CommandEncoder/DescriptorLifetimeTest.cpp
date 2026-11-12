/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Resources/TextureView.h>
#include <GraphicsFoundation/ShaderCompiler/PermutationGenerator.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>
#include <GraphicsFoundation/States/PipelineState.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>

namespace
{
  enum class DescriptorLifetimeMode
  {
    Primary,
    Secondary,
    Abandoned,
    MultiSubmit
  };

  xiiSharedPtr<xiiGALComputePipelineState> CreateDescriptorLifetimePipeline(xiiGPUTestingEnvironment& environment)
  {
    const xiiStringView  sShaderFile = "Shaders/DescriptorLifetime.xiiShader";
    xiiGALShaderCompiler compiler;
    if (!XII_TEST_RESULT(compiler.CompileShaderPermutationForPlatforms(sShaderFile, {}, xiiLog::GetThreadLocalLogSystem(), environment.GetShaderModel())))
      return {};

    xiiStringBuilder sPath(xiiGALShaderManager::GetCacheDirectory());
    sPath.AppendPath(environment.GetShaderModel(), sShaderFile);
    sPath.ChangeFileExtension("");
    if (sPath.EndsWith("."))
      sPath.Shrink(0U, 1U);
    sPath.AppendFormat("_{0}.xiiPermutation", xiiArgU(xiiGALPermutationVariable::CalculateHash({}), 8, true, 16, true));
    xiiFileReader file;
    if (!XII_TEST_RESULT(file.Open(sPath)))
      return {};
    xiiGALShaderPermutationBinary permutation;
    bool                          bOldVersion = false;
    if (!XII_TEST_RESULT(permutation.Read(file, bOldVersion)))
      return {};
    auto hash = permutation.m_ShaderStageHashes.Find(xiiGALShaderType::Compute);
    if (!XII_TEST_BOOL(hash.IsValid()))
      return {};
    auto* pBinary = xiiGALShaderStageBinary::LoadStageBinary(xiiGALShaderType::Compute, hash.Value(), environment.GetShaderModel());
    if (!XII_TEST_BOOL(pBinary != nullptr))
      return {};

    auto*                           pDevice = environment.GetDevice();
    xiiGALShaderCreationDescription shaderDescription;
    shaderDescription.m_ShaderType = xiiGALShaderType::Compute;
    shaderDescription.m_ByteCode   = const_cast<xiiGALShaderByteCode*>(pBinary->GetByteCode().Borrow());
    xiiGALComputePipelineStateCreationDescription description;
    description.m_pComputeShader = pDevice->CreateShader(shaderDescription);
    xiiGALPipelineResourceSignatureCreationDescription signature;
    for (const auto& reflected : pBinary->GetByteCode()->m_ShaderResourceBindings)
    {
      auto& resource          = signature.m_Resources.ExpandAndGetRef();
      resource.m_sName        = reflected.m_sName;
      resource.m_ShaderStages = reflected.m_ShaderStages;
      resource.m_uiArraySize  = reflected.m_uiArraySize;
      resource.m_uiBindSet    = reflected.m_uiDescriptorSet;
      resource.m_uiBindSlot   = reflected.m_uiBindIndex;
      resource.m_ResourceType = reflected.m_Type;
    }
    description.m_pPipelineResourceSignature = pDevice->CreatePipelineResourceSignature(signature);
    if (!XII_TEST_BOOL(description.m_pComputeShader && description.m_pPipelineResourceSignature))
      return {};
    return pDevice->CreateComputePipelineState(description);
  }

  void CheckDescriptorLifetime(DescriptorLifetimeMode mode)
  {
    const bool               bSecondary = mode == DescriptorLifetimeMode::Secondary;
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize()))
      return;
    auto* pDevice = environment.GetDevice();
    if (pDevice->GetGraphicsDeviceType() != xiiGALGraphicsDeviceType::Vulkan)
      return;
    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/UnitTests/GraphicsFoundationTest", "DescriptorLifetimeTest", "descriptorlifetimetest")))
      return;
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("DescriptorLifetimeTest"));
    auto pPipeline = CreateDescriptorLifetimePipeline(environment);
    if (!XII_TEST_BOOL(pPipeline != nullptr))
      return;

    xiiGALPipelineResourceDescription inputBinding, outputBinding;
    for (const auto& resource : pPipeline->GetDescription().m_pPipelineResourceSignature->GetDescription().m_Resources)
    {
      if (resource.m_sName == "Inputs")
        inputBinding = resource;
      else if (resource.m_sName == "Outputs")
        outputBinding = resource;
    }
    if (!XII_TEST_BOOL(inputBinding.m_uiArraySize == 2U && outputBinding.m_uiArraySize == 2U))
      return;

    xiiSharedPtr<xiiGALTexture>     textures[8];
    xiiSharedPtr<xiiGALTextureView> views[8];
    for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(textures); ++i)
    {
      xiiGALTextureCreationDescription description;
      description.m_Type      = xiiGALResourceDimension::Texture2D;
      description.m_Size      = {1U, 1U};
      description.m_Format    = xiiGALResourceFormat::RGBA32Float;
      description.m_BindFlags = i % 4U < 2U ? xiiGALBindFlags::ShaderResource : xiiGALBindFlags::UnorderedAccess;
      textures[i]             = pDevice->CreateTexture(description);
      if (!XII_TEST_BOOL(textures[i] != nullptr))
        return;
      views[i] = textures[i]->GetDefaultView(i % 4U < 2U ? xiiGALTextureViewType::ShaderResource : xiiGALTextureViewType::UnorderedAccess);
      if (!XII_TEST_BOOL(views[i] != nullptr))
        return;
    }

    xiiGALCommandListCreationDescription commandDescription;
    commandDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    if (mode == DescriptorLifetimeMode::MultiSubmit)
      commandDescription.m_Flags = xiiGALCommandListFlags::MultiSubmit;
    auto pPrimary = pDevice->CreateCommandList(commandDescription);
    if (!XII_TEST_BOOL(pPrimary != nullptr))
      return;
    pPrimary->Begin();
    const xiiColor colors[] = {xiiColor::Red, xiiColor::Green};
    for (xiiUInt32 i : {0U, 1U, 4U, 5U})
      pPrimary->UpdateTexture(textures[i], {}, xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), {1U, 1U, 1U}), xiiGALTextureSubResourceData(xiiConstByteBlobPtr(reinterpret_cast<const xiiUInt8*>(&colors[i % 2U]), sizeof(xiiColor)), sizeof(xiiColor), sizeof(xiiColor)));

    xiiSharedPtr<xiiGALCommandList> pRecording = pPrimary;
    if (bSecondary)
    {
      // Finish transitions before the secondary records against the shared tracked states.
      pPrimary->End();
      auto* pQueue = pDevice->GetCommandQueue();
      pQueue->WaitForFenceValue(pQueue->Submit(pPrimary));
      pPrimary->Begin();
      pRecording = pDevice->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics, .m_Flags = xiiGALCommandListFlags::Secondary});
      if (!XII_TEST_BOOL(pRecording != nullptr))
        return;
      pRecording->Begin();
    }

    for (xiiUInt32 uiBatch = 0U; uiBatch < 2U; ++uiBatch)
    {
      const xiiUInt32 uiBase = uiBatch * 4U;
      pRecording->SetPipelineState(pPipeline);
      pRecording->SetShaderResourceTextureView(inputBinding, views[uiBase]);
      pRecording->SetUnorderedAccessTextureView(outputBinding, views[uiBase + 2U]);
      xiiGALTextureView* inputs[]  = {views[uiBase].Borrow(), views[uiBase + 1U].Borrow()};
      xiiGALTextureView* outputs[] = {views[uiBase + 2U].Borrow(), views[uiBase + 3U].Borrow()};
      pRecording->SetShaderResourceTextureViews(inputBinding, 0U, inputs);
      pRecording->SetUnorderedAccessTextureViews(outputBinding, 0U, outputs);
      if (!XII_TEST_RESULT(pRecording->CommitShaderResources()))
        return;
      // Repeated descriptor snapshots must retain each object only once.
      for (xiiUInt32 uiRepeat = 0U; uiRepeat < 3U; ++uiRepeat)
      {
        pRecording->SetShaderResourceTextureView(inputBinding, views[uiBase]);
        if (!XII_TEST_RESULT(pRecording->CommitShaderResources()))
          return;
      }
      pRecording->DispatchCompute({1U, 1U, 1U});
    }
    pRecording->InvalidateState();
    pRecording->End();
    if (bSecondary)
      pPrimary->Submit(pRecording);

    bool bRetained = true;
    for (const auto& view : views)
      bRetained &= XII_TEST_INT(view->GetRefCount(), 2U);
    bRetained &= XII_TEST_INT(pPipeline->GetRefCount(), 2U);
    if (!bRetained)
    {
      // Do not submit invalid native descriptors when running against the unfixed backend.
      pPrimary->Reset();
      return;
    }

    if (mode == DescriptorLifetimeMode::Abandoned)
    {
      pPrimary->Reset();
      for (const auto& view : views)
        XII_TEST_INT(view->GetRefCount(), 1U);
      XII_TEST_INT(pPipeline->GetRefCount(), 1U);
      return;
    }

    pDevice->BeginFrame();
    xiiGALTextureView* originalViews[8];
    for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(views); ++i)
    {
      originalViews[i] = views[i].Borrow();
      views[i].Clear();
    }
    // Poll destruction while the descriptors are recorded but have not been submitted.
    pDevice->EndFrame();
    for (xiiUInt32 i = 0U; i < XII_ARRAY_SIZE(views); ++i)
    {
      views[i] = textures[i]->GetDefaultView(i % 4U < 2U ? xiiGALTextureViewType::ShaderResource : xiiGALTextureViewType::UnorderedAccess);
      XII_TEST_BOOL(views[i].Borrow() == originalViews[i]);
    }
    if (bSecondary)
    {
      pRecording.Clear();
      pPrimary->End();
    }
    auto*     pQueue  = pDevice->GetCommandQueue();
    xiiUInt64 uiFence = pQueue->Submit(pPrimary);
    if (mode == DescriptorLifetimeMode::MultiSubmit)
    {
      uiFence = pQueue->Submit(pPrimary);
      pPrimary->Reset();
    }
    for (const auto& view : views)
      XII_TEST_BOOL(view->GetRefCount() > 1U);
    pQueue->WaitForFenceValue(uiFence);
    pRecording.Clear();
    pDevice->WaitIdle();
    for (const auto& view : views)
      XII_TEST_INT(view->GetRefCount(), 1U);
    XII_TEST_INT(pPipeline->GetRefCount(), 1U);

    xiiGALTextureReadback readback(pDevice);
    for (xiiUInt32 i : {2U, 3U, 6U, 7U})
    {
      pPrimary->Begin();
      if (!XII_TEST_RESULT(readback.Enqueue(pPrimary, {.m_pTexture = textures[i]})))
        return;
      pPrimary->End();
      pQueue->WaitForFenceValue(pQueue->Submit(pPrimary));
      pPrimary->Reset();
      readback.WaitForNextCompleted();
      const auto capture = readback.GetCompleted();
      if (!XII_TEST_BOOL(static_cast<bool>(capture)))
        return;
      XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
      pPrimary->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (XII_TEST_RESULT(pPrimary->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        const float* pixel = static_cast<const float*>(mapped.m_pData);
        for (xiiUInt32 uiChannel = 0U; uiChannel < 4U; ++uiChannel)
          XII_TEST_FLOAT(pixel[uiChannel], colors[i % 2U].GetData()[uiChannel], 0.001f);
        XII_TEST_RESULT(pPrimary->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
      }
      pPrimary->End();
      pPrimary->Reset();
    }
  }
} // namespace

XII_CREATE_SIMPLE_TEST(CommandEncoder, PrimaryDescriptorLifetime)
{
  CheckDescriptorLifetime(DescriptorLifetimeMode::Primary);
}

XII_CREATE_SIMPLE_TEST(CommandEncoder, SecondaryDescriptorLifetime)
{
  CheckDescriptorLifetime(DescriptorLifetimeMode::Secondary);
}

XII_CREATE_SIMPLE_TEST(CommandEncoder, AbandonedDescriptorLifetime)
{
  CheckDescriptorLifetime(DescriptorLifetimeMode::Abandoned);
}

XII_CREATE_SIMPLE_TEST(CommandEncoder, MultiSubmitDescriptorLifetime)
{
  CheckDescriptorLifetime(DescriptorLifetimeMode::MultiSubmit);
}
