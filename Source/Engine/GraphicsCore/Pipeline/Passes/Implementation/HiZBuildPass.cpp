#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/HiZBuildPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiHiZBuildPass, 1, xiiRTTIDefaultAllocator<xiiHiZBuildPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("HiZBuildPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace { struct xiiHiZBuildAutoReg { xiiHiZBuildAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiHiZBuildPass)); } }; static xiiHiZBuildAutoReg s_AutoReg; }

xiiHiZBuildPass::xiiHiZBuildPass() : xiiRenderPipelinePass("HiZBuildPass") {}
xiiHiZBuildPass::~xiiHiZBuildPass() = default;

namespace { struct HiZBuildData { xiiRGTextureHandle hOccluderDepth; xiiRGTextureHandle hHiZPyramid; xiiUInt32 uiWidth = 1u, uiHeight = 1u, uiMips = 1u; }; }

void xiiHiZBuildPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiRGTextureHandle hOccluderDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_OccluderDepthTexture), hOccluderDepth);

  xiiUInt32 uiW = 960u, uiH = 540u;
  // HiZ pyramid is built at the occluder depth resolution.
  // Mip count spans down to 1x1.
  const xiiUInt32 uiMips = static_cast<xiiUInt32>(xiiMath::Log2i(xiiMath::Max(uiW, uiH))) + 1u;

  xiiGALTextureCreationDescription hizDesc;
  hizDesc.m_uiWidth     = uiW;
  hizDesc.m_uiHeight    = uiH;
  hizDesc.m_uiMipLevels = uiMips;
  hizDesc.m_Format      = xiiGALTextureFormat::R32Float; // single-channel max depth pyramid
  hizDesc.m_BindFlags   = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;

  auto [pData, hPass] = graph.AddPass<HiZBuildData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hOccluderDepth, hizDesc, uiW, uiH, uiMips](HiZBuildData& data, xiiRGBuilder& builder)
    {
      if (hOccluderDepth.IsValid())
        data.hOccluderDepth = builder.ReadTexture(hOccluderDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hHiZPyramid = builder.WriteTexture("HiZPyramid", hizDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiWidth     = uiW;
      data.uiHeight    = uiH;
      data.uiMips      = uiMips;
      builder.SetPassAllowMerge(false);
    },
    [](const HiZBuildData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Hi-Z Pyramid Build");
      // HiZBuild.xiiShader repeatedly dispatched per mip level (2x2 max-reduce).
      // Mip 0: from occluder depth | Mip N: from mip N-1.
      xiiUInt32 uiMipW = data.uiWidth, uiMipH = data.uiHeight;
      for (xiiUInt32 mip = 0u; mip < data.uiMips; ++mip)
      {
        cmd.Dispatch(xiiMath::Max(1u, (uiMipW + 7u) / 8u),
                     xiiMath::Max(1u, (uiMipH + 7u) / 8u), 1u);
        uiMipW = xiiMath::Max(1u, uiMipW / 2u);
        uiMipH = xiiMath::Max(1u, uiMipH / 2u);
      }
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), pData->hHiZPyramid);
}
