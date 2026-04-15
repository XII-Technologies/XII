// XII Engine - ViewPasses_Stage2.cpp
// Stage 2: Shadow map generation.
// Cascade setup (compute) -> directional shadows (graphics) -> spot/point shadows (graphics) ->
// ray-traced shadows (RT compute) -> shadow denoise (compute) -> contact shadows (compute).

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Lights/PointLightComponent.h>
#include <GraphicsCore/Lights/SpotLightComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/ShadowCascade/ShadowCascadeConstants.h>

//
// Shadow denoiser (bilateral blur on raw RT shadow mask)
//
namespace
{
  struct ShadowDenoiseData
  {
    xiiRGTextureHandle m_hRTRawShadowMask;
    xiiRGTextureHandle m_hRTFinalShadowMask;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupShadowDenoise(xiiView& view, ShadowDenoiseData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hRaw;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RTRawShadowMask), hRaw);
  data.m_hRTRawShadowMask = builder.ReadTexture(hRaw, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType        = xiiGALTextureType::Texture2D;
  desc.m_Format             = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth            = data.m_uiRenderW;
  desc.m_uiHeight           = data.m_uiRenderH;
  desc.m_uiMipLevels        = 1u;
  desc.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hRTFinalShadowMask = builder.WriteTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(sp.m_pShadowDenoisePipeline, "Shaders/Pipeline/SeparatedBilateralBlur.xiiShader");
}

static void ExecuteShadowDenoise(xiiView& view, const ShadowDenoiseData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  cmd.BeginDebugGroup("ShadowDenoise");
  cmd.SetPipelineState(sp.m_pShadowDenoisePipeline);
  cmd.ResolveAndSetShaderResourceView("g_Input", ctx.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_Output", ctx.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// Contact shadows (screen-space ray march)
//
namespace
{
  struct ContactShadowData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hContactShadow;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupContactShadow(xiiView& view, ContactShadowData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& sp = view.m_ViewPassResources.m_ShadowPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  if (hDepth.IsValid())
    data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType    = xiiGALTextureType::Texture2D;
  desc.m_Format         = xiiGALTextureFormat::R8Unorm;
  desc.m_uiWidth        = data.m_uiRenderW;
  desc.m_uiHeight       = data.m_uiRenderH;
  desc.m_uiMipLevels    = 1u;
  desc.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hContactShadow = builder.WriteTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(sp.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

static void ExecuteContactShadow(xiiView& view, const ContactShadowData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              sp  = view.m_ViewPassResources.m_ShadowPasses;

  cmd.BeginDebugGroup("ContactShadows");
  cmd.SetPipelineState(sp.m_pContactShadowPipeline);
  if (data.m_hSceneDepth.IsValid())
    cmd.ResolveAndSetShaderResourceView("g_SceneDepth", ctx.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_ContactShadowOut", ctx.GetTexture(data.m_hContactShadow)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

