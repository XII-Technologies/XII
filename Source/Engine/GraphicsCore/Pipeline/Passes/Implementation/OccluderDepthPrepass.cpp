#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/OccluderDepthPrepass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOccluderDepthPrepass, 1, xiiRTTIDefaultAllocator<xiiOccluderDepthPrepass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("OccluderDepthPrepass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiOccluderDepthPrepass::xiiOccluderDepthPrepass() : xiiRenderPipelinePass("OccluderDepthPrepass") {}
xiiOccluderDepthPrepass::~xiiOccluderDepthPrepass() = default;

namespace { struct OccluderDepthData { xiiRGTextureHandle hDepth; xiiUInt32 uiWidth = 1u, uiHeight = 1u; }; }

void xiiOccluderDepthPrepass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiRW = 1920u, uiRH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiRW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiRH);

  const xiiUInt32 uiW = xiiMath::Max(1u, uiRW  / m_uiResolutionDivisor);
  const xiiUInt32 uiH = xiiMath::Max(1u, uiRH / m_uiResolutionDivisor);

  xiiGALTextureCreationDescription depthDesc;
  depthDesc.m_uiWidth     = uiW;
  depthDesc.m_uiHeight    = uiH;
  depthDesc.m_uiMipLevels = 1u;
  depthDesc.m_Format      = xiiGALTextureFormat::D32Float;
  depthDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<OccluderDepthData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [depthDesc, uiW, uiH](OccluderDepthData& data, xiiRGBuilder& builder)
    {
      data.hDepth   = builder.WriteTexture("OccluderDepth", depthDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiWidth  = uiW;
      data.uiHeight = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const OccluderDepthData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Occluder Depth Prepass");
      xiiGALTexture* pDepth = context.GetTexture(data.hDepth);
      cmd.ClearDepthStencilView(pDepth, xiiGALClearFlags::Depth, 0.0f, 0u); // Reversed-Z: clear to 0
      // Draw occluder meshes via dedicated low-poly draw list — geometry-only, no alpha test.
      // OccluderDepth.xiiShader used (depth-only vertex transform).
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_OccluderDepthTexture), pData->hDepth);
}
