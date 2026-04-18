// XII Engine - ViewPasses_Stage3.cpp
// Stage 3: Depth & motion preparation.
// DepthPrepass (graphics) -> HiZ pyramid (compute, per-mip) -> HiZ occlusion cull (compute) ->
// Motion vectors (graphics) -> Velocity dilation (compute).

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Math.h>
#include <GraphicsCore/Pipeline/PipelineBlackboardKeys.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphBlackboard.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Pipeline/Passes/HiZPyramid/HiZBuildConstants.h>

//
// Hi-Z pyramid generation (compute - per-mip loop in execute)
//
namespace
{
  struct HiZPyramidData
  {
    xiiRGTextureHandle m_hSceneDepth; // source: scene depth (mip 0 input)
    xiiRGTextureHandle m_hHiZPyramid; // output: R32F max-depth pyramid
    xiiUInt32          m_uiRenderW   = 1920u;
    xiiUInt32          m_uiRenderH   = 1080u;
    xiiUInt32          m_uiMipLevels = 1u;
  };
} // namespace

static void SetupHiZPyramid(xiiView& view, HiZPyramidData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& dp = view.m_ViewPassResources.m_DepthPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  // Compute mip count for the Hi-Z pyramid.
  data.m_uiMipLevels = 1u;
  {
    xiiUInt32 uiW = data.m_uiRenderW, uiH = data.m_uiRenderH;
    while (uiW > 1u || uiH > 1u)
    {
      uiW = xiiMath::Max(uiW >> 1u, 1u);
      uiH = xiiMath::Max(uiH >> 1u, 1u);
      ++data.m_uiMipLevels;
    }
  }

  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  data.m_hSceneDepth = builder.ReadTexture(hDepth, xiiGALResourceStateFlags::ShaderResource);

  // Hi-Z pyramid: R32F, same base resolution as scene depth, full mip chain.
  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::R32Float;
  desc.m_uiWidth     = data.m_uiRenderW;
  desc.m_uiHeight    = data.m_uiRenderH;
  desc.m_uiMipLevels = data.m_uiMipLevels;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHiZPyramid = builder.WriteTexture(xiiRGBlackboardKeys::k_HiZPyramid, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(dp.m_pHiZBuildPipeline, "Shaders/Pipeline/HiZBuild.xiiShader");
}

static void ExecuteHiZPyramid(xiiView& view, const HiZPyramidData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              dp  = view.m_ViewPassResources.m_DepthPasses;

  cmd.BeginDebugGroup("HiZPyramid");
  cmd.SetPipelineState(dp.m_pHiZBuildPipeline);

  xiiGALTexture* pHiZ   = ctx.GetTexture(data.m_hHiZPyramid);
  xiiGALTexture* pDepth = ctx.GetTexture(data.m_hSceneDepth);

  xiiUInt32 uiSrcW = data.m_uiRenderW;
  xiiUInt32 uiSrcH = data.m_uiRenderH;

  for (xiiUInt32 uiMip = 0u; uiMip < data.m_uiMipLevels - 1u; ++uiMip)
  {
    const xiiUInt32 uiDstW = xiiMath::Max(uiSrcW >> 1u, 1u);
    const xiiUInt32 uiDstH = xiiMath::Max(uiSrcH >> 1u, 1u);

    // Upload per-mip constants.
    xiiHiZBuildConstants constants;
    constants.SrcSize[0] = uiSrcW;
    constants.SrcSize[1] = uiSrcH;
    constants.DstSize[0] = uiDstW;
    constants.DstSize[1] = uiDstH;
    constants.SrcMip     = uiMip;
    // (In a real implementation, upload via a dynamic cbuffer / push constant here)

    // Bind source mip as SRV, destination mip as UAV.
    if (uiMip == 0u)
    {
      // Mip 0: read from scene depth.
      cmd.ResolveAndSetShaderResourceView("g_DepthSrc", pDepth->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    }
    // Destination mip UAV (per-mip view created on demand from the texture).
    cmd.ResolveAndSetUnorderedAccessView("g_HiZOut", pHiZ->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

    const xiiUInt32 uiGX = (uiDstW + 7u) / 8u;
    const xiiUInt32 uiGY = (uiDstH + 7u) / 8u;
    cmd.DispatchCompute({uiGX, uiGY, 1u});

    // UAV barrier between consecutive mip levels (same resource, new mip is the next source).
    cmd.ResourceBarrier(xiiGALTextureBarrierDescription{pHiZ, xiiGALResourceStateFlags::UnorderedAccess, xiiGALResourceStateFlags::UnorderedAccess});

    uiSrcW = uiDstW;
    uiSrcH = uiDstH;
  }

  cmd.EndDebugGroup();
}

//
// Hi-Z occlusion culling (compute - 2nd phase: prunes visible-candidate list)
//
namespace
{
  struct HiZOccCullData
  {
    xiiRGTextureHandle m_hHiZPyramid;
    xiiRGBufferHandle  m_hVisibleCandidates;
    xiiRGBufferHandle  m_hSurvivingInstances;
    xiiRGBufferHandle  m_hInstanceBounds;
    xiiUInt32          m_uiInstanceCount = 0;
  };
} // namespace

static void SetupHiZOccCull(xiiView& view, HiZOccCullData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& dp = view.m_ViewPassResources.m_DepthPasses;

  xiiRGTextureHandle hHiZ;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_HiZPyramid), hHiZ);
  data.m_hHiZPyramid = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);

  xiiRGBufferHandle hCandidates;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VisibleCandidateBuffer), hCandidates);
  data.m_hVisibleCandidates = builder.ReadBuffer(hCandidates, xiiGALResourceStateFlags::ShaderResource);

  xiiRGBufferHandle hBounds;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_InstanceBoundsBuffer), hBounds);
  data.m_hInstanceBounds = builder.ReadBuffer(hBounds, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription desc;
  desc.m_uiElementByteStride = 4u;
  desc.m_uiSize              = 4u + 4u * 65536u;
  desc.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  desc.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hSurvivingInstances = builder.WriteBuffer(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, desc, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiInstanceCount     = 65536u;

  xiiView::EnsureComputePipeline(dp.m_pHiZOcclusionCullPipeline, "Shaders/Pipeline/HiZOcclusionCulling.xiiShader");
}

static void ExecuteHiZOccCull(xiiView& view, const HiZOccCullData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              dp  = view.m_ViewPassResources.m_DepthPasses;

  cmd.BeginDebugGroup("HiZOcclusionCull");
  cmd.SetPipelineState(dp.m_pHiZOcclusionCullPipeline);
  cmd.ResolveAndSetShaderResourceView("g_HiZPyramid", ctx.GetTexture(data.m_hHiZPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetShaderResourceBufferView("g_Candidates", ctx.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", ctx.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessBufferView("g_Survivors", ctx.GetBuffer(data.m_hSurvivingInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  cmd.EndDebugGroup();
}

//
// Motion vectors (graphics - renders per-object velocity to R16G16F buffer)
//
namespace
{
  struct MotionVecData
  {
    xiiRGTextureHandle m_hSceneDepth;
    xiiRGTextureHandle m_hVelocityBuffer;
    xiiRGBufferHandle  m_hDrawCommands;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupMotionVectors(xiiView& view, MotionVecData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& dp = view.m_ViewPassResources.m_DepthPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hDepth;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_SceneDepthTexture), hDepth);
  data.m_hSceneDepth = builder.WriteTexture(hDepth, xiiGALResourceStateFlags::DepthWrite);

  xiiRGBufferHandle hDraw;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_DrawIndirectCommands), hDraw);
  if (hDraw.IsValid())
    data.m_hDrawCommands = builder.ReadBuffer(hDraw, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType     = xiiGALTextureType::Texture2D;
  desc.m_Format          = xiiGALTextureFormat::RG16Float;
  desc.m_uiWidth         = data.m_uiRenderW;
  desc.m_uiHeight        = data.m_uiRenderH;
  desc.m_uiMipLevels     = 1u;
  desc.m_BindFlags       = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  desc.m_Usage           = xiiGALResourceUsage::Default;
  data.m_hVelocityBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, desc, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(false);
}

static void ExecuteMotionVectors(xiiView& view, const MotionVecData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              dp  = view.m_ViewPassResources.m_DepthPasses;

  cmd.BeginDebugGroup("MotionVectors");

  const float fClearVelocity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  cmd.ClearRenderTarget(ctx.GetTexture(data.m_hVelocityBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), fClearVelocity, xiiGALStateTransitionMode::Transition);
  cmd.SetViewports({{0.0f, 0.0f, static_cast<float>(data.m_uiRenderW), static_cast<float>(data.m_uiRenderH), 0.0f, 1.0f}}, data.m_uiRenderW, data.m_uiRenderH);

  if (dp.m_pMotionVectorPipeline && data.m_hDrawCommands.IsValid())
  {
    cmd.SetPipelineState(dp.m_pMotionVectorPipeline);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DrawIndexedIndirect(ctx.GetBuffer(data.m_hDrawCommands), 0u);
  }

  cmd.EndDebugGroup();
}

//
// Velocity dilation (compute - max-filter to push velocity at object edges)
//
namespace
{
  struct VelocityDilateData
  {
    xiiRGTextureHandle m_hVelocityIn;
    xiiRGTextureHandle m_hVelocityDilated;
    xiiUInt32          m_uiRenderW = 1920u, m_uiRenderH = 1080u;
  };
} // namespace

static void SetupVelocityDilation(xiiView& view, VelocityDilateData& data, xiiRGBuilder& builder, const xiiRenderGraphBlackboard& bb)
{
  auto& dp = view.m_ViewPassResources.m_DepthPasses;

  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderWidth), data.m_uiRenderW);
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_RenderHeight), data.m_uiRenderH);

  xiiRGTextureHandle hVel;
  bb.TryGetValue(xiiMakeHashedString(xiiRGBlackboardKeys::k_VelocityBuffer), hVel);
  data.m_hVelocityIn = builder.ReadTexture(hVel, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_TextureType = xiiGALTextureType::Texture2D;
  desc.m_Format      = xiiGALTextureFormat::RG16Float;
  desc.m_uiWidth     = data.m_uiRenderW;
  desc.m_uiHeight    = data.m_uiRenderH;
  desc.m_uiMipLevels = 1u;
  desc.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage       = xiiGALResourceUsage::Default;
  // Re-use the velocity buffer key with a new version: dilation replaces the velocity output.
  data.m_hVelocityDilated = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiView::EnsureComputePipeline(dp.m_pVelocityDilationPipeline, "Shaders/Pipeline/Downscale.xiiShader");
}

static void ExecuteVelocityDilation(xiiView& view, const VelocityDilateData& data, xiiRGPassContext& ctx)
{
  xiiGALCommandList& cmd = ctx.GetCommandList();
  auto&              dp  = view.m_ViewPassResources.m_DepthPasses;

  cmd.BeginDebugGroup("VelocityDilation");
  cmd.SetPipelineState(dp.m_pVelocityDilationPipeline);
  cmd.ResolveAndSetShaderResourceView("g_Input", ctx.GetTexture(data.m_hVelocityIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
  cmd.ResolveAndSetUnorderedAccessView("g_Output", ctx.GetTexture(data.m_hVelocityDilated)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
  cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
  cmd.DispatchCompute({(data.m_uiRenderW + 7u) / 8u, (data.m_uiRenderH + 7u) / 8u, 1u});
  cmd.EndDebugGroup();
}

//
// BuildStage3_Depth - entry point
//

void xiiView::BuildStage3_Depth(xiiRenderGraph& graph, const xiiRenderGraphBlackboard& blackboard)
{
  xiiView* self = this;

  graph.AddPass<DepthPrepassData>(
    "DepthPrepass",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](DepthPrepassData& d, xiiRGBuilder& b) { SetupDepthPrepass(*self, d, b, blackboard); },
    [self](const DepthPrepassData& d, xiiRGPassContext& c) { ExecuteDepthPrepass(*self, d, c); });

  graph.AddPass<HiZPyramidData>(
    "HiZPyramid",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](HiZPyramidData& d, xiiRGBuilder& b) { SetupHiZPyramid(*self, d, b, blackboard); },
    [self](const HiZPyramidData& d, xiiRGPassContext& c) { ExecuteHiZPyramid(*self, d, c); });

  graph.AddPass<HiZOccCullData>(
    "HiZOcclusionCull",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](HiZOccCullData& d, xiiRGBuilder& b) { SetupHiZOccCull(*self, d, b, blackboard); },
    [self](const HiZOccCullData& d, xiiRGPassContext& c) { ExecuteHiZOccCull(*self, d, c); });

  graph.AddPass<MotionVecData>(
    "MotionVectors",
    xiiGALCommandQueueFlags::Graphics,
    [self, &blackboard](MotionVecData& d, xiiRGBuilder& b) { SetupMotionVectors(*self, d, b, blackboard); },
    [self](const MotionVecData& d, xiiRGPassContext& c) { ExecuteMotionVectors(*self, d, c); });

  graph.AddPass<VelocityDilateData>(
    "VelocityDilation",
    xiiGALCommandQueueFlags::Compute,
    [self, &blackboard](VelocityDilateData& d, xiiRGBuilder& b) { SetupVelocityDilation(*self, d, b, blackboard); },
    [self](const VelocityDilateData& d, xiiRGPassContext& c) { ExecuteVelocityDilation(*self, d, c); });
}
