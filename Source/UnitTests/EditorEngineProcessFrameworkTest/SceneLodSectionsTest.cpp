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
#include <GraphicsCore/Lighting/EmissiveLightManager.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
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
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  class VisibilityDispatchLog : public xiiTestLogInterface
  {
  public:
    void HandleLogMessage(const xiiLoggingEventData& event) override
    {
      if (event.m_sText.FindSubString("DispatchComputeDescription.ThreadGroupCount") && event.m_sText.FindSubString(" is 0."))
        m_uiEmptyDispatches.Increment();
      xiiTestLogInterface::HandleLogMessage(event);
    }

    xiiAtomicInteger32 m_uiEmptyDispatches;
  };
} // namespace

static void TestLodSections(bool enableMeshShaders)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(enableMeshShaders)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  VisibilityDispatchLog log;
  log.SetLogLevel(xiiLogMsgType::All);
  xiiTestLogSystemScope logScope(&log, true);
  if (!xiiGpuSceneRasterManager::IsPickingSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native LOD section query shaders are unavailable.");
    return;
  }

  xiiMaterialResourceHandle materials[5];
  for (xiiUInt32 i = 0; i < 5; ++i)
  {
    xiiMaterialResourceDescriptor material;
    material.m_EmissiveColor = i == 2 ? xiiColor::Black : xiiColor::White;
    if (i == 2)
    {
      material.m_AlphaMode   = xiiMaterialAlphaMode::Mask;
      material.m_BaseColor.a = 0;
    }
    if (i == 3)
    {
      material.m_AlphaMode   = xiiMaterialAlphaMode::Blend;
      material.m_BlendMode   = xiiMaterialBlendMode::Translucent;
      material.m_BaseColor.a = 0.7f;
    }
    xiiStringBuilder name;
    name.SetFormat("NativeLodSectionMaterial{}", i);
    materials[i] = xiiResourceManager::CreateResource<xiiMaterialResource>(name, std::move(material));
  }
  xiiGeometry             geometry;
  xiiGeometry::GeoOptions options;
  for (const xiiVec3 center : {xiiVec3(0, -1, 0), xiiVec3(0, 1, 0), xiiVec3(1, 1, 0), xiiVec3(1, -1, 0), xiiVec3(1, 0, 0)})
  {
    options.m_Transform = xiiMat4::MakeTranslation(center);
    geometry.AddBox(xiiVec3(0.8f), true, options);
  }
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList, false);
  descriptor.AddLOD(0.5f);
  descriptor.AddLOD(0);
  descriptor.AddSection(12, 0, 0, 0);
  descriptor.AddSection(12, 12, 1, 0);
  descriptor.AddSection(12, 24, 3, 1); // Opposite spatial/material order from LOD zero.
  descriptor.AddSection(12, 36, 4, 1);
  descriptor.AddSection(12, 48, 2, 1); // Section exists only at LOD one and is fully masked.
  descriptor.AddSection(12, 0, 4, 2);  // Reuses geometry, with a different material and fewer sections.
  for (xiiUInt32 i = 0; i < 5; ++i)
    descriptor.SetMaterial(i, materials[i].GetResourceID());
  descriptor.ComputeBounds();
  const auto mesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeLodSections", std::move(descriptor));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Primitive-range capacity includes unloaded LODs without counting unrelated sections")
  {
    const auto geometryHandle = xiiGeometryResidencyManager::AcquireMeshGeometry(mesh);
    XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(geometryHandle, 0));
    xiiUInt32 counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(geometryHandle, 0, 0, xiiMakeArrayPtr(counts)));
    XII_TEST_INT(counts[0], 2);
    XII_TEST_INT(counts[1], 3);
    XII_TEST_INT(counts[2], 1);
    XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(geometryHandle, 12, 1, xiiMakeArrayPtr(counts)));
    XII_TEST_INT(counts[0], 1);
    XII_TEST_INT(counts[1], 0);
    XII_TEST_INT(counts[2], 0);
    // A custom range may cross a meshlet boundary or overlap reused geometry in another LOD.
    XII_TEST_RESULT(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(geometryHandle, 11, 2, xiiMakeArrayPtr(counts)));
    XII_TEST_INT(counts[0], 2);
    XII_TEST_INT(counts[2], 1);
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts({}, 0, 0, xiiMakeArrayPtr(counts)).Failed());
    for (const auto count : counts)
      XII_TEST_INT(count, 0);
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(geometryHandle, xiiMath::MaxValue<xiiUInt32>(), 1, xiiMakeArrayPtr(counts)).Failed());
  }

  xiiWorldDescription worldDescription("LOD section ownership");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  auto*                renderer  = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiGameObject*       object    = nullptr;
  const auto           owner     = world.CreateObject(xiiGameObjectDescription(), object);
  xiiLODMeshComponent* component = nullptr;
  xiiLODMeshComponent::CreateComponent(object, component);
  component->SetMesh(mesh);
  world.Update();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(-6, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("LOD section query", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetViewport({0, 0, 64, 64});
  const auto              projection = view->GetViewProjectionMatrix(xiiCameraEye::Left);
  const auto              frustum    = xiiFrustum::MakeFromMVP(projection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);
  xiiSceneDatabaseContext scene;
  xiiRenderGraphContext   graphContext;
  if (!XII_TEST_RESULT(scene.Initialize()) || !XII_TEST_RESULT(graphContext.Initialize("LOD section query")))
    return;
  xiiGpuVisibilityDescription visibilitySettings;
  visibilitySettings.m_uiMaxInstances       = 16;
  visibilitySettings.m_uiMaxVisibleMeshlets = 32;
  const auto visibilityContext              = xiiGpuVisibilityManager::CreateContext(visibilitySettings);
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiGpuVisibilityManager::DestroyContext(visibilityContext));
  xiiGpuFrameCompletionTracker completion;
  completion.Initialize(environment.GetDevice());
  xiiUInt64                          frame = 0;
  xiiHashTable<xiiUInt32, xiiUInt32> stableIds;

  auto render = [&](float bias, xiiUInt32 expectedLod, xiiUInt32 expectedSections, xiiUInt32 expectedCandidates,
                    xiiUInt32 forcedLod = xiiInvalidIndex, xiiUInt32 section = xiiInvalidIndex, bool excludeTransparent = false, bool onlyTransparent = false) {
    ++frame;
    component->SetForcedLOD(forcedLod);
    component->SetLodBias(bias);
    component->SetSectionIndex(section);
    xiiExtractedRenderData  extracted;
    xiiMsgExtractRenderData message;
    message.m_pView                = view;
    message.m_pExtractedRenderData = &extracted;
    XII_TEST_BOOL(component->SendMessage(message));
    extracted.SortAndBatches();
    // Ordinary indexed draw ranges remain tied to the CPU-selected LOD.
    if (forcedLod == xiiInvalidIndex && section == xiiInvalidIndex)
    {
      XII_TEST_INT(extracted.GetAllRenderData().GetCount(), 2);
      for (const auto* data : extracted.GetAllRenderData())
      {
        const auto* packet = xiiDynamicCast<const xiiMeshRenderData*>(data);
        XII_TEST_INT(packet->m_uiPrimitiveCount, 12);
        XII_TEST_INT(packet->m_uiFirstPrimitive, packet->m_uiSectionIndex * 12);
      }
    }
    if (section == 2 && forcedLod == xiiInvalidIndex)
    {
      if (!XII_TEST_INT(extracted.GetAllRenderData().GetCount(), 1))
        return;
      const auto* packet = xiiDynamicCast<const xiiMeshRenderData*>(extracted.GetAllRenderData()[0]);
      XII_TEST_INT(packet->m_uiPrimitiveCount, 0); // No indexed draw in CPU LOD zero.
    }
    if (!XII_TEST_RESULT(scene.SynchronizeExtractedMeshes(extracted, frame)) || !XII_TEST_INT(scene.GetDatabase().GetObjectCount(), expectedCandidates))
      return;
    xiiUInt64 meshletCapacity = 0;
    XII_TEST_RESULT(xiiSceneDatabaseManager::GetExtractedMeshletCapacity(scene.GetHandle(), meshletCapacity));
    // Each candidate owns one box meshlet. Whole-LOD accounting would report fourteen
    // rather than six for the unfiltered mesh, and grows quadratically with section count.
    XII_TEST_INT(meshletCapacity, expectedCandidates);
    for (const auto& instance : scene.GetDatabase().GetGpuInstances())
    {
      xiiSceneMeshSource source;
      XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(scene.GetHandle(), instance.m_uiObjectId, source));
      XII_TEST_BOOL(source.m_hOwnerObject == owner && source.m_hOwnerComponent == component->GetHandle());
      XII_TEST_INT(source.m_uiLodIndex, instance.m_uiDrawLod);
      if (forcedLod == xiiInvalidIndex && section == xiiInvalidIndex)
      {
        const auto key = source.m_uiLodIndex * 8 + source.m_uiSectionIndex;
        xiiUInt32  previous;
        if (stableIds.TryGetValue(key, previous))
          XII_TEST_INT(instance.m_uiObjectId, previous);
        stableIds.Insert(key, instance.m_uiObjectId);
      }
    }
    const auto completed = completion.PollCompletedFrames();
    xiiGALBindlessResourceTable::Collect(completed);
    xiiMaterialManager::BeginFrame(frame, completed);
    xiiGpuVisibilityManager::BeginFrame(frame, completed);
    xiiGeometryResidencyManager::ProcessStreaming(frame, completed, xiiMath::MaxValue<xiiUInt64>());
    XII_TEST_RESULT(xiiSceneDatabaseManager::GetExtractedMeshletCapacity(scene.GetHandle(), meshletCapacity));
    XII_TEST_INT(meshletCapacity, expectedCandidates);
    auto& graph = graphContext.GetGraph();
    auto& cache = graphContext.GetResourceCache();
    cache.BeginFrame(frame, completed);
    graph.BeginSetup(frame);
    const auto geometryUpload = xiiGeometryResidencyManager::AddUploadPass(graph, frame);
    const auto materialUpload = xiiMaterialManager::AddUploadPasses(graph);
    auto       visibilityView = xiiGpuVisibilitySystem::BuildView(projection, frustum, camera.GetCenterPosition(), 64, 64, 0, expectedCandidates);
    if (excludeTransparent)
      visibilityView.m_uiExcludedFlags = xiiSceneObjectFlags::Transparent;
    if (onlyTransparent)
      visibilityView.m_uiRequiredFlags |= xiiSceneObjectFlags::Transparent;
    xiiGpuVisibilityPassDescription visibilityDescription;
    visibilityDescription.m_bAsyncCompute      = false;
    visibilityDescription.m_bEnableLodFeedback = false;
    xiiRenderGraphTextureHandle hiZ;
    if (expectedCandidates == 0)
    {
      struct EmptyHiZ
      {
        xiiRenderGraphTextureHandle m_Texture;
      };
      const auto source = graph.AddPass<EmptyHiZ>("Empty visibility Hi-Z", xiiGALCommandQueueFlags::Graphics, [](EmptyHiZ& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription texture;
        texture.m_Type = xiiGALResourceDimension::Texture2D;
        texture.m_Size = {1, 1};
        texture.m_Format = xiiGALResourceFormat::R32Float;
        texture.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
        data.m_Texture = builder.WriteTexture("Empty visibility Hi-Z", texture, xiiGALResourceStateFlags::RenderTarget);
        builder.SetPassRenderPassManaged(); }, [](const EmptyHiZ& data, xiiRenderGraphPassContext& context) { context.GetCommandList().ClearRenderTargetView(context.GetTexture(data.m_Texture)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black); });
      hiZ               = source.first->m_Texture;
    }
    auto visibility = xiiGpuVisibilityManager::AddPasses(visibilityContext, graph, frame, scene.GetHandle(), visibilityView, geometryUpload, visibilityDescription, hiZ);
    // A single-section candidate contains one box meshlet. Exercise an exact 16-byte indirect
    // command allocation as well as the normal retained visibility capacity.
    if (!enableMeshShaders && expectedCandidates == 1)
      visibility.m_uiMaxVisibleMeshlets = 1;
    xiiGpuSceneGBufferRasterDescription raster;
    raster.m_ViewProjectionMatrix = projection;
    raster.m_uiWidth = raster.m_uiHeight = 64;
    raster.m_uiVertexStride              = sizeof(xiiMeshPackedVertex);
    raster.m_uiTexCoordOffset            = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
    raster.m_uiMeshDispatchGroupCountX   = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(visibilityContext);
    raster.m_uiMeshDispatchGroupCountY   = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(visibilityContext);
    xiiRenderGraphTextureHandle surface;
    if (!XII_TEST_RESULT(xiiGpuSceneRasterManager::AddPickingPass(graph, "LOD section surface", visibility, geometryUpload, materialUpload, raster, surface)))
      return;
    auto invalidRaster      = raster;
    invalidRaster.m_uiWidth = 0;
    auto rejectedSurface    = surface;
    XII_TEST_BOOL(xiiGpuSceneRasterManager::AddPickingPass(graph, "Invalid query", visibility, geometryUpload, materialUpload, invalidRaster, rejectedSurface).Failed());
    XII_TEST_BOOL(!rejectedSurface.IsValid());
    auto invalidVisibility                   = visibility;
    invalidVisibility.m_uiMaxVisibleMeshlets = 0;
    rejectedSurface                          = surface;
    XII_TEST_BOOL(xiiGpuSceneRasterManager::AddPickingPass(graph, "Invalid indexed query", invalidVisibility, geometryUpload, materialUpload, raster, rejectedSurface, false).Failed());
    XII_TEST_BOOL(!rejectedSurface.IsValid());
    xiiEmissiveLightManager::BuildDescription emissiveDescription;
    emissiveDescription.m_Visibility       = visibility;
    emissiveDescription.m_Geometry         = geometryUpload;
    emissiveDescription.m_Materials        = materialUpload;
    emissiveDescription.m_uiInstanceCount  = expectedCandidates;
    emissiveDescription.m_uiVertexStride   = sizeof(xiiMeshPackedVertex);
    emissiveDescription.m_uiTexCoordOffset = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
    const auto emissive                    = xiiEmissiveLightManager::AddBuildPass(graph, frame, emissiveDescription);
    struct Export
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Export>("LOD emitter export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
      builder.ExportBuffer(emissive.m_hHeader, xiiGALResourceStateFlags::ShaderResource);
      if (expectedCandidates == 0)
      {
        builder.ExportBuffer(visibility.m_hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
        builder.ExportBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
        if (visibility.m_hIndirectCommandCount.IsValid())
          builder.ExportBuffer(visibility.m_hIndirectCommandCount, xiiGALResourceStateFlags::ShaderResource);
      }
      if (!enableMeshShaders)
      {
        const auto commands = graph.FindBuffer("LOD section surface Indexed commands buffer");
        builder.ExportBuffer(builder.ReadBuffer(commands, xiiGALResourceStateFlags::IndirectArgument), xiiGALResourceStateFlags::IndirectArgument);
        builder.ExportBuffer(builder.ReadBuffer(visibility.m_hVisibleMeshletCount, xiiGALResourceStateFlags::IndirectArgument), xiiGALResourceStateFlags::IndirectArgument);
      } }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
    xiiScenePickingReadback readback;
    xiiUInt32               slot;
    XII_TEST_RESULT(readback.Poll(slot));
    xiiVariantDictionary         parameters;
    xiiRenderPipelinePassContext capture{view, graph, graphContext.GetBlackboard(), parameters, {}, {}, "LOD sections"};
    if (!XII_TEST_RESULT(readback.AddCapture(capture, surface, slot)))
      return;
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), view, &graphContext.GetBlackboard(), &cache)))
      return;
    completion.CaptureSubmittedFrame(frame);
    completion.WaitForFrame(frame);
    if (expectedCandidates == 0)
    {
      for (const auto counter : {visibility.m_hVisibleInstanceCount, visibility.m_hVisibleMeshletCount, visibility.m_hIndirectCommandCount})
      {
        if (!counter.IsValid()) // Indexed queries consume the meshlet count directly.
          continue;
        xiiUInt32 value = xiiInvalidIndex;
        if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(counter).Borrow(), 0, value))
          XII_TEST_INT(value, 0);
      }
    }
    if (!enableMeshShaders)
    {
      xiiUInt32 count = 0;
      xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(visibility.m_hVisibleMeshletCount).Borrow(), 0, count);
      if (count != 0)
      {
        xiiVec4U32 command;
        const auto commands = graph.FindBuffer("LOD section surface Indexed commands buffer");
        xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(commands).Borrow(), 0, command);
        XII_TEST_INT(command.x, 36);
        XII_TEST_INT(command.y, 1);
        XII_TEST_INT(command.z, 0);
        XII_TEST_INT(command.w, 0);
      }
    }
    XII_TEST_RESULT(readback.Poll(slot));
    xiiHashSet<xiiUInt32> seenSections;
    const auto            words = readback.GetPixels();
    for (xiiUInt32 pixel = 0; pixel + 3 < words.GetCount(); pixel += 4)
    {
      if (words[pixel] == 0)
        continue;
      xiiSceneMeshSource source;
      if (!XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(scene.GetHandle(), words[pixel], source)))
        return;
      if (!XII_TEST_INT(source.m_uiLodIndex, expectedLod))
        return;
      const xiiUInt32 materialSlot = expectedLod == 0 ? source.m_uiSectionIndex : expectedLod == 2 ? 4 :
        source.m_uiSectionIndex == 0                                                               ? 3 :
                                                                                                     4;
      XII_TEST_INT(source.m_uiMaterialIndex, materialSlot);
      seenSections.Insert(source.m_uiSectionIndex);
      const auto clip = projection * xiiVec4(expectedLod == 1 ? 0.6f : -0.4f, 0, 0, 1);
      // Interior front-face pixels have this depth; edge pixels can belong to box sides.
      const float depth = std::bit_cast<float>(words[pixel + 1]);
      XII_TEST_BOOL(depth <= clip.z / clip.w + 0.0001f && depth > 0);
    }
    XII_TEST_INT(seenSections.GetCount(), expectedSections);
    if (excludeTransparent)
      XII_TEST_BOOL(!seenSections.Contains(0));
    xiiGpuEmissiveLightHeader header;
    if (xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(emissive.m_hHeader).Borrow(), 0, header))
    {
      const xiiUInt32 expectedTriangles = expectedCandidates == 0 || section == 2 ? 0 : forcedLod == 1 ? 24 :
        forcedLod == 0 || section == xiiInvalidIndex                                                   ? 24 :
                                                                                                         12;
      XII_TEST_INT(header.TriangleCount, expectedTriangles);
    }
  };
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Automatic LODs preserve their own section counts, ranges, material routing and identities")
  {
    render(8, 0, 2, 6);
    render(5.5f, 1, 2, 6);
    render(0, 2, 1, 6);
    render(5.5f, 1, 1, 6, xiiInvalidIndex, xiiInvalidIndex, true);
    render(8, 0, 2, 6);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Filtered material sections share geometry LOD hysteresis")
  {
    xiiResourceLock<xiiMeshResource> resource(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    const auto                       bounds   = resource->GetBounds();
    const float                      coverage = bounds.m_fSphereRadius * 64 / (bounds.m_vCenter - camera.GetCenterPosition()).GetLength();
    // LOD one starts at 540 pixels and has a ten percent dead band. Restore a
    // previously filtered translucent section inside that band, then leave it.
    render(xiiMath::Log2(600 / coverage), 1, 2, 6);
    render(xiiMath::Log2(520 / coverage), 1, 1, 6, xiiInvalidIndex, xiiInvalidIndex, true);
    render(xiiMath::Log2(520 / coverage), 1, 2, 6);
    render(xiiMath::Log2(520 / coverage), 1, 1, 6, xiiInvalidIndex, xiiInvalidIndex, false, true);
    render(xiiMath::Log2(520 / coverage), 1, 2, 6);
    render(xiiMath::Log2(480 / coverage), 2, 1, 6);
  }
  stableIds.Clear();
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit section exists only in another LOD and never substitutes other geometry")
  {
    render(5.5f, 1, 0, 1, xiiInvalidIndex, 2);
    render(8, 0, 0, 1, xiiInvalidIndex, 2);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Forced LOD uses only its authored sections")
  {
    render(-8, 1, 2, 3, 1);
    render(8, 0, 2, 2, 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty visibility clears counters without direct zero-group dispatches")
  {
    component->SetMesh({});
    render(0, 0, 0, 0, 0);
    render(0, 0, 0, 0, 0);
    component->SetMesh(mesh);
    render(0, 0, 2, 2, 0);
    XII_TEST_INT(static_cast<xiiUInt32>(log.m_uiEmptyDispatches), 0);
  }
}

XII_CREATE_SIMPLE_TEST(NativeSceneLod, Sections)
{
  TestLodSections(true);
}

XII_CREATE_SIMPLE_TEST(NativeSceneLod, SectionsWithoutMeshShaders)
{
  TestLodSections(false);
}
