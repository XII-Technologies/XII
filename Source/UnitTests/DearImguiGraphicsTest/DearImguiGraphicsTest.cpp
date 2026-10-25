/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <DearImguiPlugin/DearImguiInput.h>
#include <DearImguiPlugin/DearImguiSubsystem.h>
#include <DearImguiPlugin/Pipeline/DearImguiRenderPass.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <GraphicsFoundationTest/TestingEnvironment/TextureReadback.h>
#include <TestFramework/Utilities/TestSetup.h>

XII_TESTFRAMEWORK_ENTRY_POINT("DearImguiGraphicsTest", "Dear ImGui Graphics Tests")
XII_CREATE_SIMPLE_TEST_GROUP(DearImguiGraphics);

XII_CREATE_SIMPLE_TEST(DearImguiGraphics, NativeCompositionAndDeferredLifetime)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize(false)))
    return;
  auto* device = environment.GetDevice();
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(device->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("Dear ImGui", xiiGetStaticRTTI<xiiImguiRenderPass>());
  definition.m_Interface.PushBack({"Color", true, definition.m_Passes[0].m_Id, "Color"});
  definition.m_Interface.PushBack({"UI", true, definition.m_Passes[0].m_Id, "UI"});
  definition.m_Interface.PushBack({"Output", false, definition.m_Passes[0].m_Id, "Color"});
  xiiSharedPtr<const xiiRenderPipeline> pipeline;
  XII_TEST_RESULT(xiiRenderPipeline::Compile(definition, pipeline));
  xiiImguiContextHandle context;
  XII_TEST_RESULT(xiiImguiSubsystem::CreateContext(context));
  xiiGALTextureCreationDescription image;
  image.m_Type                         = xiiGALResourceDimension::Texture2D;
  image.m_Size                         = {1, 1};
  image.m_Format                       = xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  image.m_Usage                        = xiiGALResourceUsage::Immutable;
  image.m_BindFlags                    = xiiGALBindFlags::ShaderResource;
  const xiiUInt8               green[] = {0, 255, 0, 255};
  xiiGALTextureSubResourceData pixel;
  pixel.m_pData    = xiiMakeByteBlobPtr(green, sizeof(green));
  pixel.m_uiStride = pixel.m_uiDepthStride = 4;
  const xiiGALTextureData initial(xiiMakeArrayPtr(&pixel, 1));
  auto                    texture = device->CreateTexture(image, &initial);
  ImTextureID             textureId;
  XII_TEST_RESULT(xiiImguiSubsystem::RegisterTexture(context, texture, textureId));
  xiiSharedPtr<const xiiImguiFrame> snapshot;
  {
    xiiImguiScopedContext    scope(context);
    xiiImguiFrameDescription frame;
    frame.m_vDisplaySize      = {64, 48};
    frame.m_vFramebufferScale = {2, 2};
    XII_TEST_RESULT(scope.BeginFrame(frame));
    auto* background = ImGui::GetBackgroundDrawList();
    for (xiiUInt32 i = 0; i < 17000; ++i)
      background->AddRectFilled({-20, -20}, {-10, -10}, IM_COL32_WHITE);
    auto* draw = ImGui::GetForegroundDrawList();
    draw->PushClipRect({12, 12}, {28, 28});
    draw->AddRectFilled({10, 10}, {30, 30}, IM_COL32(255, 0, 0, 128));
    draw->PopClipRect();
    draw->AddImage(ImTextureRef(textureId), {40, 10}, {55, 25});
    draw->AddText({4, 32}, IM_COL32_WHITE, "XII");
    XII_TEST_RESULT(scope.EndFrame(snapshot));
  }
  XII_TEST_BOOL(snapshot && snapshot->GetCommands().GetCount() >= 3);
  XII_TEST_BOOL(snapshot->GetVertices().GetCount() > 65536);
  bool offsetSeen = false;
  for (const auto& command : snapshot->GetCommands())
    offsetSeen |= command.m_uiBaseVertex > 65535;
  XII_TEST_BOOL(offsetSeen);
  XII_TEST_RESULT(xiiImguiSubsystem::UnregisterTexture(context, textureId));
  XII_TEST_BOOL(xiiImguiSubsystem::UnregisterTexture(context, textureId).Failed());
  texture.Clear();
  // Destroy all mutable ImGui state before graph setup; retained frames own geometry and texture versions.
  XII_TEST_RESULT(xiiImguiSubsystem::DestroyContext(context));

  xiiWorldDescription worldDescription("UI render destinations");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  auto*    module = world.GetOrCreateModule<xiiRenderWorldModule>();
  xiiView* view;
  auto     viewHandle = module->CreateView("UI offscreen", view);
  view->SetViewport({8, 8, 128, 96});
  image.m_Size                     = {144, 112};
  image.m_Format                   = xiiGALResourceFormat::RGBA32Float;
  image.m_Usage                    = xiiGALResourceUsage::Default;
  image.m_BindFlags                = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  auto                      target = device->CreateTexture(image);
  xiiRenderPipelineInstance instance;
  XII_TEST_RESULT(instance.Initialize(pipeline));
  auto graphHandle = xiiRenderGraphManager::CreateGraph("UI graphics test", [&](xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard, const xiiView*) -> xiiResult {
    xiiRenderGraphTextureHandle color;
    graph.AddPass<xiiRenderGraphTextureHandle>("Scene", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      color = data = builder.WriteTexture(builder.ImportTexture("Color", target, target->GetResourceState()), xiiGALResourceStateFlags::RenderTarget);
      builder.SetPassRenderPassManaged(); }, [](const auto& data, xiiRenderGraphPassContext& execution) { execution.GetCommandList().ClearRenderTargetView(execution.GetTexture(data)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.25f, 0.25f, 0.25f, 1)); });
    xiiRenderPipelineBinding binding;
    binding.m_pFrameData                         = XII_DEFAULT_NEW(xiiImguiFrameData, graph.GetSetupSerial(), snapshot);
    const xiiRenderPipelineInputBinding inputs[] = {
      {graph, pipeline->FindInterface("Color", true), color},
      {graph, pipeline->FindInterface("UI", true), binding}};
    XII_SUCCEED_OR_RETURN(instance.Build(view, graph, blackboard, {}, inputs));
    const auto output = instance.GetInterfaceBinding(pipeline->FindInterface("Output", false))->m_hTexture;
    graph.AddPass<xiiRenderGraphTextureHandle>("Export", xiiGALCommandQueueFlags::Graphics, [&](auto& data, xiiRenderGraphBuilder& builder) {
      data = builder.ReadTexture(output, xiiGALResourceStateFlags::RenderTarget);
      builder.ExportTexture(data, xiiGALResourceStateFlags::ShaderResource); }, [](const auto&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  });
  XII_SCOPE_EXIT(xiiRenderGraphManager::DestroyGraph(graphHandle).AssertSuccess(); module->DestroyView(viewHandle));
  for (xiiUInt32 frame = 0; frame < 4; ++frame)
  {
    // Recreate the destination between frames without any association left in the UI subsystem.
    if (frame == 2)
    {
      module->DestroyView(viewHandle);
      viewHandle = module->CreateView("Recreated UI destination", view);
      view->SetViewport({8, 8, 128, 96});
    }
    device->BeginFrame();
    {
      xiiRenderGraphFrame gpuFrame;
      XII_TEST_RESULT(gpuFrame.Begin(3));
      xiiStringBuilder error;
      XII_TEST_RESULT_MSG(xiiRenderGraphManager::ExecuteGraph(gpuFrame, graphHandle, view, {}, &error), "%s", error.GetData());
    }
    device->EndFrame();
  }
  xiiVec4 value;
  if (xiiGpuTestReadback::ReadTexturePixel(*device, target, 48, 48, value))
  {
    XII_TEST_FLOAT(value.x, 0.25f * (127.0f / 255.0f) + 128.0f / 255.0f, 0.005f);
    XII_TEST_FLOAT(value.y, 0.25f * (127.0f / 255.0f), 0.005f);
    XII_TEST_FLOAT(value.w, 1, 0.001f);
  }
  if (xiiGpuTestReadback::ReadTexturePixel(*device, target, 28, 48, value))
    XII_TEST_FLOAT(value.x, 0.25f, 0.001f); // Outside the command clip, inside its un-clipped rectangle.
  if (xiiGpuTestReadback::ReadTexturePixel(*device, target, 98, 38, value))
  {
    XII_TEST_FLOAT(value.x, 0, 0.001f);
    XII_TEST_FLOAT(value.y, 1, 0.001f);
    XII_TEST_FLOAT(value.z, 0, 0.001f);
  }
  if (xiiGpuTestReadback::ReadTexturePixel(*device, target, 2, 2, value))
    XII_TEST_FLOAT(value.x, 0.25f, 0.001f); // Viewport offset preserves unrelated target contents.

  // Font glyphs occupy this region. At least one covered pixel must be brighter than the scene.
  xiiGALTextureReadback readback(device);
  auto                  commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  commands->Begin();
  XII_TEST_RESULT(readback.Enqueue(commands, {.m_pTexture = target, .m_uiRegionX = 16, .m_uiRegionY = 72, .m_uiRegionW = 48, .m_uiRegionH = 30}));
  commands->End();
  auto* queue = device->GetCommandQueue();
  queue->WaitForFenceValue(queue->Submit(commands));
  readback.WaitForNextCompleted();
  const auto capture = readback.GetCompleted();
  if (XII_TEST_BOOL(static_cast<bool>(capture)))
  {
    commands->Begin();
    xiiGALMappedTextureSubresource mapped;
    if (XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, {}, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
    {
      bool glyphSeen = false;
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        const auto* row = xiiMemoryUtils::AddByteOffset(static_cast<const xiiVec4*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
          glyphSeen |= row[x].x > 0.3f && row[x].y > 0.3f && row[x].z > 0.3f;
      }
      XII_TEST_BOOL(glyphSeen);
      XII_TEST_RESULT(commands->UnmapTextureSubresource(capture.m_pStagingTexture, {}));
    }
    commands->End();
    readback.RecycleStagingTexture(capture.m_pStagingTexture);
  }
}
