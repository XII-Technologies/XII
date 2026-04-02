#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ToneMappingPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiToneMappingPass, 1, xiiRTTIDefaultAllocator<xiiToneMappingPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ToneMappingPass")), XII_MEMBER_PROPERTY("ExposureBias", m_fExposureBias)->AddAttributes(new xiiDefaultValueAttribute(0.0f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiToneMappingAutoReg { xiiToneMappingAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiToneMappingPass)); } }; static xiiToneMappingAutoReg s_AutoReg; }

xiiToneMappingPass::xiiToneMappingPass() : xiiRenderPipelinePass("ToneMappingPass") {}
xiiToneMappingPass::~xiiToneMappingPass() = default;

namespace { struct TonemapData { xiiRGTextureHandle hHDR, hBloom, hExposure, hLDR; xiiUInt32 uiW = 1u, uiH = 1u; xiiUInt8 uiOperator = 0u; float fBias = 0.0f; }; }

void xiiToneMappingPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hHDR, hBloom; xiiRGBufferHandle hExposure;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor),   hHDR);
  if (!hHDR.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hHDR);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_BloomTexture),    hBloom);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_CurrentExposure), hExposure);

  xiiGALTextureCreationDescription ldrDesc;
  ldrDesc.m_uiWidth = uiW; ldrDesc.m_uiHeight = uiH; ldrDesc.m_uiMipLevels = 1u;
  ldrDesc.m_Format  = xiiGALTextureFormat::R8G8B8A8UNorm;
  ldrDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<TonemapData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hHDR, hBloom, hExposure, ldrDesc, uiW, uiH, this](TonemapData& data, xiiRGBuilder& builder)
    {
      if (hHDR.IsValid())      data.hHDR      = builder.ReadTexture(hHDR,      xiiGALResourceStateFlags::ShaderResource);
      if (hBloom.IsValid())    data.hBloom    = builder.ReadTexture(hBloom,    xiiGALResourceStateFlags::ShaderResource);
      if (hExposure.IsValid()) data.hExposure = builder.ReadBuffer(hExposure,  xiiGALResourceStateFlags::ShaderResource);
      data.hLDR      = builder.WriteTexture("LDRSceneColor", ldrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiOperator = static_cast<xiiUInt8>(m_Operator); data.fBias = m_fExposureBias;
    },
    [](const TonemapData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Tone Mapping");
      // ToneMapping.xiiShader: HDR * exposure + bloom additive → ACES/AgX/Reinhard → LDR.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_LDRSceneColor), pData->hLDR);
}
