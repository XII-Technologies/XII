/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Visibility/GpuSceneRaster.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  bool ReadIds(xiiGALDevice& device, xiiGALTexture* texture, xiiDynamicArray<xiiUInt32>& pixels)
  {
    if (!XII_TEST_BOOL(texture != nullptr))
      return false;
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
    auto capture = readback.GetCompleted();
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
    pixels.SetCount(capture.m_uiWidth * capture.m_uiHeight);
    for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      xiiMemoryUtils::Copy(pixels.GetData() + y * capture.m_uiWidth, xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride), capture.m_uiWidth);
    const auto result = commands->UnmapTextureSubresource(capture.m_pStagingTexture, mip);
    commands->End();
    return XII_TEST_RESULT(result);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, SceneMeshIdentity)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry geometry;
  geometry.AddBox(xiiVec3(1), true);
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  descriptor.AddSubMesh(descriptor.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  descriptor.AddSubMesh(descriptor.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  descriptor.ComputeBounds();
  auto                             mesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorMeshIdentity", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;

  xiiWorldDescription worldDescription("Mesh identity owners");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject*    object          = nullptr;
  auto              owner           = world.CreateObject(xiiGameObjectDescription(), object);
  xiiMeshComponent* component       = nullptr;
  auto              firstComponent  = xiiMeshComponent::CreateComponent(object, component);
  auto              secondComponent = xiiMeshComponent::CreateComponent(object, component);

  xiiMeshRenderData first, second;
  first.m_hMesh        = mesh;
  second.m_hMesh       = mesh;
  first.m_hOwnerObject = second.m_hOwnerObject = owner;
  first.m_hOwnerComponent                      = firstComponent;
  second.m_hOwnerComponent                     = secondComponent;
  first.m_GlobalTransform = second.m_GlobalTransform = xiiTransform::MakeIdentity();
  first.m_uiUniqueID = second.m_uiUniqueID = 0; // Deliberately collide, including background user data.
  first.m_uiSectionIndex = second.m_uiSectionIndex = 0;
  first.m_uiPrimitiveCount = second.m_uiPrimitiveCount = loaded->GetSections()[0].m_uiPrimitiveCount;
  second.m_GlobalTransform.m_vPosition.x               = 3;
  xiiExtractedRenderData extracted;
  extracted.AddRenderData(&first);
  extracted.AddRenderData(&second);
  extracted.SortAndBatches();
  xiiSceneDatabaseContext context;
  if (!XII_TEST_RESULT(context.Initialize()) || !XII_TEST_RESULT(context.SynchronizeExtractedMeshes(extracted, 1)))
    return;
  auto&              scene = context.GetDatabase();
  xiiSceneMeshSource source;
  xiiUInt32          firstId = 0, secondId = 0;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Colliding application IDs preserve independent native component owners")
  {
    XII_TEST_INT(scene.GetObjectCount(), 2);
    for (const auto& instance : scene.GetGpuInstances())
    {
      XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), instance.m_uiObjectId, source));
      XII_TEST_BOOL(source.m_hOwnerObject == owner);
      XII_TEST_INT(source.m_uiSectionIndex, 0);
      XII_TEST_INT(source.m_uiRenderDataId, 0);
      if (source.m_hOwnerComponent == firstComponent)
        firstId = instance.m_uiObjectId;
      else if (source.m_hOwnerComponent == secondComponent)
        secondId = instance.m_uiObjectId;
      else
        XII_TEST_BOOL(false);
    }
    XII_TEST_BOOL(firstId != 0 && secondId != 0 && firstId != secondId);
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), 0, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerObject.IsInvalidated());
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh({}, firstId, source).Failed());
    const auto directObject = scene.CreateObject(xiiSceneObjectDesc());
    const auto directId     = directObject.GetInternalID().m_Data + 1U;
    XII_TEST_BOOL(scene.ResolveGpuObjectId(directId) == directObject);
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), directId, source).Failed());
    XII_TEST_BOOL(source.m_hSceneObject.IsInvalidated());
    XII_TEST_BOOL(scene.DestroyObject(directObject));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native mesh shader G-buffer writes the generation identity rather than user data")
  {
    if (xiiGpuSceneRasterManager::IsSupported())
    {
      xiiRenderGraphContext graphContext;
      if (!XII_TEST_RESULT(graphContext.Initialize("Mesh identity raster")))
        return;
      auto visibilityContext = xiiGpuVisibilityManager::CreateContext();
      XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiGpuVisibilityManager::DestroyContext(visibilityContext));
      auto&      graph     = graphContext.GetGraph();
      auto&      cache     = graphContext.GetResourceCache();
      const auto completed = xiiRenderGraphManager::PrepareFrame(1, 3);
      xiiGeometryResidencyManager::ProcessStreaming(1, completed, xiiMath::MaxValue<xiiUInt64>());
      cache.BeginFrame(1, completed);
      graph.BeginSetup(1);
      const auto uploadedGeometry = xiiGeometryResidencyManager::AddUploadPass(graph, 1);
      const auto materials        = xiiMaterialManager::AddUploadPasses(graph);
      xiiCamera  camera;
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
      // This raster service explicitly consumes reversed-Z depth (clear 0, greater comparison).
      const auto                      projection     = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(60), 1, 100, 0.1f,
                                                                                                                   xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      const auto                      viewProjection = projection * camera.GetViewMatrix();
      const auto                      frustum        = xiiFrustum::MakeFromMVP(viewProjection, xiiClipSpaceDepthRange::ZeroToOne, xiiHandedness::LeftHanded);
      const auto                      visibilityView = xiiGpuVisibilitySystem::BuildView(viewProjection, frustum, camera.GetCenterPosition(), 32, 32, 0, scene.GetObjectCount());
      xiiGpuVisibilityPassDescription visibilityDescription;
      visibilityDescription.m_bAsyncCompute        = false;
      visibilityDescription.m_bEnableLodFeedback   = false;
      const auto                        visibility = xiiGpuVisibilityManager::AddPasses(visibilityContext, graph, 1, context.GetHandle(), visibilityView, uploadedGeometry, visibilityDescription);
      xiiGpuSceneDepthRasterDescription depthDescription;
      depthDescription.m_ViewProjectionMatrix = viewProjection;
      depthDescription.m_uiWidth = depthDescription.m_uiHeight = 32;
      depthDescription.m_uiVertexStride                        = sizeof(xiiMeshPackedVertex);
      depthDescription.m_uiTexCoordOffset                      = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
      depthDescription.m_uiMeshDispatchGroupCountX             = xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(visibilityContext);
      depthDescription.m_uiMeshDispatchGroupCountY             = xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(visibilityContext);
      const auto depth                                         = xiiGpuSceneRasterManager::AddDepthPrepass(graph, "Identity depth", visibility, uploadedGeometry, materials, depthDescription);
      struct Velocity
      {
        xiiRenderGraphTextureHandle m_Target;
      };
      auto                                velocity = graph.AddPass<Velocity>("Identity velocity", xiiGALCommandQueueFlags::Graphics, [](Velocity& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription target;
        target.m_Type = xiiGALResourceDimension::Texture2D;
        target.m_Size = {32, 32};
        target.m_Format = xiiGALResourceFormat::RG16Float;
        target.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
        data.m_Target = builder.WriteTexture("Identity velocity", target, xiiGALResourceStateFlags::RenderTarget);
        builder.SetPassRenderPassManaged(); }, [](const Velocity& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data.m_Target)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero()); });
      xiiGpuSceneGBufferRasterDescription raster;
      raster.m_ViewProjectionMatrix = raster.m_PreviousViewProjectionMatrix = viewProjection;
      raster.m_uiWidth = raster.m_uiHeight = 32;
      raster.m_uiVertexStride              = sizeof(xiiMeshPackedVertex);
      raster.m_uiNormalOffset              = offsetof(xiiMeshPackedVertex, m_vNormal);
      raster.m_uiTangentOffset             = offsetof(xiiMeshPackedVertex, m_vTangent);
      raster.m_uiTexCoordOffset            = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
      raster.m_uiMeshDispatchGroupCountX   = depthDescription.m_uiMeshDispatchGroupCountX;
      raster.m_uiMeshDispatchGroupCountY   = depthDescription.m_uiMeshDispatchGroupCountY;
      const auto outputs                   = xiiGpuSceneRasterManager::AddGBufferPass(graph, "Identity G-buffer", depth, velocity.first->m_Target, visibility, uploadedGeometry, materials, raster);
      if (!XII_TEST_BOOL(outputs.IsValid()))
        return;
      struct Export
      {
        XII_DECLARE_POD_TYPE();
      };
      graph.AddPass<Export>("Identity export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
        XII_IGNORE_UNUSED(builder.ReadTexture(outputs.m_hObjectId, xiiGALResourceStateFlags::ShaderResource));
        builder.ExportTexture(outputs.m_hObjectId, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphContext.GetBlackboard(), &cache)))
        return;
      auto                       texture = graph.GetExportedTexture(outputs.m_hObjectId);
      xiiDynamicArray<xiiUInt32> pixels;
      if (ReadIds(*environment.GetDevice(), texture.Borrow(), pixels))
      {
        XII_TEST_INT(pixels[16 * 32 + 16], firstId);
        XII_TEST_INT(pixels[0], 0);
        XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), pixels[16 * 32 + 16], source));
        XII_TEST_BOOL(source.m_hOwnerComponent == firstComponent);
      }
      cache.EndFrame();
    }
    else
      xiiTestFramework::Output(xiiTestOutput::Message, "Mesh-shader raster identity was not exercised on this device.");
  }

  xiiMeshRenderData section;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sections of the same component preserve independent identities")
  {
    section.m_hMesh            = mesh;
    section.m_hOwnerObject     = owner;
    section.m_hOwnerComponent  = firstComponent;
    section.m_GlobalTransform  = xiiTransform::MakeIdentity();
    section.m_uiUniqueID       = 0;
    section.m_uiSectionIndex   = 1;
    section.m_uiPrimitiveCount = first.m_uiPrimitiveCount;
    extracted.AddRenderData(&section);
    extracted.SortAndBatches();
    XII_TEST_RESULT(context.SynchronizeExtractedMeshes(extracted, 2));
    XII_TEST_INT(scene.GetObjectCount(), 3);
    xiiUInt32 sectionId = 0;
    for (const auto& instance : scene.GetGpuInstances())
    {
      XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), instance.m_uiObjectId, source));
      if (source.m_uiSectionIndex == 1)
      {
        XII_TEST_BOOL(source.m_hOwnerComponent == firstComponent);
        sectionId = instance.m_uiObjectId;
      }
    }
    XII_TEST_BOOL(sectionId != 0 && sectionId != firstId && sectionId != secondId);
    XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), firstId, source));
    XII_TEST_INT(source.m_uiSectionIndex, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Retirement, clearing and destroyed contexts reject stale mesh identities")
  {
    extracted.Clear();
    extracted.AddRenderData(&second);
    extracted.SortAndBatches();
    XII_TEST_RESULT(context.SynchronizeExtractedMeshes(extracted, 3));
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), firstId, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerComponent.IsInvalidated());
    XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), secondId, source));
    scene.Clear();
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), secondId, source).Failed());
    XII_TEST_RESULT(context.SynchronizeExtractedMeshes(extracted, 4));
    XII_TEST_INT(scene.GetObjectCount(), 1);
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), secondId, source).Failed());
    const auto replacementId = scene.GetGpuInstances()[0].m_uiObjectId;
    XII_TEST_RESULT(xiiSceneDatabaseManager::ResolveExtractedMesh(context.GetHandle(), replacementId, source));
    XII_TEST_BOOL(source.m_hOwnerComponent == secondComponent);
    const auto oldContext = context.GetHandle();
    context.Shutdown();
    XII_TEST_BOOL(xiiSceneDatabaseManager::ResolveExtractedMesh(oldContext, replacementId, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerObject.IsInvalidated());
  }
}
