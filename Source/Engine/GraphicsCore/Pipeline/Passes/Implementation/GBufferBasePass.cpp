#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/GBufferBasePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGBufferBasePass, 1, xiiRTTIDefaultAllocator<xiiGBufferBasePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("GBufferBasePass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiGBufferBasePass::xiiGBufferBasePass() : xiiRenderPipelinePass("GBufferBasePass") {}
xiiGBufferBasePass::~xiiGBufferBasePass() = default;

namespace { struct GBufferData { xiiRGTextureHandle hDepth; xiiRGBufferHandle hDrawArgs; xiiRGTextureHandle hAlbedo, hNormal, hMaterial, hEmissive; xiiUInt32 uiW = 1u, uiH = 1u; }; }

void xiiGBufferBasePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDepth; xiiRGBufferHandle hDrawArgs;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture),    hDepth);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDrawArgs);

  auto makeRT = [&](const char* szName, xiiGALTextureFormat fmt) {
    xiiGALTextureCreationDescription d; d.m_uiWidth = uiW; d.m_uiHeight = uiH; d.m_uiMipLevels = 1u;
    d.m_Format = fmt; d.m_BindFlags = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    return d;
  };

  auto albedoDesc   = makeRT("GBufferAlbedo",   xiiGALTextureFormat::R8G8B8A8UNorm);
  auto normalDesc   = makeRT("GBufferNormal",   xiiGALTextureFormat::R16G16SNorm);
  auto materialDesc = makeRT("GBufferMaterial", xiiGALTextureFormat::R8G8B8A8UNorm);
  auto emissiveDesc = makeRT("GBufferEmissive", xiiGALTextureFormat::R16G16B16A16Float);

  auto [pData, hPass] = graph.AddPass<GBufferData>(
    GetName(), xiiGALCommandQueueFlags::Graphics,
    [hDepth, hDrawArgs, albedoDesc, normalDesc, materialDesc, emissiveDesc, uiW, uiH](GBufferData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid())    data.hDepth    = builder.ReadTexture(hDepth,    xiiGALResourceStateFlags::DepthRead);
      if (hDrawArgs.IsValid()) data.hDrawArgs = builder.ReadBuffer(hDrawArgs,  xiiGALResourceStateFlags::IndirectArgument);
      data.hAlbedo   = builder.WriteTexture("GBufferAlbedo",   albedoDesc,   xiiGALResourceStateFlags::RenderTarget);
      data.hNormal   = builder.WriteTexture("GBufferNormal",   normalDesc,   xiiGALResourceStateFlags::RenderTarget);
      data.hMaterial = builder.WriteTexture("GBufferMaterial", materialDesc, xiiGALResourceStateFlags::RenderTarget);
      data.hEmissive = builder.WriteTexture("GBufferEmissive", emissiveDesc, xiiGALResourceStateFlags::RenderTarget);
      data.uiW = uiW; data.uiH = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const GBufferData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("GBuffer Base Pass");
      // GBufferBase.xiiShader: indirect MRT draw — albedo+alpha/oct-normal/roughness-metallic-AO/emissive.
      if (auto* pArgs = context.GetBuffer(data.hDrawArgs))
        cmd.DrawIndexedIndirect(pArgs, 0u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferAlbedo),   pData->hAlbedo);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferNormal),   pData->hNormal);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferMaterial), pData->hMaterial);
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive), pData->hEmissive);
}
