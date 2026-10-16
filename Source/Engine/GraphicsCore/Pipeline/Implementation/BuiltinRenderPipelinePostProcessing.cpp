/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupLuminanceHistogram(xiiLuminanceHistogramData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRIn = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = description.m_uiElementByteStride * 256U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hHistogram                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureHistogramConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Histogram Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TemporalPasses.m_pLuminanceHistogramPipeline, "Shaders/Pipeline/ExposureHistogram.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLuminanceHistogram(const xiiLuminanceHistogramData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = m_View.GetRenderResolutionHeight();

  cmd.BeginDebugGroup("LuminanceHistogram");
  {
    const xiiExposureSettings& settings = m_DisplayOutputSettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureHistogramConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, 1.0f / fRange, fRange);
      pConstants->InputResolution   = xiiVec2U32(uiRenderWidth, uiRenderHeight);
      pConstants->_Padding          = xiiVec2::MakeZero();
    }

    xiiUInt32 zeroHistogram[256] = {};
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hHistogram), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroHistogram), sizeof(zeroHistogram)));
    cmd.SetPipelineState(m_Resources.m_TemporalPasses.m_pLuminanceHistogramPipeline);
    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiExposureHistogramConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRInput", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 15U) / 16U, (uiRenderHeight + 15U) / 16U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Auto Exposure Data //////////
//
// Collects all GPU resources related to histogram-based exposure adaptation.

void xiiBuiltinRenderPipelineRuntime::SetupAutoExposure(xiiAutoExposureData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHistogram = builder.ReadBuffer(xiiRGBlackboardKeys::k_LuminanceHistogram, xiiGALResourceStateFlags::ShaderResource);

  data.m_hExposure     = builder.ImportBuffer(xiiRGBlackboardKeys::k_CurrentExposure, m_Resources.m_TemporalPasses.m_pExposureBuffer, m_Resources.m_TemporalPasses.m_pExposureBuffer->GetResourceState());
  data.m_hExposure     = builder.WriteBuffer(data.m_hExposure, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_bHistoryValid = m_Resources.m_TemporalPasses.m_bExposureHistoryValid;

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiExposureAdaptationConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Exposure Adaptation Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TemporalPasses.m_pAutoExposurePipeline, "Shaders/Pipeline/ExposureAdaptation.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteAutoExposure(const xiiAutoExposureData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AutoExposure");
  {
    const xiiExposureSettings& settings = m_DisplayOutputSettings.m_Exposure;
    const float                fRange   = settings.m_fMaximumLogLuminance - settings.m_fMinimumLogLuminance;
    {
      xiiGALMapHelper<xiiExposureAdaptationConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->LogLuminanceRange = xiiVec4(settings.m_fMinimumLogLuminance, settings.m_fMaximumLogLuminance, fRange, 0.0f);
      pConstants->Metering          = xiiVec4(settings.m_fLowPercentile, settings.m_fHighPercentile, settings.m_fAdaptationSpeedBright, settings.m_fAdaptationSpeedDark);
      pConstants->Exposure          = xiiVec4(xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 0.25f), settings.m_fMinimumEV100, settings.m_fMaximumEV100, settings.m_fExposureCompensation);
      pConstants->ManualExposure    = m_View.GetCamera() != nullptr ? m_View.GetCamera()->GetExposure() : 1.0f;
      pConstants->AutomaticExposure = settings.m_Mode == xiiExposureMode::Automatic ? 1U : 0U;
      pConstants->HistoryValid      = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding          = 0.0f;
    }

    cmd.SetPipelineState(m_Resources.m_TemporalPasses.m_pAutoExposurePipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiExposureAdaptationConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Histogram", context.GetBuffer(data.m_hHistogram)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Exposure", context.GetBuffer(data.m_hExposure)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({1U, 1U, 1U});
    m_Resources.m_TemporalPasses.m_bExposureHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Temporal Anti-Aliasing Data //////////
//
// Collects all GPU resources related to temporal anti-aliasing resolve.

void xiiBuiltinRenderPipelineRuntime::SetupTemporalAntiAliasing(xiiTemporalAntiAliasingData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = m_View.GetRenderResolutionHeight();

  data.m_hHDRIn    = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  auto& temporalResources    = m_Resources.m_TemporalPasses;
  data.m_uiHistoryWriteIndex = temporalResources.m_uiTAAHistoryWriteIndex;
  data.m_bHistoryValid       = temporalResources.m_bTAAHistoryValid;

  const xiiUInt32 uiHistoryReadIndex = 1U - data.m_uiHistoryWriteIndex;
  data.m_hHistory                    = builder.ImportTexture("TAAHistory", temporalResources.m_pTAAHistoryBuffers[uiHistoryReadIndex], temporalResources.m_pTAAHistoryBuffers[uiHistoryReadIndex]->GetResourceState());
  data.m_hHistory                    = builder.ReadTexture(data.m_hHistory, xiiGALResourceStateFlags::ShaderResource);

  data.m_hTAAOut = builder.ImportTexture(xiiRGBlackboardKeys::k_TAAResolvedColor, temporalResources.m_pTAAHistoryBuffers[data.m_uiHistoryWriteIndex], temporalResources.m_pTAAHistoryBuffers[data.m_uiHistoryWriteIndex]->GetResourceState());
  data.m_hTAAOut = builder.WriteTexture(data.m_hTAAOut, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiTAAConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("TAA Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_Constants.HistoryValid     = data.m_bHistoryValid ? 1U : 0U;
  data.m_Constants.BaseBlendAlpha   = 0.08f;
  data.m_Constants.MotionBlendScale = 12.0f;
  data.m_Constants.Padding          = 0.0f;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TemporalPasses.m_pTAAPipeline, "Shaders/Pipeline/TAA.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteTemporalAntiAliasing(const xiiTemporalAntiAliasingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = m_View.GetRenderResolutionHeight();

  cmd.BeginDebugGroup("TAA");
  {
    {
      xiiGALMapHelper<xiiTAAConstants> constants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *constants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_TemporalPasses.m_pTAAPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiTAAConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentFrame", context.GetTexture(data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VelocityBuffer", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HistoryFrame", context.GetTexture(data.m_bHistoryValid ? data.m_hHistory : data.m_hHDRIn)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Resolved", context.GetTexture(data.m_hTAAOut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});

    m_Resources.m_TemporalPasses.m_uiTAAHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
    m_Resources.m_TemporalPasses.m_bTAAHistoryValid       = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Upscale Data //////////
//
// Collects all GPU resources related to temporal upscaling.



////////// GPU Bloom Data //////////
//
// Collects all GPU resources related to bloom generation.



////////// GPU Color Grading Data //////////
//
// Collects all GPU resources related to color grading.



////////// GPU Tone Mapping Data //////////
//
// Collects all GPU resources related to tone mapping from HDR to LDR.



////////// GPU Final Blit Data //////////
//
// Collects all GPU resources related to final backbuffer presentation.

void xiiBuiltinRenderPipelineRuntime::SetupFinalBlit(xiiFinalBlitData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDisplayLinear = builder.ReadTexture(xiiRGBlackboardKeys::k_GradedColor, xiiGALResourceStateFlags::ShaderResource);

  if (const xiiGALSwapChain* pSwapChain = m_View.GetSwapChain(); pSwapChain != nullptr)
  {
    xiiSharedPtr<xiiGALTexture> pBackbufferTexture = pSwapChain->GetBackBufferTexture();
    if (pBackbufferTexture)
    {
      data.m_hBackbuffer        = builder.ImportTexture("Backbuffer", pBackbufferTexture, xiiGALResourceStateFlags::RenderTarget);
      data.m_hBackbuffer        = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
      data.m_bApplySRGBTransfer = !xiiGALResourceFormat::IsSrgb(pBackbufferTexture->GetDescription().m_Format);
    }
  }

  else if (auto* target = m_View.GetRenderTargetView(); target != nullptr)
  {
    auto texture              = target->GetTexture();
    data.m_hBackbuffer        = builder.ImportTexture("ViewOutput", texture, texture->GetResourceState());
    data.m_hBackbuffer        = builder.WriteTexture(data.m_hBackbuffer, xiiGALResourceStateFlags::RenderTarget);
    data.m_bApplySRGBTransfer = !xiiGALResourceFormat::IsSrgb(texture->GetDescription().m_Format);
    // Offscreen targets need not be sampleable. Preserve the useful sampled-output state when
    // supported, but do not request a layout excluded by the caller's texture bind flags.
    builder.ExportTexture(data.m_hBackbuffer, texture->GetDescription().m_BindFlags.IsSet(xiiGALBindFlags::ShaderResource) ? xiiGALResourceStateFlags::ShaderResource : xiiGALResourceStateFlags::RenderTarget);
  }

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiFinalBlitConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Final Blit Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
  builder.SetPassRenderPassManaged(data.m_hBackbuffer.IsValid());
  builder.SetPassPreparation([this, &data](xiiRenderGraphPassContext& context) {
    auto& cmd = context.GetCommandList();
    {
      xiiGALMapHelper<xiiFinalBlitConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->OutputMode         = m_DisplayOutputSettings.m_OutputMode.GetValue();
      pConstants->ApplySRGBTransfer  = data.m_bApplySRGBTransfer ? 1U : 0U;
      pConstants->PaperWhiteNits     = m_DisplayOutputSettings.m_fPaperWhiteNits;
      pConstants->MaximumDisplayNits = m_DisplayOutputSettings.m_fMaximumDisplayNits;
    }
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = context.GetBuffer(data.m_hConstants);
    transition.m_NewState        = xiiGALResourceStateFlags::ConstantBuffer;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    cmd.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  });
}

void xiiBuiltinRenderPipelineRuntime::ExecuteFinalBlit(const xiiFinalBlitData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList&                        cmd = context.GetCommandList();
  xiiSharedPtr<xiiGALGraphicsPipelineState> pPipeline;
  if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && context.GetRenderPass() != nullptr)
  {
    pPipeline = xiiBuiltinRenderPipelineRuntime::EnsureGraphicsPipeline(m_Resources.m_OutputPasses.m_pFinalBlitPipeline, "Shaders/Pipeline/FinalBlit.xiiShader", context.GetRenderPass(), context.GetSubpassIndex());
  }

  cmd.BeginDebugGroup("BackbufferPresent");
  {
    if (data.m_hDisplayLinear.IsValid() && data.m_hBackbuffer.IsValid() && data.m_hConstants.IsValid() && pPipeline != nullptr)
    {


      ClearRenderTargetPreservingState(cmd, context.GetTexture(data.m_hBackbuffer)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(0.0f, 0.0f, 0.0f, 1.0f));
      cmd.SetViewport({0.0f, 0.0f, m_View.m_Data.m_ViewPortRect.width, m_View.m_Data.m_ViewPortRect.height, 0.0f, 1.0f});
      cmd.SetPipelineState(pPipeline.Borrow());
      BindFrameResources(cmd);
      cmd.ResolveAndSetConstantBuffer("xiiFinalBlitConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Pixel);
      cmd.ResolveAndSetShaderResourceTextureView("g_FinalColor", context.GetTexture(data.m_hDisplayLinear)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Verify).IgnoreResult();
      cmd.Draw({3U, 1U, 0U, 0U});
    }
  }
  cmd.EndDebugGroup();
}
