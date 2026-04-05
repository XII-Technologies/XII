#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Shaders/Pipeline/Passes/DynamicResolution/DynamicResolutionConstants.h>

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

void xiiRenderWorldModule::Initialize()
{
  // Register ExtractRenderData (concurrent, runs on async worker threads per component manager).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    description.m_Phase                     = xiiWorldUpdatePhase::Async;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete).
  {
    auto description                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    description.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    description.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(description);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  m_Views.Clear();

  m_uiRenderFrameIndex = 0;
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

xiiView* xiiRenderWorldModule::CreateView(xiiStringView sName)
{
  xiiUniquePtr<xiiView> pView = XII_DEFAULT_NEW(xiiView);
  pView->SetName(sName);
  xiiView* pRet = pView.Borrow();
  m_Views.PushBack(std::move(pView));
  return pRet;
}

void xiiRenderWorldModule::DestroyView(xiiView* pView)
{
  for (xiiUInt32 i = 0; i < m_Views.GetCount(); ++i)
  {
    if (m_Views[i].Borrow() == pView)
    {
      m_Views.RemoveAtAndCopy(i);
      return;
    }
  }
}

void xiiRenderWorldModule::BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  auto [pFrameSetupData, hFrameSetupPass] = graph.AddPass<PassData::FrameSetupPassData>("FrameSetup", xiiGALCommandQueueFlags::Graphics,
                                                                                        xiiMakeDelegate(&xiiRenderWorldModule::SetupFrameSetupPass, this),
                                                                                        xiiMakeDelegate(&xiiRenderWorldModule::ExecuteFrameSetupPass, this));

  auto [pDynamicResolutionData, hDynamicResolutionPass] = graph.AddPass<PassData::DynamicResolutionPassData>("DynamicResolution", xiiGALCommandQueueFlags::Compute,
                                                                                                             xiiMakeDelegate(&xiiRenderWorldModule::SetupDynamicResolutionPass, this),
                                                                                                             xiiMakeDelegate(&xiiRenderWorldModule::ExecuteDynamicResolutionPass, this));
}

void xiiRenderWorldModule::ExtractRenderData(const xiiWorldModule::UpdateContext& context)
{
  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiExtractedRenderData* pExtractedData = pView->GetExtractedRenderData();
    pExtractedData->Clear();

    xiiMsgExtractRenderData msg;
    msg.m_pView                = pView.Borrow();
    msg.m_pExtractedRenderData = pExtractedData;

    // Broadcast to all objects; each object routes to matching component message handlers.
    {
      XII_LOCK(GetWorld()->GetReadMarker());
      for (auto it = GetWorld()->GetObjects(); it.IsValid(); ++it)
      {
        it->SendMessage(msg);
      }
    }

    // Flatten concurrent batches, then radix-sort each category by sort key.
    pExtractedData->SortAndBatches();
  }
}

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt64 uiFrameIndex = m_uiRenderFrameIndex++;

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph        = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard    = pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = pView->GetResourceCache();

    // Clear the per-view blackboard at the start of each frame so passes start with a clean slate.
    // History data must live inside persistent GPU buffers owned by each pass.
    blackboard.Clear();
    blackboard.Set(xiiMakeHashedString("FrameIndex"), static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    const xiiView::RenderGraphBuilder& graphBuilder = pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      graphBuilder(*pView, *pGraph, blackboard);
    }
    else
    {
      BuildDefaultRenderGraph(*pView, *pGraph, blackboard);
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());
    }
  }
}

void xiiRenderWorldModule::SetupFrameSetupPass(PassData::FrameSetupPassData& data, xiiRGBuilder& builder)
{
  // No resources for this pass, it just updates some global constants.
  // Wrap around to prevent floating point issues. A wrap around of 1000 allows all frequencies with 3 digits after the decimal.
  constexpr double fWrapAround = 1000.0;
  data.m_fDeltaTime            = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds());
  data.m_fGlobalTime           = static_cast<float>(xiiMath::Mod(xiiClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), fWrapAround));
  data.m_fWorldTime            = static_cast<float>(xiiMath::Mod(GetWorld()->GetClock().GetAccumulatedTime().GetSeconds(), fWrapAround));
  data.m_ExposureControl       = xiiExposureControl::EyeAdaptationTemporal;

  builder.SetPassSideEffects(true); // Ensures this pass runs even if no resources are read/written, since it updates global time constants used by other passes.
  builder.SetPassAllowMerge(false); // Do not merge with other passes since this is a logical "start" of the frame and we want it to be a distinct point in GPU profiling.
}

void xiiRenderWorldModule::ExecuteFrameSetupPass(const PassData::FrameSetupPassData& data, xiiRGPassContext& context)
{
  XII_IGNORE_UNUSED(data);
  XII_IGNORE_UNUSED(context);
}

void xiiRenderWorldModule::SetupDynamicResolutionPass(PassData::DynamicResolutionPassData& data, xiiRGBuilder& builder)
{
  if (!m_PersistentFrameResources.m_DynamicResolution.m_pTimingInputBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiDynamicResolutionPassData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;

    m_PersistentFrameResources.m_DynamicResolution.m_pTimingInputBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  if (!m_PersistentFrameResources.m_DynamicResolution.m_pCameraVelocityInputBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(PersistentFrameResources::DynamicResolution::CameraVelocityData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;

    m_PersistentFrameResources.m_DynamicResolution.m_pCameraVelocityInputBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  if (!m_PersistentFrameResources.m_DynamicResolution.m_pResolutionScalingBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(PersistentFrameResources::DynamicResolution::ResolutionScalingData);
    description.m_uiSize              = description.m_uiElementByteStride;
    description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_PersistentFrameResources.m_DynamicResolution.m_pResolutionScalingBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_fFrameDeltaTimeMs     = static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()) * 1000.0f;
  data.m_fTargetFrameTimeMs    = 16.67f; // Target 60 FPS, this would be adjustable and possibly dynamic based on performance metrics.
  data.m_fMininimumRenderScale = 0.5f;   // Don't go below 50% resolution to maintain some level of visual fidelity.
  data.m_fMaximumRenderScale   = 1.0f;   // Don't upscale above native resolution to avoid blurriness.

  data.m_hTimingInputBuffer             = builder.ImportBuffer("DynamicResolution_Timing", m_PersistentFrameResources.m_DynamicResolution.m_pTimingInputBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hTimingInputBuffer             = builder.ReadBuffer(data.m_hTimingInputBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCameraVelocityInputBuffer     = builder.ImportBuffer("DynamicResolution_Velocity", m_PersistentFrameResources.m_DynamicResolution.m_pCameraVelocityInputBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCameraVelocityInputBuffer     = builder.ReadBuffer(data.m_hCameraVelocityInputBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hResolutionScalingOutputBuffer = builder.ImportBuffer("DynamicResolution_Scaling", m_PersistentFrameResources.m_DynamicResolution.m_pResolutionScalingBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hResolutionScalingOutputBuffer = builder.WriteBuffer(data.m_hResolutionScalingOutputBuffer, xiiGALResourceStateFlags::UnorderedAccess);
}

void xiiRenderWorldModule::ExecuteDynamicResolutionPass(const PassData::DynamicResolutionPassData& data, xiiRGPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("Dynamic Resolution Scaling");

  cmd.DispatchCompute({1U, 1U, 1U}); // Only one threadgroup is needed since this shader just outputs a single scaling factor for the whole frame.
  cmd.EndDebugGroup();
}
