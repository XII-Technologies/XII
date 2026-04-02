#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/DirectionalShadowRenderPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDirectionalShadowRenderPass, 1, xiiRTTIDefaultAllocator<xiiDirectionalShadowRenderPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("DirectionalShadowRenderPass")), XII_MEMBER_PROPERTY("AtlasSize", m_uiAtlasSize)->AddAttributes(new xiiDefaultValueAttribute(4096u)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
namespace { struct xiiDirShadowAutoReg { xiiDirShadowAutoReg() { xiiRenderWorldModule::RegisterPass(XII_DEFAULT_NEW(xiiDirectionalShadowRenderPass)); } }; static xiiDirShadowAutoReg s_AutoReg; }

xiiDirectionalShadowRenderPass::xiiDirectionalShadowRenderPass() : xiiRenderPipelinePass("DirectionalShadowRenderPass") {}
xiiDirectionalShadowRenderPass::~xiiDirectionalShadowRenderPass() = default;

namespace { struct DirShadowData { xiiRGBufferHandle hCascadeMatrices; xiiRGTextureHandle hShadowAtlas; xiiUInt32 uiAtlasSize = 4096u; xiiUInt32 uiCascadeCount = 4u; }; }

void xiiDirectionalShadowRenderPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiGALTextureCreationDescription atlasDesc;
  atlasDesc.m_uiWidth = m_uiAtlasSize; atlasDesc.m_uiHeight = m_uiAtlasSize; atlasDesc.m_uiMipLevels = 1u;
  atlasDesc.m_Format    = xiiGALTextureFormat::D16UNorm;
  atlasDesc.m_BindFlags = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;

  xiiRGBufferHandle hCascMat;
  xiiUInt32 uiCascadeCount = 4u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeMatrices), hCascMat);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_ShadowCascadeCount),    uiCascadeCount);

  auto [pData, hPass] = graph.AddPass<DirShadowData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hCascMat, atlasDesc, uiCascadeCount, this](DirShadowData& data, xiiRGBuilder& builder)
    {
      if (hCascMat.IsValid()) data.hCascadeMatrices = builder.ReadBuffer(hCascMat, xiiGALResourceStateFlags::ConstantBuffer);
      data.hShadowAtlas  = builder.WriteTexture("DirShadowAtlas", atlasDesc, xiiGALResourceStateFlags::DepthWrite);
      data.uiAtlasSize   = m_uiAtlasSize;
      data.uiCascadeCount = uiCascadeCount;
      builder.SetPassAllowMerge(false);
    },
    [](const DirShadowData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Directional Shadow Render");
      xiiGALTexture* pAtlas = context.GetTexture(data.hShadowAtlas);
      cmd.ClearDepthStencilView(pAtlas, xiiGALClearFlags::Depth, 0.0f, 0u);
      // Indirect draws per cascade into shadow atlas tiles — ShadowDepth.xiiShader.
      // Atlas is laid out: cascade 0 at [0,0], cascade 1 at [half,0], etc.
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectionalShadowAtlas), pData->hShadowAtlas);
}
