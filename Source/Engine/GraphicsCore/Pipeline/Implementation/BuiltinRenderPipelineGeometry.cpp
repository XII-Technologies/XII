/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupDepthPrepass(xiiDepthPrepassData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::D32Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSceneDepth        = builder.WriteTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, description, xiiGALResourceStateFlags::DepthWrite);

  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_CoarseDrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDepthPrepass(const xiiDepthPrepassData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DepthPrepass");
  {
    xiiGALTexture* pDepth = context.GetTexture(data.m_hSceneDepth);
    cmd.ClearDepthStencilView(pDepth->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, true, 0.0f, 0U);
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_DepthPasses.m_pDepthPrepassPipeline)
    {
      cmd.SetPipelineState(m_Resources.m_DepthPasses.m_pDepthPrepassPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Pyramid Data //////////
//
// Collects all GPU resources related to hierarchical depth generation for the current frame, including the scene depth source and Hi-Z pyramid target.

void xiiBuiltinRenderPipelineRuntime::SetupHiZPyramid(xiiHiZPyramidData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiBaseWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiBaseHeight = m_View.GetRenderResolutionHeight();

  xiiUInt32 uiMipWidth  = uiBaseWidth;
  xiiUInt32 uiMipHeight = uiBaseHeight;
  while (uiMipWidth > 1U || uiMipHeight > 1U)
  {
    uiMipWidth  = xiiMath::Max(uiMipWidth >> 1U, 1U);
    uiMipHeight = xiiMath::Max(uiMipHeight >> 1U, 1U);
    ++data.m_uiMipLevels;
  }

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R32Float;
  description.m_Size.width  = uiBaseWidth;
  description.m_Size.height = uiBaseHeight;
  description.m_uiMipLevels = data.m_uiMipLevels;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHiZPyramid        = builder.WriteTexture(xiiRGBlackboardKeys::k_HiZPyramid, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hHiZClosestPyramid = builder.WriteTexture(xiiRGBlackboardKeys::k_HiZClosestPyramid, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiHiZBuildConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiHiZBuildConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DepthPasses.m_pHiZBuildPipeline, "Shaders/Pipeline/HiZBuild.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteHiZPyramid(const xiiHiZPyramidData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZPyramid");
  {
    if (m_Resources.m_DepthPasses.m_pHiZBuildPipeline)
    {
      cmd.SetPipelineState(m_Resources.m_DepthPasses.m_pHiZBuildPipeline);

      xiiGALTexture* pDepth       = context.GetTexture(data.m_hSceneDepth);
      xiiGALTexture* pFarthestHiZ = context.GetTexture(data.m_hHiZPyramid);
      xiiGALTexture* pClosestHiZ  = context.GetTexture(data.m_hHiZClosestPyramid);
      xiiGALBuffer*  pConstants   = context.GetBuffer(data.m_hConstants);

      auto buildPyramid = [&](xiiGALTexture* pHiZ, xiiUInt32 uiReductionMode) {
        xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> sourceViews;
        xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> destinationViews;
        sourceViews.SetCount(data.m_uiMipLevels);
        destinationViews.SetCount(data.m_uiMipLevels);
        for (xiiUInt32 uiMip = 0U; uiMip < data.m_uiMipLevels; ++uiMip)
        {
          xiiGALTextureViewCreationDescription viewDescription;
          viewDescription.m_ViewType          = xiiGALTextureViewType::ShaderResource;
          viewDescription.m_uiMostDetailedMip = uiMip;
          viewDescription.m_uiMipLevelCount   = 1U;
          sourceViews[uiMip]                  = pHiZ->CreateView(viewDescription);

          viewDescription.m_ViewType = xiiGALTextureViewType::UnorderedAccess;
          destinationViews[uiMip]    = pHiZ->CreateView(viewDescription);
          XII_ASSERT_DEV(sourceViews[uiMip] != nullptr && destinationViews[uiMip] != nullptr, "Failed to create Hi-Z mip views.");
        }

        auto transitionMip = [&cmd, pHiZ](xiiUInt32 uiMip, xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState) {
          xiiGALStateTransitionDescription transition;
          transition.m_pResource       = pHiZ;
          transition.m_uiFirstMipLevel = uiMip;
          transition.m_uiMipLevelCount = 1U;
          transition.m_OldState        = oldState;
          transition.m_NewState        = newState;
          cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
        };

        // Completed mips become shader resources while the destination mip
        // remains writable. Whole-resource tracking resumes after the loop.
        pHiZ->SetResourceState(xiiGALResourceStateFlags::Unknown);
        cmd.ResolveAndSetConstantBuffer("xiiHiZBuildConstants", pConstants, xiiGALShaderType::Compute);

        xiiUInt32 uiSrcWidth  = m_View.GetRenderResolutionWidth();
        xiiUInt32 uiSrcHeight = m_View.GetRenderResolutionHeight();
        for (xiiUInt32 uiMip = 0U; uiMip < data.m_uiMipLevels; ++uiMip)
        {
          const bool      bCopyDepth          = uiMip == 0U;
          const xiiUInt32 uiDestinationWidth  = bCopyDepth ? uiSrcWidth : xiiMath::Max(uiSrcWidth >> 1U, 1U);
          const xiiUInt32 uiDestinationHeight = bCopyDepth ? uiSrcHeight : xiiMath::Max(uiSrcHeight >> 1U, 1U);

          {
            xiiGALMapHelper<xiiHiZBuildConstants> constants(cmd, pConstants, xiiGALMapType::Write, xiiGALMapFlags::Discard);
            constants->SrcSize = xiiVec2U32(uiSrcWidth, uiSrcHeight);
            constants->DstSize = xiiVec2U32(uiDestinationWidth, uiDestinationHeight);
            constants->Reduce  = bCopyDepth ? 0U : uiReductionMode;
          }
          xiiGALStateTransitionDescription constantsTransition;
          constantsTransition.m_pResource       = pConstants;
          constantsTransition.m_OldState        = xiiGALResourceStateFlags::CopyDestination;
          constantsTransition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
          constantsTransition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
          cmd.TransitionResourceStates(xiiMakeArrayPtr(&constantsTransition, 1U));

          xiiGALTextureView* pSourceView = bCopyDepth ? pDepth->GetDefaultView(xiiGALTextureViewType::ShaderResource).Borrow() : sourceViews[uiMip - 1U].Borrow();
          cmd.ResolveAndSetShaderResourceTextureView("g_DepthSrc", pSourceView, xiiGALShaderType::Compute);
          cmd.ResolveAndSetUnorderedAccessTextureView("g_HiZOut", destinationViews[uiMip].Borrow(), xiiGALShaderType::Compute);
          cmd.CommitShaderResources(xiiGALStateTransitionMode::None).AssertSuccess();
          cmd.DispatchCompute({(uiDestinationWidth + 7U) / 8U, (uiDestinationHeight + 7U) / 8U, 1U});

          if (uiMip + 1U < data.m_uiMipLevels)
            transitionMip(uiMip, xiiGALResourceStateFlags::UnorderedAccess, xiiGALResourceStateFlags::ShaderResource);

          uiSrcWidth  = uiDestinationWidth;
          uiSrcHeight = uiDestinationHeight;
        }

        for (xiiUInt32 uiMip = 0U; uiMip + 1U < data.m_uiMipLevels; ++uiMip)
          transitionMip(uiMip, xiiGALResourceStateFlags::ShaderResource, xiiGALResourceStateFlags::UnorderedAccess);
        pHiZ->SetResourceState(xiiGALResourceStateFlags::UnorderedAccess);
      };

      buildPyramid(pFarthestHiZ, 1U);
      buildPyramid(pClosestHiZ, 2U);
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Hi-Z Occlusion Culling Data //////////
//
// Collects all GPU resources related to Hi-Z occlusion culling for the current frame, including the Hi-Z pyramid input, candidate instance list, and surviving output list.

void xiiBuiltinRenderPipelineRuntime::SetupHiZOcclusionCull(xiiHiZOcclusionCullData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHiZPyramid        = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds    = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  const bool                      bHasExtractedMeshCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);
  description.m_uiElementByteStride = sizeof(xiiUInt32);
  description.m_uiSize              = sizeof(xiiUInt32) * (xiiMath::Max(1U, data.m_uiInstanceCount) + 1U);
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hSurvivingInstances        = builder.WriteBuffer(xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiHiZOcclusionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiHiZOcclusionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_Constants.ViewProjectionMatrix = m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_Constants.HiZSize              = xiiVec2U32(m_View.GetRenderResolutionWidth(), m_View.GetRenderResolutionHeight());
  data.m_Constants.HiZMipCount          = 1U;
  for (xiiUInt32 uiWidth = m_View.GetRenderResolutionWidth(), uiHeight = m_View.GetRenderResolutionHeight(); uiWidth > 1U || uiHeight > 1U;)
  {
    uiWidth  = xiiMath::Max(uiWidth >> 1U, 1U);
    uiHeight = xiiMath::Max(uiHeight >> 1U, 1U);
    ++data.m_Constants.HiZMipCount;
  }
  data.m_Constants.CandidateCapacity = data.m_uiInstanceCount;
  data.m_Constants.DepthBias         = 0.0001f;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DepthPasses.m_pHiZOcclusionCullPipeline, "Shaders/Pipeline/HiZOcclusionCulling.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteHiZOcclusionCull(const xiiHiZOcclusionCullData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HiZOcclusionCull");
  {
    const xiiUInt32 uiZero = 0U;
    cmd.UpdateBuffer(context.GetBuffer(data.m_hSurvivingInstances), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    if (data.m_uiInstanceCount > 0U && m_Resources.m_DepthPasses.m_pHiZOcclusionCullPipeline)
    {
      {
        xiiGALMapHelper<xiiHiZOcclusionConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *constants = data.m_Constants;
      }

      cmd.SetPipelineState(m_Resources.m_DepthPasses.m_pHiZOcclusionCullPipeline);
      cmd.ResolveAndSetConstantBuffer("xiiHiZOcclusionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_HiZPyramid", context.GetTexture(data.m_hHiZPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Candidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_SurvivingOut", context.GetBuffer(data.m_hSurvivingInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Motion Vectors Data //////////
//
// Collects all GPU resources related to motion vector rendering for the current frame, including scene depth and the velocity render target.

void xiiBuiltinRenderPipelineRuntime::SetupMotionVectors(xiiMotionVectorsData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityBuffer    = builder.WriteTexture(xiiRGBlackboardKeys::k_VelocityBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiMotionVectorConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Motion Vector Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  auto& depthPasses                             = m_Resources.m_DepthPasses;
  data.m_CurrentViewProjection                  = m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_vCurrentJitter                         = xiiVec2::MakeZero();
  data.m_Constants.PreviousViewProjectionMatrix = depthPasses.m_bMotionHistoryValid ? depthPasses.m_PreviousViewProjectionMatrix : data.m_CurrentViewProjection;
  data.m_Constants.CurrentJitter                = data.m_vCurrentJitter;
  data.m_Constants.PreviousJitter               = depthPasses.m_bMotionHistoryValid ? depthPasses.m_vPreviousJitter : data.m_vCurrentJitter;

  builder.SetPassAllowMerge(false);
  builder.SetPassRenderPassManaged(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteMotionVectors(const xiiMotionVectorsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList&                        cmd = context.GetCommandList();
  xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline;
  if (data.m_hSceneDepth.IsValid() && data.m_hVelocityBuffer.IsValid() && data.m_hConstants.IsValid() && context.GetRenderPass() != nullptr)
  {
    pPipeline = xiiBuiltinRenderPipelineRuntime::EnsureGraphicsPipeline(m_Resources.m_DepthPasses.m_pMotionVectorPipeline, "Shaders/Pipeline/MotionVectors.xiiShader", context.GetRenderPass(), context.GetSubpassIndex());
  }

  cmd.BeginDebugGroup("MotionVectors");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hVelocityBuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (pPipeline != nullptr)
    {
      {
        xiiGALMapHelper<xiiMotionVectorConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
        *constants = data.m_Constants;
      }

      cmd.SetPipelineState(pPipeline.Borrow());
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Pixel);
      cmd.ResolveAndSetConstantBuffer("xiiMotionVectorConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.Draw({3U, 1U, 0U, 0U});

      auto& depthPasses                          = m_Resources.m_DepthPasses;
      depthPasses.m_PreviousViewProjectionMatrix = data.m_CurrentViewProjection;
      depthPasses.m_vPreviousJitter              = data.m_vCurrentJitter;
      depthPasses.m_bMotionHistoryValid          = true;
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Velocity Dilation Data //////////
//
// Collects all GPU resources related to velocity dilation for the current frame, including the input velocity texture and the dilated velocity output texture.

void xiiBuiltinRenderPipelineRuntime::SetupVelocityDilation(xiiVelocityDilationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hVelocityInput = builder.ReadTexture(xiiRGBlackboardKeys::k_VelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RG16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hVelocityDilated   = builder.WriteTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DepthPasses.m_pVelocityDilationPipeline, "Shaders/Pipeline/VelocityDilation.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVelocityDilation(const xiiVelocityDilationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VelocityDilation");
  {
    if (m_Resources.m_DepthPasses.m_pVelocityDilationPipeline)
    {
      cmd.SetPipelineState(m_Resources.m_DepthPasses.m_pVelocityDilationPipeline);
      cmd.ResolveAndSetShaderResourceTextureView("g_VelocityInput", context.GetTexture(data.m_hVelocityInput)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_VelocityOutput", context.GetTexture(data.m_hVelocityDilated)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU G-Buffer Base Data //////////
//
// Collects all GPU resources related to base G-Buffer generation for the current frame, including the scene depth input, material targets, stable object identity, and indirect draw commands.

void xiiBuiltinRenderPipelineRuntime::SetupGBufferBase(xiiGBufferBaseData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;

  description.m_Format  = xiiGALResourceFormat::RGBA8UNormalized;
  data.m_hGBufferAlbedo = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format  = xiiGALResourceFormat::RG16SNormalized;
  data.m_hGBufferNormal = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferNormal, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format    = xiiGALResourceFormat::RGBA8UNormalized;
  data.m_hGBufferMaterial = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferMaterial, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format    = xiiGALResourceFormat::RGBA16Float;
  data.m_hGBufferEmissive = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferEmissive, description, xiiGALResourceStateFlags::RenderTarget);

  description.m_Format    = xiiGALResourceFormat::R32UInt;
  data.m_hGBufferObjectId = builder.WriteTexture(xiiRGBlackboardKeys::k_GBufferObjectId, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteGBufferBase(const xiiGBufferBaseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GBufferBase");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.5f, 0.0f, 1.0f, 0.0f));
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hGBufferObjectId)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());

    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_GBufferPasses.m_pGBufferPipeline)
    {
      cmd.SetPipelineState(m_Resources.m_GBufferPasses.m_pGBufferPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Normal Roughness Prepass Data //////////
//
// Collects all GPU resources related to compact normal-roughness generation for the current frame.

void xiiBuiltinRenderPipelineRuntime::SetupNormalRoughnessPrepass(xiiNormalRoughnessPrepassData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hNormalRoughness   = builder.WriteTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, description, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassAllowMerge(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteNormalRoughnessPrepass(const xiiNormalRoughnessPrepassData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("NormalRoughnessPrepass");
  {
    cmd.ClearRenderTargetView(context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.5f, 1.0f));
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_GBufferPasses.m_pNormalRoughnessPipeline)
    {
      cmd.SetPipelineState(m_Resources.m_GBufferPasses.m_pNormalRoughnessPipeline);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU BRDF LUT Generation Data //////////
//
// Collects all GPU resources related to BRDF LUT generation, persisted across frames.
