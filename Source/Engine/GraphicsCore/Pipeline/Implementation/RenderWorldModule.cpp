#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Core/World/World.h>

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
  DiscoverPipelinePasses();

  // Register ExtractRenderData
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiRenderWorldModule::ExtractRenderData, this);
    desc.m_Phase                     = xiiWorldUpdatePhase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;
    RegisterUpdateFunction(desc);
  }

  // Register ExecuteRenderGraphs
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

  for (auto* pPass : m_PipelinePasses)
  {
    pPass->GetDynamicRTTI()->GetAllocator()->Deallocate(pPass);
  }
  m_PipelinePasses.Clear();
}

void xiiRenderWorldModule::OnSimulationStarted()
{
}

void xiiRenderWorldModule::DiscoverPipelinePasses()
{
  xiiRTTI::ForEachDerivedType<xiiRenderPipelinePass>([&](const xiiRTTI* pRtti)
    {
      if (pRtti->GetAllocator() && pRtti->GetAllocator()->CanAllocate())
      {
        xiiRenderPipelinePass* pPass = pRtti->GetAllocator()->Allocate<xiiRenderPipelinePass>();
        m_PipelinePasses.PushBack(pPass);
      }
    });
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

    // Ideally, we'd send the message to all component managers. 
    // They can then concurrently process all their components and push to pExtractedData.
    GetWorld()->BroadcastMessage(msg);

    // After extraction, lock down sorting.
    pExtractedData->SortAndBatches();
  }
}

void xiiRenderWorldModule::ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  if (!pDevice)
    return;

  for (auto& pView : m_Views)
  {
    if (!pView->IsValid())
      continue;

    xiiRenderGraph* pGraph = pView->GetRenderGraph();

    // Reconstruct the graph for the current frame
    pGraph->BeginSetup(GetWorld()->GetClock().GetAccumulatedTime().GetSeconds()); // using time as frame for now

    // Evaluate dynamic pipeline passes to contribute passes to this graph
    for (xiiRenderPipelinePass* pPass : m_PipelinePasses)
    {
      if (pPass->IsActive())
      {
        pPass->AddToGraph(*pView, *pGraph);
      }
    }

    pGraph->EndSetup();

    if (pGraph->Compile().Succeeded())
    {
      // Execute compiled graph to GPU
      pGraph->Execute(pDevice, pView.Borrow(), &m_Blackboard, &m_ResourceCache);
    }
  }
}
