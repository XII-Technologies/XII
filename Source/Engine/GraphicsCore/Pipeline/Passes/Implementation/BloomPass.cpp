#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/BloomPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBloomPass, 1, xiiRTTIDefaultAllocator<xiiBloomPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("BloomPass")), XII_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new xiiDefaultValueAttribute(1.0f)), XII_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new xiiDefaultValueAttribute(0.04f)), XII_MEMBER_PROPERTY("MipLevels", m_uiMipLevels)->AddAttributes(new xiiDefaultValueAttribute(7u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiBloomAutoReg { xiiBloomAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiBloomPass)); } }; static xiiBloomAutoReg s_AutoReg; }

xiiBloomPass::xiiBloomPass() : xiiRenderPipelinePass("BloomPass") {}
xiiBloomPass::~xiiBloomPass() = default;

namespace { struct BloomData { xiiRGTextureHandle hInput, hBloomOutput; xiiUInt32 uiW = 1u, uiH = 1u, uiMips = 7u; float fThreshold = 1.0f, fKnee = 0.5f, fIntensity = 0.04f; }; }

void xiiBloomPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hInput;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_UpscaledColor), hInput);
  if (!hInput.IsValid()) blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_TAAResolvedColor), hInput);

  // Bloom chain: mip pyramid R16G16B16A16Float.
  xiiGALTextureCreationDescription bloomDesc;
  bloomDesc.m_uiWidth = uiW; bloomDesc.m_uiHeight = uiH; bloomDesc.m_uiMipLevels = m_uiMipLevels;
  bloomDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  bloomDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<BloomData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hInput, bloomDesc, uiW, uiH, this](BloomData& data, xiiRGBuilder& builder)
    {
      if (hInput.IsValid()) data.hInput = builder.ReadTexture(hInput, xiiGALResourceStateFlags::ShaderResource);
      data.hBloomOutput = builder.WriteTexture("BloomTexture", bloomDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiMips = m_uiMipLevels;
      data.fThreshold = m_fThreshold; data.fKnee = m_fKnee; data.fIntensity = m_fIntensity;
    },
    [](const BloomData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Bloom");
      // Pass 57: BloomPrefilter.xiiShader — soft-knee bright-pass at full resolution.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      // Pass 58: BloomDownsample.xiiShader — 13-tap filter per mip level.
      xiiUInt32 uiMipW = data.uiW / 2u, uiMipH = data.uiH / 2u;
      for (xiiUInt32 mip = 1u; mip < data.uiMips; ++mip) {
        cmd.Dispatch(xiiMath::Max(1u, (uiMipW + 7u) / 8u), xiiMath::Max(1u, (uiMipH + 7u) / 8u), 1u);
        uiMipW = xiiMath::Max(1u, uiMipW / 2u); uiMipH = xiiMath::Max(1u, uiMipH / 2u);
      }
      // Pass 59: BloomUpsample.xiiShader — tent filter upsample chain + intensity blend.
      for (xiiInt32 mip = static_cast<xiiInt32>(data.uiMips) - 2; mip >= 0; --mip)
        cmd.Dispatch(xiiMath::Max(1u, (data.uiW >> mip) / 8u + 1u), xiiMath::Max(1u, (data.uiH >> mip) / 8u + 1u), 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_BloomTexture), pData->hBloomOutput);
}
