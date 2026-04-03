#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/NormalRoughnessPrepassPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNormalRoughnessPrepassPass, 1, xiiRTTIDefaultAllocator<xiiNormalRoughnessPrepassPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("NormalRoughnessPrepassPass")), XML_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiNormalRoughnessPrepassPass::xiiNormalRoughnessPrepassPass() : xiiRenderPipelinePass("NormalRoughnessPrepassPass") {}
xiiNormalRoughnessPrepassPass::~xiiNormalRoughnessPrepassPass() = default;

namespace { struct NRPrepassData { xiiRGTextureHandle hDepth; xiiRGTextureHandle hNormalRoughness; xiiUInt32 uiWidth = 1u, uiHeight = 1u; }; }

void xiiNormalRoughnessPrepassPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  if (!m_bEnabled) return;

  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription nrDesc;
  nrDesc.m_uiWidth = uiW; nrDesc.m_uiHeight = uiH; nrDesc.m_uiMipLevels = 1u;
  nrDesc.m_Format  = xiiGALTextureFormat::R8G8B8A8UNorm; // oct(normal XY) + roughness + AO
  nrDesc.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto [pData, hPass] = graph.AddPass<NRPrepassData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hDrawArgs, nrDesc, uiW, uiH](NRPrepassData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth          = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::DepthRead);
      if (hDrawArgs.IsValid()) builder.ReadBuffer(hDrawArgs, xiiGALResourceStateFlags::IndirectArgument);
      data.hNormalRoughness = builder.WriteTexture("NormalRoughness", nrDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiWidth = uiW; data.uiHeight = uiH;
    },
    [](const NRPrepassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Normal-Roughness Prepass");
      // NormalRoughnessPrepass.xiiShader: indirect draw, outputs oct-normal + roughness.
      cmd.DrawIndexedIndirect(context.GetBuffer(data.hDepth), 0u); // reuse draw args from indirect buffer
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_NormalRoughnessBuffer), pData->hNormalRoughness);
}
