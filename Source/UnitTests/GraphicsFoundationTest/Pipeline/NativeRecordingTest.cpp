/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Streaming/GpuResidencyManager.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeRecording);

XII_CREATE_SIMPLE_TEST(NativeRecording, RetainedGeometryUploads)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshBufferResourceDescriptor descriptor;
  descriptor.AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList, true);
  const auto                mesh = xiiResourceManager::CreateResource<xiiMeshBufferResource>("Retained upload geometry", std::move(descriptor));
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Retained geometry uploads")))
    return;

  auto&                      graph = context.GetGraph();
  auto&                      cache = context.GetResourceCache();
  xiiRenderGraphBufferHandle metadata;
  xiiGeometryHandle          handles[2];
  xiiUInt64                  uiLastFrame = 0U;
  XII_SCOPE_EXIT(for (const auto handle : handles) if (!handle.IsInvalidated()) xiiGpuResidencyManager::UnregisterGeometry(handle, uiLastFrame));
  for (xiiUInt32 i = 0U; i < 2U; ++i)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    uiLastFrame = frame.GetFrameIndex();
    cache.BeginFrame(uiLastFrame, frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());

    if (i == 0U)
    {
      graph.BeginSetup(uiLastFrame);
      metadata = xiiGpuResidencyManager::DeclareUploadPass(graph).m_hGeometryMetadata;
      if (!XII_TEST_BOOL(metadata.IsValid()))
        return;
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile({}, nullptr, environment.GetDevice())))
        return;
    }
    else
      graph.BeginExecutionFrame(uiLastFrame);

    // Register a distinct record after declaration. Preparation must see the latest slots.
    xiiGeometryDescription description;
    auto&                  lod   = description.m_Lods.ExpandAndGetRef();
    lod.m_hMeshBuffer            = mesh;
    lod.m_fMinimumScreenCoverage = 0.25f * float(i + 1U);
    handles[i]                   = xiiGpuResidencyManager::RegisterGeometry(description);
    if (!XII_TEST_BOOL(!handles[i].IsInvalidated()))
      return;
    if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    environment.GetDevice()->WaitIdle();

    const auto* pRecord = xiiGpuResidencyManager::GetGpuRecord(handles[i]);
    if (!XII_TEST_BOOL(pRecord && pRecord->m_uiLodCount == 1U))
      return;
    const xiiUInt32      uiIndex = xiiGpuResidencyManager::GetMetadataBaseIndex(uiLastFrame) + handles[i].GetInternalID().m_InstanceIndex;
    xiiGpuGeometryRecord actual;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(metadata).Borrow(), xiiUInt64(uiIndex) * sizeof(actual), actual))
      return;
    XII_TEST_INT(actual.m_uiLodCount, 1U);
    XII_TEST_INT(actual.m_uiGeneration, pRecord->m_uiGeneration);
    XII_TEST_FLOAT(actual.m_Lods[0].m_fMinimumScreenCoverage, lod.m_fMinimumScreenCoverage, 0.0f);
    XII_TEST_INT(graph.GetPlanGeneration(), 1U);
  }
}

XII_CREATE_SIMPLE_TEST(NativeRecording, RetainedMaterialUploads)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiMaterialGpuStorage     storage;
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(storage.Initialize(environment.GetDevice(), {.m_uiMaxMaterials = 4U})) ||
      !XII_TEST_RESULT(context.Initialize("Retained material uploads")))
    return;

  xiiMaterialSchemaDescription description;
  description.m_sName = "Retained Material Upload";
  description.AddParameter("BaseColor", xiiMaterialParameterType::Color, xiiColor(1, 0, 0));
  xiiSharedPtr<xiiMaterialSchema>   pSchema   = XII_DEFAULT_NEW(xiiMaterialSchema);
  xiiSharedPtr<xiiMaterialInstance> pMaterial = XII_DEFAULT_NEW(xiiMaterialInstance);
  if (!XII_TEST_RESULT(pSchema->Build(description)) || !XII_TEST_RESULT(pMaterial->Initialize(pSchema, {})))
    return;
  const auto material = storage.RegisterMaterial(pMaterial);
  if (!XII_TEST_BOOL(!material.IsInvalidated()))
    return;

  auto&                      graph = context.GetGraph();
  auto&                      cache = context.GetResourceCache();
  xiiRenderGraphBufferHandle surface;
  for (xiiUInt32 i = 0; i < 3; ++i)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());

    const xiiColor color(1.0f - i * 0.5f, 0.0f, i * 0.5f);
    XII_TEST_RESULT(pMaterial->SetParameter("BaseColor", color));
    if (i == 0)
    {
      graph.BeginSetup(frame.GetFrameIndex());
      surface = storage.DeclareUploadPass(graph).m_hSurfaceData;
      XII_TEST_BOOL(surface == storage.DeclareUploadPass(graph).m_hSurfaceData);
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile({}, nullptr, environment.GetDevice())))
        return;
    }
    else
      graph.BeginExecutionFrame(frame.GetFrameIndex());

    if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache)))
      return;
    const auto            pSurface  = graph.GetExportedBuffer(surface);
    const xiiUInt32       uiElement = storage.GetSurfaceBaseIndex(frame.GetFrameIndex()) + material.GetInternalID().m_InstanceIndex;
    xiiGpuSurfaceMaterial record;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), pSurface.Borrow(), xiiUInt64(uiElement) * sizeof(record), record))
      return;
    XII_TEST_BOOL(record.BaseColorOpacity == xiiVec4(color.r, color.g, color.b, color.a));
    XII_TEST_INT(graph.GetPlanGeneration(), 1U);
  }

  storage.Shutdown();
  xiiRenderGraphFrame frame;
  if (!XII_TEST_RESULT(frame.Begin()))
    return;
  cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
  XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
  graph.BeginExecutionFrame(frame.GetFrameIndex());
  xiiStringBuilder error;
  XII_TEST_BOOL(graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error).Failed());
  XII_TEST_BOOL(error.FindSubString("material storage that declared this graph has retired") != nullptr);
}
XII_CREATE_SIMPLE_TEST(NativeRecording, FailureStopsDependentOutputAndRecovers)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Native recording failure")))
    return;
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    auto& graph = context.GetGraph();
    auto& cache = context.GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    graph.BeginSetup(frame.GetFrameIndex());
    struct Data
    {
      xiiRenderGraphTextureHandle m_Color;
      xiiUInt32                   m_uiPhase;
    };
    bool       dependentRecorded = false;
    xiiUInt32  uiPublications    = 0U;
    const auto source            = graph.AddPass<Data>("Native raster", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
      data.m_uiPhase=phase;
      xiiGALTextureCreationDescription description; description.m_Type=xiiGALResourceDimension::Texture2D; description.m_Size={8,8};
      description.m_Format=xiiGALResourceFormat::RGBA8UNormalized; description.m_BindFlags=xiiGALBindFlags::RenderTarget|xiiGALBindFlags::ShaderResource;
      data.m_Color=builder.WriteTexture("Native output",description,xiiGALResourceStateFlags::RenderTarget);
      builder.SetPassRenderPassManaged();
      builder.SetPassSubmissionCompletion([&uiPublications] { ++uiPublications; });
      builder.SetPassPreparation([&data](xiiRenderGraphPassContext& execute) {
        if(data.m_uiPhase==0) execute.ReportFailure("Injected preparation failure.");
      }); }, [](const Data& data, xiiRenderGraphPassContext& execute) {
      auto& commands=execute.GetCommandList();
      auto* texture=execute.GetTexture(data.m_Color);
      const auto state=texture->GetResourceState();
      commands.ClearRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget),xiiColor(0,1,0,1));
      if(texture->GetResourceState()!=state)
      {
        xiiGALStateTransitionDescription transition;
        transition.m_pResource=texture; transition.m_NewState=state; transition.m_TransitionFlags=xiiGALStateTransitionFlags::UpdateState;
        commands.TransitionResourceStates(xiiMakeArrayPtr(&transition,1));
      }
      if(data.m_uiPhase==1) execute.ReportFailure("Injected native raster pipeline failure."); });
    graph.AddPass<xiiRenderGraphTextureHandle>("Dependent display output", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      data=builder.ReadTexture(source.first->m_Color,xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data,xiiGALResourceStateFlags::ShaderResource); }, [&](const auto&, xiiRenderGraphPassContext&) { dependentRecorded = true; }, true);
    graph.EndSetup();
    xiiStringBuilder error;
    if (!XII_TEST_RESULT(graph.Compile({}, &error)))
      return;
    const auto executed = graph.Execute(environment.GetDevice(), nullptr, &context.GetBlackboard(), &cache, nullptr, &error);
    environment.GetDevice()->WaitIdle();
    XII_TEST_INT(uiPublications, phase == 2U ? 1U : 0U);
    if (phase < 2)
    {
      XII_TEST_BOOL(executed.Failed() && !dependentRecorded);
      XII_TEST_BOOL(error.FindSubString(phase == 0 ? "preparation failure" : "raster pipeline failure") != nullptr);
    }
    else
    {
      XII_TEST_RESULT(executed);
      XII_TEST_BOOL(dependentRecorded);
      xiiUInt32 pixel = 0;
      if (!xiiGpuTestReadback::ReadTexturePixel(*environment.GetDevice(), graph.GetExportedTexture(source.first->m_Color).Borrow(), 4, 4, pixel))
        return;
      XII_TEST_INT(pixel, 0xFF00FF00U);
    }
  }
}

XII_CREATE_SIMPLE_TEST(NativeRecording, MergeAndReuseRecording)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false, false)))
  {
    return;
  }
  xiiGALDevice* pDevice = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(pDevice->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderGraphTestContext context;
  if (!XII_TEST_RESULT(context.Initialize("Merged and parallel recording")))
  {
    return;
  }

  auto&      graph     = context.GetGraph();
  auto&      cache     = context.GetResourceCache();
  const bool bParallel = pDevice->GetGraphicsDeviceType() == xiiGALGraphicsDeviceType::Vulkan &&
    pDevice->GetFeatures().m_MultithreadedResourceCreation == xiiGALDeviceFeatureState::Enabled &&
    xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks) > 1U;
  xiiAtomicInteger32          iRecordedItems;
  xiiRenderGraphTextureHandle hOutput;
  for (xiiUInt32 uiIteration = 0U; uiIteration < 6U; ++uiIteration)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
    {
      return;
    }
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame(); frame.End());
    iRecordedItems = 0;
    if (uiIteration == 0U)
    {
      graph.BeginSetup(frame.GetFrameIndex());
      const auto AddRasterPass = [&](xiiStringView sName, xiiColor color, xiiUInt32 uiItems, bool bAllowMerge) {
        graph.AddPass<xiiRenderGraphTextureHandle>(sName, xiiGALCommandQueueFlags::Graphics, [&](xiiRenderGraphTextureHandle& ref_color, xiiRenderGraphBuilder& ref_builder) {
          if (!hOutput.IsValid())
          {
            xiiGALTextureCreationDescription description;
            description.m_Type = xiiGALResourceDimension::Texture2D;
            description.m_Size = {8U, 8U};
            description.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
            description.m_BindFlags = xiiGALBindFlags::RenderTarget;
            hOutput = ref_builder.DeclareTexture("Color", description);
          }
          hOutput = ref_color = ref_builder.WriteTexture(hOutput, xiiGALResourceStateFlags::RenderTarget);
          ref_builder.SetPassRenderPassManaged();
          ref_builder.SetPassAllowMerge(bAllowMerge);
          ref_builder.SetPassRecordingItemCount(uiItems);
          if (!bAllowMerge)
          {
            ref_builder.ExportTexture(ref_color, xiiGALResourceStateFlags::CopySource);
          } }, [&, color, uiItems](const xiiRenderGraphTextureHandle& hColor, xiiRenderGraphPassContext& ref_execute) {
          if (uiItems > 1U)
          {
            iRecordedItems.Add(static_cast<xiiInt32>(ref_execute.GetRecordingEnd() - ref_execute.GetRecordingStart()));
          }
          ref_execute.GetCommandList().ClearRenderTargetView(ref_execute.GetTexture(hColor)->GetDefaultView(xiiGALTextureViewType::RenderTarget), color); }, true);
      };
      AddRasterPass("Red", xiiColor::Red, 1U, true);
      AddRasterPass("Green", xiiColor::Green, 1U, true);
      AddRasterPass("Blue", xiiColor::Blue, 64U, false);
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile({}, nullptr, pDevice)))
      {
        return;
      }
      XII_TEST_INT(graph.GetMergeGroups().GetCount(), 2U);
      XII_TEST_INT(graph.GetStatistics().m_uiMergedPasses, 1U);
      XII_TEST_BOOL(graph.GetCompiledPasses()[2].m_MergeBlocker == xiiRenderGraphMergeBlocker::MergeDisabled);
    }
    else
    {
      graph.BeginExecutionFrame(frame.GetFrameIndex());
    }

    xiiStringBuilder sError;
    if (!XII_TEST_RESULT_MSG(graph.Execute(pDevice, nullptr, &context.GetBlackboard(), &cache, nullptr, &sError), "%s", sError.GetData()))
    {
      return;
    }
    XII_TEST_INT(graph.GetPlanGeneration(), 1U);
    XII_TEST_INT(graph.GetStatistics().m_uiSubmittedCount, 1U);
    XII_TEST_INT(graph.GetStatistics().m_uiReusedPrimaryLists, uiIteration == 0U ? 0U : 1U);
    XII_TEST_INT(graph.GetStatistics().m_uiSecondaryLists, bParallel ? 2U : 0U);
    if (uiIteration > 0U)
    {
      XII_TEST_INT(graph.GetStatistics().m_uiScratchAllocations, 0U);
    }
    XII_TEST_INT(iRecordedItems, 64);
    if (uiIteration == 5U)
    {
      xiiUInt32 uiPixel = 0U;
      if (xiiGpuTestReadback::ReadTexturePixel(*pDevice, graph.GetExportedTexture(hOutput).Borrow(), 4U, 4U, uiPixel))
      {
        XII_TEST_INT(uiPixel, 0xFFFF0000U);
      }
    }
  }
}
