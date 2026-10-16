/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>
#include <EditorEngineProcessFrameworkTest/GpuBufferReadback.h>
#include <GraphicsCore/Lighting/DynamicGlobalIllumination.h>
#include <GraphicsCore/Lighting/SparseVoxelRadiance.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

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
    if (!XII_TEST_RESULT(histories[i].Initialize(environment.GetDevice(), description)) || !XII_TEST_RESULT(histories[i].Resize(8, 8)) || !XII_TEST_RESULT(contexts[i].Initialize("Interleaved Hi-Z")))
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

/// Test-only resource pass observes actual GAL bindings after every default pipeline stage.
/// Each view owns its own instance, just like a production pass with temporal state.
class xiiPipelineHistoryCheck : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineHistoryCheck, xiiRenderPipelinePass);

public:
  static constexpr xiiUInt32 s_uiHistoryCount               = 9;
  xiiGALTexture*             m_LastOutput[s_uiHistoryCount] = {}; ///< Borrowed from the live view; this test never resizes it.
  xiiGALTexture*             m_GIAtlases[2]                 = {};
  xiiGALBuffer*              m_pRadiancePool                = nullptr;
  xiiGALBuffer*              m_pProbeStates                 = nullptr;
  xiiGALBuffer*              m_pClipmapLevels               = nullptr;
  xiiUInt32                  m_uiExecutions                 = 0;
  xiiUInt64                  m_uiLastFrame                  = 0;

  void GetPins(xiiDynamicArray<xiiRenderPipelinePin>& pins) const override
  {
    pins.PushBack({"Before", xiiRenderPipelinePin::Order, true});
    pins.PushBack({"PreviousTAA", xiiRenderPipelinePin::Texture, true, true});
    pins.PushBack({"CurrentTAA", xiiRenderPipelinePin::Texture, true});
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    static constexpr xiiStringView names[s_uiHistoryCount][2] = {
      {"AO Previous History", "StableAO"},
      {"RT Shadow Previous History", "RTFinalShadows"},
      {"ReSTIRDIReservoirPrevious", "DirectLightReservoir"},
      {"ReSTIR GI Previous Sample", "ReSTIR GI Current Sample"},
      {"RTGI Previous History", "RTFinalGI"},
      {"RT Reflection Previous History", "RTFinalReflections"},
      {"SSR Previous History", "SSRTerm"},
      {"Volumetric Scattering Previous", "VolumetricScattering"},
      {"TAAHistory", "TAAResolved"},
    };
    struct Bindings
    {
      xiiRenderGraphTextureHandle m_Previous[s_uiHistoryCount];
      xiiRenderGraphTextureHandle m_Current[s_uiHistoryCount];
      xiiRenderGraphTextureHandle m_GIAtlases[2];
      xiiRenderGraphBufferHandle  m_ProbeStates;
      xiiRenderGraphBufferHandle  m_ClipmapLevels;
      xiiRenderGraphBufferHandle  m_RadiancePool;
    };
    // All names must exist. An absent producer is a test failure, not skipped coverage.
    for (xiiUInt32 index = 0; index + 1 < s_uiHistoryCount; ++index)
      if (!context.m_Graph.FindTexture(names[index][0]).IsValid() || !context.m_Graph.FindTexture(names[index][1]).IsValid())
        return XII_FAILURE;
    if (!context.GetInputTexture("CurrentTAA").IsValid() || (m_uiExecutions && !context.GetInputTexture("PreviousTAA").IsValid()))
      return XII_FAILURE;
    if (!context.m_Graph.FindTexture("DDGIProbeIrradianceAtlas").IsValid() || !context.m_Graph.FindTexture("DDGIProbeDistanceAtlas").IsValid() || !context.m_Graph.FindBuffer("SparseVoxelRadiancePool").IsValid())
      return XII_FAILURE;
    context.m_Graph.AddPass<Bindings>("Check per-view temporal bindings", xiiGALCommandQueueFlags::Graphics, [&](Bindings& data, xiiRenderGraphBuilder& builder) {
        data.m_GIAtlases[0] = builder.ReadTexture(context.m_Graph.FindTexture("DDGIProbeIrradianceAtlas"), xiiGALResourceStateFlags::ShaderResource);
        data.m_GIAtlases[1] = builder.ReadTexture(context.m_Graph.FindTexture("DDGIProbeDistanceAtlas"), xiiGALResourceStateFlags::ShaderResource);
        data.m_ProbeStates = builder.ReadBuffer(context.m_Graph.FindBuffer("DDGIProbeStates"), xiiGALResourceStateFlags::ShaderResource);
        data.m_ClipmapLevels = builder.ReadBuffer(context.m_Graph.FindBuffer("SparseVoxelLevelData"), xiiGALResourceStateFlags::ShaderResource);
        data.m_RadiancePool = builder.ReadBuffer(context.m_Graph.FindBuffer("SparseVoxelRadiancePool"), xiiGALResourceStateFlags::ShaderResource);
        for (xiiUInt32 i = 0; i < s_uiHistoryCount; ++i)
        {
          const auto previous = i + 1 == s_uiHistoryCount ? context.GetInputTexture("PreviousTAA") : context.m_Graph.FindTexture(names[i][0]);
          const auto current = i + 1 == s_uiHistoryCount ? context.GetInputTexture("CurrentTAA") : context.m_Graph.FindTexture(names[i][1]);
          if (previous.IsValid()) data.m_Previous[i] = builder.ReadTexture(previous, xiiGALResourceStateFlags::ShaderResource);
          data.m_Current[i] = builder.ReadTexture(current, xiiGALResourceStateFlags::ShaderResource);
        } }, [this](const Bindings& data, xiiRenderGraphPassContext& execute) {
        XII_TEST_BOOL(execute.GetFrameIndex() > m_uiLastFrame);
        m_uiLastFrame = execute.GetFrameIndex();
        for (xiiUInt32 i = 0; i < s_uiHistoryCount; ++i)
        {
          auto* previous = data.m_Previous[i].IsValid() ? execute.GetTexture(data.m_Previous[i]) : nullptr;
          auto* current = execute.GetTexture(data.m_Current[i]);
          XII_TEST_BOOL(previous != current);
          if (m_uiExecutions != 0)
            XII_TEST_BOOL_MSG(previous == m_LastOutput[i], "History %u must read this view's last recorded output.", i);
          m_LastOutput[i] = current;
        }
        for (xiiUInt32 i = 0; i < 2; ++i)
        {
          auto* atlas = execute.GetTexture(data.m_GIAtlases[i]);
          XII_TEST_BOOL(atlas != nullptr);
          if (m_uiExecutions != 0)
            XII_TEST_BOOL(atlas == m_GIAtlases[i]);
          m_GIAtlases[i] = atlas;
        }
        auto* pool = execute.GetBuffer(data.m_RadiancePool);
        XII_TEST_BOOL(pool != nullptr);
        if (m_uiExecutions != 0)
          XII_TEST_BOOL(pool == m_pRadiancePool);
        m_pRadiancePool = pool;
        m_pProbeStates = execute.GetBuffer(data.m_ProbeStates);
        m_pClipmapLevels = execute.GetBuffer(data.m_ClipmapLevels);
        ++m_uiExecutions; }, true);
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineHistoryCheck, 1, xiiRTTIDefaultAllocator<xiiPipelineHistoryCheck>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(TemporalHistory, InterleavedDefaultPipelines)
{
  // This fixture checks allocation continuity at a fixed resolution. The production PID
  // changes resolution under load and correctly recreates all nine temporal histories.
  // Pin both bounds so shader compilation, validation and other tests cannot resize it.
  auto* minimum = xiiCVar::FindCVarByName("Graphics.DynamicResolution.MinimumRenderScale");
  auto* maximum = xiiCVar::FindCVarByName("Graphics.DynamicResolution.MaximumRenderScale");
  if (!XII_TEST_BOOL(minimum && maximum) || !XII_TEST_BOOL(minimum->GetType() == xiiCVarType::Float && maximum->GetType() == xiiCVarType::Float))
    return;
  auto&      minimumScale    = *static_cast<xiiCVarFloat*>(minimum);
  auto&      maximumScale    = *static_cast<xiiCVarFloat*>(maximum);
  const auto previousMinimum = minimumScale.GetValue();
  const auto previousMaximum = maximumScale.GetValue();
  XII_SCOPE_EXIT(minimumScale = previousMinimum; maximumScale = previousMaximum);
  minimumScale = 1.0f;
  maximumScale = 1.0f;

  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderPipelineResourceDescriptor definition;
  if (!XII_TEST_RESULT(xiiLoadPipelineLibraryDescriptor(definition)))
    return;
  const auto inspect = definition.AddPass("Temporal history check", "xiiPipelineHistoryCheck");
  if (!XII_TEST_RESULT(definition.Connect(inspect - 1, "After", inspect, "Before")))
    return;
  for (xiiUInt32 index = 0; index < inspect; ++index)
    if (definition.m_Passes[index].m_sType == "xiiTemporalAntiAliasingPass")
    {
      XII_TEST_RESULT(definition.Connect(index, "PreviousColor", inspect, "PreviousTAA"));
      XII_TEST_RESULT(definition.Connect(index, "TAAResolved", inspect, "CurrentTAA"));
    }
  const auto             pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Interleaved default pipeline history", std::move(definition));
  xiiCamera              cameras[2];
  xiiUniquePtr<xiiWorld> worlds[2];
  xiiView*               views[2] = {};
  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiWorldDescription worldDescription("Independent temporal view");
    worlds[i] = XII_DEFAULT_NEW(xiiWorld, worldDescription);
    XII_LOCK(worlds[i]->GetWriteMarker());
    worlds[i]->SetWorldSimulationEnabled(false);
    worlds[i]->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Temporal view", views[i]);
    xiiGALTextureCreationDescription target;
    target.m_Type      = xiiGALResourceDimension::Texture2D;
    target.m_Size      = {32, 32};
    target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
    target.m_BindFlags = xiiGALBindFlags::RenderTarget;
    auto texture       = environment.GetDevice()->CreateTexture(target);
    if (!XII_TEST_BOOL(texture != nullptr))
      return;
    views[i]->SetRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
    views[i]->SetViewport({0, 0, 32, 32});
    cameras[i].SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
    const xiiVec3 position(i * 1000.0f, i * 2000.0f, -static_cast<float>(i) * 3000.0f);
    cameras[i].LookAt(position, position + xiiVec3(1, 0, 0), xiiVec3(0, 0, 1));
    views[i]->SetCamera(&cameras[i]);
    views[i]->SetRenderPipelineResource(pipeline);
  }
  const xiiPipelineHistoryCheck* checks[2] = {};
  for (xiiUInt32 iteration = 0; iteration < 3; ++iteration)
  {
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      XII_LOCK(worlds[view]->GetWriteMarker());
      worlds[view]->Update();
      environment.GetDevice()->WaitIdle();
      XII_TEST_INT(views[view]->GetRenderResolutionWidth(), 32);
      XII_TEST_INT(views[view]->GetRenderResolutionHeight(), 32);
      checks[view] = xiiDynamicCast<const xiiPipelineHistoryCheck*>(views[view]->GetRenderPipelineInstance().GetPass(inspect));
      if (!XII_TEST_BOOL(checks[view] != nullptr))
        return;
      XII_TEST_INT(checks[view]->m_uiExecutions, iteration + 1);
      // Read actual uploaded metadata as well as checking allocation identity. Each clipmap
      // must describe its own camera even when another world rendered between its frames.
      xiiGpuDDGIProbeState   probe;
      xiiGpuSparseVoxelLevel level;
      if (!xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), checks[view]->m_pProbeStates, 0, probe) || !xiiGpuTestReadback::ReadBufferRecord(*environment.GetDevice(), checks[view]->m_pClipmapLevels, 0, level))
        return;
      const auto position = cameras[view].GetPosition();
      XII_TEST_BOOL(xiiMath::Abs(probe.m_vPositionAndLastUpdate.x - position.x) <= 48.0f);
      XII_TEST_BOOL(xiiMath::Abs(probe.m_vPositionAndLastUpdate.y - position.y) <= 24.0f);
      XII_TEST_BOOL(xiiMath::Abs(probe.m_vPositionAndLastUpdate.z - position.z) <= 48.0f);
      XII_TEST_INT(level.m_vMinimumCellAndPageOffset.x, static_cast<xiiInt32>(xiiMath::Floor(position.x / 4.0f)) - 4);
      XII_TEST_INT(level.m_vMinimumCellAndPageOffset.y, static_cast<xiiInt32>(xiiMath::Floor(position.y / 4.0f)) - 4);
      XII_TEST_INT(level.m_vMinimumCellAndPageOffset.z, static_cast<xiiInt32>(xiiMath::Floor(position.z / 4.0f)) - 4);
    }
    XII_TEST_BOOL(checks[0] != checks[1]);
    for (xiiUInt32 i = 0; i < 2; ++i)
      XII_TEST_BOOL(checks[0]->m_GIAtlases[i] != checks[1]->m_GIAtlases[i]);
    XII_TEST_BOOL(checks[0]->m_pRadiancePool != checks[1]->m_pRadiancePool);
    for (xiiUInt32 history = 0; history < xiiPipelineHistoryCheck::s_uiHistoryCount; ++history)
      XII_TEST_BOOL(checks[0]->m_LastOutput[history] != checks[1]->m_LastOutput[history]);
  }
  // Keep multiple frames in flight. Persistent GI storage must also remain valid when the
  // next view starts recording before the previous view's GPU commands have completed.
  for (xiiUInt32 iteration = 0; iteration < 6; ++iteration)
    for (xiiUInt32 view = 0; view < 2; ++view)
    {
      XII_LOCK(worlds[view]->GetWriteMarker());
      worlds[view]->Update();
    }
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(checks[0]->m_uiExecutions, 9);
  XII_TEST_INT(checks[1]->m_uiExecutions, 9);
}

class xiiPipelineHistoryFailure : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelineHistoryFailure, xiiRenderPipelinePass);

public:
  xiiUInt32                   m_uiRecordings    = 0;
  xiiUInt32                   m_uiInvalidations = 0;
  bool                        m_bHistoryValid   = false;
  xiiSharedPtr<xiiGALTexture> m_pImage;
  void                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  void                        InvalidateTemporalHistory() override
  {
    if (m_uiRecordings != 0)
      ++m_uiInvalidations;
    m_bHistoryValid = false;
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    if (!m_pImage)
    {
      xiiGALTextureCreationDescription texture;
      texture.m_Type      = xiiGALResourceDimension::Texture2D;
      texture.m_Size      = {8, 8};
      texture.m_Format    = xiiGALResourceFormat::R32Float;
      texture.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
      m_pImage            = xiiGALDevice::GetDefaultDevice()->CreateTexture(texture);
      if (!m_pImage)
        return XII_FAILURE;
    }
    struct History
    {
      xiiRenderGraphBufferHandle  m_Token;
      xiiRenderGraphTextureHandle m_Image;
    };
    const auto history = context.m_Graph.AddPass<History>("Record history before failure", xiiGALCommandQueueFlags::Graphics, [this](History& data, xiiRenderGraphBuilder& builder) {
        xiiGALBufferCreationDescription buffer;
        buffer.m_uiSize = sizeof(xiiUInt32);
        buffer.m_BindFlags = xiiGALBindFlags::ShaderResource;
        buffer.m_Mode = xiiGALBufferMode::Raw;
        data.m_Token = builder.WriteBuffer("History ordering token", buffer, xiiGALResourceStateFlags::CopyDestination);
        data.m_Image = builder.WriteTexture(builder.ImportTexture("Recoverable history image", m_pImage, m_pImage->GetResourceState()), xiiGALResourceStateFlags::CopyDestination); }, [this](const History& data, xiiRenderGraphPassContext& execute) {
        execute.GetCommandList().ClearRenderTargetView(execute.GetTexture(data.m_Image)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.25f, 0, 0, 1));
        ++m_uiRecordings;
        m_bHistoryValid = true; }, true);
    if (m_uiRecordings == 0)
    {
      struct InvalidAttachments
      {
        XII_DECLARE_POD_TYPE();
      };
      // Both images are valid GAL textures. Only their combined framebuffer is invalid, which
      // fails during materialization after the earlier history callback has already recorded.
      context.m_Graph.AddPass<InvalidAttachments>("Fail after recording history", xiiGALCommandQueueFlags::Graphics, [&](InvalidAttachments&, xiiRenderGraphBuilder& builder) {
          static_cast<void>(builder.ReadBuffer(history.first->m_Token, xiiGALResourceStateFlags::ShaderResource));
          xiiGALTextureCreationDescription texture;
          texture.m_Type = xiiGALResourceDimension::Texture2D;
          texture.m_Size = {8, 8};
          texture.m_Format = xiiGALResourceFormat::RGBA8UNormalized;
          texture.m_BindFlags = xiiGALBindFlags::RenderTarget;
          static_cast<void>(builder.WriteTexture("Full-size attachment", texture, xiiGALResourceStateFlags::RenderTarget));
          texture.m_Size = {4, 4};
          static_cast<void>(builder.WriteTexture("Mismatched attachment", texture, xiiGALResourceStateFlags::RenderTarget));
          builder.SetPassRenderPassManaged(); }, [](const InvalidAttachments&, xiiRenderGraphPassContext&) {}, true);
    }
    return XII_SUCCESS;
  }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelineHistoryFailure, 1, xiiRTTIDefaultAllocator<xiiPipelineHistoryFailure>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(TemporalHistory, FailedRecordingInvalidatesHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());

  xiiRenderPipelineResourceDescriptor definition;
  definition.AddPass("History failure regression", "xiiPipelineHistoryFailure");
  const auto          pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Recoverable history failure", std::move(definition));
  xiiWorldDescription description("Failed recording recovery");
  xiiWorld            world(description);
  XII_LOCK(world.GetWriteMarker());
  world.SetWorldSimulationEnabled(false);
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Recovering temporal view", view);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {8, 8};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto texture       = environment.GetDevice()->CreateTexture(target);
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  view->SetRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  view->SetViewport({0, 0, 8, 8});
  view->SetCamera(&camera);
  view->SetRenderPipelineResource(pipeline);
  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope logScope(&log, true);
    log.ExpectMessage("dimensions do not match the framebuffer", xiiLogMsgType::ErrorMsg);
    world.Update();
  }
  const auto* pass = xiiDynamicCast<const xiiPipelineHistoryFailure*>(view->GetRenderPipelineInstance().GetPass(0));
  if (!XII_TEST_BOOL(pass != nullptr))
    return;
  XII_TEST_INT(pass->m_uiRecordings, 1);
  XII_TEST_INT(pass->m_uiInvalidations, 1);
  XII_TEST_BOOL(!pass->m_bHistoryValid);
  environment.GetDevice()->WaitIdle();
  CheckDepthHistory(*environment.GetDevice(), pass->m_pImage.Borrow(), 0.25f);
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(pass->m_uiRecordings, 2);
  XII_TEST_INT(pass->m_uiInvalidations, 1);
  XII_TEST_BOOL(pass->m_bHistoryValid);
  // Wrap the profiler's query ring after the aborted recording, then verify it remains usable.
  for (xiiUInt32 i = 0; i < 6; ++i)
    world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_INT(pass->m_uiRecordings, 8);
  XII_TEST_INT(pass->m_uiInvalidations, 1);
}
