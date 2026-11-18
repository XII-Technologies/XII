/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Output/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearColorPass.h>
#include <GraphicsCore/Pipeline/Passes/Utility/ClearTexturePass.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST(PipelineScheduling, ClearColorFallback)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;

  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(
    environment.GetDevice()->WaitIdle();
    xiiStartup::ShutdownHighLevelSystems();
    xiiGALDevice::SetDefaultDevice(nullptr);
    environment.Shutdown(););

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid texture and color properties are rejected without GPU work")
  {
    xiiClearTexturePass  create;
    xiiVariantDictionary properties;
    create.m_OutputTexture.SetFormat(xiiGALResourceFormat::D32Float);
    XII_TEST_RESULT(create.GetProperties(properties));
    XII_TEST_BOOL(create.Configure(properties).Failed());
    properties.Clear();
    create.m_OutputTexture.SetFormat(xiiGALResourceFormat::RGBA16Float);
    auto requirements      = create.m_OutputTexture.GetRequirements();
    requirements.m_uiWidth = 65537U;
    create.m_OutputTexture.SetRequirements(requirements);
    XII_TEST_RESULT(create.GetProperties(properties));
    XII_TEST_BOOL(create.Configure(properties).Failed());

    xiiClearColorPass clear;
    properties.Clear();
    properties.Insert("Color", xiiColor(xiiMath::NaN<float>(), 0, 0));
    XII_TEST_BOOL(clear.Configure(properties).Failed());
  }

  xiiRenderPipelineResourceDescriptor descriptor;
  xiiVariantDictionary                properties;
  properties.Insert("Color", xiiColor::Red);
  const auto uiClear   = descriptor.AddPass("Clear Color", xiiGetStaticRTTI<xiiClearTexturePass>(), properties);
  const auto uiPresent = descriptor.AddPass("Present", xiiGetStaticRTTI<xiiPresentPass>());
  descriptor.m_Interface.PushBack({"ViewTarget", true, descriptor.m_Passes[uiPresent].m_Id, "Target"});
  if (!XII_TEST_RESULT(descriptor.Connect(uiClear, "Texture", uiPresent, "GradedColor")))
    return;

  auto                fixedDescriptor = descriptor;
  xiiClearTexturePass fixedTexture;
  fixedTexture.m_Color = xiiColor::Red;
  fixedTexture.m_OutputTexture.SetFormat(xiiGALResourceFormat::RGBA8UNormalized);
  auto fixedRequirements       = fixedTexture.m_OutputTexture.GetRequirements();
  fixedRequirements.m_uiWidth  = 8;
  fixedRequirements.m_uiHeight = 12;
  fixedTexture.m_OutputTexture.SetRequirements(fixedRequirements);
  fixedTexture.m_OutputTexture.SetResolution({xiiRenderPipelineTextureResolutionMode::Fixed, 8, 12});
  XII_TEST_RESULT(fixedTexture.GetProperties(fixedDescriptor.m_Passes[uiClear].m_Properties));
  const auto hFixed                   = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Explicit texture pin pipeline", std::move(fixedDescriptor));
  const auto hFallback                = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Clear color pipeline test fallback", std::move(descriptor));
  const auto hPreviousLoadingFallback = xiiResourceManager::GetResourceTypeLoadingFallback<xiiRenderPipelineResource>();
  const auto hPreviousMissingFallback = xiiResourceManager::GetResourceTypeMissingFallback<xiiRenderPipelineResource>();
  xiiResourceManager::SetResourceTypeLoadingFallback<xiiRenderPipelineResource>(hFallback);
  xiiResourceManager::SetResourceTypeMissingFallback<xiiRenderPipelineResource>(hFallback);
  XII_SCOPE_EXIT(
    xiiResourceManager::SetResourceTypeLoadingFallback<xiiRenderPipelineResource>(hPreviousLoadingFallback);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiRenderPipelineResource>(hPreviousMissingFallback););

  const auto hMissing = xiiResourceManager::LoadResource<xiiRenderPipelineResource>("ClearColorPipelineTest/Missing.xiiRenderPipeline");
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("Missing Resource of Type 'xiiRenderPipelineResource'", xiiLogMsgType::SeriousWarningMsg);
    log.ExpectMessage("is missing or failed to compile", xiiLogMsgType::ErrorMsg);
    xiiRenderPipelineInstance loadingInstance;
    XII_TEST_RESULT(loadingInstance.Synchronize(hMissing));
    XII_TEST_BOOL(loadingInstance.GetPipeline() != nullptr);
    xiiResourceLock<xiiRenderPipelineResource> missing(hMissing, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    XII_TEST_BOOL(missing.IsValid());
    XII_TEST_BOOL(missing.GetAcquireResult() == xiiResourceAcquireResult::MissingFallback);
    // The first synchronization may have selected a loading fallback. Observe
    // the completed missing-resource state before requiring its diagnostic.
    XII_TEST_RESULT(loadingInstance.Synchronize(hMissing));
  }

  xiiWorldDescription worldDescription("Clear color fallback test");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  auto*     pModule = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60.0f, 0.1f, 100.0f);
  xiiView*   pView = nullptr;
  const auto hView = pModule->CreateView("Clear color fallback", pView);
  XII_SCOPE_EXIT(pModule->DestroyView(hView););
  pView->SetCamera(&camera);
  pView->SetRenderPipelineResource(hMissing);

  xiiTestLogInterface   fallbackLog;
  xiiTestLogSystemScope fallbackLogScope(&fallbackLog, true);
  fallbackLog.ExpectMessage("is missing or failed to compile", xiiLogMsgType::ErrorMsg);
  xiiUInt32 uiFrame = 0;
  for (const xiiSizeU32 size : {xiiSizeU32(16, 24), xiiSizeU32(32, 16), xiiSizeU32(24, 24)})
  {
    const bool bFixedTexture = uiFrame++ == 2;
    if (bFixedTexture)
      pView->SetRenderPipelineResource(hFixed);
    xiiGALTextureCreationDescription targetDescription;
    targetDescription.m_Type      = xiiGALResourceDimension::Texture2D;
    targetDescription.m_Size      = size;
    targetDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    targetDescription.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto pTarget                  = environment.GetDevice()->CreateTexture(targetDescription);
    if (!XII_TEST_BOOL(pTarget != nullptr))
      return;

    pView->SetViewport(xiiRectFloat(0, 0, static_cast<float>(size.width), static_cast<float>(size.height)));
    pView->SetRenderTargetView(pTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow());
    world.Update();
    environment.GetDevice()->WaitIdle();
    const auto& instance = pView->GetRenderPipelineInstance();
    if (!XII_TEST_BOOL(instance.GetPipeline() != nullptr))
      return;
    XII_TEST_INT(instance.GetPipeline()->GetExecutionOrder().GetCount(), 2);
    XII_TEST_BOOL(instance.GetPipeline()->GetExtractionRequirements().IsNoFlagSet());
    XII_TEST_INT(pView->GetRenderGraph()->GetStatistics().m_uiCompiledPassCount, 2);
    const auto* pCreated = instance.GetOutputBinding(uiClear, "Texture");
    if (XII_TEST_BOOL(pCreated != nullptr))
    {
      const auto* pDescription = pView->GetRenderGraph()->GetTextureDescription(pCreated->m_hTexture);
      if (XII_TEST_BOOL(pDescription != nullptr))
      {
        XII_TEST_INT(pDescription->m_Size.width, bFixedTexture ? 8 : size.width);
        XII_TEST_INT(pDescription->m_Size.height, bFixedTexture ? 12 : size.height);
        XII_TEST_BOOL(pDescription->m_Format == (bFixedTexture ? xiiGALResourceFormat::RGBA8UNormalized : xiiGALResourceFormat::RGBA16Float));
      }
    }

    xiiGALTextureReadback                readback(environment.GetDevice());
    xiiGALCommandListCreationDescription commandDescription;
    commandDescription.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto pCommands                  = environment.GetDevice()->CreateCommandList(commandDescription);
    pCommands->Begin();
    xiiGALTextureReadback::ReadbackRequest request;
    request.m_pTexture = pTarget.Borrow();
    XII_TEST_RESULT(readback.Enqueue(pCommands.Borrow(), request));
    pCommands->End();
    auto* pQueue = environment.GetDevice()->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    pQueue->WaitForFenceValue(pQueue->Submit(pCommands.Borrow()));
    readback.WaitForNextCompleted();
    auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return;

    pCommands->Begin();
    xiiGALMappedTextureSubresource mapped;
    xiiGALTextureMipLevelData      mip;
    if (XII_TEST_RESULT(pCommands->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      for (xiiUInt32 y = 0; y < size.height; ++y)
      {
        const auto* pRow = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt8*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < size.width; ++x)
        {
          XII_TEST_INT(pRow[x * 4], 255);
          XII_TEST_INT(pRow[x * 4 + 1], 0);
          XII_TEST_INT(pRow[x * 4 + 2], 0);
          XII_TEST_INT(pRow[x * 4 + 3], 255);
        }
      }
      XII_TEST_RESULT(pCommands->UnmapTextureSubresource(capture.m_pStagingTexture, mip));
    }
    pCommands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
    pView->SetRenderTargetView(nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Authored presentation refreshes a retained declaration")
  {
    auto*                            pDevice = environment.GetDevice();
    xiiGALTextureCreationDescription description;
    description.m_Type                    = xiiGALResourceDimension::Texture2D;
    description.m_Size                    = {8U, 8U};
    description.m_Format                  = xiiGALResourceFormat::RGBA8UNormalized;
    description.m_BindFlags               = xiiGALBindFlags::RenderTarget;
    xiiSharedPtr<xiiGALTexture> targets[] = {pDevice->CreateTexture(description), pDevice->CreateTexture(description)};
    if (!XII_TEST_BOOL(targets[0] != nullptr && targets[1] != nullptr))
      return;

    xiiRenderPipelineInstance instance;
    if (!XII_TEST_RESULT(instance.Synchronize(hFixed)))
      return;

    const xiiUInt32             targetInterface = instance.GetPipeline()->FindInterface("ViewTarget", true);
    xiiRenderGraphTextureHandle hTarget;
    const auto                  hGraph = xiiRenderGraphManager::CreateGraph("Authored presentation", [&](xiiRenderGraph& ref_graph, xiiRenderGraphBlackboard& ref_blackboard, const xiiView* pExecutionView) {
      ref_graph.AddPass<xiiRenderGraphTextureHandle>("ViewTarget", xiiGALCommandQueueFlags::Graphics, [&](auto& ref_handle, xiiRenderGraphBuilder& ref_builder) {
        hTarget = ref_handle = ref_builder.DeclareImportedTexture("ViewTarget", description); }, [](const auto&, xiiRenderGraphPassContext&) {});
      const xiiRenderPipelineInputBinding input(ref_graph, targetInterface, hTarget);
      XII_TEST_BOOL(pExecutionView == pView);
      return instance.Build(pView, ref_graph, ref_blackboard, {}, xiiMakeArrayPtr(&input, 1U)); }, [&](xiiRenderGraph& ref_graph, xiiRenderGraphBlackboard&, const xiiView* pExecutionView) {
      auto pTexture = pExecutionView->GetRenderTargetView()->GetTexture();
      return ref_graph.BindImportedTexture(hTarget, pTexture, pTexture->GetResourceState()); });
    XII_SCOPE_EXIT(xiiRenderGraphManager::DestroyGraph(hGraph).AssertSuccess());
    constexpr xiiUInt32 uiExecutions = 1000U;
    for (xiiUInt32 uiFrameIndex = 0U; uiFrameIndex < uiExecutions; ++uiFrameIndex)
    {
      const auto& pTarget = targets[uiFrameIndex % XII_ARRAY_SIZE(targets)];
      pView->SetRenderTargetView(pTarget->GetDefaultView(xiiGALTextureViewType::RenderTarget).Borrow());
      const float fOffset = uiFrameIndex < uiExecutions / 2U ? 0.0f : 4.0f;
      pView->SetViewport({fOffset, fOffset, 4.0f, 4.0f});
      pDevice->BeginFrame();
      XII_SCOPE_EXIT(pDevice->EndFrame());
      xiiRenderGraphFrame frame;
      if (!XII_TEST_RESULT(frame.Begin()))
        return;

      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(xiiRenderGraphManager::ExecuteGraph(frame, hGraph, pView, {}, &error), "%s", error.GetData()))
        return;
      const auto* pGraph  = xiiRenderGraphManager::GetGraph(hGraph);
      const auto* pOutput = instance.GetOutputBinding(uiPresent, "ViewOutput");
      XII_TEST_BOOL(pOutput != nullptr && pGraph->GetExportedTexture(pOutput->m_hTexture) == pTarget);
      XII_TEST_INT(pGraph->GetPlanGeneration(), 1U);
      XII_TEST_INT(pGraph->GetStatistics().m_uiStructuralBuildCount, 1U);
      XII_TEST_INT(pGraph->GetStatistics().m_uiPlanReuseCount, uiFrameIndex);
    }
    xiiUInt32 uiPixel = 0U;
    if (xiiGpuTestReadback::ReadTexturePixel(*pDevice, targets[1].Borrow(), 6U, 6U, uiPixel))
      XII_TEST_INT(uiPixel, 0xFF0000FFU);
    pView->SetRenderTargetView(nullptr);
  }
}
