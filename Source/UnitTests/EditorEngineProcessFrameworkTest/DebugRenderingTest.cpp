/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

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
          data.m_Depth = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, depth, depth->GetResourceState()), xiiGALResourceStateFlags::DepthWrite); }, [](const ClearData& data, xiiRenderGraphPassContext& context) {
          auto& commandList = context.GetCommandList();
          commandList.ClearRenderTargetView(context.GetTexture(data.m_Color)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black);
          commandList.ClearDepthStencilView(context.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 1.0f, 0); }, true);
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
}
