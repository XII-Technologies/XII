#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/LightingCombinePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLightingCombinePass, 1, xiiRTTIDefaultAllocator<xiiLightingCombinePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("LightingCombinePass")), XII_MEMBER_PROPERTY("RTShadows", m_bEnableRTShadows)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("RTReflections", m_bEnableRTReflections)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiLightingCombinePass::xiiLightingCombinePass() : xiiRenderPipelinePass("LightingCombinePass") {}
xiiLightingCombinePass::~xiiLightingCombinePass() = default;

namespace
{
  struct LightingData
  {
    xiiRGTextureHandle hAlbedo, hNormal, hMaterial, hDepth, hAO;
    xiiRGTextureHandle hDirShadow, hContactShadow, hRTShadow;
    xiiRGTextureHandle hRTRefl, hSSR, hRTGI;
    xiiRGBufferHandle  hLightGrid, hLightIndex, hCameraBuffer;
    xiiRGTextureHandle hDirectLight, hIndirectLight;
    xiiUInt32 uiW = 1u, uiH = 1u;
    bool bRTShadows = true, bContactShadows = true, bRTRefl = true, bSSR = true;
  };
}

void xiiLightingCombinePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Read all GBuffer + lighting inputs.
  xiiRGTextureHandle hAlbedo, hNormal, hMaterial, hDepth, hAO;
  xiiRGTextureHandle hDirShadow, hContactShadow, hRTShadow, hRTRefl, hSSR, hRTGI;
  xiiRGBufferHandle hLightGrid, hLightIndex, hCamera;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo),         hAlbedo);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal),         hNormal);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferMaterial),       hMaterial);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),     hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_StableAOTexture),       hAO);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectionalShadowAtlas), hDirShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ContactShadowTerm),     hContactShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalShadowMask),     hRTShadow);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalReflections),    hRTRefl);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SSRTexture),            hSSR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTFinalGI),             hRTGI);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightGridBuffer),       hLightGrid);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_LightIndexBuffer),      hLightIndex);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_PerFrameCameraBuffer),  hCamera);

  xiiGALTextureCreationDescription hdrDesc;
  hdrDesc.m_uiWidth = uiW; hdrDesc.m_uiHeight = uiH; hdrDesc.m_uiMipLevels = 1u;
  hdrDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  hdrDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<LightingData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [=, this](LightingData& data, xiiRGBuilder& builder)
    {
      auto read = [&](auto h) { return h.IsValid() ? builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource) : h; };
      auto readB = [&](auto h) { return h.IsValid() ? builder.ReadBuffer(h,  xiiGALResourceStateFlags::ShaderResource) : h; };
      data.hAlbedo        = read(hAlbedo);
      data.hNormal        = read(hNormal);
      data.hMaterial      = read(hMaterial);
      data.hDepth         = read(hDepth);
      data.hAO            = read(hAO);
      data.hDirShadow     = read(hDirShadow);
      data.hContactShadow = read(hContactShadow);
      data.hRTShadow      = read(hRTShadow);
      data.hRTRefl        = read(hRTRefl);
      data.hSSR           = read(hSSR);
      data.hRTGI          = read(hRTGI);
      data.hLightGrid     = readB(hLightGrid);
      data.hLightIndex    = readB(hLightIndex);
      data.hCameraBuffer  = readB(hCamera);
      data.hDirectLight   = builder.WriteTexture("DirectLighting",   hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.hIndirectLight = builder.WriteTexture("IndirectLighting", hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH;
      data.bRTShadows = m_bEnableRTShadows; data.bContactShadows = m_bEnableContactShadows;
      data.bRTRefl = m_bEnableRTReflections; data.bSSR = m_bEnableSSRFallback;
      builder.SetPassAllowMerge(false);
    },
    [](const LightingData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Lighting Combine");
      // Pass 46: DirectLighting.xiiShader — GGX BRDF + cluster lights + cascade shadows.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 47: IndirectLighting.xiiShader — AO-modulated RT GI / IBL + RT/SSR reflections.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer),   pData->hDirectLight);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_IndirectLightingBuffer), pData->hIndirectLight);
}
