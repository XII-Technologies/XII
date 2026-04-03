#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/ContactShadowPass.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiContactShadowPass, 1, xiiRTTIDefaultAllocator<xiiContactShadowPass>)
{ XII_BEGIN_PROPERTIES { XII_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new xiiDefaultValueAttribute(true)), XII_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new xiiDefaultValueAttribute("ContactShadowPass")), XII_MEMBER_PROPERTY("RaySteps", m_uiRaySteps)->AddAttributes(new xiiDefaultValueAttribute(16u)), XII_MEMBER_PROPERTY("MaxRayDist", m_fMaxRayDistance)->AddAttributes(new xiiDefaultValueAttribute(0.5f)), } XII_END_PROPERTIES; }
XII_END_DYNAMIC_REFLECTED_TYPE;
xiiContactShadowPass::xiiContactShadowPass() : xiiRenderPipelinePass("ContactShadowPass") {}
xiiContactShadowPass::~xiiContactShadowPass() = default;

namespace { struct ContactShadowData { xiiRGTextureHandle hDepth; xiiRGTextureHandle hContactShadow; xiiUInt32 uiWidth = 1u, uiHeight = 1u, uiSteps = 16u; float fMaxDist = 0.5f; }; }

void xiiContactShadowPass::AddToGraph(xiiView& view, xiiRenderGraph& graph, xiiRenderGraphBlackboard& blackboard)
{
  xiiUInt32 uiW = 1920u, uiH = 1080u;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth),  uiW);
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), uiH);
  xiiRGTextureHandle hDepth;
  blackboard.TryGet(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);

  xiiGALTextureCreationDescription cDesc;
  cDesc.m_uiWidth = uiW; cDesc.m_uiHeight = uiH; cDesc.m_uiMipLevels = 1u;
  cDesc.m_Format  = xiiGALTextureFormat::R8UNorm;
  cDesc.m_BindFlags = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;

  auto [pData, hPass] = graph.AddPass<ContactShadowData>(
    GetName(), xiiGALCommandQueueFlags::Compute,
    [hDepth, cDesc, uiW, uiH, this](ContactShadowData& data, xiiRGBuilder& builder)
    {
      if (hDepth.IsValid()) data.hDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);
      data.hContactShadow = builder.WriteTexture("ContactShadows", cDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.uiWidth = uiW; data.uiHeight = uiH; data.uiSteps = m_uiRaySteps; data.fMaxDist = m_fMaxRayDistance;
    },
    [](const ContactShadowData& data, xiiRGPassContext& context)
    {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.PushDebugGroup("Contact Shadows");
      // ContactShadow.xiiShader: ray march in view-space along light dir.
      cmd.Dispatch((data.uiWidth + 7u) / 8u, (data.uiHeight + 7u) / 8u, 1u);
      cmd.PopDebugGroup();
    }
  );
  blackboard.Set(xiiMakeHashedString(xiiRGBlackboardKeys::k_ContactShadowTerm), pData->hContactShadow);
}
