/// Copyright (c) Theophilus Eriata. All Rights Reserved.



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
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreview.h>
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

  float clearDepth = 0.0f; // Reversed Z: zero is the far plane; one occludes ordinary geometry.

  struct ClearData
  {
    xiiRenderGraphTextureHandle m_Color;
    xiiRenderGraphTextureHandle m_Depth;
  };
  auto configure = [&](xiiWorld& owner) -> xiiView* {
    xiiView* view = nullptr;
    owner.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Debug rendering test", view);
    view->SetCamera(&camera);
    const auto& size = color->GetDescription().m_Size;
    view->SetViewport({0, 0, static_cast<float>(size.width), static_cast<float>(size.height)});
    view->SetRenderTargetView(color->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    view->SetRenderGraphBuilder([&](xiiView&, xiiRenderGraph& graph, xiiRenderGraphBlackboard&) {
      graph.AddPass<ClearData>("Debug test clear", xiiGALCommandQueueFlags::Graphics, [&](ClearData& data, xiiRenderGraphBuilder& builder) {
          data.m_Color = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DisplayLinearColor, color, color->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
          data.m_Depth = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, depth, depth->GetResourceState()), xiiGALResourceStateFlags::DepthWrite); }, [&](const ClearData& data, xiiRenderGraphPassContext& context) {
          auto& commandList = context.GetCommandList();
          commandList.ClearRenderTargetView(context.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
          commandList.ClearDepthStencilView(context.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, clearDepth, 0);
          // Out-of-render-pass clears may use transfers. Honor the states declared to the graph.
          xiiGALStateTransitionDescription transitions[2];
          transitions[0].m_pResource = context.GetTexture(data.m_Color);
          transitions[0].m_NewState = xiiGALResourceStateFlags::RenderTarget;
          transitions[0].m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
          transitions[1].m_pResource = context.GetTexture(data.m_Depth);
          transitions[1].m_NewState = xiiGALResourceStateFlags::DepthWrite;
          transitions[1].m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
          commandList.TransitionResourceStates(xiiMakeArrayPtr(transitions)); }, true);
      auto debugColor = graph.FindTexture(xiiRGBlackboardKeys::k_DisplayLinearColor);
      auto debugDepth = graph.FindTexture(xiiRGBlackboardKeys::k_SceneDepthTexture);
      XII_TEST_RESULT(xiiDebugRenderer::AddRenderGraphPasses(graph, debugColor, debugDepth));
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
    xiiImage    image;
    const auto& size = capturedColor->GetDescription().m_Size;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, size, image)))
    {
      const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, size.width / 2, size.height / 2);
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reduced-resolution depth preserves debug occlusion on an odd display target")
  {
    environment.GetDevice()->WaitIdle();
    const auto originalColor   = color;
    const auto originalCapture = capturedColor;
    const auto originalDepth   = depth;
    auto       display         = colorDescription;
    display.m_Size             = {65, 63};
    color                      = environment.GetDevice()->CreateTexture(display);
    capturedColor              = environment.GetDevice()->CreateTexture(display);
    auto sceneDepth            = depthDescription;
    sceneDepth.m_Size          = {32, 30};
    sceneDepth.m_BindFlags |= xiiGALBindFlags::ShaderResource;
    depth = environment.GetDevice()->CreateTexture(sceneDepth);
    first->SetViewport({0, 0, 65, 63});
    first->SetRenderTargetView(color->GetDefaultView(xiiGALTextureViewType::RenderTarget));

    for (const auto& viewport : {xiiRectFloat(0, 0, 65, 63), xiiRectFloat(37, 19, 65, 63)})
    {
      // Intermediates remain view-local even when presentation uses an offset viewport.
      first->SetViewport(viewport);
      clearDepth = 0.0f;
      xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red);
      world.Update();
      checkCenter(255, 0, 0);
      clearDepth = 1.0f;
      xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red);
      world.Update();
      checkCenter(0, 0, 0);
      xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Blue, xiiDebugRendererDepthMode::Overlay);
      world.Update();
      checkCenter(0, 0, 255);
    }

    environment.GetDevice()->WaitIdle();
    color         = originalColor;
    capturedColor = originalCapture;
    depth         = originalDepth;
    clearDepth    = 0.0f;
    first->SetViewport({0, 0, 64, 64});
    first->SetRenderTargetView(color->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Skeleton preview geometry reaches the GPU through regular debug passes")
  {
    xiiSkeletonResourceDescriptor skeleton;
    skeleton.AddJoint("Root", xiiMath::MaxValue<xiiUInt16>(), xiiTransform::Make(xiiVec3(-1, 0, 0)));
    skeleton.AddJoint("Child", 0, xiiTransform::Make(xiiVec3(2, 0, 0)));
    skeleton.BuildModelSpaceRestPose();
    xiiSkeletonPreviewSettings settings;
    settings.m_sHighlightedBones = "Child;";
    xiiSkeletonPreview preview;
    XII_TEST_RESULT(preview.Build(skeleton.m_Joints, settings));
    XII_TEST_RESULT(xiiDebugRenderer::DrawGeometry(*first, preview.GetGeometry(), xiiColor::White));
    world.Update();
    xiiImage image;
    if (XII_TEST_RESULT(xiiCaptureThumbnail(capturedColor, {64, 64}, image)))
    {
      bool foundBone = false;
      for (xiiUInt32 y = 30; y < 34; ++y)
      {
        const auto* pixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, 32, y);
        foundBone |= pixel[0] == 255 && pixel[1] == 255 && pixel[2] == 0;
      }
      XII_TEST_BOOL(foundBone);
    }
    world.Update();
    checkCenter(0, 0, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Overlay geometry bypasses occlusion and preserves scene depth")
  {
    clearDepth = 1.0f;
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
    XII_TEST_RESULT(readback.Enqueue(commandList, {.m_pTexture = depth}));
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
            XII_TEST_FLOAT(row[x], 1.0f, 0.0f);
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
    clearDepth = 0.0f;
    xiiDebugRenderer::DrawSolidTriangles(&world, xiiMakeArrayPtr(&triangle, 1), xiiColor::Red, xiiDebugRendererDepthMode::Overlay);
    xiiDebugRenderer::DrawSolidTriangles(*first, xiiMakeArrayPtr(&triangle, 1), xiiColor::Blue);
    world.Update();
    checkCenter(255, 0, 0); // World overlays follow the view's ordinary primitives too.
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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Textured overlays bypass depth without changing depth-tested rectangles")
  {
    auto sampledDescription                     = colorDescription;
    sampledDescription.m_Size                   = {1, 1};
    const xiiUInt8                     pixels[] = {0, 0, 255, 255};
    const xiiGALTextureSubResourceData subresource(xiiConstByteBlobPtr(pixels, 4), 4, 4);
    const xiiGALTextureData            initialData(xiiMakeArrayPtr(&subresource, 1));
    auto                               sampled = environment.GetDevice()->CreateTexture(sampledDescription, &initialData);
    clearDepth                                 = 1.0f;
    xiiDebugRenderer::Draw2DRectangle(*first, xiiRectFloat(0, 0, 64, 64), 0, xiiColor::White, sampled->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    world.Update();
    checkCenter(0, 0, 0);
    xiiDebugRenderer::Draw2DRectangle(*first, xiiRectFloat(0, 0, 64, 64), 0, xiiColor::White, sampled->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiVec2(1), xiiDebugRendererDepthMode::Overlay);
    world.Update();
    checkCenter(0, 0, 255);
    world.Update();
    checkCenter(0, 0, 0);
    clearDepth = 0.0f;
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
