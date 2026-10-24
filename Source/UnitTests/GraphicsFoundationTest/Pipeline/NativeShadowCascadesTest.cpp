/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundationTest/GraphicsFoundationTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Pipeline/Passes/Shadows/ShadowCascadePass.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>
#include <GraphicsFoundationTest/TestingEnvironment/BufferReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/RenderGraphTestContext.h>
#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeShadowCascades);
XII_CREATE_SIMPLE_TEST(NativeShadowCascades, SharedDefinitionPrivateCameraSnapshots)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  const auto                                 asset = xiiCreateShadowCascadesTestResource();
  xiiResourceLock<xiiRenderPipelineResource> loaded(asset, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.IsValid() && loaded->GetAuthoring() && loaded->GetPipeline()))
    return;
  const auto                          authoring = loaded->GetAuthoring();
  const auto                          sourceId  = authoring->m_Descriptor.m_Passes[0].m_Id;
  xiiRenderPipelineResourceDescriptor root;
  root.m_References.PushBack({authoring->m_Descriptor.m_Id, asset.GetResourceID()});
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    const auto instance = root.AddSubgraph(i ? "Three cascades" : "One cascade", authoring->m_Descriptor.m_Id);
    root.m_Interface.PushBack({i ? "Three" : "One", false, root.m_Passes[instance].m_Id, "CascadeConstants"});
    if (i)
    {
      auto& override = root.m_Passes[instance].m_Overrides.ExpandAndGetRef();
      override.m_Path.PushBack(sourceId);
      auto settings = authoring->m_Descriptor.m_Passes[0].m_Properties.GetValue("Settings")->Get<xiiVariantDictionary>();
      settings.Insert("CascadeCount", 3U);
      settings.Insert("MaximumShadowDistance", 90.0f);
      override.m_Properties.Insert("Settings", settings);
      override.m_Properties.Insert("LightDirection", xiiVec3(1, 0, -1));
    }
  }
  const auto                                 resource = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Nested directional cascades", std::move(root));
  xiiResourceLock<xiiRenderPipelineResource> resolved(resource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  const auto                                 compiled = resolved->GetPipeline();
  if (!XII_TEST_BOOL(compiled != nullptr))
    return;
  XII_TEST_BOOL(compiled->GetExtractionRequirements().IsNoFlagSet() && compiled->GetRuntimeFeatures().IsNoFlagSet());
  xiiWorldDescription description("Cascade snapshot views");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  auto*                     module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*                  views[2];
  xiiCamera                 cameras[2];
  xiiRenderPipelineInstance pipelines[2];
  xiiRenderGraphTestContext contexts[2];
  for (xiiUInt32 v = 0; v < 2; ++v)
  {
    module->CreateView(v ? "Portrait orthographic" : "Landscape perspective", views[v]);
    views[v]->SetCamera(&cameras[v]);
    views[v]->SetViewport(v ? xiiRectFloat(0, 0, 32, 64) : xiiRectFloat(0, 0, 64, 32));
    cameras[v].SetCameraMode(v ? xiiCameraMode::OrthoFixedWidth : xiiCameraMode::PerspectiveFixedFovY, v ? 20.0f : 60.0f, 0.1f, 150);
    if (!XII_TEST_RESULT(pipelines[v].Initialize(compiled)) || !XII_TEST_RESULT(contexts[v].Initialize("Cascade constants view")))
      return;
  }
  XII_TEST_BOOL(pipelines[0].GetPass(0) != pipelines[1].GetPass(0) && pipelines[0].GetPass(0) != pipelines[0].GetPass(1));
  for (xiiUInt32 phase = 0; phase < 3; ++phase)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin()))
      return;
    xiiMat4 firstViewProjection = xiiMat4::MakeIdentity();
    for (xiiUInt32 v = 0; v < 2; ++v)
    {
      auto& graph = contexts[v].GetGraph();
      auto& cache = contexts[v].GetResourceCache();
      cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
      XII_SCOPE_EXIT(cache.EndFrame());
      graph.BeginSetup(frame.GetFrameIndex());
      const auto* oldSnapshot = pipelines[v].GetOutputBinding(0, "CascadeSnapshot");
      XII_TEST_BOOL(!oldSnapshot->m_pFrameData || oldSnapshot->m_pFrameData->GetSetupSerial() != graph.GetSetupSerial());
      cameras[v].LookAt(xiiVec3(float(phase * 2), float(v * 3), 4), xiiVec3(4, 0, 0), xiiVec3(0, 0, 1));
      xiiVariantDictionary parameters;
      if (phase == 2)
        parameters.Insert("ShadowLightDirection", xiiVec3(0, 1, -1));
      if (!XII_TEST_RESULT(pipelines[v].Build(views[v], graph, contexts[v].GetBlackboard(), parameters)))
        return;
      xiiStaticArray<xiiShadowCascadeDescription, 4> snapshots[2];
      xiiRenderGraphBufferHandle                     buffers[2];
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const auto* pass      = static_cast<const xiiShadowCascadePass*>(pipelines[v].GetPass(i));
        const auto  frameData = pipelines[v].GetOutputBinding(i, "CascadeSnapshot")->m_pFrameData.Downcast<const xiiShadowCascadeSnapshot>();
        if (!XII_TEST_BOOL(frameData && frameData->GetSetupSerial() == graph.GetSetupSerial()))
          return;
        const auto snapshot = frameData->m_Cascades.GetArrayPtr();
        XII_TEST_INT(snapshot.GetCount(), i ? 3 : 1);
        XII_TEST_FLOAT(snapshot[snapshot.GetCount() - 1].m_fSplitFar, i ? 90.0f : 120.0f, 0.001f);
        XII_TEST_BOOL(!snapshot[0].m_mViewProjection.IsIdentity());
        XII_TEST_BOOL(pass->m_LightDirection.IsEqual(i ? xiiVec3(1, 0, -1) : xiiVec3(0, 0, -1), 0));
        if (i == 0)
        {
          if (v == 0)
            firstViewProjection = snapshot[0].m_mViewProjection;
          else
            XII_TEST_BOOL(!firstViewProjection.IsEqual(snapshot[0].m_mViewProjection, 0.00001f));
        }
        for (const auto& cascade : snapshot)
          snapshots[i].PushBack(cascade);
        const auto binding = pipelines[v].GetInterfaceBinding(compiled->FindInterface(i ? "Three" : "One", false));
        buffers[i]         = binding->m_hBuffer;
        graph.AddPass<xiiRenderGraphBufferHandle>("Export cascade snapshot", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
          data = builder.ReadBuffer(buffers[i], xiiGALResourceStateFlags::ConstantBuffer);
          builder.ExportBuffer(data, xiiGALResourceStateFlags::ConstantBuffer); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
      }
      // Camera mutation after setup must not alter already captured execution data.
      cameras[v].LookAt(xiiVec3(100, 0, 10), xiiVec3(100, 1, 0), xiiVec3(0, 0, 1));
      graph.EndSetup();
      xiiStringBuilder error;
      if (!XII_TEST_RESULT_MSG(graph.Compile({.m_bEnableAsyncQueues = (phase & 1U) != 0}, &error), "%s", error.GetData()) || !XII_TEST_RESULT_MSG(graph.Execute(environment.GetDevice(), nullptr, &contexts[v].GetBlackboard(), &cache, nullptr, &error), "%s", error.GetData()))
        return;
      environment.GetDevice()->WaitIdle();
      XII_TEST_INT(graph.GetStatistics().m_uiQueueSubmissionCount, 1);
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        const auto                buffer = graph.GetExportedBuffer(buffers[i]);
        xiiShadowCascadeConstants gpu;
        if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), buffer.Borrow(), 0, gpu))
          return;
        XII_TEST_INT(gpu.ActiveCascadeCount, snapshots[i].GetCount());
        for (xiiUInt32 c = 0; c < 4; ++c)
        {
          const xiiShaderMat4 expected(c < snapshots[i].GetCount() ? snapshots[i][c].m_mViewProjection : xiiMat4::MakeIdentity());
          XII_TEST_BOOL(xiiMemoryUtils::IsEqual(&gpu.CascadeViewProjection[c], &expected, 1));
          XII_TEST_FLOAT(gpu.CascadeSplitDepths.GetData()[c], c < snapshots[i].GetCount() ? snapshots[i][c].m_fSplitFar : 0, 0.00001f);
          XII_TEST_FLOAT(gpu.CascadeWorldRadii.GetData()[c], c < snapshots[i].GetCount() ? snapshots[i][c].m_fWorldRadius : 0, 0.00001f);
        }
      }
      XII_TEST_BOOL(!xiiGeometryResidencyManager::IsInitialized() && !xiiMaterialManager::IsInitialized());
    }
  }
  // A failed frame must never expose the previous setup's CPU snapshot.
  auto& graph = contexts[0].GetGraph();
  graph.BeginSetup(100);
  views[0]->SetCamera(nullptr);
  XII_TEST_BOOL(pipelines[0].Build(views[0], graph, contexts[0].GetBlackboard()).Failed());
  XII_TEST_BOOL(!pipelines[0].GetOutputBinding(0, "CascadeSnapshot")->m_pFrameData);
  graph.EndSetup();
}
