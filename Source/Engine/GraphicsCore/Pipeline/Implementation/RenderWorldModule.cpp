#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>
#include <GraphicsCore/Pipeline/Passes/AccelerationStructurePass.h>
#include <GraphicsCore/Pipeline/Passes/AmbientOcclusionPass.h>
#include <GraphicsCore/Pipeline/Passes/AtmosphereCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/AtmosphereLUTPass.h>
#include <GraphicsCore/Pipeline/Passes/BloomPass.h>
#include <GraphicsCore/Pipeline/Passes/ClusterGridAndLightListPass.h>
#include <GraphicsCore/Pipeline/Passes/CoarseFrustumCullPass.h>
#include <GraphicsCore/Pipeline/Passes/ColorGradingPass.h>
#include <GraphicsCore/Pipeline/Passes/ContactShadowPass.h>
#include <GraphicsCore/Pipeline/Passes/DecalPass.h>
#include <GraphicsCore/Pipeline/Passes/DirectionalShadowRenderPass.h>
#include <GraphicsCore/Pipeline/Passes/DrawCommandBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/DynamicResolutionPass.h>
#include <GraphicsCore/Pipeline/Passes/EmissiveAuxPass.h>
#include <GraphicsCore/Pipeline/Passes/ExposurePass.h>
#include <GraphicsCore/Pipeline/Passes/FrameSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/GBufferBasePass.h>
#include <GraphicsCore/Pipeline/Passes/HiZBuildPass.h>
#include <GraphicsCore/Pipeline/Passes/HiZOcclusionCullPass.h>
#include <GraphicsCore/Pipeline/Passes/InstanceTransformPass.h>
#include <GraphicsCore/Pipeline/Passes/LightingCombinePass.h>
#include <GraphicsCore/Pipeline/Passes/LocalLightShadowPass.h>
#include <GraphicsCore/Pipeline/Passes/LodAndMeshletPass.h>
#include <GraphicsCore/Pipeline/Passes/MainDepthPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/MotionVectorPass.h>
#include <GraphicsCore/Pipeline/Passes/NormalRoughnessPrepassPass.h>
#include <GraphicsCore/Pipeline/Passes/OccluderDepthPrepass.h>
#include <GraphicsCore/Pipeline/Passes/OpaqueCompositePass.h>
#include <GraphicsCore/Pipeline/Passes/ParticleVFXPass.h>
#include <GraphicsCore/Pipeline/Passes/PerFrameBufferUploadPass.h>
#include <GraphicsCore/Pipeline/Passes/PresentPass.h>
#include <GraphicsCore/Pipeline/Passes/RTGlobalIlluminationPass.h>
#include <GraphicsCore/Pipeline/Passes/RTReflectionPass.h>
#include <GraphicsCore/Pipeline/Passes/RTShadowPass.h>
#include <GraphicsCore/Pipeline/Passes/ReadbackAndTelemetryPass.h>
#include <GraphicsCore/Pipeline/Passes/ScreenSpaceReflectionsPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCascadeSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/ShadowCasterCullPass.h>
#include <GraphicsCore/Pipeline/Passes/SharpeningPass.h>
#include <GraphicsCore/Pipeline/Passes/SkinningAndMorphPass.h>
#include <GraphicsCore/Pipeline/Passes/TemporalResolvePass.h>
#include <GraphicsCore/Pipeline/Passes/ToneMappingPass.h>
#include <GraphicsCore/Pipeline/Passes/TransparentPass.h>
#include <GraphicsCore/Pipeline/Passes/UICompositePass.h>
#include <GraphicsCore/Pipeline/Passes/UpscalingPass.h>
#include <GraphicsCore/Pipeline/Passes/VolumetricFroxelSetupPass.h>
#include <GraphicsCore/Pipeline/Passes/VolumetricIntegrationPass.h>
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

// -----------------------------------------------------------------------
// Construction / destruction

xiiRenderWorldModule::xiiRenderWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
  InitializeDefaultPasses();
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
  m_DefaultPasses.Clear();
  m_uiRenderFrameIndex = 0;
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

void xiiRenderWorldModule::InitializeDefaultPasses()
{
  if (!m_DefaultPasses.IsEmpty())
    return;

  m_DefaultPasses.Reserve(47);

  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiFrameSetupPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiDynamicResolutionPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiPerFrameBufferUploadPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiSkinningAndMorphPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiInstanceTransformPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiLodAndMeshletPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiCoarseFrustumCullPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiOccluderDepthPrepass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiHiZBuildPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiHiZOcclusionCullPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiDrawCommandBuildPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiMainDepthPrepassPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiMotionVectorPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiNormalRoughnessPrepassPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiShadowCascadeSetupPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiShadowCasterCullPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiDirectionalShadowRenderPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiLocalLightShadowPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiClusterGridAndLightListPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiContactShadowPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiDecalPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiAtmosphereLUTPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiVolumetricFroxelSetupPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiGBufferBasePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiEmissiveAuxPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiAmbientOcclusionPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiScreenSpaceReflectionsPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiAccelerationStructurePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiRTShadowPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiRTReflectionPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiRTGlobalIlluminationPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiLightingCombinePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiVolumetricIntegrationPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiAtmosphereCompositePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiOpaqueCompositePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiTransparentPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiParticleVFXPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiExposurePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiTemporalResolvePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiUpscalingPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiBloomPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiToneMappingPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiColorGradingPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiSharpeningPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiUICompositePass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiReadbackAndTelemetryPass));
  m_DefaultPasses.PushBack(XII_DEFAULT_NEW(xiiPresentPass));
}

void xiiRenderWorldModule::BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  for (const auto& pPass : m_DefaultPasses)
  {
    if (pPass->IsActive())
    {
      pPass->AddToGraph(view, graph, blackboard);
    }
  }
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

// -----------------------------------------------------------------------
// Frame update — graph execution

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

    xiiRenderGraph*              pGraph       = pView->GetRenderGraph();
    xiiRenderGraphBlackboard&    blackboard   = pView->GetBlackboard();
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
      pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
    }
  }
}
