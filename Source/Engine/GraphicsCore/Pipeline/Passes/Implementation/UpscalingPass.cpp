#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/UpscalingPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUpscalingPass, 1, xiiRTTIDefaultAllocator<xiiUpscalingPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("UpscalingPass")), XII_MEMBER_PROPERTY("SharpeningStrength", m_fSharpeningStrength)->AddAttributes(new xiiDefaultValueAttribute(0.4f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiUpscaleAutoReg { xiiUpscaleAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiUpscalingPass)); } }; static xiiUpscaleAutoReg s_AutoReg; }

xiiUpscalingPass::xiiUpscalingPass() : xiiRenderPipelinePass("UpscalingPass") {}
xiiUpscalingPass::~xiiUpscalingPass() = default;

namespace { struct UpscaleData { xiiRGTextureHandle hTAAResolved, hUpscaled; xiiUInt32 uiSrcW = 1u, uiSrcH = 1u, uiDstW = 1u, uiDstH = 1u; float fSharpening = 0.4f; }; }

void xiiUpscalingPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiSrcW = 1920u, uiSrcH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiSrcW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiSrcH);

  // Output is always at native display resolution (from swap chain).
  const xiiViewData& vd  = view.GetData();
  const xiiUInt32 uiDstW = static_cast<xiiUInt32>(vd.m_ViewPortRect.width);
  const xiiUInt32 uiDstH = static_cast<xiiUInt32>(vd.m_ViewPortRect.height);

  xiiRGTextureHandle hTAA;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hTAA);

  xiiGALTextureCreationDescription upscaleDesc;
  upscaleDesc.m_uiWidth = uiDstW; upscaleDesc.m_uiHeight = uiDstH; upscaleDesc.m_uiMipLevels = 1u;
  upscaleDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  upscaleDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<UpscaleData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hTAA, upscaleDesc, uiSrcW, uiSrcH, uiDstW, uiDstH, this](UpscaleData& data, xiiRGBuilder& builder)
    {
      if (hTAA.IsValid()) data.hTAAResolved = builder.ReadTexture(hTAA, xiiGALResourceStateFlags::ShaderResource);
      data.hUpscaled  = builder.WriteTexture("UpscaledColor", upscaleDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiSrcW = uiSrcW; data.uiSrcH = uiSrcH; data.uiDstW = uiDstW; data.uiDstH = uiDstH;
      data.fSharpening = m_fSharpeningStrength;
    },
    [](const UpscaleData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("CAS Upscaling");
      // CASUpscale.xiiShader: Contrast-Adaptive Sharpening spatial upscaler.
      // One thread per output pixel; sharpening strength parameterised by fSharpening.
      cmd.Dispatch((data.uiDstW + 7u) / 8u, (data.uiDstH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor), pData->hUpscaled);
}
