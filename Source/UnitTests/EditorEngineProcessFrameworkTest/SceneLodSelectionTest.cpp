/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EditorEngineProcessFrameworkTest/GpuBufferReadback.h>

#include <bit>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/Implementation/ScenePickingReadback.h>

XII_CREATE_SIMPLE_TEST_GROUP(NativeSceneLod);

XII_CREATE_SIMPLE_TEST(NativeSceneLod, Selection)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiGpuSceneRasterManager::IsSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native LOD visibility and surface tests require mesh shaders.");
    return;
  }

  // Different source primitive ranges and depths make a substituted LOD observable in both
  // the compact visibility stream and the actual nearest-surface query.
  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiGeometry::GeoOptions options;
  options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(2, 0, 0));
  geometry.AddBox(xiiVec3(1), true, options);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddLOD(0);
  descriptor.AddSection(12, 0, 0, 0);
  descriptor.AddSection(12, 12, 1, 1);
  descriptor.ComputeBounds();
  const auto                       mesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeLodSelectionMesh", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded && loaded->GetLODs().GetCount() == 2))
    return;

  xiiWorldDescription worldDescription("Native LOD component");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*          renderer = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject* object   = nullptr;
  world.CreateObject(xiiGameObjectDescription(), object);
  xiiLODMeshComponent* component = nullptr;
  xiiLODMeshComponent::CreateComponent(object, component);
  component->SetMesh(mesh);
  world.Update();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(-6, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("LOD query projection", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 64, 64});
  const auto projection = view->GetViewProjectionMatrix(xiiCameraEye::Left);
  const auto frustum    = xiiFrustum::MakeFromMVP(projection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);

  xiiSceneDatabaseContext sceneContext;
  xiiRenderGraphContext   graphContext;
  if (!XII_TEST_RESULT(sceneContext.Initialize()) || !XII_TEST_RESULT(graphContext.Initialize("Native LOD selection")))
    return;
  const auto visibilityContext = xiiGpuVisibilityManager::CreateContext();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiGpuVisibilityManager::DestroyContext(visibilityContext));
  if (!XII_TEST_BOOL(!visibilityContext.IsInvalidated()))
    return;
  const auto geometryHandle = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
  XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(geometryHandle, 8));

  xiiGpuFrameCompletionTracker completion;
  completion.Initialize(environment.GetDevice());
  xiiUInt64 frame = 0;
  auto      run   = [&](xiiUInt32 forcedLod, float bias, xiiUInt32 expectedLod, bool useHiZ, bool missingForced = false, xiiUInt32 missingLod = 0) {
    ++frame;
    component->SetForcedLOD(forcedLod);
    component->SetLodBias(bias);
    xiiExtractedRenderData  extracted;
    xiiMsgExtractRenderData message;
    message.m_pView                = view;
    message.m_pExtractedRenderData = &extracted;
    component->SendMessage(message);
    extracted.SortAndBatches();
    for (auto* data : extracted.GetAllRenderData())
    {
      auto* packet = xiiDynamicCast<xiiMeshRenderData*>(data);
      // Exercise custom primitive-range semantics independently of authored section expansion.
      packet->m_RangeMode = xiiMeshRenderData::RangeMode::PrimitiveRange;
      // Forced packets retain the actual extracted section range. Automatic coverage is
      // tested with a whole-LOD packet, independently of per-LOD section remapping.
      if (!packet->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD))
        packet->m_uiFirstPrimitive = packet->m_uiPrimitiveCount = 0;
      if (missingForced)
        packet->m_uiLODIndex = missingLod;
    }
    if (!XII_TEST_RESULT(sceneContext.SynchronizeExtractedMeshes(extracted, frame)))
      return;
    const auto instances = sceneContext.GetDatabase().GetGpuInstances();
    if (!XII_TEST_INT(instances.GetCount(), 1))
      return;
    XII_TEST_INT(instances[0].m_uiForcedLod, missingForced ? missingLod : forcedLod);
    XII_TEST_FLOAT(instances[0].m_fLodBias, bias, 0);
    const auto completed = completion.PollCompletedFrames();
    xiiGALBindlessResourceTable::Collect(completed);
    xiiMaterialManager::BeginFrame(frame, completed);
    xiiGpuVisibilityManager::BeginFrame(frame, completed);
    xiiGeometryResidencyManager::ProcessStreaming(frame, completed, missingForced ? 0 : xiiMath::MaxValue<xiiUInt64>());
    auto& graph = graphContext.GetGraph();
    auto& cache = graphContext.GetResourceCache();
    cache.BeginFrame(frame, completed);
    graph.BeginSetup(frame);
    const auto                      uploaded       = xiiGeometryResidencyManager::AddUploadPass(graph, frame);
    const auto                      materials      = xiiMaterialManager::AddUploadPasses(graph);
    auto                            visibilityView = xiiGpuVisibilitySystem::BuildView(projection, frustum, camera.GetCenterPosition(), 64, 64, 0, 1);
    xiiGpuVisibilityPassDescription description;
    description.m_sName              = "LOD regression";
    description.m_bAsyncCompute      = false;
    description.m_bEnableLodFeedback = false;
    xiiRenderGraphTextureHandle hiZ;
    if (useHiZ)
    {
      struct Depth
      {
        xiiRenderGraphTextureHandle m_Depth;
      };
      graph.AddPass<Depth>("LOD empty occluder", xiiGALCommandQueueFlags::Graphics, [&](Depth& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription texture;
        texture.m_Type = xiiGALResourceDimension::Texture2D;
        texture.m_Size = {64, 64};
        texture.m_Format = xiiGALResourceFormat::D32Float;
        texture.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        hiZ = data.m_Depth = builder.WriteTexture("LOD Hi-Z input", texture, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged(); }, [](const Depth& data, xiiRenderGraphPassContext& context) { context.GetCommandList().ClearDepthStencilView(context.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0, 0); });
    }
    const auto                          visible = xiiGpuVisibilityManager::AddPasses(visibilityContext, graph, frame, sceneContext.GetHandle(), visibilityView, uploaded, description, hiZ);
    xiiGpuSceneGBufferRasterDescription raster;
    raster.m_ViewProjectionMatrix = projection;
    raster.m_uiWidth = raster.m_uiHeight = 64;
    raster.m_uiVertexStride              = sizeof(xiiMeshPackedVertex);
    raster.m_uiTexCoordOffset            = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
    raster.m_uiMeshDispatchGroupCountX   = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(visibilityContext);
    raster.m_uiMeshDispatchGroupCountY   = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(visibilityContext);
    xiiRenderGraphTextureHandle surface;
    if (!XII_TEST_RESULT(xiiGpuSceneRasterManager::AddPickingPass(graph, "LOD surface", visible, uploaded, materials, raster, surface)))
      return;
    struct Export
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Export>("LOD visibility export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
      builder.ExportBuffer(visible.m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportBuffer(visible.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
    xiiScenePickingReadback readback;
    xiiUInt32               slot;
    XII_TEST_RESULT(readback.Poll(slot));
    xiiVariantDictionary         parameters;
    xiiRenderPipelinePassContext capture{view, graph, graphContext.GetBlackboard(), parameters, {}, {}, "LOD regression"};
    if (!XII_TEST_RESULT(readback.AddCapture(capture, surface, slot)))
      return;
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), view, &graphContext.GetBlackboard(), &cache)))
      return;
    completion.CaptureSubmittedFrame(frame);
    completion.WaitForFrame(frame);
    xiiUInt32 count = 0;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(visible.m_hVisibleMeshletCount).Borrow(), 0, count) || !XII_TEST_INT(count, expectedLod == xiiInvalidIndex ? 0 : 1))
      return;
    if (expectedLod == xiiInvalidIndex)
    {
      XII_TEST_RESULT(readback.Poll(slot));
      const auto words = readback.GetPixels();
      if (XII_TEST_INT(words.GetCount(), 64 * 64 * 4))
        XII_TEST_INT(words[(32 * 64 + 32) * 4], 0);
      return;
    }
    xiiVec2U32 meshlet = xiiVec2U32::MakeZero();
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(visible.m_hVisibleMeshlets).Borrow(), 0, meshlet))
      return;
    xiiMeshlet selected;
    if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), xiiGeometryResidencyManager::GetMeshletMetadataBuffer().Borrow(), meshlet.y * sizeof(selected), selected))
      return;
    XII_TEST_INT(selected.m_uiLodIndex, expectedLod);
    if (forcedLod != xiiInvalidIndex)
    {
      xiiSceneMeshSource source;
      XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(sceneContext.GetHandle(), instances[0].m_uiObjectId, source));
      XII_TEST_BOOL(source.m_hOwnerComponent == component->GetHandle());
      XII_TEST_INT(source.m_uiMaterialIndex, expectedLod);
      XII_TEST_INT(instances[0].m_uiFirstPrimitive, expectedLod * 12);
      XII_TEST_INT(instances[0].m_uiPrimitiveCount, 12);
    }
    XII_TEST_RESULT(readback.Poll(slot));
    if (XII_TEST_INT(readback.GetPixels().GetCount(), 64 * 64 * 4))
    {
      const auto      words  = readback.GetPixels();
      const xiiUInt32 center = (32 * 64 + 32) * 4;
      XII_TEST_INT(words[center], instances[0].m_uiObjectId);
      const auto clip = projection * xiiVec4(expectedLod == 0 ? -0.5f : 1.5f, 0, 0, 1);
      XII_TEST_FLOAT(std::bit_cast<float>(words[center + 1]), clip.z / clip.w, 0.0001f);
    }
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component forced LOD bypasses coverage, bias and previous history")
  {
    run(1, 16, 1, false);
    XII_TEST_INT(xiiGeometryResidencyManager::GetGpuRecord(geometryHandle)->m_uiResidentLodMask, 2);
    run(1, 0, xiiInvalidIndex, false, true);   // LOD zero is missing: no substitution of resident LOD one.
    run(1, 0, xiiInvalidIndex, true, true, 7); // The geometry has no LOD seven: no invalid residency-bit shift.
    run(0, -16, 0, true);
    run(1, 0, 1, true);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Automatic coverage bias works in ordinary and temporal Hi-Z visibility")
  {
    run(xiiInvalidIndex, 0, 1, false);
    run(xiiInvalidIndex, 8, 0, true);
    run(xiiInvalidIndex, -8, 1, false);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid extracted selection controls preserve the synchronized scene")
  {
    const auto        revision = sceneContext.GetDatabase().GetRevision();
    xiiMeshRenderData invalid;
    invalid.m_hMesh          = mesh;
    invalid.m_fLodBias       = xiiMath::NaN<float>();
    xiiRenderData* packets[] = {&invalid};
    XII_TEST_BOOL(sceneContext.SynchronizeExtractedMeshes(xiiMakeArrayPtr(packets), frame + 1).Failed());
    invalid.m_fLodBias = 0;
    invalid.m_Flags.Add(xiiMeshRenderDataFlags::ForceLOD);
    invalid.m_uiLODIndex = 8;
    XII_TEST_BOOL(sceneContext.SynchronizeExtractedMeshes(xiiMakeArrayPtr(packets), frame + 1).Failed());
    invalid.m_Flags.Remove(xiiMeshRenderDataFlags::ForceLOD);
    invalid.m_RangeMode = static_cast<xiiMeshRenderData::RangeMode>(255);
    XII_TEST_BOOL(sceneContext.SynchronizeExtractedMeshes(xiiMakeArrayPtr(packets), frame + 1).Failed());
    XII_TEST_INT(sceneContext.GetDatabase().GetRevision(), revision);
    XII_TEST_INT(sceneContext.GetDatabase().GetObjectCount(), 1);
  }
}
