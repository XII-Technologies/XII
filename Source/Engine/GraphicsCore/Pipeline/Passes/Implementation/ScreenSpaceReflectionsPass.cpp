#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ScreenSpaceReflectionsPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenSpaceReflectionsPass, 1, xiiRTTIDefaultAllocator<xiiScreenSpaceReflectionsPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ScreenSpaceReflectionsPass")), XII_MEMBER_PROPERTY("MaxSteps", m_uiMaxRaySteps)->AddAttributes(new xiiDefaultValueAttribute(64u)), XII_MEMBER_PROPERTY("RoughnessThreshold", m_fRoughnessThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.5f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiScreenSpaceReflectionsPass::xiiScreenSpaceReflectionsPass() : xiiRenderPipelinePass("ScreenSpaceReflectionsPass") {}
xiiScreenSpaceReflectionsPass::~xiiScreenSpaceReflectionsPass() = default;

namespace { struct SSRData { xiiRGTextureHandle hDepth, hNR, hHiZ, hSSR; xiiUInt32 uiW = 1u, uiH = 1u, uiMaxSteps = 64u; float fRoughThresh = 0.5f, fThickness = 0.05f; }; }

void xiiScreenSpaceReflectionsPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hNR, hHiZ;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), hNR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid),            hHiZ);

  xiiGALTextureCreationDescription ssrDesc;
  ssrDesc.m_uiWidth = uiW; ssrDesc.m_uiHeight = uiH; ssrDesc.m_uiMipLevels = 1u;
  ssrDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  ssrDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<SSRData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hNR, hHiZ, ssrDesc, uiW, uiH, this](SSRData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      if (hNR.IsValid())    data.hNR    = builder.ReadTexture(hNR,    xiiGALResourceStateFlags::ShaderResource);
      if (hHiZ.IsValid())   data.hHiZ   = builder.ReadTexture(hHiZ,   xiiGALResourceStateFlags::ShaderResource);
      data.hSSR           = builder.WriteTexture("SSRTerm", ssrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH;
      data.uiMaxSteps     = m_uiMaxRaySteps;
      data.fRoughThresh   = m_fRoughnessThreshold;
      data.fThickness     = m_fThickness;
    },
    [](const SSRData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Screen-Space Reflections");
      // SSR.xiiShader: hierarchical DDA ray march via Hi-Z pyramid.
      // Skip pixels with roughness > fRoughThresh — they fall back to IBL.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture), pData->hSSR);
}
