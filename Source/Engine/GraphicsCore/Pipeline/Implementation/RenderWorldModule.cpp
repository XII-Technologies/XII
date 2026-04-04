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
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

#include <Core/World/World.h>

XII_IMPLEMENT_WORLD_MODULE(xiiRenderWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

struct xiiRenderWorldModule::DefaultPasses
{
  xiiFrameSetupPass             m_FrameSetup;
  xiiDynamicResolutionPass      m_DynamicResolution;
  xiiPerFrameBufferUploadPass   m_PerFrameBufferUpload;
  xiiSkinningAndMorphPass       m_SkinningAndMorph;
  xiiInstanceTransformPass      m_InstanceTransform;
  xiiLodAndMeshletPass          m_LodAndMeshlet;
  xiiCoarseFrustumCullPass      m_CoarseFrustumCull;
  xiiOccluderDepthPrepass       m_OccluderDepthPrepass;
  xiiHiZBuildPass               m_HiZBuild;
  xiiHiZOcclusionCullPass       m_HiZOcclusionCull;
  xiiDrawCommandBuildPass       m_DrawCommandBuild;
  xiiMainDepthPrepassPass       m_MainDepthPrepass;
  xiiMotionVectorPass           m_MotionVector;
  xiiNormalRoughnessPrepassPass m_NormalRoughnessPrepass;
  xiiShadowCascadeSetupPass     m_ShadowCascadeSetup;
  xiiShadowCasterCullPass       m_ShadowCasterCull;
  xiiDirectionalShadowRenderPass m_DirectionalShadowRender;
  xiiLocalLightShadowPass       m_LocalLightShadow;
  xiiClusterGridAndLightListPass m_ClusterGridAndLightList;
  xiiContactShadowPass          m_ContactShadow;
  xiiDecalPass                  m_Decal;
  xiiAtmosphereLUTPass          m_AtmosphereLUT;
  xiiVolumetricFroxelSetupPass  m_VolumetricFroxelSetup;
  xiiGBufferBasePass            m_GBufferBase;
  xiiEmissiveAuxPass            m_EmissiveAux;
  xiiAmbientOcclusionPass       m_AmbientOcclusion;
  xiiScreenSpaceReflectionsPass m_ScreenSpaceReflections;
  xiiAccelerationStructurePass  m_AccelerationStructure;
  xiiRTShadowPass               m_RTShadow;
  xiiRTReflectionPass           m_RTReflection;
  xiiRTGlobalIlluminationPass   m_RTGlobalIllumination;
  xiiLightingCombinePass        m_LightingCombine;
  xiiVolumetricIntegrationPass  m_VolumetricIntegration;
  xiiAtmosphereCompositePass    m_AtmosphereComposite;
  xiiOpaqueCompositePass        m_OpaqueComposite;
  xiiTransparentPass            m_Transparent;
  xiiParticleVFXPass            m_ParticleVFX;
  xiiExposurePass               m_Exposure;
  xiiTemporalResolvePass        m_TemporalResolve;
  xiiUpscalingPass              m_Upscaling;
  xiiBloomPass                  m_Bloom;
  xiiToneMappingPass            m_ToneMapping;
  xiiColorGradingPass           m_ColorGrading;
  xiiSharpeningPass             m_Sharpening;
  xiiUICompositePass            m_UIComposite;
  xiiReadbackAndTelemetryPass   m_ReadbackAndTelemetry;
  xiiPresentPass                m_Present;
};

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
  m_pDefaultPasses = nullptr;
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
  if (m_pDefaultPasses != nullptr)
    return;

  m_pDefaultPasses = XII_DEFAULT_NEW(DefaultPasses);
}

void xiiRenderWorldModule::BuildDefaultRenderGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  XII_ASSERT_DEV(m_pDefaultPasses != nullptr, "Default pass set has not been initialized.");

  auto AddIfActive = [&view, &graph, &blackboard](auto& pass) {
    if (pass.IsActive())
    {
      pass.AddToGraph(view, graph, blackboard);
    }
  };

  AddIfActive(m_pDefaultPasses->m_FrameSetup);
  AddIfActive(m_pDefaultPasses->m_DynamicResolution);
  AddIfActive(m_pDefaultPasses->m_PerFrameBufferUpload);
  AddIfActive(m_pDefaultPasses->m_SkinningAndMorph);
  AddIfActive(m_pDefaultPasses->m_InstanceTransform);
  AddIfActive(m_pDefaultPasses->m_LodAndMeshlet);
  AddIfActive(m_pDefaultPasses->m_CoarseFrustumCull);
  AddIfActive(m_pDefaultPasses->m_OccluderDepthPrepass);
  AddIfActive(m_pDefaultPasses->m_HiZBuild);
  AddIfActive(m_pDefaultPasses->m_HiZOcclusionCull);
  AddIfActive(m_pDefaultPasses->m_DrawCommandBuild);
  AddIfActive(m_pDefaultPasses->m_MainDepthPrepass);
  AddIfActive(m_pDefaultPasses->m_MotionVector);
  AddIfActive(m_pDefaultPasses->m_NormalRoughnessPrepass);
  AddIfActive(m_pDefaultPasses->m_ShadowCascadeSetup);
  AddIfActive(m_pDefaultPasses->m_ShadowCasterCull);
  AddIfActive(m_pDefaultPasses->m_DirectionalShadowRender);
  AddIfActive(m_pDefaultPasses->m_LocalLightShadow);
  AddIfActive(m_pDefaultPasses->m_ClusterGridAndLightList);
  AddIfActive(m_pDefaultPasses->m_ContactShadow);
  AddIfActive(m_pDefaultPasses->m_Decal);
  AddIfActive(m_pDefaultPasses->m_AtmosphereLUT);
  AddIfActive(m_pDefaultPasses->m_VolumetricFroxelSetup);
  AddIfActive(m_pDefaultPasses->m_GBufferBase);
  AddIfActive(m_pDefaultPasses->m_EmissiveAux);
  AddIfActive(m_pDefaultPasses->m_AmbientOcclusion);
  AddIfActive(m_pDefaultPasses->m_ScreenSpaceReflections);
  AddIfActive(m_pDefaultPasses->m_AccelerationStructure);
  AddIfActive(m_pDefaultPasses->m_RTShadow);
  AddIfActive(m_pDefaultPasses->m_RTReflection);
  AddIfActive(m_pDefaultPasses->m_RTGlobalIllumination);
  AddIfActive(m_pDefaultPasses->m_LightingCombine);
  AddIfActive(m_pDefaultPasses->m_VolumetricIntegration);
  AddIfActive(m_pDefaultPasses->m_AtmosphereComposite);
  AddIfActive(m_pDefaultPasses->m_OpaqueComposite);
  AddIfActive(m_pDefaultPasses->m_Transparent);
  AddIfActive(m_pDefaultPasses->m_ParticleVFX);
  AddIfActive(m_pDefaultPasses->m_Exposure);
  AddIfActive(m_pDefaultPasses->m_TemporalResolve);
  AddIfActive(m_pDefaultPasses->m_Upscaling);
  AddIfActive(m_pDefaultPasses->m_Bloom);
  AddIfActive(m_pDefaultPasses->m_ToneMapping);
  AddIfActive(m_pDefaultPasses->m_ColorGrading);
  AddIfActive(m_pDefaultPasses->m_Sharpening);
  AddIfActive(m_pDefaultPasses->m_UIComposite);
  AddIfActive(m_pDefaultPasses->m_ReadbackAndTelemetry);
  AddIfActive(m_pDefaultPasses->m_Present);
}

// -----------------------------------------------------------------------
// Frame update - extraction

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
// Frame update - graph execution

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
      const xiiResult executeResult = pGraph->Execute(pDevice, pView.Borrow(), &blackboard, &resourceCache);
      XII_ASSERT_DEV(executeResult.Succeeded(), "Render graph execution failed for view '{0}'.", pView->GetName());
    }
  }
}
