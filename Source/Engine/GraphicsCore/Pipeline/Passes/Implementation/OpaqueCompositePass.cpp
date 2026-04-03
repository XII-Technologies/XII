#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/OpaqueCompositePass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOpaqueCompositePass, 1, xiiRTTIDefaultAllocator<xiiOpaqueCompositePass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("OpaqueCompositePass")), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiOpaqueCompositePass::xiiOpaqueCompositePass() : xiiRenderPipelinePass("OpaqueCompositePass") {}
xiiOpaqueCompositePass::~xiiOpaqueCompositePass() = default;

namespace { struct OpaqueCompData { xiiRGTextureHandle hDirect, hIndirect, hEmissive, hVolumetric, hSky, hHDRScene; xiiUInt32 uiW = 1u, uiH = 1u; }; }

void xiiOpaqueCompositePass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hDirect, hIndirect, hEmissive, hVolumetric, hSky;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_DirectLightingBuffer),   hDirect);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_IndirectLightingBuffer), hIndirect);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GBufferEmissive),        hEmissive);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_VolumetricScattering),   hVolumetric);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SkyRadiance),            hSky);

  xiiGALTextureCreationDescription hdrDesc;
  hdrDesc.m_uiWidth = uiW; hdrDesc.m_uiHeight = uiH; hdrDesc.m_uiMipLevels = 1u;
  hdrDesc.m_Format  = xiiGALTextureFormat::R16G16B16A16Float;
  hdrDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::RenderTarget;

  auto [pData, hPass] = graph.AddPass<OpaqueCompData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [=](OpaqueCompData& data, xiiRGBuilder& builder)
    {
      auto read = [&](xiiRGTextureHandle h) { return h.IsValid() ? builder.ReadTexture(h, xiiGALResourceStateFlags::ShaderResource) : h; };
      data.hDirect     = read(hDirect);
      data.hIndirect   = read(hIndirect);
      data.hEmissive   = read(hEmissive);
      data.hVolumetric = read(hVolumetric);
      data.hSky        = read(hSky);
      data.hHDRScene   = builder.WriteTexture("HDRSceneColor", hdrDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH;
      builder.SetPassAllowMerge(false);
    },
    [](const OpaqueCompData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Opaque Composite");
      // OpaqueComposite.xiiShader: fuses direct + indirect + emissive + volumetric + sky into HDR.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_HDRSceneColor), pData->hHDRScene);
}
