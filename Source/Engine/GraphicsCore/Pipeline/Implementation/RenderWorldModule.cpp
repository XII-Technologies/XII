#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule)
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
  // Destroying views releases all per-view GPU resources (ViewPassResources, profiler, etc.)
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
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
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

    // Clear the per-view blackboard at the start of each frame so passes start clean.
    // History data lives in persistent GPU resources inside ViewPassResources, not here.
    blackboard.Clear();
    blackboard.Set(xiiRGBlackboardKeys::k_FrameIndex, static_cast<xiiUInt32>(uiFrameIndex));

    // Reconstruct the graph for this frame.
    pGraph->BeginSetup(uiFrameIndex);

    const xiiView::RenderGraphBuilder& graphBuilder = pView->GetRenderGraphBuilder();
    if (graphBuilder.IsValid())
    {
      // Application-supplied custom graph (e.g. editor, headless, cinematic passes).
      graphBuilder(*pView, *pGraph, blackboard);
    }
    else
    {
      // Standard full-featured pipeline: 12 stage render graph built by xiiView.
      pView->BuildDefaultRenderGraph(*pGraph, blackboard);
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling  = true;
    compileSettings.m_bEnablePassCulling   = true;
    compileSettings.m_bEnableSplitBarriers = true;
    compileSettings.m_bEnableAsyncQueues   = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      // Pass the view's per-view profiler into the executor so every pass gets Duration bracketed.
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache, &pView->GetProfiler());
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());

      // Tick the profiler so it advances its ring and schedules readback on the oldest slot.
      pView->GetProfiler().OnFrameEnd(uiFrameIndex);
    }
  }
}
