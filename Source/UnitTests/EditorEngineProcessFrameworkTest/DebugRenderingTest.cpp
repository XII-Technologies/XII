/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorEngineProcessFramework/Rendering/EditorGrid.h>
#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/EngineProcess/ThumbnailCapture.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <Texture/Image/Image.h>

XII_CREATE_SIMPLE_TEST(EditorViews, DebugRendering)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiWorldDescription firstDescription("Debug renderer first world");
  xiiWorldDescription secondDescription("Debug renderer second world");
  xiiWorld            world(firstDescription);
  xiiWorld            otherWorld(secondDescription);
  XII_LOCK(world.GetWriteMarker());
  XII_LOCK(otherWorld.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  otherWorld.SetWorldSimulationEnabled(false);

  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4.0f, 0.1f, 100.0f);
  camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3(0, 1, 0));

  xiiGALTextureCreationDescription colorDescription;
  colorDescription.m_Type      = xiiGALResourceDimension::Texture2D;
  colorDescription.m_Size      = {64, 64};
  colorDescription.m_Format    = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  colorDescription.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto color                   = environment.GetDevice()->CreateTexture(colorDescription);
  auto capturedColor           = environment.GetDevice()->CreateTexture(colorDescription);
  auto depthDescription        = colorDescription;
  depthDescription.m_Format    = xiiGALResourceFormat::D32Float;
  depthDescription.m_BindFlags = xiiGALBindFlags::DepthStencil;
  auto depth                   = environment.GetDevice()->CreateTexture(depthDescription);

  float clearDepth = 1.0f;

  struct ClearData
  {
    xiiRenderGraphTextureHandle m_Color;
    xiiRenderGraphTextureHandle m_Depth;
  };
  auto configure = [&](xiiWorld& owner) -> xiiView* {
    xiiView* view = nullptr;
    owner.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Debug rendering test", view);
    view->SetCamera(&camera);
    view->SetViewport({0, 0, 64, 64});
    view->SetRenderTargetView(color->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetRenderGraphBuilder([&](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
      graph.AddPass<ClearData>("Debug test clear", xiiGALCommandQueueFlags::Graphics, [&](ClearData& data, xiiRenderGraphBuilder& builder) {
          data.m_Color = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, color, color->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
          data.m_Depth = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, depth, depth->GetResourceState()), xiiGALResourceStateFlags::DepthWrite); }, [&](const ClearData& data, xiiRenderGraphPassContext& context) {
          auto& commandList = context.GetCommandList();
          commandList.ClearRenderTargetView(context.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
          commandList.ClearDepthStencilView(context.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, clearDepth, 0); }, true);
      xiiDebugRenderer::AddRenderGraphPasses(graph);
      graph.AddPass<ClearData>("Debug test capture", xiiGALCommandQueueFlags::Graphics, [&](ClearData& data, xiiRenderGraphBuilder& builder) {
          data.m_Color = builder.ReadTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, xiiGALResourceStateFlags::CopySource);
          data.m_Depth = builder.WriteTexture(builder.ImportTexture("Captured debug output", capturedColor, capturedColor->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
          builder.ExportTexture(data.m_Depth, xiiGALResourceStateFlags::CopySource); }, [](const ClearData& data, xiiRenderGraphPassContext& context) { context.GetCommandList().CopyTexture(context.GetTexture(data.m_Color), context.GetTexture(data.m_Depth)); });
    });
    return view;
  };
  auto* first  = configure(world);
  auto* second = configure(otherWorld);
  XII_TEST_BOOL(first->GetHandle() == second->GetHandle()); // Handles are local to their owning world.
  XII_TEST_BOOL(!(xiiDebugRendererContext(*first) == xiiDebugRendererContext(*second)));
  XII_TEST_BOOL(!(xiiDebugRendererContext(&world) == xiiDebugRendererContext(*first)));
  XII_TEST_BOOL(first->GetExtractedRenderData()->GetViewDebugContext() == xiiDebugRendererContext(*first));
  XII_TEST_BOOL(first->GetExtractedRenderData()->GetWorldDebugContext() == xiiDebugRendererContext(&world));

  auto checkCenter = [&](xiiUInt8 red, xiiUInt8 green, xiiUInt8 blue) {
    xiiImage image;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, image)))
    {
      const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, 32);
      XII_TEST_INT(pixel[0], red);
      XII_TEST_INT(pixel[1], green);
      XII_TEST_INT(pixel[2], blue);
      XII_TEST_INT(pixel[3], 255);
    }
  };
  xiiDebugRendererTriangle triangle(xiiVec3(-1, -1, 0), xiiVec3(1, -1, 0), xiiVec3(0, 1, 0));
  xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red);
  otherWorld.Update();
  checkCenter(0, 0, 0);
  world.Update();
  checkCenter(255, 0, 0);
  world.Update();
  checkCenter(0, 0, 0);

  xiiDebugRenderer::DrawSolidTriangles(&world, xiiMakeArrayPtr(&triangle, 1), xiiColor::Blue);
  world.Update();
  checkCenter(0, 0, 255);
  world.Update();
  checkCenter(0, 0, 0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlay geometry bypasses occlusion and preserves scene depth")
  {
    clearDepth = 0.0f;
    xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red);
    world.Update();
    checkCenter(0, 0, 0);
    xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red, xiiDebugRendererDepthMode::Overlay);
    world.Update();
    checkCenter(255, 0, 0);

    xiiDebugRendererLine line(xiiVec3(-1, 0, 0), xiiVec3(1, 0, 0));
    xiiDebugRenderer::DrawLines(*first, xiiMakeArrayPtr(&line, 1), xiiColor::Blue, xiiMat4::MakeIdentity(), xiiDebugRendererDepthMode::Overlay);
    world.Update();
    xiiImage lineImage;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, lineImage)))
    {
      bool foundLine = false;
      for (xiiUInt32 y = 30; y < 34; ++y)
        foundLine |= lineImage.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, y)[2] == 255;
      XII_TEST_BOOL(foundLine);
    }

    xiiGeometry geometry;
    geometry.AddBox(xiiVec3(1), false);
    geometry.TriangulatePolygons();
    XII_TEST_RESULT(xiiDebugRenderer::DrawGeometry(*first, geometry, xiiColor::Blue, xiiMat4::MakeTranslation(xiiVec3(0.2f, 0, 0)), xiiDebugRendererDepthMode::Overlay));
    world.Update();
    checkCenter(0, 0, 255);

    xiiGeometry mirroredTriangle;
    mirroredTriangle.AddVertex(xiiVec3(-1, -1, 0), xiiVec3::MakeAxisZ());
    mirroredTriangle.AddVertex(xiiVec3(1, -1, 0), xiiVec3::MakeAxisZ());
    mirroredTriangle.AddVertex(xiiVec3(0, 1, 0), xiiVec3::MakeAxisZ());
    xiiUInt32 triangleIndices[] = {0, 1, 2};
    mirroredTriangle.AddPolygon(xiiMakeArrayPtr(triangleIndices), false);
    XII_TEST_RESULT(xiiDebugRenderer::DrawGeometry(*first, mirroredTriangle, xiiColor(0, 1, 0), xiiMat4::MakeScaling(xiiVec3(-1, 1, 1)), xiiDebugRendererDepthMode::Overlay));
    world.Update();
    checkCenter(0, 255, 0);

    // Read depth as floats, with the staging row pitch supplied by the backend.
    auto                  commandList = environment.GetDevice()->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    auto                  queue       = environment.GetDevice()->GetCommandQueue();
    xiiGALTextureReadback readback(environment.GetDevice());
    commandList->Begin();
    readback.Enqueue(commandList, {.m_pTexture = depth});
    commandList->End();
    queue->WaitForFenceValue(queue->Submit(commandList));
    readback.WaitForNextCompleted();
    const auto capture = readback.GetCompleted();
    if (XII_TEST_BOOL(static_cast<bool>(capture)))
    {
      commandList->Begin();
      xiiGALMappedTextureSubresource mapped;
      if (XII_TEST_RESULT(commandList->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        for (xiiUInt32 y = 0; y < 64; ++y)
        {
          const float* row = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride);
          for (xiiUInt32 x = 0; x < 64; ++x)
            XII_TEST_FLOAT(row[x], 0.0f, 0.0f);
        }
        XII_TEST_RESULT(commandList->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
      }
      commandList->End();
      queue->WaitForFenceValue(queue->Submit(commandList));
      readback.RecycleStagingTexture(capture.m_pStagingTexture);
    }

    // Failure is atomic even when malformed lines follow otherwise-valid triangles.
    geometry.GetLines().PushBack({0, geometry.GetVertices().GetCount()});
    XII_TEST_BOOL(xiiDebugRenderer::DrawGeometry(*first, geometry, xiiColor::Red, xiiMat4::MakeIdentity(), xiiDebugRendererDepthMode::Overlay).Failed());
    world.Update();
    checkCenter(0, 0, 0);
    geometry.GetLines().Clear();
    geometry.GetPolygons()[0].m_Vertices[0] = geometry.GetVertices().GetCount();
    XII_TEST_BOOL(xiiDebugRenderer::DrawGeometry(*first, geometry, xiiColor::Red).Failed());
    geometry.Clear();
    geometry.AddBox(xiiVec3(1), false);
    XII_TEST_BOOL(xiiDebugRenderer::DrawGeometry(*first, geometry, xiiColor::Red).Failed()); // Triangulation is explicit.
    geometry.TriangulatePolygons();
    xiiMat4 projection = xiiMat4::MakeIdentity();
    projection.SetRow(3, xiiVec4(0, 0, 1, 1));
    XII_TEST_BOOL(xiiDebugRenderer::DrawGeometry(*first, geometry, xiiColor::Red, projection).Failed());
    auto invalidColor = xiiColor::White;
    invalidColor.r    = xiiMath::Infinity<float>();
    XII_TEST_BOOL(xiiDebugRenderer::DrawGeometry(*first, geometry, invalidColor).Failed());
    clearDepth = 1.0f;
    xiiDebugRenderer::DrawSolidTriangles(&world, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red, xiiDebugRendererDepthMode::Overlay);
    xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Blue);
    world.Update();
    checkCenter(255, 0, 0); // World overlays follow the view's ordinary primitives too.
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Synchronized handles use debug batches, overlay depth, and wire visualizers")
  {
    xiiEngineGizmoHandle handle;
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop);
    handle.SetVisible(true);
    XII_TEST_BOOL(handle.SetupForEngine(&world, 41));
    XII_TEST_BOOL(!handle.SetupForEngine(&world, 42));
    clearDepth = 0;
    XII_TEST_RESULT(handle.Draw(*first, 0));
    world.Update();
    checkCenter(255, 0, 0);
    xiiImage solid;
    XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, solid));
    xiiUInt32 solidPixels = 0;
    for (xiiUInt64 i = 0; i < solid.GetByteBlobPtr().GetCount(); i += 4)
      solidPixels += solid.GetByteBlobPtr()[i] > 0 ? 1 : 0;
    XII_TEST_BOOL(solidPixels > 100);
    XII_TEST_RESULT(handle.Draw(*first, 41));
    world.Update();
    xiiImage highlighted;
    XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, highlighted));
    const auto* pixel = highlighted.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, 32);
    XII_TEST_BOOL(pixel[0] > 200 && pixel[1] == pixel[0] && pixel[2] < 150);
    handle.SetVisible(false);
    XII_TEST_RESULT(handle.Draw(*first, 0));
    world.Update();
    checkCenter(0, 0, 0);
    handle.SetVisible(true);
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Red, xiiGizmoFlags::ShowInOrtho);
    XII_TEST_RESULT(handle.Draw(*first, 0));
    world.Update();
    checkCenter(0, 0, 0); // Ordinary handles retain scene depth testing.
    clearDepth = 1;
    handle.ConfigureHandle(nullptr, xiiEngineGizmoHandleType::Box, xiiColor::Blue, xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Visualizer | xiiGizmoFlags::Pickable);
    XII_TEST_RESULT(handle.Draw(*first, 0));
    world.Update();
    xiiImage wire;
    XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, wire));
    xiiUInt32 wirePixels = 0;
    for (xiiUInt64 i = 0; i < wire.GetByteBlobPtr().GetCount(); i += 4)
      wirePixels += wire.GetByteBlobPtr()[i + 2] > 0 ? 1 : 0;
    XII_TEST_BOOL(wirePixels > 0 && wirePixels < solidPixels);
    float fTime = -1;
    XII_TEST_BOOL(handle.PickRay(*first, xiiVec3(0, 0, 5), -xiiVec3::MakeAxisZ(), fTime).Failed());
    XII_TEST_FLOAT(fTime, -1, 0); // Visualizers never steal manipulator interaction.
    XII_TEST_INT(world.GetObjectCount(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Editor grids share debug passes and remain bounded across zoom levels")
  {
    xiiEditorGrid grid;
    XII_TEST_RESULT(grid.Build(*first, 1.0f));
    XII_TEST_INT(grid.GetLines().GetCount(), 10);
    const float expectedDepth = camera.GetNearPlane() * 0.05f + camera.GetFarPlane() * 0.95f;
    for (const auto& line : grid.GetLines())
    {
      XII_TEST_FLOAT((line.m_vStart - camera.GetCenterPosition()).Dot(camera.GetCenterDirForwards()), expectedDepth, 0.001f);
      XII_TEST_FLOAT((line.m_vEnd - camera.GetCenterPosition()).Dot(camera.GetCenterDirForwards()), expectedDepth, 0.001f);
    }
    xiiDebugRenderer::DrawLines(*first, grid.GetLines(), xiiColor::White);
    world.Update();
    xiiImage gridImage;
    XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, gridImage));
    bool hasGrid = false;
    for (xiiUInt64 i = 0; i < gridImage.GetByteBlobPtr().GetCount(); i += 4)
      hasGrid |= gridImage.GetByteBlobPtr()[i] > 0 || gridImage.GetByteBlobPtr()[i + 2] > 0;
    XII_TEST_BOOL(hasGrid);
    clearDepth = 0;
    xiiDebugRenderer::DrawLines(*first, grid.GetLines(), xiiColor::White);
    world.Update();
    XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, gridImage));
    bool hasOccludedGrid = false;
    for (xiiUInt64 i = 0; i < gridImage.GetByteBlobPtr().GetCount(); i += 4)
      hasOccludedGrid |= gridImage.GetByteBlobPtr()[i] > 0 || gridImage.GetByteBlobPtr()[i + 2] > 0;
    XII_TEST_BOOL(!hasOccludedGrid);
    clearDepth = 1;

    first->SetViewport({0, 0, 100000, 100000});
    XII_TEST_RESULT(grid.Build(*first, 1.0e-35f));
    XII_TEST_BOOL(grid.GetLines().GetCount() > 0 && grid.GetLines().GetCount() <= 4102);
    first->SetViewport({0, 0, 64, 64});
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 1.0e20f, -100, 100);
    XII_TEST_RESULT(grid.Build(*first, 0.001f));
    XII_TEST_BOOL(grid.GetLines().GetCount() <= 4102);
    for (const auto& line : grid.GetLines())
      XII_TEST_BOOL(line.m_vStart.IsValid() && line.m_vEnd.IsValid());

    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    camera.LookAt(xiiVec3(5, 0, 0), xiiVec3::MakeZero(), xiiVec3::MakeAxisZ());
    auto placement = xiiTransform::MakeIdentity();
    placement.m_vPosition.Set(0, 0.25f, 0.5f);
    XII_TEST_RESULT(grid.Build(*first, 1, placement, false));
    for (const auto& line : grid.GetLines())
    {
      const auto relative = line.m_vStart - placement.m_vPosition;
      XII_TEST_FLOAT(relative.Dot(camera.GetCenterDirRight()), xiiMath::Round(relative.Dot(camera.GetCenterDirRight())), 0.001f);
      XII_TEST_FLOAT(relative.Dot(camera.GetCenterDirUp()), xiiMath::Round(relative.Dot(camera.GetCenterDirUp())), 0.001f);
    }
    camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    placement.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3::MakeAxisY(), xiiAngle::MakeFromDegree(90));
    XII_TEST_RESULT(grid.Build(*first, 0.25f, placement, false));
    XII_TEST_INT(grid.GetLines().GetCount(), 202);
    for (const auto& line : grid.GetLines())
    {
      XII_TEST_FLOAT(line.m_vStart.x, placement.m_vPosition.x, 0.001f);
      XII_TEST_FLOAT(line.m_vEnd.x, placement.m_vPosition.x, 0.001f);
    }
    XII_TEST_BOOL(grid.Build(*first, -1).Failed());
    XII_TEST_BOOL(grid.GetLines().IsEmpty());
    XII_TEST_BOOL(grid.Build(*first, xiiMath::Infinity<float>()).Failed());
    XII_TEST_BOOL(grid.Build(*first, xiiMath::MaxValue<float>()).Failed());
    XII_TEST_RESULT(grid.Build(*first, 0));
    XII_TEST_BOOL(grid.GetLines().IsEmpty());
    placement.m_vScale.SetZero();
    XII_TEST_RESULT(grid.Build(*first, 1, placement));
    XII_TEST_BOOL(grid.GetLines().IsEmpty());
    camera.SetCameraMode(xiiCameraMode::OrthoFixedHeight, 4, 0.1f, 100);
    camera.LookAt(xiiVec3(0, 0, 5), xiiVec3::MakeZero(), xiiVec3::MakeAxisY());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Indexed boxes and textured primitives use their actual shaders")
  {
    xiiDebugRenderer::DrawSolidBox(*first, xiiBoundingBox::MakeFromMinMax(xiiVec3(-1), xiiVec3(1)), xiiColor::Yellow);
    world.Update();
    checkCenter(255, 255, 0);

    auto sampledDescription                     = colorDescription;
    sampledDescription.m_Size                   = {1, 1};
    const xiiUInt8                     pixels[] = {0, 0, 255, 255};
    const xiiGALTextureSubResourceData subresource(xiiConstByteBlobPtr(pixels, 4), 4, 4);
    const xiiGALTextureData            initialData(xiiMakeArrayPtr(&subresource, 1));
    auto                               sampled = environment.GetDevice()->CreateTexture(sampledDescription, &initialData);
    xiiDebugRenderer::Draw2DRectangle(*first, xiiRectFloat(0, 0, 64, 64), 0, xiiColor::White, sampled->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    world.Update();
    checkCenter(0, 0, 255);
  }

  // Both views write the same capture target, so the second view must see the
  // world draw too. Consuming the context after the first view leaves it black.
  auto* third = configure(world);
  XII_TEST_BOOL(!(xiiDebugRendererContext(*first) == xiiDebugRendererContext(*third)));
  xiiDebugRenderer::DrawSolidTriangles(&world, xiiMakeArrayPtr(&triangle, 1), xiiColor::Green);
  world.Update();
  checkCenter(0, 128, 0);
  world.Update();
  checkCenter(0, 0, 0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "World text uses each viewport and persistent text is prepared once")
  {
    first->SetRenderEnabled(false);
    third->SetViewport({0, 0, 32, 64});
    const xiiColor translucentWhite(1, 1, 1, 0.5f);
    xiiDebugRenderer::DrawInfoText(*third, xiiDebugTextPlacement::BottomRight, "Test", "X", translucentWhite);
    world.Update();
    xiiImage reference;
    if (!XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, reference)))
      return;
    bool hasText = false;
    for (xiiUInt64 i = 0; i < reference.GetByteBlobPtr().GetCount(); i += 4)
      hasText |= reference.GetByteBlobPtr()[i] > 0;
    XII_TEST_BOOL(hasText);

    first->SetRenderEnabled(true);
    xiiDebugRenderer::AddPersistentInfoText(&world, xiiDebugTextPlacement::BottomRight, "X", xiiTime::MakeFromSeconds(60), translucentWhite);
    for (xiiUInt32 frame = 0; frame < 3; ++frame)
    {
      world.Update();
      xiiImage actual;
      if (XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, actual)))
      {
        const xiiImageView& actualView    = actual;
        const xiiImageView& referenceView = reference;
        XII_TEST_BOOL(actualView.GetByteBlobPtr() == referenceView.GetByteBlobPtr());
      }
    }
    xiiDebugRenderer::ClearContext(&world);
    world.Update();
    checkCenter(0, 0, 0);
  }
}
