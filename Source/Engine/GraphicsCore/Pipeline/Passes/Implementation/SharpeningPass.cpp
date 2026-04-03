#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/SharpeningPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSharpeningPass, 1, xiiRTTIDefaultAllocator<xiiSharpeningPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("SharpeningPass")), XII_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new xiiDefaultValueAttribute(0.5f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiSharpeningPass::xiiSharpeningPass() : xiiRenderPipelinePass("SharpeningPass") {}
xiiSharpeningPass::~xiiSharpeningPass() = default;

namespace { struct SharpenData { xiiRGTextureHandle hInput, hOutput; xiiUInt32 uiW = 1u, uiH = 1u; float fStrength = 0.5f; }; }

void xiiSharpeningPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);

  xiiRGTextureHandle hInput;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_GradedColor), hInput);

  xiiGALTextureCreationDescription outDesc;
  outDesc.m_uiWidth = uiW; outDesc.m_uiHeight = uiH; outDesc.m_uiMipLevels = 1u;
  outDesc.m_Format  = xiiGALTextureFormat::R8G8B8A8UNorm;
  outDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<SharpenData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hInput, outDesc, uiW, uiH, this](SharpenData& data, xiiRGBuilder& builder)
    {
      if (hInput.IsValid()) data.hInput = builder.ReadTexture(hInput, xiiGALResourceStateFlags::ShaderResource);
      data.hOutput   = builder.WriteTexture("SharpenedColor", outDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiW = uiW; data.uiH = uiH; data.fStrength = m_fStrength;
    },
    [](const SharpenData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Luma Sharpening");
      // LumaSharpening.xiiShader: fast 1-pass luminance-weighted unsharp mask.
      cmd.Dispatch((data.uiW + 7u) / 8u, (data.uiH + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );

  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_SharpenedColor), pData->hOutput);
}
