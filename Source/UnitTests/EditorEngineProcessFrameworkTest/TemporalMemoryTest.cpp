/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/TemporalHistory.h>
#include <GraphicsCore/Pipeline/Passes/Temporal/TemporalAntiAliasingPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  void AddTAAInputs(xiiView& view, xiiRenderGraph& graph)
  {
    struct Inputs
    {
      xiiRenderGraphTextureHandle m_Color, m_Velocity, m_Depth;
    };
    graph.AddPass<Inputs>("Temporal test inputs", xiiGALCommandQueueFlags::Compute, [&](Inputs& data, xiiRenderGraphBuilder& builder) {
      xiiGALTextureCreationDescription texture;
      texture.m_Type = xiiGALResourceDimension::Texture2D;
      texture.m_Size = {view.GetRenderResolutionWidth(), view.GetRenderResolutionHeight()};
      texture.m_Format = xiiGALResourceFormat::RGBA16Float;
      texture.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
      data.m_Color = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, texture, xiiGALResourceStateFlags::UnorderedAccess);
      texture.m_Format = xiiGALResourceFormat::RG16Float;
      data.m_Velocity = builder.WriteTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, texture, xiiGALResourceStateFlags::UnorderedAccess);
      texture.m_Format = xiiGALResourceFormat::R32Float;
      data.m_Depth = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, texture, xiiGALResourceStateFlags::UnorderedAccess); }, [](const Inputs&, xiiRenderGraphPassContext&) {});
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(TemporalMemory);

XII_CREATE_SIMPLE_TEST(TemporalMemory, PairAllocationFailure)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); environment.Shutdown());
  xiiSharedPtr<xiiGALTexture>      textures[2];
  xiiGALTextureCreationDescription description;
  description.m_Type      = xiiGALResourceDimension::Texture2D;
  description.m_Size      = {16, 16};
  description.m_Format    = xiiGALResourceFormat::RGBA16Float;
  description.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  bool valid              = true;
  auto create             = [&](const auto& desc) {
    return environment.GetDevice()->CreateTexture(desc);
  };
  if (!XII_TEST_RESULT(xiiRenderPassUtils::EnsureTemporalHistoryTextures(textures, description, valid, create)))
    return;
  XII_TEST_BOOL(!valid);
  valid           = true;
  xiiUInt32 calls = 0;
  XII_TEST_RESULT(xiiRenderPassUtils::EnsureTemporalHistoryTextures(textures, description, valid, [&](const auto&) -> xiiSharedPtr<xiiGALTexture> { ++calls; return nullptr; }));
  XII_TEST_INT(calls, 0);
  XII_TEST_BOOL(valid);
  const auto first   = textures[0];
  const auto second  = textures[1];
  description.m_Size = {32, 16};
  for (xiiUInt32 failAt : {1U, 2U})
  {
    calls = 0;
    xiiSharedPtr<xiiGALTexture> partial;
    auto                        fail = [&](const auto& desc) -> xiiSharedPtr<xiiGALTexture> {
      if (++calls == failAt)
        return nullptr;
      partial = create(desc);
      return partial;
    };
    XII_TEST_BOOL(xiiRenderPassUtils::EnsureTemporalHistoryTextures(textures, description, valid, fail).Failed());
    XII_TEST_INT(calls, failAt);
    XII_TEST_BOOL(textures[0] == first && textures[1] == second && valid);
    if (partial != nullptr)
      XII_TEST_INT(partial->GetRefCount(), 1);
  }
  XII_TEST_RESULT(xiiRenderPassUtils::EnsureTemporalHistoryTextures(textures, description, valid, create));
  XII_TEST_BOOL(!valid && textures[0] != first && textures[1] != second);
  const auto resized   = textures[0];
  valid                = true;
  description.m_Format = xiiGALResourceFormat::RGBA32UInt;
  XII_TEST_RESULT(xiiRenderPassUtils::EnsureTemporalHistoryTextures(textures, description, valid, create));
  XII_TEST_BOOL(!valid && textures[0] != resized);
  XII_TEST_BOOL(textures[0]->GetDescription() == description && textures[1]->GetDescription() == description);
}

XII_CREATE_SIMPLE_TEST(TemporalMemory, TAAResizeReleasesOldHistory)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription worldDescription("TAA allocation retention");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Resizing TAA", view);
  xiiTemporalAntiAliasingPass pass, sibling;
  xiiRenderGraph              graph("TAA retention");
  xiiRenderGraphBlackboard    blackboard;
  xiiVariantDictionary        parameters;
  xiiSharedPtr<xiiGALTexture> original;
  xiiRenderGraphResourceCache cache;
  cache.Initialize(environment.GetDeviceShared());
  for (xiiUInt32 iteration = 0; iteration < 64; ++iteration)
  {
    xiiRenderGraphFrame frame;
    if (!XII_TEST_RESULT(frame.Begin(2)))
      return;
    cache.BeginFrame(frame.GetFrameIndex(), frame.GetCompletedFrame());
    XII_SCOPE_EXIT(cache.EndFrame());
    view->SetViewport({0, 0, static_cast<float>(32 + iteration * 2), 32});
    graph.BeginSetup(frame.GetFrameIndex());
    AddTAAInputs(*view, graph);
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    pass.GetPins(pins);
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    xiiRenderPipelinePassContext context{view, graph, blackboard, parameters, pins, bindings, "TAA retention"};
    for (xiiUInt32 index = 0; index < pins.GetCount(); ++index)
      if (pins[index].m_bInput && pins[index].m_Type == xiiRenderPipelinePin::Texture)
        bindings[index].m_hTexture = graph.FindTexture(pins[index].m_sName);
    const auto                   result          = pass.Build(context);
    auto                         siblingBindings = bindings;
    xiiRenderPipelinePassContext siblingContext{view, graph, blackboard, parameters, pins, siblingBindings, "Sibling TAA"};
    XII_TEST_RESULT(sibling.Build(siblingContext));
    struct Probe
    {
      xiiRenderGraphTextureHandle m_History, m_Sibling;
    };
    auto       probe   = graph.AddPass<Probe>("Export prior TAA allocation", xiiGALCommandQueueFlags::Compute, [&](Probe& data, xiiRenderGraphBuilder& builder) {
      data.m_History = builder.ReadTexture(bindings[context.FindPin("TAAResolved", false)].m_hTexture, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data.m_History, xiiGALResourceStateFlags::ShaderResource);
      data.m_Sibling = builder.ReadTexture(siblingBindings[siblingContext.FindPin("TAAResolved", false)].m_hTexture, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data.m_Sibling, xiiGALResourceStateFlags::ShaderResource); }, [](const Probe&, xiiRenderGraphPassContext&) {}, true);
    const auto history = probe.first->m_History;
    graph.EndSetup();
    if (!XII_TEST_RESULT(result))
      return;
    if (!XII_TEST_RESULT(graph.Compile()) || !XII_TEST_RESULT(graph.Execute(environment.GetDevice(), view, &blackboard, &cache)))
      return;
    const auto current = graph.GetExportedTexture(history);
    if (!XII_TEST_BOOL(current != nullptr))
      return;
    XII_TEST_BOOL(current != graph.GetExportedTexture(probe.first->m_Sibling));
    XII_TEST_INT(current->GetDescription().m_Size.width, view->GetRenderResolutionWidth());
    if (iteration == 0)
      original = current;
    else
      XII_TEST_INT(original->GetRefCount(), 1);
  }
}

class xiiTemporalAllocationTestPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTemporalAllocationTestPass, xiiRenderPipelinePass);

public:
  xiiTemporalAntiAliasingPass m_TAA;
  xiiRenderGraphTextureHandle m_Output;
  xiiUInt32                   m_uiInvalidations = 0;
  bool                        m_bHistoryValid   = false;
  bool                        m_bFailAllocation = true;
  void                        GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  void                        InvalidateTemporalHistory() override
  {
    if (m_bHistoryValid)
      ++m_uiInvalidations;
    m_bHistoryValid = false;
    m_TAA.InvalidateTemporalHistory();
  }
  xiiResult Build(xiiRenderPipelinePassContext& context) override
  {
    m_bHistoryValid = true;
    if (m_bFailAllocation)
    {
      // Fail this pass's allocation without changing the device registry used by
      // asynchronous resource loading and other rendering work.
      xiiSharedPtr<xiiGALTexture>      history[2];
      xiiGALTextureCreationDescription description;
      description.m_Type      = xiiGALResourceDimension::Texture2D;
      description.m_Size      = {16, 16};
      description.m_Format    = xiiGALResourceFormat::RGBA16Float;
      description.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
      XII_SUCCEED_OR_RETURN(xiiRenderPassUtils::EnsureTemporalHistoryTextures(history, description, m_bHistoryValid,
                                                                              [](const auto&) -> xiiSharedPtr<xiiGALTexture> { return nullptr; }));
    }
    AddTAAInputs(*context.m_pView, context.m_Graph);
    xiiDynamicArray<xiiRenderPipelinePin> pins;
    m_TAA.GetPins(pins);
    xiiDynamicArray<xiiRenderPipelineBinding> bindings;
    bindings.SetCount(pins.GetCount());
    xiiRenderPipelinePassContext operation{context.m_pView, context.m_Graph, context.m_Blackboard, context.m_Parameters, pins, bindings, context.m_sPassName};
    for (xiiUInt32 index = 0; index < pins.GetCount(); ++index)
      if (pins[index].m_bInput && pins[index].m_Type == xiiRenderPipelinePin::Texture)
        bindings[index].m_hTexture = context.m_Graph.FindTexture(pins[index].m_sName);
    XII_SUCCEED_OR_RETURN(m_TAA.Build(operation));
    m_Output = bindings[operation.FindPin("TAAResolved", false)].m_hTexture;
    struct Export
    {
      xiiRenderGraphTextureHandle m_Texture;
    };
    context.m_Graph.AddPass<Export>("Export TAA test result", xiiGALCommandQueueFlags::Compute, [&](Export& data, xiiRenderGraphBuilder& builder) {
      data.m_Texture = builder.ReadTexture(m_Output, xiiGALResourceStateFlags::ShaderResource);
      builder.ExportTexture(data.m_Texture, xiiGALResourceStateFlags::ShaderResource); }, [](const Export&, xiiRenderGraphPassContext&) {}, true);
    return XII_SUCCESS;
  }
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalAllocationTestPass, 1, xiiRTTIDefaultAllocator<xiiTemporalAllocationTestPass>)

XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST(TemporalMemory, FailedAllocationInvalidatesView)
{
  xiiGPUTestingEnvironment environment;
  if (!XII_TEST_RESULT(environment.Initialize()))
    return;
  xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
  xiiStartup::StartupHighLevelSystems();
  XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
  xiiWorldDescription worldDescription("Temporal allocation failure");
  xiiWorld            world(worldDescription);
  XII_LOCK(world.GetWriteMarker());
  xiiView* view = nullptr;
  world.GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Failing temporal operation", view);
  view->SetViewport({0, 0, 16, 16});
  world.SetWorldSimulationEnabled(false);
  xiiCamera camera;
  camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovY, 60, 0.1f, 100);
  xiiGALTextureCreationDescription target;
  target.m_Type      = xiiGALResourceDimension::Texture2D;
  target.m_Size      = {16, 16};
  target.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  target.m_BindFlags = xiiGALBindFlags::RenderTarget;
  auto texture       = environment.GetDevice()->CreateTexture(target);
  if (!XII_TEST_BOOL(texture != nullptr))
    return;
  view->SetCamera(&camera);
  view->SetRenderTargetView(texture->GetDefaultView(xiiGALTextureViewType::RenderTarget));
  {
    xiiRenderPipelineResourceDescriptor definition;
    definition.AddPass("Allocation failure", "xiiTemporalAllocationTestPass");
    auto pipeline = xiiResourceManager::CreateResource<xiiRenderPipelineResource>("Temporal allocation failure", std::move(definition));
    view->SetRenderPipelineResource(pipeline);
    {
      xiiTestLogInterface   log;
      xiiTestLogSystemScope logScope(&log, true);
      log.ExpectMessage("Failed to build render pipeline", xiiLogMsgType::ErrorMsg);
      world.Update();
      XII_TEST_BOOL(!view->GetRenderGraph()->FindTexture("TAAHistory").IsValid());
    }
    const auto* pass = xiiDynamicCast<const xiiTemporalAllocationTestPass*>(view->GetRenderPipelineInstance().GetPass(0));
    if (!XII_TEST_BOOL(pass != nullptr))
      return;
    XII_TEST_INT(pass->m_uiInvalidations, 1);
    XII_TEST_BOOL(!pass->m_bHistoryValid);
  }
  auto* pass              = const_cast<xiiTemporalAllocationTestPass*>(xiiDynamicCast<const xiiTemporalAllocationTestPass*>(view->GetRenderPipelineInstance().GetPass(0)));
  pass->m_bFailAllocation = false;
  world.Update();
  environment.GetDevice()->WaitIdle();
  XII_TEST_BOOL(view->GetRenderGraph()->GetTextureDescription(pass->m_Output) != nullptr);
  XII_TEST_BOOL(pass != nullptr && pass->m_bHistoryValid && pass->m_uiInvalidations == 1);
}
