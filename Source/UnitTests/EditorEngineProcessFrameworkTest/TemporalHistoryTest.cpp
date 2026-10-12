/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>

namespace
{
  bool CheckDepthHistory(xiiGALDevice& device, xiiGALTexture* texture, float expected)
  {
    if (!XII_TEST_BOOL(texture != nullptr))
      return false;
    xiiGALTextureReadback                readback(&device);
    xiiGALCommandListCreationDescription description;
    description.m_QueueFlags = xiiGALCommandQueueFlags::Graphics;
    auto commands            = device.CreateCommandList(description);
    if (!XII_TEST_BOOL(commands != nullptr))
      return false;
    // Read every mip, including the final reduction, from the actual imported history image.
    for (xiiUInt32 mip = 0; mip < texture->GetDescription().m_uiMipLevels; ++mip)
    {
      xiiGALTextureReadback::ReadbackRequest request;
      request.m_pTexture   = texture;
      request.m_uiMipLevel = mip;
      commands->Begin();
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
      xiiGALTextureMipLevelData      subresource;
      if (!XII_TEST_RESULT(commands->MapTextureSubresource(capture.m_pStagingTexture, subresource, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mapped)))
      {
        commands->End();
        return false;
      }
      for (xiiUInt32 y = 0; y < capture.m_uiHeight; ++y)
      {
        const float* row = xiiMemoryUtils::AddByteOffset(static_cast<const float*>(mapped.m_pData), y * mapped.m_uiStride);
        for (xiiUInt32 x = 0; x < capture.m_uiWidth; ++x)
          XII_TEST_FLOAT(row[x], expected, 0.0001f);
      }
      const auto unmapped = commands->UnmapTextureSubresource(capture.m_pStagingTexture, subresource);
      commands->End();
      commands->Reset();
      if (!XII_TEST_RESULT(unmapped))
        return false;
    }
    return true;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(TemporalHistory);
XII_CREATE_SIMPLE_TEST(TemporalHistory, InterleavedHiZ)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiGpuHiZPyramid      histories[2];
  xiiRenderGraphContext contexts[2];
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiGpuHiZPyramidDescription description;
    description.m_uiFramesInFlight = i + 2;
    if (!XII_TEST_RESULT(histories[i].Initialize(environment.GetDevice(), description)) ||
        !XII_TEST_RESULT(histories[i].Resize(8, 8)) || !XII_TEST_RESULT(contexts[i].Initialize("Interleaved Hi-Z")))
      return;
  }

  float     previous[2]  = {};
  bool      valid[2]     = {};
  xiiUInt64 lastBuild[2] = {};
  auto      render       = [&](xiiUInt32 view, float depth, bool execute = true) {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin(1)))
      return;
    auto& graph = contexts[view].GetGraph();
    auto& cache = contexts[view].GetResourceCache();
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    graph.BeginSetup(frame.GetFrameIndex());
    auto& history = histories[view];
    XII_TEST_BOOL(!history.ImportPrevious(graph, lastBuild[view]).IsValid());
    const auto imported = history.ImportPrevious(graph, frame.GetFrameIndex());
    XII_TEST_BOOL(imported.IsValid() == valid[view]);
    struct Depth
    {
      xiiRenderGraphTextureHandle m_Depth;
    };
    const auto source = graph.AddPass<Depth>("Hi-Z source depth", xiiGALCommandQueueFlags::Graphics, [](Depth& data, xiiRenderGraphBuilder& builder) {
        xiiGALTextureCreationDescription texture;
        texture.m_Type = xiiGALResourceDimension::Texture2D;
        texture.m_Size = {8, 8};
        texture.m_Format = xiiGALResourceFormat::D32Float;
        texture.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
        data.m_Depth = builder.WriteTexture("Hi-Z depth", texture, xiiGALResourceStateFlags::DepthWrite);
        builder.SetPassRenderPassManaged(); }, [depth](const Depth& data, xiiRenderGraphPassContext& context) { context.GetCommandList().ClearDepthStencilView(context.GetTexture(data.m_Depth)->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, depth, 0); });
    history.AddBuildPass(graph, frame.GetFrameIndex(), source.first->m_Depth, true);
    struct Export
    {
      XII_DECLARE_POD_TYPE();
    };
    if (imported.IsValid())
      graph.AddPass<Export>("Export previous Hi-Z", xiiGALCommandQueueFlags::Graphics, [imported](Export&, xiiRenderGraphBuilder& builder) { builder.ExportTexture(imported, xiiGALResourceStateFlags::CopySource); }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
    graph.EndSetup();
    if (!XII_TEST_RESULT(graph.Compile()))
      return;
    // Merely compiling an abandoned frame must not rotate or promote temporal history.
    XII_TEST_BOOL(history.HasValidHistory() == valid[view]);
    if (!execute)
      return;
    if (!XII_TEST_RESULT(graph.Execute(environment.GetDevice(), nullptr, &contexts[view].GetBlackboard(), &cache)))
      return;
    lastBuild[view] = frame.GetFrameIndex();
    frame.End();
    environment.GetDevice()->WaitIdle();
    if (imported.IsValid())
      CheckDepthHistory(*environment.GetDevice(), graph.GetExportedTexture(imported).Borrow(), previous[view]);
    XII_TEST_BOOL(history.HasValidHistory());
    previous[view] = depth;
    valid[view]    = true;
  };

  for (xiiUInt32 iteration = 0; iteration < 5; ++iteration)
  {
    render(0, 0.1f + iteration * 0.1f);
    render(1, 0.9f - iteration * 0.1f);
    render(0, 0.99f, false);
  }
  histories[0].Invalidate();
  valid[0] = false;
  render(0, 0.25f, false);
  render(0, 0.35f);
  render(1, 0.75f);
  render(0, 0.45f);
  XII_TEST_RESULT(histories[0].Resize(4, 4));
  XII_TEST_BOOL(!histories[0].HasValidHistory());
}
