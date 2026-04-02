#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/MainDepthPrepassPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMainDepthPrepassPass, 1, xiiRTTIDefaultAllocator<xiiMainDepthPrepassPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("MainDepthPrepassPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiMainDepthAutoReg { xiiMainDepthAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiMainDepthPrepassPass)); } }; static xiiMainDepthAutoReg s_AutoReg; }

xiiMainDepthPrepassPass::xiiMainDepthPrepassPass() : xiiRenderPipelinePass("MainDepthPrepassPass") {}
xiiMainDepthPrepassPass::~xiiMainDepthPrepassPass() = default;

namespace { struct MainDepthData { xiiRGBufferHandle hDrawArgs; xiiRGBufferHandle hDrawCount; xiiRGTextureHandle hSceneDepth; xiiUInt32 uiWidth = 1u, uiHeight = 1u; }; }

void xiiMainDepthPrepassPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiGALTextureCreationDescription depthDesc;
  depthDesc.m_uiWidth     = uiW;  depthDesc.m_uiHeight    = uiH;  depthDesc.m_uiMipLevels = 1u;
  depthDesc.m_Format      = xiiGALTextureFormat::D32Float;
  depthDesc.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hDrawArgs, hDrawCount;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawCountBuffer),      hDrawCount);

  auto [pData, hPass] = graph.AddPass<MainDepthData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDrawArgs, hDrawCount, depthDesc, uiW, uiH](MainDepthData& data, xiiRGBuilder& builder)
    {
      if (hDrawArgs.IsValid())  data.hDrawArgs  = builder.ReadBuffer(hDrawArgs,  xiiGALResourceStateFlags::IndirectArgument);
      if (hDrawCount.IsValid()) data.hDrawCount = builder.ReadBuffer(hDrawCount, xiiGALResourceStateFlags::IndirectArgument);
      data.hSceneDepth = builder.WriteTexture("SceneDepth", depthDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiWidth = uiW; data.uiHeight = uiH;
    },
    [](const MainDepthData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Main Depth Prepass");
      xiiGALTexture* pDepth = context.GetTexture(data.hSceneDepth);
      xiiGALBuffer*  pArgs  = context.GetBuffer(data.hDrawArgs);
      cmd.ClearDepthStencilView(pDepth, xiiGALClearFlags::Depth, 0.0f, 0u);
      // Indirect depth-only draw per material bin — DepthPrepass.xiiShader (VS only, no PS).
      if (pArgs) cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), pData->hSceneDepth);
}
