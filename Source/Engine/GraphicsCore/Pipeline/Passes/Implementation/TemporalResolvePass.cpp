#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/TemporalResolvePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTemporalResolvePass, 1, xiiRTTIDefaultAllocator<xiiTemporalResolvePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("TemporalResolvePass")), XII_MEMBER_PROPERTY("BlendAlpha", m_fBlendAlpha)->AddAttributes(new xiiDefaultValueAttribute(0.1f)), XII_MEMBER_PROPERTY("Sharpness", m_fSharpness)->AddAttributes(new xiiDefaultValueAttribute(0.25f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiTemporalResolvePass::xiiTemporalResolvePass() : xiiRenderPipelinePass("TemporalResolvePass") {}
xiiTemporalResolvePass::~xiiTemporalResolvePass() = default;

namespace { struct TAAData { xiiRGTextureHandle hCurrent, hHistory, hVelocity, hDepth, hResolved; xiiUInt32 uiW = 1u, uiH = 1u; float fAlpha = 0.1f, fSharpness = 0.25f; }; }

void xiiTemporalResolvePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  // Lazy-create ping-pong history textures.
  for (xiiUInt32 i = 0u; i < 2u; ++i)
  {
    if (!m_pHistoryTexture[i])
    {
      xiiGALTextureCreationDescription td;
      td.m_uiWidth = uiW; td.m_uiHeight = uiH; td.m_uiMipLevels = 1u;
      td.m_Format = xiiGALTextureFormat::R16G16B16A16Float;
      td.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
      m_pHistoryTexture[i] = pDevice->CreateTexture(td);
    }
  }

  xiiRGTextureHandle hHDR, hVelocity, hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), hHDR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVelocity);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  // Compute Halton jitter for this frame and apply to camera projection (view must expose this).
  const xiiUInt32 uiJitter = m_uiFrameIndex % m_uiJitterSequenceLength;
  m_uiFrameIndex++;

  const xiiUInt32 uiCurrent  = m_uiCurrentHistory;
  const xiiUInt32 uiHistSrc  = 1u - uiCurrent;
  m_uiCurrentHistory         = uiHistSrc; // swap for next frame

  xiiGALTextureCreationDescription resolvedDesc;
  resolvedDesc.m_uiWidth = uiW; resolvedDesc.m_uiHeight = uiH; resolvedDesc.m_uiMipLevels = 1u;
  resolvedDesc.m_Format = xiiGALTextureFormat::R16G16B16A16Float;
  resolvedDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<TAAData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, hVelocity, hDepth, uiW, uiH, resolvedDesc, uiCurrent, uiHistSrc, this](TAAData& data, xiiRGBuilder& builder)
    {
      if (hHDR.IsValid())      data.hCurrent  = builder.ReadTexture(hHDR,      xiiGALResourceStateFlags::ShaderResource);
      if (hVelocity.IsValid()) data.hVelocity = builder.ReadTexture(hVelocity, xiiGALResourceStateFlags::ShaderResource);
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::ShaderResource);
      // History src is a persistent texture imported for reading.
      data.hHistory  = builder.ImportTexture("TAAHistory",  m_pHistoryTexture[uiHistSrc],           xiiGALResourceStateFlags::ShaderResource);
      data.hHistory  = builder.ReadTexture(data.hHistory,   xiiGALResourceStateFlags::ShaderResource);
      // Resolved output written into the history dst slot (ping-pong).
      data.hResolved = builder.ImportTexture("TAAResolved", m_pHistoryTexture[uiCurrent], xiiGALResourceStateFlags::UnorderedAccess);
      data.hResolved = builder.WriteTexture(data.hResolved, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.fAlpha = m_fBlendAlpha; data.fSharpness = m_fSharpness;
      builder.SetPassSideEffects(true); // writes persistent history
    },
    [](const TAAData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("TAA Resolve");
      // TAA.xiiShader: YCoCg neighbourhood AABB clamping + velocity-weighted reprojection.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), pData->hResolved);
}
