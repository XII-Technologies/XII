#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/RenderGraphResourceCache.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Core/World/World.h>

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// Static pass list — outlives any individual world.
xiiDynamicArray<xiiUniquePtr<xiiRenderPipelinePass>> xiiRenderWorldModule::s_PipelinePasses;

// -----------------------------------------------------------------------
// Construction / destruction

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiRenderWorldModule::~xiiRenderWorldModule() = default;

// -----------------------------------------------------------------------
// WorldModule overrides

void xiiRenderWorldModule::Initialize()
{
  // Register ExtractRenderData (concurrent, runs on async worker threads per component manager)
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    desc.m_Phase                     = xiiWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(desc);
  }

  // Register ExecuteRenderGraphs (post-async, single-threaded, after all extraction is complete)
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExecuteRenderGraphs, this);
    desc.m_Phase                     = xiiWorldUpdatePhase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(desc);
  }
}

void xiiRenderWorldModule::Deinitialize()
{
  m_Views.Clear();
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

// -----------------------------------------------------------------------
// View management

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

// -----------------------------------------------------------------------
// Pass registry

void xiiRenderWorldModule::RegisterPass(xiiUniquePtr<xiiRenderPipelinePass> pPass)
{
  XII_ASSERT_DEV(pPass != nullptr, "Cannot register a null pipeline pass.");
  s_PipelinePasses.PushBack(std::move(pPass));
}

void xiiRenderWorldModule::UnregisterPass(xiiStringView sName)
{
  for (xiiUInt32 i = 0; i < s_PipelinePasses.GetCount(); ++i)
  {
    if (s_PipelinePasses[i]->GetName() == sName)
    {
      s_PipelinePasses.RemoveAtAndCopy(i);
      return;
    }
  }
}

xiiArrayPtr<xiiRenderPipelinePass* const> xiiRenderWorldModule::GetRegisteredPasses()
{
  // Build a temporary raw-pointer view. Callers must not store this across frames.
  static thread_local xiiDynamicArray<xiiRenderPipelinePass*> s_RawPtrs;
  s_RawPtrs.Clear();
  s_RawPtrs.Reserve(s_PipelinePasses.GetCount());
  for (auto& pPass : s_PipelinePasses)
    s_RawPtrs.PushBack(pPass.Borrow());
  return s_RawPtrs;
}

// -----------------------------------------------------------------------
// Frame update — extraction

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

    // Broadcast to all component managers concurrently (world handles task fan-out).
    GetWorld()->BroadcastMessage(msg);

    // Flatten concurrent batches, then radix-sort each category by sort key.
    pExtractedData->SortAndBatches();
  }
}

// -----------------------------------------------------------------------
// Frame update — graph execution

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  const xiiUInt64 uiFrameIndex = GetWorld()->GetClock().GetAccumulatedTime().GetTicks();

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph*              pGraph       = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard   = pView->GetBlackboard();
    xiiRenderGraphResourceCache& resourceCache = pView->GetResourceCache();

    // Clear the per-view blackboard at the start of each frame so passes start with a clean slate.
    // History data must live inside persistent GPU buffers owned by each pass.
    blackboard.Clear();

    // Reconstruct the graph for this frame. Every registered pass adds its nodes in order.
    pGraph->BeginSetup(uiFrameIndex);

    for (const auto& pPass : s_PipelinePasses)
    {
      if (pPass->IsActive())
      {
        pPass->AddToGraph(*pView, *pGraph, blackboard);
      }
    }

    pGraph->EndSetup();

    xiiRGCompileSettings compileSettings;
    compileSettings.m_bEnableGPUProfiling = true;

    if (pGraph->Compile(compileSettings).Succeeded())
    {
      pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
    }
  }
}
