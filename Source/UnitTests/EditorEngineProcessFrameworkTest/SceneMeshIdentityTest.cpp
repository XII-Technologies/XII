/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EditorEngineProcessFrameworkTest/NativeSurfaceRasterFixture.h>

#include <bit>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFrameworkTest/NativeVisibilityFixture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/GpuFrameCompletionTracker.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <SharedPluginScene/Scene/Implementation/ScenePickingReadback.h>

namespace
{
  void SetTestSkinningPose(xiiMeshRenderData& packet, float offset)
  {
    // Two influences with a non-255 weight sum must normalize to the requested translation.
    packet.m_SkinningMatrices.SetCount(2);
    packet.m_SkinningMatrices[0] = xiiMat4::MakeTranslation(xiiVec3(0, offset - 0.5f, 0));
    packet.m_SkinningMatrices[1] = xiiMat4::MakeTranslation(xiiVec3(0, offset + 0.25f, 0));
  }

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
    const xiiUInt32 wordsPerPixel = capture.m_uiBytesPerPixel / sizeof(xiiUInt32);
    const xiiUInt32 rowWords      = capture.m_uiWidth * wordsPerPixel;
    pixels.SetCount(rowWords * capture.m_uiHeight);
    for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      xiiMemoryUtils::Copy(pixels.GetData() + y * rowWords, xiiMemoryUtils::AddByteOffset(static_cast<const xiiUInt32*>(mapped.m_pData), y * mapped.m_uiStride), rowWords);
    const auto result = commands->UnmapTextureSubresource(capture.m_pStagingTexture, mip);
    commands->End();
    return XII_TEST_RESULT(result);
  }
} // namespace

static void RunSceneMeshIdentity(bool denseMeshlet, bool skinned = false, bool meshPicking = true)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGeometry geometry;
  if (denseMeshlet)
  {
    // An 8x8 vertex grid forms 98 triangles within a single meshlet. Its front surface
    // coincides with the box fixture so projection/depth/normal assertions remain shared.
    xiiGeometry::GeoOptions options;
    options.m_Transform = xiiMat4::MakeTranslation(xiiVec3(-0.5f, 0, 0)) *
      xiiMat4::MakeAxisRotation(xiiVec3::MakeAxisY(), xiiAngle::MakeFromDegree(-90));
    geometry.AddRect(xiiVec2(1), 7, 7, options);
  }
  else
    geometry.AddBox(xiiVec3(1), true);
  if (skinned)
    for (auto& vertex : geometry.GetVertices())
    {
      vertex.m_BoneIndices = xiiVec4U16(0, 1, 65000, 65000);
      vertex.m_BoneWeights = xiiColorLinearUB(64, 128, 0, 0);
    }
  xiiMeshResourceDescriptor descriptor;
  descriptor.GetMeshBufferDescriptor().AllocateStreamsFromGeometry(geometry, xiiGALPrimitiveTopology::TriangleList);
  descriptor.AddSubMesh(descriptor.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  descriptor.AddSubMesh(descriptor.GetMeshBufferDescriptor().GetPrimitiveCount(), 0, 0);
  descriptor.ComputeBounds();
  auto                             mesh = xiiResourceManager::CreateResource<xiiMeshResource>("EditorMeshIdentity", std::move(descriptor));
  xiiResourceLock<xiiMeshResource> loaded(mesh, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(loaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    return;

  if (denseMeshlet)
  {
    xiiResourceLock<xiiMeshBufferResource> buffer(loaded->GetMeshBuffer(), xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    const auto                             meshlets                     = buffer->GetMeshlets();
    bool                                   hasMorePrimitivesThanThreads = false;
    for (const auto& meshlet : meshlets)
      hasMorePrimitivesThanThreads |= meshlet.m_uiPrimitiveCount > 64 && meshlet.m_uiVertexCount <= 64;
    if (!XII_TEST_BOOL(hasMorePrimitivesThanThreads))
      return;
  }

  xiiWorldDescription worldDescription("Mesh identity owners");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiGameObject*    object          = nullptr;
  auto              owner           = world.CreateObject(xiiGameObjectDescription(), object);
  xiiMeshComponent* component       = nullptr;
  auto              firstComponent  = xiiMeshComponent::CreateComponent(object, component);
  auto              secondComponent = xiiMeshComponent::CreateComponent(object, component);

  world.Update();
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
  if (skinned)
  {
    SetTestSkinningPose(first, 2);
    SetTestSkinningPose(second, -2);
  }
  xiiExtractedRenderData extracted;
  extracted.AddRenderData(&first);
  extracted.AddRenderData(&second);
  extracted.SortAndBatches();
  auto* renderer = world.GetOrCreateModule<xiiRenderWorldModule>();
  if (!XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, 1)))
    return;
  const auto&          context = *renderer->GetGpuInstanceData();
  const auto&          scene   = context;
  xiiGpuInstanceSource source;
  xiiUInt32            firstId = 0, secondId = 0;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Colliding application IDs preserve independent native component owners")
  {
    XII_TEST_INT(scene.GetInstanceCount(), 2);
    for (const auto& instance : scene.GetInstances())
    {
      XII_TEST_RESULT(context.Resolve(instance.m_uiObjectId, source));
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
    XII_TEST_BOOL(context.Resolve(0, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerObject.IsInvalidated());
    xiiWorldDescription otherDescription("Foreign identity world");
    xiiWorld            other(otherDescription);
    xiiGpuInstanceData  foreign(other);
    XII_TEST_BOOL(foreign.Resolve(firstId, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerComponent.IsInvalidated());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native mesh shader G-buffer writes the generation identity rather than user data")
  {
    if (environment.GetDevice()->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled)
    {
      xiiRenderGraphContext graphContext;
      if (!XII_TEST_RESULT(graphContext.Initialize("Mesh identity raster")))
        return;
      xiiTestNativeVisibility visibilityPipeline;
      if (!XII_TEST_RESULT(visibilityPipeline.Initialize()))
        return;
      auto&               graph = graphContext.GetGraph();
      auto&               cache = graphContext.GetResourceCache();
      xiiRenderGraphFrame frame;
      if (!XII_TEST_RESULT(frame.Begin()))
        return;
      XII_TEST_INT(frame.GetFrameIndex(), 1);
      const auto completed = frame.GetCompletedFrame();
      xiiGeometryResidencyManager::ProcessStreaming(1, completed, xiiMath::MaxValue<xiiUInt64>());
      xiiUInt64 capacity = 0;
      capacity           = context.GetMaximumMeshletCount();
      XII_TEST_BOOL(capacity > 0);
      cache.BeginFrame(1, completed);
      graph.BeginSetup(1);
      const auto uploadedGeometry = xiiGeometryResidencyManager::AddUploadPass(graph, 1);
      const auto materials        = xiiMaterialManager::AddUploadPasses(graph);
      xiiCamera  camera;
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      const float poseOffset = skinned ? 2.0f : 0.0f;
      camera.LookAt(xiiVec3(-2, poseOffset, 0), xiiVec3(0, poseOffset, 0), xiiVec3::MakeAxisZ());
      auto*      worldRenderer = world.GetOrCreateModule<xiiRenderWorldModule>();
      xiiView*   view          = nullptr;
      const auto viewHandle    = worldRenderer->CreateView("Identity projection", view);
      XII_SCOPE_EXIT(worldRenderer->DestroyView(viewHandle));
      view->SetCamera(&camera);
      view->SetViewport({0, 0, 32, 32});
      view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
      const auto viewProjection = view->GetViewProjectionMatrix(xiiCameraEye::Left);
      view->SetRenderEnabled(false);
      xiiGpuVisibilityOutputs visibility;
      if (!XII_TEST_RESULT(visibilityPipeline.Build(view, graph, graphContext.GetBlackboard(), viewProjection, camera.GetCenterPosition(), true, visibility)))
        return;
      xiiTestSurfaceLayout raster;
      raster.m_ViewProjectionMatrix = raster.m_PreviousViewProjectionMatrix = viewProjection;
      raster.m_uiWidth = raster.m_uiHeight = 32;
      raster.m_uiVertexStride              = sizeof(xiiMeshPackedVertex);
      raster.m_uiNormalOffset              = offsetof(xiiMeshPackedVertex, m_vNormal);
      raster.m_uiTangentOffset             = offsetof(xiiMeshPackedVertex, m_vTangent);
      raster.m_uiTexCoordOffset            = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
      raster.m_uiMeshDispatchGroupCountX   = visibilityPipeline.m_Commands.m_uiMeshGroupCountX;
      raster.m_uiMeshDispatchGroupCountY   = visibilityPipeline.m_Commands.m_uiMeshGroupCountY;
      xiiRenderGraphTextureHandle depth;
      if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Identity depth", visibility, uploadedGeometry, materials, raster, depth, meshPicking, xiiGALFillMode::Solid, nullptr, nullptr, true)))
        return;
      xiiTestMaterialTargets      outputs;
      xiiRenderGraphTextureHandle unusedMaterialOutput;
      if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Identity G-buffer", visibility, uploadedGeometry, materials, raster, unusedMaterialOutput, meshPicking, xiiGALFillMode::Solid, nullptr, nullptr, false, &outputs, depth)))
        return;
      if (!XII_TEST_BOOL(outputs.IsValid()))
        return;
      xiiRenderGraphTextureHandle query;
      if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Identity query", visibility, uploadedGeometry, materials, raster, query, meshPicking)))
        return;
      xiiRenderGraphTextureHandle wireframe;
      const bool                  wireframeSupported = environment.GetDevice()->GetFeatures().m_WireframeFill == xiiGALDeviceFeatureState::Enabled;
      if (!denseMeshlet && wireframeSupported)
      {
        if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Wireframe identity query", visibility, uploadedGeometry, materials, raster, wireframe, meshPicking, xiiGALFillMode::Wireframe)))
          return;
      }
      xiiRenderGraphTextureHandle invalidFill = query;
      XII_TEST_BOOL(xiiTestNativeSurfaceQuery(graph, view, "Invalid query fill", visibility, uploadedGeometry, materials, raster, invalidFill, meshPicking, xiiGALFillMode::Undefined).Failed());
      XII_TEST_BOOL(!invalidFill.IsValid());
      if (!wireframeSupported)
      {
        XII_TEST_BOOL(xiiTestNativeSurfaceQuery(graph, view, "Unsupported wireframe", visibility, uploadedGeometry, materials, raster, invalidFill, meshPicking, xiiGALFillMode::Wireframe).Failed());
        XII_TEST_BOOL(!invalidFill.IsValid());
      }
      struct Export
      {
        XII_DECLARE_POD_TYPE();
      };
      graph.AddPass<Export>("Identity export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
        XII_IGNORE_UNUSED(builder.ReadTexture(outputs.m_hObjectId, xiiGALResourceStateFlags::ShaderResource));
        builder.ExportTexture(outputs.m_hObjectId, xiiGALResourceStateFlags::ShaderResource);
        XII_IGNORE_UNUSED(builder.ReadTexture(query, xiiGALResourceStateFlags::ShaderResource));
        builder.ExportTexture(query, xiiGALResourceStateFlags::ShaderResource);
        if (wireframe.IsValid())
          builder.ExportTexture(builder.ReadTexture(wireframe, xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
      xiiScenePickingReadback asyncReadback;
      xiiUInt32               completedSlot, reservedSlot;
      XII_TEST_RESULT(asyncReadback.Poll(completedSlot));
      XII_TEST_INT(completedSlot, xiiInvalidIndex);
      xiiVariantDictionary         parameters;
      xiiRenderPipelinePassContext captureContext{view, graph, graphContext.GetBlackboard(), parameters, {}, {}, "Mesh identity"};
      if (!XII_TEST_RESULT(asyncReadback.AddCapture(captureContext, query, reservedSlot)))
        return;
      XII_TEST_BOOL(reservedSlot != xiiInvalidIndex);
      for (xiiUInt32 i = 1; i < xiiScenePickingReadback::SlotCount; ++i)
      {
        XII_TEST_RESULT(asyncReadback.Poll(completedSlot));
        XII_TEST_RESULT(asyncReadback.AddCapture(captureContext, query, reservedSlot));
        XII_TEST_INT(reservedSlot, i);
      }
      xiiUInt32 backpressureSlot;
      XII_TEST_RESULT(asyncReadback.Poll(completedSlot));
      XII_TEST_RESULT(asyncReadback.AddCapture(captureContext, query, backpressureSlot));
      XII_TEST_INT(backpressureSlot, xiiInvalidIndex);
      graph.EndSetup();
      if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphContext.GetBlackboard(), &cache)))
        return;
      auto                       texture = graph.GetExportedTexture(outputs.m_hObjectId);
      xiiDynamicArray<xiiUInt32> pixels;
      if (ReadIds(*environment.GetDevice(), texture.Borrow(), pixels))
      {
        XII_TEST_INT(pixels[16 * 32 + 16], firstId);
        if (denseMeshlet)
          for (xiiUInt32 y = 8; y < 24; ++y)
            for (xiiUInt32 x = 8; x < 24; ++x)
              XII_TEST_INT(pixels[y * 32 + x], firstId);
        XII_TEST_INT(pixels[0], 0);
        XII_TEST_RESULT(context.Resolve(pixels[16 * 32 + 16], source));
        XII_TEST_BOOL(source.m_hOwnerComponent == firstComponent);
      }
      auto queryTexture = graph.GetExportedTexture(query);
      if (ReadIds(*environment.GetDevice(), queryTexture.Borrow(), pixels))
      {
        const xiiUInt32 center = (16 * 32 + 16) * 4;
        XII_TEST_INT(pixels[center], firstId);
        if (denseMeshlet)
          for (xiiUInt32 y = 8; y < 24; ++y)
            for (xiiUInt32 x = 8; x < 24; ++x)
              XII_TEST_INT(pixels[(y * 32 + x) * 4], firstId);
        XII_TEST_INT(pixels[0], 0);
        const auto  surfaceClip = viewProjection * xiiVec4(-0.5f, poseOffset, 0, 1);
        const float depth       = std::bit_cast<float>(pixels[center + 1]);
        XII_TEST_FLOAT(depth, surfaceClip.z / surfaceClip.w, 0.0001f);
        XII_TEST_FLOAT(std::bit_cast<float>(pixels[center + 2]), 0.0f, 0.0001f);
        XII_TEST_FLOAT(std::bit_cast<float>(pixels[center + 3]), 0.5f, 0.0001f);
      }
      // ReadIds waited for a subsequent graphics submission; the capture transport itself never waits.
      XII_TEST_RESULT(asyncReadback.Poll(completedSlot));
      XII_TEST_INT(completedSlot, reservedSlot);
      XII_TEST_BOOL(asyncReadback.Matches(*view));
      XII_TEST_INT(asyncReadback.GetWordsPerPixel(), 4);
      XII_TEST_INT(asyncReadback.GetPixels().GetCount(), 32 * 32 * 4);
      if (asyncReadback.GetPixels().GetCount() == pixels.GetCount())
        XII_TEST_BOOL(asyncReadback.GetPixels() == pixels.GetArrayPtr());
      XII_TEST_RESULT(asyncReadback.Poll(completedSlot));
      XII_TEST_INT(completedSlot, xiiInvalidIndex);
      view->SetViewport({1, 0, 32, 32});
      XII_TEST_BOOL(!asyncReadback.Matches(*view));
      view->SetViewport({0, 0, 32, 32});
      XII_TEST_BOOL(asyncReadback.Matches(*view));
      asyncReadback.Invalidate();
      XII_TEST_BOOL(!asyncReadback.Matches(*view));
      XII_TEST_BOOL(asyncReadback.GetPixels().IsEmpty());
      if (wireframe.IsValid())
      {
        xiiDynamicArray<xiiUInt32> edgePixels;
        if (ReadIds(*environment.GetDevice(), graph.GetExportedTexture(wireframe).Borrow(), edgePixels))
        {
          xiiUInt32 solidCount = 0, wireCount = 0;
          for (xiiUInt32 pixel = 0; pixel < 32 * 32; ++pixel)
          {
            solidCount += pixels[pixel * 4] != 0;
            wireCount += edgePixels[pixel * 4] != 0;
            if (edgePixels[pixel * 4] != 0)
              XII_TEST_BOOL(edgePixels[pixel * 4] == firstId || edgePixels[pixel * 4] == secondId);
          }
          XII_TEST_BOOL(wireCount > 0 && wireCount < solidCount);
          XII_TEST_INT(edgePixels[0], 0);
        }
      }
      cache.EndFrame();
      frame.End();
      if (skinned)
      {
        // Keep the world transform fixed: only vertex deformation moves in and out of
        // the query. Both native raster routes must follow updates and clearing.
        for (xiiUInt64 poseFrame = 2; poseFrame <= 6; ++poseFrame)
        {
          xiiRenderGraphFrame poseScope;
          if (!XII_TEST_RESULT(poseScope.Begin()))
            return;
          XII_TEST_INT(poseScope.GetFrameIndex(), poseFrame);
          const auto poseRevision = context.GetDataRevision();
          if (poseFrame == 4)
            first.m_SkinningMatrices.Clear();
          else
          {
            SetTestSkinningPose(first, poseFrame == 2 ? 0.0f : 2.0f);
            if (poseFrame >= 5)
              for (auto& matrix : first.m_SkinningMatrices)
              {
                matrix.Element(1, 0) = 0.25f;
                matrix.Element(1, 1) = 2.0f;
              }
          }
          const xiiUInt32 expectedId = poseFrame == 2 || poseFrame == 4 ? 0 : firstId;
          XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, poseFrame));
          XII_TEST_BOOL(context.GetDataRevision() != poseRevision);
          xiiMaterialManager::BeginFrame(poseFrame, poseFrame - 1);
          xiiGeometryResidencyManager::ProcessStreaming(poseFrame, poseFrame - 1, xiiMath::MaxValue<xiiUInt64>());
          cache.BeginFrame(poseFrame, poseFrame - 1);
          graph.BeginSetup(poseFrame);
          const auto              poseGeometry  = xiiGeometryResidencyManager::AddUploadPass(graph, poseFrame);
          const auto              poseMaterials = xiiMaterialManager::AddUploadPasses(graph);
          xiiGpuVisibilityOutputs poseVisibility;
          if (!XII_TEST_RESULT(visibilityPipeline.Build(view, graph, graphContext.GetBlackboard(), viewProjection, camera.GetCenterPosition(), true, poseVisibility)))
            return;
          xiiRenderGraphTextureHandle poseDepth;
          if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Posed depth", poseVisibility, poseGeometry, poseMaterials, raster, poseDepth, meshPicking, xiiGALFillMode::Solid, nullptr, nullptr, true)))
            return;
          xiiTestMaterialTargets poseSurface;
          if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Posed surface", poseVisibility, poseGeometry, poseMaterials, raster, unusedMaterialOutput, meshPicking, xiiGALFillMode::Solid, nullptr, nullptr, false, &poseSurface, poseDepth)))
            return;
          if (!XII_TEST_BOOL(poseSurface.IsValid()))
            return;
          xiiRenderGraphTextureHandle poseQuery;
          XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Posed query", poseVisibility, poseGeometry, poseMaterials, raster, poseQuery, meshPicking));
          graph.AddPass<Export>("Posed query export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
            XII_IGNORE_UNUSED(builder.ReadTexture(poseQuery, xiiGALResourceStateFlags::ShaderResource));
            builder.ExportTexture(poseQuery, xiiGALResourceStateFlags::ShaderResource);
            XII_IGNORE_UNUSED(builder.ReadTexture(poseSurface.m_hVelocity, xiiGALResourceStateFlags::ShaderResource));
            builder.ExportTexture(poseSurface.m_hVelocity, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
          graph.EndSetup();
          if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphContext.GetBlackboard(), &cache)))
            return;
          auto poseTexture = graph.GetExportedTexture(poseQuery);
          if (ReadIds(*environment.GetDevice(), poseTexture.Borrow(), pixels))
          {
            XII_TEST_INT(pixels[(16 * 32 + 16) * 4], expectedId);
            if (poseFrame >= 5)
            {
              // Inverse transpose of the sheared, nonuniform bone transform: (-1, 1/8, 0).
              XII_TEST_FLOAT(std::bit_cast<float>(pixels[(16 * 32 + 16) * 4 + 2]), 1.0f / 18.0f, 0.0001f);
              XII_TEST_FLOAT(std::bit_cast<float>(pixels[(16 * 32 + 16) * 4 + 3]), 5.0f / 9.0f, 0.0001f);
            }
          }
          auto velocityTexture = graph.GetExportedTexture(poseSurface.m_hVelocity);
          if (ReadIds(*environment.GetDevice(), velocityTexture.Borrow(), pixels) && (poseFrame == 3 || poseFrame == 6))
          {
            xiiFloat16 velocityX, velocityY;
            velocityX.SetRawData(static_cast<xiiUInt16>(pixels[16 * 32 + 16]));
            velocityY.SetRawData(static_cast<xiiUInt16>(pixels[16 * 32 + 16] >> 16));
            const auto currentClip  = viewProjection * xiiVec4(-0.5f, 2, 0, 1);
            const auto previousClip = viewProjection * xiiVec4(-0.5f, poseFrame == 3 ? 0.0f : 2.0f, 0, 1);
            XII_TEST_FLOAT(static_cast<float>(velocityX), currentClip.x / currentClip.w - previousClip.x / previousClip.w, 0.003f);
            XII_TEST_FLOAT(static_cast<float>(velocityY), 0.0f, 0.003f);
          }
          cache.EndFrame();
        }
      }
    }
    else
      xiiTestFramework::Output(xiiTestOutput::Message, "Mesh-shader raster identity was not exercised on this device.");
  }

  if (skinned)
    return;

  xiiUInt64 nextFrame = skinned ? 7 : 2;
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native surface queries respect authored opacity and transparent visibility")
  {
    if (environment.GetDevice()->GetFeatures().m_MeshShaders == xiiGALDeviceFeatureState::Enabled)
    {
      xiiRenderGraphContext graphContext;
      if (!XII_TEST_RESULT(graphContext.Initialize("Material coverage queries")))
        return;
      xiiTestNativeVisibility visibilityPipeline;
      if (!XII_TEST_RESULT(visibilityPipeline.Initialize()))
        return;
      xiiCamera camera;
      camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
      camera.LookAt(xiiVec3(-2, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
      const auto           projection     = xiiGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(xiiAngle::MakeFromDegree(60), 1, 100, 0.1f, xiiClipSpaceDepthRange::ZeroToOne, xiiClipSpaceYMode::Regular, xiiHandedness::LeftHanded);
      const auto           viewProjection = projection * camera.GetViewMatrix();
      xiiTestSurfaceLayout raster;
      raster.m_ViewProjectionMatrix = viewProjection;
      raster.m_uiWidth = raster.m_uiHeight = 32;
      raster.m_uiVertexStride              = sizeof(xiiMeshPackedVertex);
      raster.m_uiNormalOffset              = offsetof(xiiMeshPackedVertex, m_vNormal);
      raster.m_uiTangentOffset             = offsetof(xiiMeshPackedVertex, m_vTangent);
      raster.m_uiTexCoordOffset            = offsetof(xiiMeshPackedVertex, m_vTexCoord0);
      raster.m_uiMeshDispatchGroupCountX   = visibilityPipeline.m_Commands.m_uiMeshGroupCountX;
      raster.m_uiMeshDispatchGroupCountY   = visibilityPipeline.m_Commands.m_uiMeshGroupCountY;

      auto*      coverageRenderer = world.GetOrCreateModule<xiiRenderWorldModule>();
      xiiView*   view             = nullptr;
      const auto coverageView     = coverageRenderer->CreateView("Coverage surface fixture", view);
      XII_SCOPE_EXIT(coverageRenderer->DestroyView(coverageView));
      view->SetCamera(&camera);
      view->SetViewport({0, 0, 32, 32});
      const auto renderCoverage = [&](xiiMaterialAlphaMode::Enum alphaMode, float opacity, bool pickTransparent, xiiUInt32 expectedId) {
        xiiRenderGraphFrame gpuFrame;
        if (!XII_TEST_RESULT(gpuFrame.Begin()))
          return;
        const auto frame                        = gpuFrame.GetFrameIndex();
        nextFrame                               = frame + 1;
        const auto                    completed = gpuFrame.GetCompletedFrame();
        xiiMaterialResourceDescriptor materialDescription;
        materialDescription.m_AlphaMode   = alphaMode;
        materialDescription.m_BlendMode   = alphaMode == xiiMaterialAlphaMode::Blend ? xiiMaterialBlendMode::Translucent :
            alphaMode == xiiMaterialAlphaMode::Mask                                  ? xiiMaterialBlendMode::Masked :
                                                                                       xiiMaterialBlendMode::Opaque;
        materialDescription.m_BaseColor.a = opacity;
        xiiStringBuilder materialName;
        materialName.SetFormat("Query coverage material {}", frame);
        const auto                           material = xiiResourceManager::CreateResource<xiiMaterialResource>(materialName, std::move(materialDescription));
        xiiResourceLock<xiiMaterialResource> loadedMaterial(material, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (!XII_TEST_BOOL(loadedMaterial.GetAcquireResult() == xiiResourceAcquireResult::Final))
          return;
        first.m_hMaterials.SetCount(1);
        first.m_hMaterials[0] = material;
        if (!XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, frame)))
          return;
        xiiMaterialManager::PrepareGpuResources();
        xiiGeometryResidencyManager::ProcessStreaming(frame, completed, xiiMath::MaxValue<xiiUInt64>());
        auto& graph = graphContext.GetGraph();
        auto& cache = graphContext.GetResourceCache();
        cache.BeginFrame(frame, completed);
        XII_SCOPE_EXIT(cache.EndFrame());
        graph.BeginSetup(frame);
        const auto uploadedGeometry = xiiGeometryResidencyManager::AddUploadPass(graph, frame);
        const auto materials        = xiiMaterialManager::AddUploadPasses(graph);
        view->SetRenderEnabled(false);
        xiiGpuVisibilityOutputs visibility;
        if (!XII_TEST_RESULT(visibilityPipeline.Build(view, graph, graphContext.GetBlackboard(), viewProjection, camera.GetCenterPosition(), true, visibility,
                                                      pickTransparent ? 0U : xiiUInt32(xiiGpuInstanceFlags::Transparent))))
          return;
        xiiRenderGraphTextureHandle query;
        auto                        invalidRaster = raster;
        invalidRaster.m_uiWidth                   = 0;
        XII_TEST_BOOL(xiiTestNativeSurfaceQuery(graph, view, "Invalid coverage query", visibility, uploadedGeometry, materials, invalidRaster, query).Failed());
        XII_TEST_BOOL(!query.IsValid());
        if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Coverage query", visibility, uploadedGeometry, materials, raster, query)))
          return;
        XII_TEST_BOOL(xiiTestNativeSurfaceQuery(graph, view, "Missing visibility query", {}, uploadedGeometry, materials, raster, query).Failed());
        XII_TEST_BOOL(!query.IsValid());
        if (!XII_TEST_RESULT(xiiTestNativeSurfaceQuery(graph, view, "Coverage final query", visibility, uploadedGeometry, materials, raster, query)))
          return;
        struct Export
        {
          XII_DECLARE_POD_TYPE();
        };
        graph.AddPass<Export>("Coverage export", xiiGALCommandQueueFlags::Graphics, [&](Export&, xiiRenderGraphBuilder& builder) {
          XII_IGNORE_UNUSED(builder.ReadTexture(query, xiiGALResourceStateFlags::ShaderResource));
          builder.ExportTexture(query, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {});
        graph.EndSetup();
        if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &graphContext.GetBlackboard(), &cache)))
          return;
        gpuFrame.End();
        auto                       texture = graph.GetExportedTexture(query);
        xiiDynamicArray<xiiUInt32> pixels;
        if (ReadIds(*environment.GetDevice(), texture.Borrow(), pixels))
        {
          const xiiUInt32 center = (16 * 32 + 16) * 4;
          XII_TEST_INT(pixels[center], expectedId);
          XII_TEST_INT(pixels[0], 0);
          const auto expectedClip = viewProjection * xiiVec4(expectedId == firstId ? -0.5f : 2.5f, 0, 0, 1);
          XII_TEST_FLOAT(std::bit_cast<float>(pixels[center + 1]), expectedClip.z / expectedClip.w, 0.0001f);
          XII_TEST_RESULT(context.Resolve(pixels[center], source));
          XII_TEST_BOOL(source.m_hOwnerComponent == (expectedId == firstId ? firstComponent : secondComponent));
        }
      };
      renderCoverage(xiiMaterialAlphaMode::Opaque, 0.0f, false, firstId);
      renderCoverage(xiiMaterialAlphaMode::Mask, 0.25f, false, secondId);
      renderCoverage(xiiMaterialAlphaMode::Mask, 0.75f, false, firstId);
      renderCoverage(xiiMaterialAlphaMode::Blend, 0.0f, true, secondId);
      renderCoverage(xiiMaterialAlphaMode::Blend, 0.25f, true, firstId);
      renderCoverage(xiiMaterialAlphaMode::Blend, 0.25f, false, secondId);
      first.m_hMaterials.Clear();
    }
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
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, nextFrame++));
    XII_TEST_INT(scene.GetInstanceCount(), 3);
    xiiUInt32 sectionId = 0;
    for (const auto& instance : scene.GetInstances())
    {
      XII_TEST_RESULT(context.Resolve(instance.m_uiObjectId, source));
      if (source.m_uiSectionIndex == 1)
      {
        XII_TEST_BOOL(source.m_hOwnerComponent == firstComponent);
        sectionId = instance.m_uiObjectId;
      }
    }
    XII_TEST_BOOL(sectionId != 0 && sectionId != firstId && sectionId != secondId);
    XII_TEST_RESULT(context.Resolve(firstId, source));
    XII_TEST_INT(source.m_uiSectionIndex, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Retirement, clearing and destroyed contexts reject stale mesh identities")
  {
    const auto     frame = nextFrame++;
    xiiRenderData  nonMesh;
    xiiRenderData* filtered[] = {nullptr, &second, &nonMesh};
    XII_TEST_RESULT(renderer->UpdateGpuInstances(xiiMakeArrayPtr(filtered), frame));
    XII_TEST_INT(scene.GetInstanceCount(), 1);
    XII_TEST_BOOL(context.Resolve(firstId, source).Failed());
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, frame));
    XII_TEST_INT(scene.GetInstanceCount(), 3);
    // Repeated membership changes in a single GPU frame must not keep absent packets alive.
    XII_TEST_RESULT(renderer->UpdateGpuInstances(xiiMakeArrayPtr(filtered), frame));
    XII_TEST_INT(scene.GetInstanceCount(), 1);
    XII_TEST_RESULT(context.Resolve(secondId, source));
    extracted.Clear();
    extracted.AddRenderData(&second);
    extracted.SortAndBatches();
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, nextFrame++));
    XII_TEST_BOOL(context.Resolve(firstId, source).Failed());
    XII_TEST_BOOL(source.m_hOwnerComponent.IsInvalidated());
    XII_TEST_RESULT(context.Resolve(secondId, source));
    XII_TEST_RESULT(renderer->UpdateGpuInstances(xiiArrayPtr<xiiRenderData* const>{}, nextFrame++));
    XII_TEST_BOOL(context.Resolve(secondId, source).Failed());
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, nextFrame++));
    XII_TEST_INT(scene.GetInstanceCount(), 1);
    XII_TEST_BOOL(context.Resolve(secondId, source).Failed());
    const auto replacementId = scene.GetInstances()[0].m_uiObjectId;
    XII_TEST_RESULT(context.Resolve(replacementId, source));
    XII_TEST_BOOL(source.m_hOwnerComponent == secondComponent);
    world.DeleteObjectNow(owner);
    XII_TEST_BOOL(context.Resolve(replacementId, source).Failed());
    XII_TEST_RESULT(renderer->UpdateGpuInstances(extracted, nextFrame++));
    XII_TEST_INT(context.GetInstanceCount(), 0);
    XII_TEST_BOOL(source.m_hOwnerObject.IsInvalidated());
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(NativeSceneIdentity);
XII_CREATE_SIMPLE_TEST(NativeSceneIdentity, SceneMeshIdentity)
{
  RunSceneMeshIdentity(false);
}

XII_CREATE_SIMPLE_TEST_GROUP(MeshletRaster);
XII_CREATE_SIMPLE_TEST(MeshletRaster, DensePrimitiveCoverage)
{
  RunSceneMeshIdentity(true);
}

XII_CREATE_SIMPLE_TEST_GROUP(SkinningRaster);
XII_CREATE_SIMPLE_TEST(SkinningRaster, MeshSurfaceAndQuery)
{
  RunSceneMeshIdentity(false, true, true);
}
XII_CREATE_SIMPLE_TEST(SkinningRaster, IndexedQuery)
{
  RunSceneMeshIdentity(false, true, false);
}
