#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/DecalPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalPass, 1, xiiRTTIDefaultAllocator<xiiDecalPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("DecalPass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiDecalPass::xiiDecalPass() : xiiRenderPipelinePass("DecalPass") {}
xiiDecalPass::~xiiDecalPass() = default;

namespace { struct DecalPassData { xiiRGTextureHandle hDepth; xiiRGBufferHandle hDecalTileList; xiiRGTextureHandle hGBufAlbedo; xiiRGTextureHandle hGBufNormal; xiiUInt32 uiW = 1u, uiH = 1u, uiTileSize = 8u; }; }

void xiiDecalPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth, hAlbedo, hNormal;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo),     hAlbedo);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal),     hNormal);

  const xiiUInt32 uiTilesX = (uiW + m_uiTileSize - 1u) / m_uiTileSize;
  const xiiUInt32 uiTilesY = (uiH + m_uiTileSize - 1u) / m_uiTileSize;

  xiiGALBufferCreationDescription tileDesc;
  tileDesc.m_uiSize        = sizeof(xiiUInt32) * m_uiMaxDecalsPerTile * uiTilesX * uiTilesY;
  tileDesc.m_BufferFlags   = xiiGALBufferUsageFlags::StructuredBuffer | xiiGALBufferUsageFlags::UnorderedAccess;
  tileDesc.m_ResourceUsage = xiiGALResourceUsage::Default;

  auto [pData, hPass] = graph.AddPass<DecalPassData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, hAlbedo, hNormal, tileDesc, uiW, uiH, this](DecalPassData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())  data.hDepth     = builder.ReadTexture(hDepth,  xiiGALResourceStateFlags::ShaderResource);
      if (hAlbedo.IsValid()) data.hGBufAlbedo = builder.WriteTexture(hAlbedo, xiiGALResourceStateFlags::UnorderedAccess);
      if (hNormal.IsValid()) data.hGBufNormal = builder.WriteTexture(hNormal, xiiGALResourceStateFlags::UnorderedAccess);
      data.hDecalTileList = builder.WriteBuffer("DecalTileList", tileDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.uiTileSize = m_uiTileSize;
    },
    [](const DecalPassData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Decal Classification + Resolve");
      // Pass 23: DecalClassification.xiiShader — builds per-tile decal index list.
      const xiiUInt32 uiTX = (data.uiW + data.uiTileSize - 1u) / data.uiTileSize;
      const xiiUInt32 uiTY = (data.uiH + data.uiTileSize - 1u) / data.uiTileSize;
      cmd.Dispatch(uiTX, uiTY, 1u);
      // Pass 24: DecalResolve.xiiShader — applies decal attributes to GBuffer targets.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_DecalTileList), pData->hDecalTileList);
}
