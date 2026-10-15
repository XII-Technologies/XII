/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EditorEngineProcessFrameworkTest/GpuBufferReadback.h>

#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Material/MaterialAssetFile.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>
#include <SharedPluginScene/Scene/SceneRenderPipeline.h>
#include <SharedPluginScene/Scene/SceneSelectionPass.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

namespace
{
  using xiiGpuTestReadback::ReadBufferRecord;

  /// A minimal resource producer exercises the same explicit frame-table contract as PrepareFrame
  /// without involving lighting/postprocessing in a focused native query fixture.
  class MeshQueryTables : public xiiRenderPipelinePass
  {
    XII_ADD_DYNAMIC_REFLECTION(MeshQueryTables, xiiRenderPipelinePass);

  public:
    void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
    {
      for (const xiiStringView name : {"GPU Geometry Metadata", "GPU Meshlet Metadata", "Surface Material GPU Storage"})
        pins.PushBack({name, xiiRenderPipelinePin::Buffer, false, false});
      pins.PushBack({"Color", xiiRenderPipelinePin::Texture, false, false});
      pins.PushBack({"SceneDepth", xiiRenderPipelinePin::Texture, false, false});
    }
    xiiResult Build(xiiRenderPipelinePassContext& context) override
    {
      const auto geometry = xiiGeometryResidencyManager::AddUploadPass(context.m_Graph, context.m_Graph.GetFrameIndex());
      const auto material = xiiMaterialManager::AddUploadPasses(context.m_Graph);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuGeometryBaseIndex, geometry.m_uiGeometryBaseIndex);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuMaximumResidentMeshlets, geometry.m_uiMaximumResidentMeshletCount);
      context.m_Blackboard.Set(xiiRGBlackboardKeys::k_GpuSurfaceMaterialBaseIndex, material.m_uiSurfaceBaseIndex);
      XII_SUCCEED_OR_RETURN(context.SetOutput("GPU Geometry Metadata", geometry.m_hGeometryMetadata));
      XII_SUCCEED_OR_RETURN(context.SetOutput("GPU Meshlet Metadata", geometry.m_hMeshletMetadata));
      XII_SUCCEED_OR_RETURN(context.SetOutput("Surface Material GPU Storage", material.m_hSurfaceData));
      struct Data
      {
        xiiRenderGraphTextureHandle m_Color, m_Depth;
        float                       m_fDepth = 0;
      };
      xiiRenderGraphTextureHandle color, depth;
      auto                        target = context.m_pView->GetRenderTargetView()->GetTexture();
      context.m_Graph.AddPass<Data>("Query test clear", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
        color = data.m_Color = builder.WriteTexture(builder.ImportTexture("Query test Color", target, target->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
        builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Blue); });
      context.m_Graph.AddPass<Data>("Query test depth", xiiGALCommandQueueFlags::Graphics, [&](Data& data, xiiRenderGraphBuilder& builder) {
        auto description = target->GetDescription();
        description.m_Size = {16, 16}; // Depth can have a different resolution from the query surface.
        description.m_Format = xiiGALResourceFormat::D32Float;
        description.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        depth = data.m_Depth = builder.WriteTexture("Query test SceneDepth", description, xiiGALResourceStateFlags::DepthWrite);
        if (const auto* setting = context.m_Parameters.GetValue("Test.SceneDepth")) data.m_fDepth = setting->Get<float>();
        builder.SetPassRenderPassManaged(); }, [](const Data& data, xiiRenderGraphPassContext& execution) {
        auto& commands = execution.GetCommandList();
        commands.ClearDepthStencilView(execution.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, data.m_fDepth, 0); });
      XII_SUCCEED_OR_RETURN(context.SetOutput("Color", color));
      return context.SetOutput("SceneDepth", depth);
    }
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(MeshQueryTables, 1, xiiRTTIDefaultAllocator<MeshQueryTables>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  bool ReadColorPixel(xiiGALDevice& device, xiiGALTexture* texture, xiiUInt32 x, xiiUInt32 y, xiiUInt32& out_pixel)
  {
    xiiGALTextureReadback                readback(&device);
    xiiGALCommandListCreationDescription description;
    description.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto commands            = device.CreateCommandList(description);
    commands->Begin();
    xiiGALTextureReadback::ReadbackRequest request;
    request.m_pTexture  = texture;
    const auto enqueued = readback.Enqueue(commands.Borrow(), request);
    commands->End();
    if (!XII_TEST_RESULT(enqueued))
      return false;
    auto* queue = device.GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    queue->WaitForFenceValue(queue->Submit(commands.Borrow()));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (!XII_TEST_BOOL(static_cast<bool>(capture)))
      return false;
    XII_SCOPE_EXIT(readback.RecycleStagingTexture(capture.m_pStagingTexture));
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    xiiGALTextureMipLevelData      mip;
    if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, mip, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      commands->End();
      return false;
    }
    const auto* row     = xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride);
    out_pixel           = row[x];
    const auto unmapped = commands->UnmapTextureSubresource(capture.m_pStagingTexture, mip);
    commands->End();
    return XII_TEST_RESULT(unmapped);
  }
} // namespace

static void TestSceneMeshPicking(bool enableMeshShaders)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(enableMeshShaders)))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  // Destroy extracted packets after GPU completion and before resource-manager shutdown.
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiFrameAllocator::Reset(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  if (!xiiGpuSceneRasterManager::IsPickingSupported())
  {
    xiiTestFramework::Output(xiiTestOutput::Message, "Native mesh query shaders are unavailable; GPU queries were not exercised on this device.");
    return;
  }
  if (!enableMeshShaders)
  {
    XII_TEST_BOOL(environment.GetDevice()->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Disabled);
    XII_TEST_BOOL(!xiiGpuSceneRasterManager::IsSupported());
  }
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Tables", MeshQueryTables::GetStaticRTTI()->GetTypeName());
  xiiVariantDictionary querySettings;
  querySettings.Insert("UseMeshShaders", enableMeshShaders);
  definition.AddPass("Query", xiiSceneMeshPickingPass::GetStaticRTTI()->GetTypeName(), querySettings);
  xiiVariantDictionary selectedSettings;
  selectedSettings.Insert("SelectionOnly", true);
  selectedSettings.Insert("UseMeshShaders", enableMeshShaders);
  definition.AddPass("Selected query", xiiSceneMeshPickingPass::GetStaticRTTI()->GetTypeName(), selectedSettings);
  xiiVariantDictionary overlaySettings;
  overlaySettings.Insert("VisibleColor", xiiColor::Red);
  overlaySettings.Insert("OccludedColor", xiiColor::Lime);
  overlaySettings.Insert("FillOpacity", 0.5f);
  definition.AddPass("Selection", xiiSceneSelectionPass::GetStaticRTTI()->GetTypeName(), overlaySettings);
  for (const xiiStringView name : {"GPU Geometry Metadata", "GPU Meshlet Metadata", "Surface Material GPU Storage"})
  {
    XII_TEST_RESULT(definition.Connect(0, name, 1, name));
    XII_TEST_RESULT(definition.Connect(0, name, 2, name));
  }
  XII_TEST_RESULT(definition.Connect(0, "Color", 3, "Color"));
  XII_TEST_RESULT(definition.Connect(0, "SceneDepth", 3, "SceneDepth"));
  XII_TEST_RESULT(definition.Connect(2, "Surface", 3, "SelectionSurface"));
  auto                      pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("NativeMeshQueryPipeline", std::move(definition));
  xiiRenderPipelineInstance instance, other;
  if (!XII_TEST_RESULT(instance.Synchronize(pipeline)) || !XII_TEST_RESULT(other.Synchronize(pipeline)))
    return;
  const auto* pass      = xiiDynamicCast<const xiiSceneMeshPickingPass*>(instance.GetPass(1));
  const auto* otherPass = xiiDynamicCast<const xiiSceneMeshPickingPass*>(other.GetPass(1));
  if (!XII_TEST_BOOL(pass && otherPass && pass != otherPass))
    return;
  XII_TEST_BOOL(instance.GetPipeline() == other.GetPipeline());
  xiiRenderPipelineResourceDescriptor   editorDefinition;
  xiiSharedPtr<const xiiRenderPipeline> editorPipeline;
  XII_TEST_RESULT(xiiSceneRenderPipeline::GetDefaultDescriptor(editorDefinition));
  XII_TEST_RESULT(xiiRenderPipeline::Compile(editorDefinition, editorPipeline));

  xiiGeometry box;
  box.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor meshDefinition;
  meshDefinition.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(box, xiiGALPrimitiveTopology::TriangleList);
  meshDefinition.AddSubMesh(meshDefinition.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  meshDefinition.ComputeBounds();
  const auto                       mesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeQueryBox", std::move(meshDefinition));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;
  xiiWorldDescription worldDescription("Native mesh query world");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObjectDescription objectDescription;
  objectDescription.m_bDynamic = true;
  xiiGameObject* frontObject   = nullptr;
  const auto     frontOwner    = world.CreateObject(objectDescription, frontObject);
  xiiGameObject* backObject    = nullptr;
  const auto     backOwner     = world.CreateObject(objectDescription, backObject);
  backObject->SetLocalPosition(xiiVec3(3, 0, 0));
  xiiLODMeshComponent* frontLodComponent = nullptr;
  const auto           frontComponent    = xiiLODMeshComponent::CreateComponent(frontObject, frontLodComponent);
  frontLodComponent->SetMesh(mesh);
  xiiMeshComponent* backMeshComponent = nullptr;
  const auto        backComponent     = xiiMeshComponent::CreateComponent(backObject, backMeshComponent);
  backMeshComponent->SetMesh(mesh);
  xiiMeshComponentBase* component = nullptr;
  world.Update();
  xiiMeshRenderData front, back;
  front.m_hMesh                        = mesh;
  back.m_hMesh                         = mesh;
  front.m_hOwnerObject                 = frontOwner;
  back.m_hOwnerObject                  = backOwner;
  front.m_hOwnerComponent              = frontComponent;
  back.m_hOwnerComponent               = backComponent;
  front.m_GlobalTransform              = xiiTransform::MakeIdentity();
  back.m_GlobalTransform               = xiiTransform::MakeIdentity();
  back.m_GlobalTransform.m_vPosition.x = 3;
  front.m_uiPrimitiveCount = back.m_uiPrimitiveCount = loaded->GetSections()[0].m_uiPrimitiveCount;
  front.m_uiSectionIndex                             = 1024; // Full-width packet section identity, not legacy 7-bit packing.
  xiiExtractedRenderData extracted;
  extracted.AddRenderData(&front);
  extracted.AddRenderData(&back);
  extracted.SortAndBatches();
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
  auto*      renderer   = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView*   view       = nullptr;
  const auto viewHandle = renderer->CreateView("Native query", view);
  XII_SCOPE_EXIT(renderer->DestroyView(viewHandle));
  view->SetCamera(&camera);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  view->SetViewport({8, 12, 32, 32});
  view->GetExtractedRenderData()->AddRenderData(&front);
  view->GetExtractedRenderData()->AddRenderData(&back);
  view->GetExtractedRenderData()->SortAndBatches();
  xiiGALTextureCreationDescription output;
  output.m_Type      = xiiGALResourceDimension::Texture2D;
  output.m_Size      = {48, 48};
  output.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  output.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto target        = environment.GetDevice()->CreateTexture(output);
  view->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  xiiSceneDatabaseContext mainScene;
  if (!XII_TEST_RESULT(mainScene.Initialize()))
    return;
  xiiRenderGraphContext graphContext;
  if (!XII_TEST_RESULT(graphContext.Initialize("Native query graph")))
    return;
  xiiGpuFrameCompletionTracker completion;
  completion.Initialize(environment.GetDevice());
  xiiUInt64            nextFrame = 1;
  xiiVariantDictionary parameters;
  auto                 render = [&](xiiRenderPipelineInstance* runtime = nullptr, xiiView* queryView = nullptr) -> bool {
    const auto frame     = nextFrame++;
    const auto completed = completion.PollCompletedFrames();
    xiiGALBindlessResourceTable::Collect(completed);
    xiiMaterialManager::BeginFrame(frame, completed);
    xiiGpuVisibilityManager::BeginFrame(frame, completed);
    if (!XII_TEST_RESULT(mainScene.SynchronizeExtractedMeshes(extracted, frame)))
      return false;
    xiiMaterialManager::PrepareGpuResources();
    xiiGeometryResidencyManager::ProcessStreaming(frame, completed, xiiMath::MaxValue<xiiUInt64>());
    auto& cache = graphContext.GetResourceCache();
    cache.BeginFrame(frame, completed);
    XII_SCOPE_EXIT(cache.EndFrame());
    auto& graph = graphContext.GetGraph();
    graph.BeginSetup(frame);
    graphContext.GetBlackboard().ClearFrame();
    auto& execution = runtime ? *runtime : instance;
    if (!XII_TEST_RESULT(execution.Build(queryView ? queryView : view, graph, graphContext.GetBlackboard(), parameters)))
      return false;
    struct Export
    {
      XII_DECLARE_POD_TYPE();
    };
    graph.AddPass<Export>("Query test export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
      const auto color = builder.ReadTexture(graph.FindTexture("Query test Color"), xiiGALResourceStateFlags::CopySource);
      builder.ExportTexture(color, xiiGALResourceStateFlags::CopySource);
      for (const xiiStringView name : {"Query GPU Scene Instances", "Query GPU Visible Instance Count", "Query GPU Visible Meshlet Count"})
        if (const auto buffer = graph.FindBuffer(name); buffer.IsValid())
          builder.ExportBuffer(builder.ReadBuffer(buffer, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), queryView ? queryView : view, &graphContext.GetBlackboard(), &cache)))
      return false;
    completion.CaptureSubmittedFrame(frame);
    completion.WaitForFrame(frame); // Test-only wait; production queries remain nonblocking.
    return true;
  };
  xiiSceneMeshPickingPass::PickingResult hit;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native owner, surface position, normal and nonzero viewport origin")
  {
    if (!render() || !render()) // First shader/resource loads conservatively invalidate capture.
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    XII_TEST_BOOL(hit.m_Source.m_hOwnerComponent == frontComponent);
    XII_TEST_INT(hit.m_Source.m_uiSectionIndex, 1024);
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, -0.5f, 0.001f);
    XII_TEST_VEC3(hit.m_vWorldNormal, -xiiVec3::MakeAxisX(), 0.001f);
    XII_TEST_FLOAT(hit.m_vRayStart.x, -1.9f, 0.001f);
    XII_TEST_RESULT(pass->Pick(*view, 8, 12, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
    XII_TEST_BOOL(pass->Pick(*view, 7, 28, hit).Failed());
    XII_TEST_BOOL(otherPass->Pick(*view, 24, 28, hit).Failed());
    xiiDynamicArray<xiiGameObjectHandle> objects;
    XII_TEST_RESULT(pass->PickObjectsInRect(*view, {8, 12, 32, 32}, objects));
    XII_TEST_BOOL(objects.Contains(frontOwner));
    XII_TEST_BOOL(!objects.Contains(backOwner));
    XII_TEST_INT(objects.GetCount(), 1);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Odd triangle tails preserve UInt16 and UInt32 source geometry")
  {
    for (const auto indexType : {xiiGALValueType::UInt16, xiiGALValueType::UInt32})
    {
      xiiGeometry triangle;
      triangle.AddVertex(xiiVec3(-0.5f, -0.5f, -0.5f), -xiiVec3::MakeAxisX());
      triangle.AddVertex(xiiVec3(-0.5f, 0.0f, 0.5f), -xiiVec3::MakeAxisX());
      triangle.AddVertex(xiiVec3(-0.5f, 0.5f, -0.5f), -xiiVec3::MakeAxisX());
      xiiUInt32 vertices[] = {0, 1, 2};
      triangle.AddPolygon(xiiMakeArrayPtr(vertices), false);
      xiiMeshResourceDescriptor description;
      auto&                     buffer = description.GetMeshBufferDescriptor();
      buffer.AllocateStreamsFromGeometry(triangle, xiiGALPrimitiveTopology::TriangleList, false);
      if (indexType == xiiGALValueType::UInt16)
      {
        const xiiUInt16 indices[] = {0, 1, 2};
        buffer.SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(indices), sizeof(indices)), 3, indexType);
      }
      else
      {
        const xiiUInt32 indices[] = {0, 1, 2};
        buffer.SetIndexData(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(indices), sizeof(indices)), 3, indexType);
      }
      description.AddSubMesh(1, 0, 0);
      description.ComputeBounds();
      const auto triangleMesh  = xiiResourceManager::CreateResource<xiiMeshResource>(indexType == xiiGALValueType::UInt16 ? "NativeQueryTriangle16" : "NativeQueryTriangle32", std::move(description));
      front.m_hMesh            = triangleMesh;
      front.m_uiPrimitiveCount = 1;
      frontLodComponent->SetMesh(triangleMesh);
      if (!render() || !render())
        return;
      XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
      XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
      XII_TEST_FLOAT(hit.m_vWorldPosition.x, -0.5f, 0.001f);
      XII_TEST_VEC3(hit.m_vWorldNormal, -xiiVec3::MakeAxisX(), 0.001f);
      xiiResourceLock<xiiMeshResource>       resource(triangleMesh, xiiResourceAcquireMode::PointerOnly);
      xiiResourceLock<xiiMeshBufferResource> resident(resource->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
      XII_TEST_INT(resident->GetIndexCount(), 3);
      XII_TEST_INT(resident->GetIndexType().GetValue(), indexType);
      XII_TEST_INT(resident->GetIndexBuffer()->GetDescription().m_uiSize, indexType == xiiGALValueType::UInt16 ? 8 : 12);
      XII_TEST_INT(resident->GetMeshletPrimitiveIndexBuffer()->GetDescription().m_uiSize, 4);
    }
    front.m_hMesh            = mesh;
    front.m_uiPrimitiveCount = loaded->GetSections()[0].m_uiPrimitiveCount;
    frontLodComponent->SetMesh(mesh);
    if (!render() || !render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Component LOD controls and section selection invalidate captured picking images")
  {
    frontLodComponent->SetLodBias(1);
    front.m_fLodBias = 1;
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    frontLodComponent->SetForcedLOD(0);
    front.m_Flags.Add(xiiMeshRenderDataFlags::ForceLOD);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    frontLodComponent->SetSectionIndex(1);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    frontLodComponent->SetSectionIndex(xiiInvalidIndex);
    frontLodComponent->SetForcedLOD(xiiInvalidIndex);
    frontLodComponent->SetLodBias(0);
    front.m_Flags.Remove(xiiMeshRenderDataFlags::ForceLOD);
    front.m_fLodBias = 0;
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Material resource reload rejects old images and refreshes native coverage")
  {
    const auto material = xiiResourceManager::LoadResource<xiiMaterialResource>("NativeQueryReloadMaterial");
    auto       publish  = [&](float opacity) -> bool {
      xiiMaterialResourceDescriptor description;
      description.m_AlphaMode                          = xiiMaterialAlphaMode::Mask;
      description.m_BlendMode                          = xiiMaterialBlendMode::Masked;
      description.m_BaseColor.a                        = opacity;
      xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
      xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
      stream << "NativeQueryReloadMaterial.xiiBinMaterial";
      xiiAssetFileHeader header;
      if (!XII_TEST_RESULT(header.Write(stream)) || !XII_TEST_RESULT(xiiMaterialAssetFile::Write(stream, description)))
        return false;
      xiiResourceManager::UpdateResourceWithCustomLoader(material, std::move(loader));
      xiiResourceLock<xiiMaterialResource> loadedMaterial(material, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      return XII_TEST_BOOL(loadedMaterial.GetAcquireResult() == xiiResourceAcquireResult::Final);
    };
    if (!publish(1.0f))
      return;
    front.m_hMaterials.PushBack(material);
    if (!XII_TEST_BOOL(world.TryGetComponent(frontComponent, component)))
      return;
    component->SetMaterialOverride(0, material);
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    if (!publish(0.0f))
      return;
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == backOwner);
    if (!publish(1.0f))
      return;
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    front.m_hMaterials.Clear();
    component->ClearMaterialOverrides();
    if (!render() || !render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Mesh reload publishes new geometry while delayed views retain their exact revision")
  {
    const auto reloadedMesh = xiiResourceManager::LoadResource<xiiMeshResource>("NativeQueryReloadMesh");
    const auto publish      = [&](float size) -> bool {
      xiiGeometry shape;
      shape.AddBox(xiiVec3(size), true);
      xiiMeshResourceDescriptor description;
      description.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(shape, xiiGALPrimitiveTopology::TriangleList);
      description.AddSubMesh(description.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
      description.ComputeBounds();
      xiiUniquePtr<xiiResourceLoaderFromMemory> loader = XII_DEFAULT_NEW(xiiResourceLoaderFromMemory);
      xiiMemoryStreamWriter                     stream(&loader->m_CustomData);
      description.Save(stream);
      xiiResourceManager::UpdateResourceWithCustomLoader(reloadedMesh, std::move(loader));
      xiiResourceLock<xiiMeshResource> resource(reloadedMesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      return XII_TEST_BOOL(resource.GetAcquireResult() == xiiResourceAcquireResult::Final);
    };
    if (!publish(1.0f))
      return;
    front.m_hMesh = reloadedMesh;
    if (!XII_TEST_BOOL(world.TryGetComponent(frontComponent, component)))
      return;
    component->SetMesh(reloadedMesh);
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, -0.5f, 0.001f);
    const auto oldGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(reloadedMesh);
    if (!XII_TEST_BOOL(!oldGeometry.IsInvalidated()))
      return;
    bool releasedOld = false;
    XII_SCOPE_EXIT(if (!releasedOld) xiiGeometryResidencyManager::ReleaseMeshGeometry(oldGeometry, nextFrame - 1));
    xiiSceneDatabaseContext delayed;
    if (!XII_TEST_RESULT(delayed.Initialize()) || !XII_TEST_RESULT(delayed.SynchronizeExtractedMeshes(extracted, nextFrame - 1)))
      return;
    if (!publish(2.0f))
      return;
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, -1.0f, 0.001f);
    const auto currentGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(reloadedMesh);
    if (!XII_TEST_BOOL(!currentGeometry.IsInvalidated()))
      return;
    XII_SCOPE_EXIT(xiiGeometryResidencyManager::ReleaseMeshGeometry(currentGeometry, nextFrame - 1));
    XII_TEST_BOOL(currentGeometry != oldGeometry);
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(oldGeometry) == xiiGeometryResidencyState::Resident);
    const auto retained = xiiGeometryResidencyManager::GetGpuRecord(oldGeometry);
    const auto current  = xiiGeometryResidencyManager::GetGpuRecord(currentGeometry);
    if (XII_TEST_BOOL(retained && current))
    {
      XII_TEST_FLOAT(retained->m_BoundsExtents.x, 0.5f, 0.001f);
      XII_TEST_FLOAT(current->m_BoundsExtents.x, 1.0f, 0.001f);
      XII_TEST_BOOL(retained->m_Lods[0].m_uiMeshletMetadataOffset != current->m_Lods[0].m_uiMeshletMetadataOffset);
      XII_TEST_BOOL(retained->m_Lods[0].m_uiVertexBufferIndex != current->m_Lods[0].m_uiVertexBufferIndex);
      xiiGpuGeometryRecord uploaded;
      const auto&          configuration = xiiGeometryResidencyManager::GetConfiguration();
      const auto           index         = ((nextFrame - 1) % configuration.m_uiFramesInFlight) * configuration.m_uiMaxGeometries + currentGeometry.GetInternalID().m_InstanceIndex;
      if (ReadBufferRecord(*environment.GetDevice(), xiiGeometryResidencyManager::GetMetadataBuffer().Borrow(), index * sizeof(uploaded), uploaded))
        XII_TEST_BOOL(xiiMemoryUtils::RawByteCompare(&uploaded, current, sizeof(uploaded)) == 0);
      auto&               graph = graphContext.GetGraph();
      xiiGpuSceneInstance gpuInstance;
      if (ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(graph.FindBuffer("Query GPU Scene Instances")).Borrow(), 0, gpuInstance))
      {
        XII_TEST_INT(gpuInstance.m_uiGeometryIndex, currentGeometry.GetInternalID().m_InstanceIndex);
        XII_TEST_FLOAT(gpuInstance.m_BoundsExtents.x, 1.0f, 0.001f);
      }
      xiiUInt32 visibleInstances = 0, visibleMeshlets = 0;
      ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(graph.FindBuffer("Query GPU Visible Instance Count")).Borrow(), 0, visibleInstances);
      ReadBufferRecord(*environment.GetDevice(), graph.GetExportedBuffer(graph.FindBuffer("Query GPU Visible Meshlet Count")).Borrow(), 0, visibleMeshlets);
      XII_TEST_INT(visibleInstances, 2);
      XII_TEST_INT(visibleMeshlets, 2);
      xiiResourceLock<xiiMeshResource>       resource(reloadedMesh, xiiResourceAcquireMode::PointerOnly);
      xiiResourceLock<xiiMeshBufferResource> meshBuffer(resource->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
      xiiMeshPackedVertex                    vertex;
      if (ReadBufferRecord(*environment.GetDevice(), meshBuffer->GetVertexBuffer().Borrow(), 0, vertex))
        XII_TEST_FLOAT(xiiMath::Abs(vertex.m_vPosition.x), 1.0f, 0.001f);
      xiiMeshlet meshlet;
      if (ReadBufferRecord(*environment.GetDevice(), xiiGeometryResidencyManager::GetMeshletMetadataBuffer().Borrow(), current->m_Lods[0].m_uiMeshletMetadataOffset * sizeof(meshlet), meshlet))
      {
        XII_TEST_BOOL(xiiMemoryUtils::RawByteCompare(&meshlet, meshBuffer->GetMeshlets().GetPtr(), sizeof(meshlet)) == 0);
        XII_TEST_INT(meshlet.m_uiFirstPrimitive, 0);
        XII_TEST_INT(meshlet.m_uiPrimitiveCount, 12);
        XII_TEST_FLOAT(meshlet.m_Bounds.m_fRadius, xiiMath::Sqrt(3.0f), 0.001f);
      }
    }
    bool delayedUsesOld = false;
    for (const auto& entry : delayed.GetDatabase().GetGpuInstances())
      delayedUsesOld |= entry.m_uiGeometryIndex == oldGeometry.GetInternalID().m_InstanceIndex;
    XII_TEST_BOOL(delayedUsesOld);
    XII_TEST_RESULT(delayed.SynchronizeExtractedMeshes(extracted, nextFrame - 1));
    for (const auto& entry : delayed.GetDatabase().GetGpuInstances())
      XII_TEST_BOOL(entry.m_uiGeometryIndex != oldGeometry.GetInternalID().m_InstanceIndex);
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(oldGeometry) == xiiGeometryResidencyState::Resident);
    xiiGeometryResidencyManager::ReleaseMeshGeometry(oldGeometry, nextFrame - 1);
    releasedOld = true;
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(oldGeometry) == xiiGeometryResidencyState::EvictPending);
    xiiGeometryResidencyManager::ProcessStreaming(nextFrame, nextFrame - 2, xiiMath::MaxValue<xiiUInt64>());
    XII_TEST_BOOL(xiiGeometryResidencyManager::IsValid(oldGeometry));
    xiiGeometryResidencyManager::ProcessStreaming(nextFrame, nextFrame - 1, xiiMath::MaxValue<xiiUInt64>());
    XII_TEST_BOOL(!xiiGeometryResidencyManager::IsValid(oldGeometry));
    XII_TEST_BOOL(xiiGeometryResidencyManager::GetState(currentGeometry) == xiiGeometryResidencyState::Resident);
    // Releasing the previous revision must not remove the current revision's shared lookup.
    const auto shared = xiiGeometryResidencyManager::AcquireMeshGeometry(reloadedMesh);
    XII_TEST_BOOL(shared == currentGeometry);
    xiiGeometryResidencyManager::ReleaseMeshGeometry(shared, nextFrame - 1);
    front.m_hMesh = mesh;
    component->SetMesh(mesh);
    if (!render() || !render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shared resource views retain independent native query captures")
  {
    xiiView*   secondView   = nullptr;
    const auto secondHandle = renderer->CreateView("Independent query", secondView);
    XII_SCOPE_EXIT(renderer->DestroyView(secondHandle));
    secondView->SetCamera(&camera);
    secondView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
    secondView->SetViewport(view->GetViewport());
    secondView->SetRenderTargetView(target->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    secondView->GetExtractedRenderData()->AddRenderData(&front);
    secondView->GetExtractedRenderData()->SortAndBatches();
    if (!render(&other, secondView) || !render(&other, secondView))
      return;
    XII_TEST_RESULT(otherPass->Pick(*secondView, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    XII_TEST_BOOL(otherPass->Pick(*view, 24, 28, hit).Failed());
    XII_TEST_BOOL(pass->Pick(*secondView, 24, 28, hit).Failed());
    other.InvalidateTemporalHistory();
    XII_TEST_BOOL(otherPass->Pick(*secondView, 24, 28, hit).Failed());
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Selected object click-through preserves the main scene")
  {
    parameters.Insert("Editor.PickSelected", false);
    xiiVariantArray selected;
    selected.PushBack(static_cast<xiiUInt64>(frontOwner.GetInternalID().m_Data));
    parameters.Insert("Editor.SelectedObjects", selected);
    if (!render() || !render())
      return;
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == backOwner);
    XII_TEST_FLOAT(hit.m_vWorldPosition.x, 2.5f, 0.001f);
    XII_TEST_INT(mainScene.GetDatabase().GetObjectCount(), 2);
    for (const auto& entry : mainScene.GetDatabase().GetGpuInstances())
      XII_TEST_BOOL((entry.m_uiFlags & xiiSceneObjectFlags::Enabled) != 0);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native selected surfaces compose visible and occluded selection independently of click-through")
  {
    xiiUInt32 pixel = 0;
    if (ReadColorPixel(*environment.GetDevice(), target.Borrow(), 24, 28, pixel))
    {
      XII_TEST_FLOAT(static_cast<float>(pixel & 255), 128, 1);
      XII_TEST_INT((pixel >> 8) & 255, 0);
      XII_TEST_FLOAT(static_cast<float>((pixel >> 16) & 255), 128, 1);
      XII_TEST_INT(pixel >> 24, 255);
    }
    const auto* selectedPass = xiiDynamicCast<const xiiSceneMeshPickingPass*>(instance.GetPass(2));
    if (ReadColorPixel(*environment.GetDevice(), target.Borrow(), 15, 28, pixel))
    {
      XII_TEST_INT(pixel & 255, 255); // Outline has full coverage rather than interior fill opacity.
      XII_TEST_INT((pixel >> 16) & 255, 0);
    }
    if (ReadColorPixel(*environment.GetDevice(), target.Borrow(), 8, 12, pixel))
      XII_TEST_INT(pixel, 0xFFFF0000U); // Background outside the selected silhouette is preserved.
    XII_TEST_BOOL(selectedPass && selectedPass->m_bSelectionOnly);
    XII_TEST_BOOL(selectedPass->Pick(*view, 24, 28, hit).Failed());
    parameters.Insert("Test.SceneDepth", 1.0f);
    if (!render())
      return;
    if (ReadColorPixel(*environment.GetDevice(), target.Borrow(), 24, 28, pixel))
    {
      XII_TEST_INT(pixel & 255, 0);
      XII_TEST_FLOAT(static_cast<float>((pixel >> 8) & 255), 128, 1);
      XII_TEST_FLOAT(static_cast<float>((pixel >> 16) & 255), 128, 1);
    }
    parameters.Insert("Editor.SelectionOverlay", false);
    if (!render())
      return;
    if (ReadColorPixel(*environment.GetDevice(), target.Borrow(), 24, 28, pixel))
      XII_TEST_INT(pixel, 0xFFFF0000U); // Original opaque blue, including alpha.
    XII_TEST_BOOL(!graphContext.GetGraph().FindTexture("Selected query Surface").IsValid());
    parameters.Insert("Editor.SelectionOverlay", true);
    parameters.Insert("Test.SceneDepth", 0.0f);
    if (!render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Whole-mesh extraction preserves section identity and distinct material slots through GPU queries")
  {
    xiiGeometry             sectionGeometry;
    xiiGeometry::GeoOptions options;
    options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, -0.65f, 0));
    sectionGeometry.AddBox(xiiVec3(0.8f), true, options);
    options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(0, 0.65f, 0));
    sectionGeometry.AddBox(xiiVec3(0.8f), true, options);
    xiiMeshResourceDescriptor sections;
    sections.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(sectionGeometry, xiiGALPrimitiveTopology::TriangleList);
    sections.AddSubMesh(12, 0, 5);
    sections.AddSubMesh(12, 12, 2);
    sections.SetMaterial(5, "");
    sections.SetMaterial(2, "NativeQueryReloadMaterial");
    sections.ComputeBounds();
    const auto sectionMesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeQuerySectionMesh", std::move(sections));
    XII_TEST_BOOL(world.TryGetComponent(frontComponent, component));
    component->SetMesh(sectionMesh);
    component->SetSectionIndex(xiiInvalidIndex);
    xiiExtractedRenderData  sectionPackets;
    xiiMsgExtractRenderData message;
    message.m_pView                = view;
    message.m_pExtractedRenderData = &sectionPackets;
    XII_TEST_BOOL(component->SendMessage(message));
    sectionPackets.SortAndBatches();
    if (!XII_TEST_INT(sectionPackets.GetAllRenderData().GetCount(), 2))
      return;
    view->GetExtractedRenderData()->Clear();
    extracted.Clear();
    for (auto* packet : sectionPackets.GetAllRenderData())
    {
      const auto* section = xiiDynamicCast<const xiiMeshRenderData*>(packet);
      if (!XII_TEST_BOOL(section && section->m_uiSectionIndex < 2))
        return;
      XII_TEST_INT(section->m_uiFirstPrimitive, section->m_uiSectionIndex * 12);
      XII_TEST_INT(section->m_uiPrimitiveCount, 12);
      XII_TEST_INT(section->m_uiMeshletCount, 1);
      XII_TEST_BOOL(section->m_hOwnerObject == frontOwner);
      view->GetExtractedRenderData()->AddRenderData(packet);
      extracted.AddRenderData(packet);
    }
    view->GetExtractedRenderData()->SortAndBatches();
    extracted.SortAndBatches();
    parameters.Insert("Editor.PickSelected", true);
    parameters.Insert("Editor.SelectedObjects", xiiVariantArray{});
    if (!render() || !render())
      return;
    bool seen[2] = {};
    for (xiiUInt32 x = 8; x < 40; ++x)
    {
      if (!XII_TEST_RESULT(pass->Pick(*view, x, 28, hit)))
        return;
      if (hit.m_Source.m_hOwnerObject.IsInvalidated())
        continue;
      const xiiUInt32 section = hit.m_vWorldPosition.y < 0 ? 0 : 1;
      XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
      XII_TEST_INT(hit.m_Source.m_uiSectionIndex, section);
      XII_TEST_INT(hit.m_Source.m_uiMaterialIndex, section == 0 ? 5 : 2);
      seen[section] = true;
    }
    XII_TEST_BOOL(seen[0] && seen[1]);
    component->SetSectionIndex(1);
    sectionPackets.Clear();
    XII_TEST_BOOL(component->SendMessage(message));
    sectionPackets.SortAndBatches();
    XII_TEST_INT(sectionPackets.GetAllRenderData().GetCount(), 1);
    if (sectionPackets.GetAllRenderData().GetCount() == 1)
      XII_TEST_INT(static_cast<xiiMeshRenderData*>(sectionPackets.GetAllRenderData()[0])->m_uiFirstPrimitive, 12);
    component->SetSectionIndex(99);
    sectionPackets.Clear();
    XII_TEST_BOOL(component->SendMessage(message));
    sectionPackets.SortAndBatches();
    XII_TEST_BOOL(sectionPackets.GetAllRenderData().IsEmpty());
    component->SetMesh(mesh);
    component->SetSectionIndex(xiiInvalidIndex);
    view->GetExtractedRenderData()->Clear();
    extracted.Clear();
    for (auto* packet : {&front, &back})
    {
      view->GetExtractedRenderData()->AddRenderData(packet);
      extracted.AddRenderData(packet);
    }
    view->GetExtractedRenderData()->SortAndBatches();
    extracted.SortAndBatches();
    parameters.Insert("Editor.PickSelected", false);
    xiiVariantArray selected;
    selected.PushBack(static_cast<xiiUInt64>(frontOwner.GetInternalID().m_Data));
    parameters.Insert("Editor.SelectedObjects", selected);
    if (!render() || !render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Thousands of material sections allocate the actual meshlet stream")
  {
    // The previous whole-LOD-per-section bound requested 8,388,608 slots for this
    // small mesh, exceeding common mesh dispatch limits just like imported FeatureLab assets.
    constexpr xiiUInt32 sectionCount = 2200;
    xiiGeometry         geometry;
    for (xiiUInt32 section = 0; section < sectionCount; ++section)
      geometry.AddBox(xiiVec3(0.8f), true);
    xiiMeshResourceDescriptor definition;
    definition.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
    for (xiiUInt32 section = 0; section < sectionCount; ++section)
      definition.AddSection(12, section * 12, 0);
    const auto sectionMesh = xiiResourceManager::CreateResource<xiiMeshResource>("NativeQueryManySections", std::move(definition));
    component->SetMesh(sectionMesh);
    parameters.Insert("Editor.PickSelected", true);
    xiiExtractedRenderData  packets;
    xiiMsgExtractRenderData message;
    message.m_pView                = view;
    message.m_pExtractedRenderData = &packets;
    XII_TEST_BOOL(component->SendMessage(message));
    packets.SortAndBatches();
    XII_TEST_INT(packets.GetAllRenderData().GetCount(), sectionCount);
    view->GetExtractedRenderData()->Clear();
    extracted.Clear();
    for (auto* packet : packets.GetAllRenderData())
    {
      view->GetExtractedRenderData()->AddRenderData(packet);
      extracted.AddRenderData(packet);
    }
    view->GetExtractedRenderData()->SortAndBatches();
    extracted.SortAndBatches();
    if (!render() || !render())
      return;
    xiiUInt64 capacity = 0;
    XII_TEST_RESULT(xiiSceneDatabaseManager::GetExtractedMeshletCapacity(mainScene.GetHandle(), capacity));
    XII_TEST_INT(capacity, sectionCount);
    XII_TEST_RESULT(pass->Pick(*view, 24, 28, hit));
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject == frontOwner);
    component->SetMesh(mesh);
    parameters.Insert("Editor.PickSelected", false);
    view->GetExtractedRenderData()->Clear();
    extracted.Clear();
    for (auto* packet : {&front, &back})
    {
      view->GetExtractedRenderData()->AddRenderData(packet);
      extracted.AddRenderData(packet);
    }
    view->GetExtractedRenderData()->SortAndBatches();
    extracted.SortAndBatches();
    if (!render() || !render())
      return;
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Camera, viewport, live transform, deletion and explicit history invalidation")
  {
    view->SetViewport({9, 12, 32, 32});
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    view->SetViewport({8, 12, 32, 32});
    camera.MoveLocally(0.1f, 0, 0);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    XII_TEST_BOOL(world.TryGetObject(backOwner, backObject));
    backObject->SetGlobalPosition(xiiVec3(4, 0, 0));
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    backObject->SetGlobalPosition(xiiVec3(3, 0, 0));
    world.DeleteObjectNow(backOwner);
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
    XII_TEST_BOOL(hit.m_Source.m_hOwnerObject.IsInvalidated());
    instance.InvalidateTemporalHistory();
    XII_TEST_BOOL(pass->Pick(*view, 24, 28, hit).Failed());
  }
  view->GetExtractedRenderData()->Clear();
}

XII_CREATE_SIMPLE_TEST(EditorViews, SceneMeshPicking)
{
  TestSceneMeshPicking(true);
}

XII_CREATE_SIMPLE_TEST(EditorViews, SceneMeshPickingWithoutMeshShaders)
{
  TestSceneMeshPicking(false);
}
