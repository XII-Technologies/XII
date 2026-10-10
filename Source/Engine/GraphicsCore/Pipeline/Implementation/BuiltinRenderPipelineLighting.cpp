/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupBRDFLutGeneration(xiiBRDFLutGenerationData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  XII_VERIFY(xiiLightingManager::EnsureBRDFLUTResources().Succeeded(), "The shared BRDF LUT resource is unavailable.");
  xiiSharedPtr<xiiGALTexture> pBRDFLUT = xiiLightingManager::GetBRDFLUT();
  data.m_bNeedsGeneration              = xiiLightingManager::IsBRDFLUTGenerationPending();

  data.m_hBRDFLut = builder.ImportTexture(xiiRGBlackboardKeys::k_BRDFLut, pBRDFLUT, pBRDFLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hBRDFLut = builder.WriteTexture(data.m_hBRDFLut, xiiGALResourceStateFlags::UnorderedAccess);
  }

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pBRDFLutPipeline, "Shaders/Pipeline/BRDFLUTGenerate.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteBRDFLutGeneration(const xiiBRDFLutGenerationData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("BRDFLUTGenerate");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pBRDFLutPipeline);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_BRDFLutOut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({32U, 32U, 1U});
    xiiLightingManager::MarkBRDFLUTGenerated();
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Transmittance Data //////////
//
// Collects all GPU resources related to atmosphere transmittance LUT generation, persisted across frames.

void xiiBuiltinRenderPipelineRuntime::SetupAtmosphereTransmittance(xiiAtmosphereTransmittanceData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hCache = m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT;
  if (data.m_hCache.IsInvalidated())
  {
    data.m_hCache = xiiAtmosphereManager::GetDefaultLUTHandle();
  }
  XII_VERIFY(xiiAtmosphereManager::EnsureGpuResources(data.m_hCache).Succeeded(), "Atmosphere LUT resources are unavailable.");
  data.m_bNeedsGeneration                       = xiiAtmosphereManager::IsGenerationPending(data.m_hCache);
  xiiSharedPtr<xiiGALTexture> pTransmittanceLUT = xiiAtmosphereManager::GetTransmittanceLUT(data.m_hCache);
  data.m_hTransmittanceLUT                      = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, pTransmittanceLUT, pTransmittanceLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hTransmittanceLUT = builder.WriteTexture(data.m_hTransmittanceLUT, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants        = CreateAtmosphereConstantsBuffer(builder, "AtmosphereTransmittanceConstants");
    data.m_Constants         = MakeAtmosphereConstants(data.m_hCache);
  }

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pAtmTransmittancePipeline, "Shaders/Pipeline/AtmosphereTransmittance.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteAtmosphereTransmittance(const xiiAtmosphereTransmittanceData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereTransmittanceLUT");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pAtmTransmittancePipeline);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TransmittanceLUT", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({32U, 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Multi-Scatter Data //////////
//
// Collects all GPU resources related to atmosphere multi-scatter LUT generation, persisted across frames.

void xiiBuiltinRenderPipelineRuntime::SetupAtmosphereMultiScatter(xiiAtmosphereMultiScatterData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hCache = m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT;
  if (data.m_hCache.IsInvalidated())
  {
    data.m_hCache = xiiAtmosphereManager::GetDefaultLUTHandle();
  }
  XII_VERIFY(xiiAtmosphereManager::EnsureGpuResources(data.m_hCache).Succeeded(), "Atmosphere LUT resources are unavailable.");
  data.m_bNeedsGeneration                      = xiiAtmosphereManager::IsGenerationPending(data.m_hCache);
  data.m_hTransmittanceLUT                     = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  xiiSharedPtr<xiiGALTexture> pMultiScatterLUT = xiiAtmosphereManager::GetMultiScatterLUT(data.m_hCache);
  data.m_hMultiScatterLUT                      = builder.ImportTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, pMultiScatterLUT, pMultiScatterLUT->GetResourceState());

  if (data.m_bNeedsGeneration)
  {
    data.m_hMultiScatterLUT = builder.WriteTexture(data.m_hMultiScatterLUT, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_hConstants       = CreateAtmosphereConstantsBuffer(builder, "AtmosphereMultiScatterConstants");
    data.m_Constants        = MakeAtmosphereConstants(data.m_hCache);
  }

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pAtmMultiScatterPipeline, "Shaders/Pipeline/AtmosphereMultiScatter.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteAtmosphereMultiScatter(const xiiAtmosphereMultiScatterData& data, xiiRenderGraphPassContext& context)
{
  if (!data.m_bNeedsGeneration)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("AtmosphereMultiScatterLUT");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pAtmMultiScatterPipeline);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TransmittanceLUT", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_MultiScatterLUT", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({4U, 4U, 1U});
    xiiAtmosphereManager::MarkLUTsGenerated(data.m_hCache);
  }
  cmd.EndDebugGroup();
}

////////// GPU Sky Irradiance Convolution Data //////////
//
// Collects all GPU resources related to sky irradiance convolution.

void xiiBuiltinRenderPipelineRuntime::SetupSkyIrradianceConvolution(xiiSkyIrradianceConvolutionData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hSkyRadiance       = builder.WriteTexture(xiiRGBlackboardKeys::k_SkyRadiance, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants         = CreateAtmosphereConstantsBuffer(builder, "SkyRadianceConstants");
  data.m_Constants          = MakeAtmosphereConstants(m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pSkyIrradiancePipeline, "Shaders/Pipeline/SkyRadiance.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteSkyIrradianceConvolution(const xiiSkyIrradianceConvolutionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SkyIrradianceConvolution");
  {
    UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hConstants), data.m_Constants);
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pSkyIrradiancePipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SkyOut", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Initialization Data //////////
//
// Collects all GPU resources related to volumetric fog froxel initialization.

void xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogInitialization(xiiVolumetricFogInitializationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelMetadata   = builder.ReadBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hFroxelScattering = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pFroxelFogInitPipeline, "Shaders/Pipeline/FroxelScatteringInitialize.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogInitialization(const xiiVolumetricFogInitializationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogInitialization");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pFroxelFogInitPipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_FroxelMeta", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScatterOut", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 8U});
  }
  cmd.EndDebugGroup();
}

////////// GPU DDGI Probe Sampling Data //////////
//
// Collects all GPU resources related to DDGI final gather probe sampling.

void xiiBuiltinRenderPipelineRuntime::SetupDDGIProbeSampling(xiiDDGIProbeSamplingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal        = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeIrradianceAtlas = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIProbeIrradianceAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeDistanceAtlas   = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIProbeDistanceAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeStates          = builder.ReadBuffer(xiiRGBlackboardKeys::k_DDGIProbeStates, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProbeConstants       = builder.ReadBuffer(xiiRGBlackboardKeys::k_DDGIProbeConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hDDGIIrradiance    = builder.WriteTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pDDGIProbePipeline, "Shaders/Pipeline/DDGIFinalGather.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDDGIProbeSampling(const xiiDDGIProbeSamplingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DDGIProbeSampling");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pDDGIProbePipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiDDGIProbeConstants", context.GetBuffer(data.m_hProbeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIIrradianceAtlas", context.GetTexture(data.m_hProbeIrradianceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIDistanceAtlas", context.GetTexture(data.m_hProbeDistanceAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_DDGIProbeStates", context.GetBuffer(data.m_hProbeStates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DDGIOut", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupSparseVoxelRadianceGather(xiiSparseVoxelRadianceGatherData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRadiancePool  = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelRadiancePool, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPageTable     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelPageTable, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLevelData     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelLevelData, xiiGALResourceStateFlags::ShaderResource);
  data.m_hConstants     = builder.ReadBuffer(xiiRGBlackboardKeys::k_SparseVoxelConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hIrradiance        = builder.WriteTexture(xiiRGBlackboardKeys::k_SparseVoxelIrradiance, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPrepPasses.m_pSparseVoxelGatherPipeline, "Shaders/Pipeline/SparseVoxelRadianceGather.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteSparseVoxelRadianceGather(const xiiSparseVoxelRadianceGatherData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("SparseVoxelRadianceGather");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pSparseVoxelGatherPipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSparseVoxelRadianceConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelPageTable", context.GetBuffer(data.m_hPageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelLevels", context.GetBuffer(data.m_hLevelData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SparseVoxelRadiancePool", context.GetBuffer(data.m_hRadiancePool)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SparseVoxelIrradianceOut", context.GetTexture(data.m_hIrradiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Data //////////
//
// Collects all GPU resources related to ground-truth ambient occlusion generation.

void xiiBuiltinRenderPipelineRuntime::SetupGroundTruthAmbientOcclusion(xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hNormalRoughness = builder.ReadTexture(xiiRGBlackboardKeys::k_NormalRoughnessBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type          = xiiGALResourceDimension::Texture2D;
  description.m_Format        = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width    = m_View.GetRenderResolutionWidth();
  description.m_Size.height   = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels   = 1U;
  description.m_BindFlags     = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage         = xiiGALResourceUsage::Default;
  data.m_hRawAmbientOcclusion = builder.WriteTexture(xiiRGBlackboardKeys::k_RawAOTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto&       lightingPrepPasses = m_Resources.m_LightingPrepPasses;
  const auto& lightingPasses     = m_Resources.m_LightingPasses;
  data.m_bUseHardwareRayTracing  = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingAmbientOcclusionResources();
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = lightingPrepPasses.m_pRTAOPipeline;
    data.m_uiShaderRecordStride = lightingPrepPasses.m_uiRTAOShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT AO Shader Binding Table", lightingPrepPasses.m_pRTAOShaderBindingTable, lightingPrepPasses.m_pRTAOShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
    builder.SetPassAllowMerge(false);
  }
  else
  {
    xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(lightingPrepPasses.m_pGTAOFallbackPipeline, "Shaders/Pipeline/GTAO.xiiShader");
  }
}

void xiiBuiltinRenderPipelineRuntime::ExecuteGroundTruthAmbientOcclusion(const xiiGroundTruthAmbientOcclusionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusion");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", xiiGALShaderType::RayAnyHit);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
      xiiGALUpdateSBTDescription sbtUpdate;
      sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
      sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
      sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
      sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
      sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
      cmd.UpdateSBT(sbtUpdate);

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, m_View.GetRenderResolutionWidth(), m_View.GetRenderResolutionHeight());
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pGTAOFallbackPipeline);
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_NormalRoughness", context.GetTexture(data.m_hNormalRoughness)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_AOOut", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ground Truth Ambient Occlusion Denoise Data //////////
//
// Collects all GPU resources related to denoising ground-truth ambient occlusion.

void xiiBuiltinRenderPipelineRuntime::SetupGroundTruthAmbientOcclusionDenoise(xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = m_View.GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiFrameIndex & 1U;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_LightingPrepPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pAmbientOcclusionHistory, xiiGALResourceFormat::R8UNormalized, uiWidth, uiHeight))
  {
    resources.m_bAmbientOcclusionHistoryValid = false;
  }

  data.m_bHistoryValid             = resources.m_bAmbientOcclusionHistoryValid;
  data.m_hRawAmbientOcclusion      = builder.ReadTexture(xiiRGBlackboardKeys::k_RawAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth               = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal            = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                 = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface          = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousAmbientOcclusion = builder.ReadTexture(builder.ImportTexture("AO Previous History", resources.m_pAmbientOcclusionHistory[uiPreviousSlot], resources.m_pAmbientOcclusionHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion   = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_StableAOTexture, resources.m_pAmbientOcclusionHistory[uiCurrentSlot], resources.m_pAmbientOcclusionHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hConstants = CreateTemporalDenoiseConstants(builder, "AO Temporal Denoise Constants");

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pAOTemporalDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteGroundTruthAmbientOcclusionDenoise(const xiiGroundTruthAmbientOcclusionDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GroundTruthAmbientOcclusionDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.92f;
      pConstants->DepthThreshold  = 0.08f;
      pConstants->NormalThreshold = 0.82f;
      pConstants->SpatialWeight   = 0.35f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 0U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPrepPasses.m_pAOTemporalDenoisePipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRawAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPrepPasses.m_bAmbientOcclusionHistoryValid = true;
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Direct Lighting Data //////////
//
// Collects all GPU resources related to deferred direct lighting.

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRDITemporal(xiiReSTIRDITemporalData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiHeight = m_View.GetRenderResolutionHeight();

  auto HistoryMatchesResolution = [uiWidth, uiHeight](const xiiSharedPtr<xiiGALTexture>& pTexture) {
    return pTexture != nullptr && pTexture->GetDescription().m_Size.width == uiWidth && pTexture->GetDescription().m_Size.height == uiHeight;
  };

  auto& lightingPasses = m_Resources.m_LightingPasses;
  bool  bHistoryReset  = false;

  if (!HistoryMatchesResolution(lightingPasses.m_pDirectReservoirHistory[0]) ||
      !HistoryMatchesResolution(lightingPasses.m_pDirectReservoirHistory[1]))
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA32UInt;
    description.m_Size.width  = uiWidth;
    description.m_Size.height = uiHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    for (xiiUInt32 uiSlot = 0U; uiSlot < 2U; ++uiSlot)
    {
      lightingPasses.m_pDirectReservoirHistory[uiSlot] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    }
    bHistoryReset = true;
  }

  if (!HistoryMatchesResolution(lightingPasses.m_pDirectSurfaceHistory[0]) ||
      !HistoryMatchesResolution(lightingPasses.m_pDirectSurfaceHistory[1]))
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = uiWidth;
    description.m_Size.height = uiHeight;
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    for (xiiUInt32 uiSlot = 0U; uiSlot < 2U; ++uiSlot)
    {
      lightingPasses.m_pDirectSurfaceHistory[uiSlot] = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
    }
    bHistoryReset = true;
  }

  if (bHistoryReset)
    lightingPasses.m_bDirectReservoirHistoryValid = false;

  data.m_bHistoryValid           = lightingPasses.m_bDirectReservoirHistoryValid;
  const xiiUInt32 uiPreviousSlot = (lightingPasses.m_uiFrameIndex + 1U) & 1U;

  data.m_hSceneDepth              = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer         = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer        = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightSamplingTotals     = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightSamplingTotals, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangles       = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveLightHeader     = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangleAliases = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangleAliases, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveBlockAliases    = builder.ReadBuffer(lightingPasses.m_hEmissiveBlockAliases, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveAliasHeader     = builder.ReadBuffer(lightingPasses.m_hEmissiveAliasHeader, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReservoir       = builder.ReadTexture(builder.ImportTexture("ReSTIRDIReservoirPrevious", lightingPasses.m_pDirectReservoirHistory[uiPreviousSlot], lightingPasses.m_pDirectReservoirHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface         = builder.ReadTexture(builder.ImportTexture("ReSTIRDISurfacePrevious", lightingPasses.m_pDirectSurfaceHistory[uiPreviousSlot], lightingPasses.m_pDirectSurfaceHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRDIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hConstants                     = builder.WriteBuffer("xiiReSTIRDIConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA32UInt;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hTemporalReservoir = builder.WriteTexture("ReSTIRDITemporalReservoir", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pReSTIRDITemporalPipeline, "Shaders/Pipeline/ReSTIRDITemporal.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDITemporal(const xiiReSTIRDITemporalData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDITemporal");
  {
    {
      xiiGALMapHelper<xiiReSTIRDIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding     = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReSTIRDITemporalPipeline);
    m_Resources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRDIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightSamplingTotals", context.GetBuffer(data.m_hLightSamplingTotals)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangleAliases", context.GetBuffer(data.m_hEmissiveTriangleAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveBlockAliases", context.GetBuffer(data.m_hEmissiveBlockAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveAliasHeader", context.GetBuffer(data.m_hEmissiveAliasHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousReservoir", context.GetTexture(data.m_hPreviousReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalReservoir", context.GetTexture(data.m_hTemporalReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRDISpatial(xiiReSTIRDISpatialData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiFrameIndex & 1U;
  auto&           lightingPasses = m_Resources.m_LightingPasses;

  data.m_hSceneDepth          = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo       = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal       = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalReservoir   = builder.ReadTexture("ReSTIRDITemporalReservoir", xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangles   = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveLightHeader = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoir           = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DirectLightReservoir, lightingPasses.m_pDirectReservoirHistory[uiCurrentSlot], lightingPasses.m_pDirectReservoirHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirSurface    = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_DirectLightReservoirSurface, lightingPasses.m_pDirectSurfaceHistory[uiCurrentSlot], lightingPasses.m_pDirectSurfaceHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pReSTIRDISpatialPipeline, "Shaders/Pipeline/ReSTIRDISpatial.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDISpatial(const xiiReSTIRDISpatialData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDISpatial");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReSTIRDISpatialPipeline);
    m_Resources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalReservoir", context.GetTexture(data.m_hTemporalReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirOut", context.GetTexture(data.m_hReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SurfaceOut", context.GetTexture(data.m_hReservoirSurface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRDIVisibility(xiiReSTIRDIVisibilityData& data, xiiRenderGraphBuilder& builder)
{
  auto& lightingPasses = m_Resources.m_LightingPasses;

  data.m_hSceneDepth                  = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal               = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangles           = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveLightHeader         = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoir                   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoir, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(lightingPasses.m_pReSTIRDIVisibilityPipeline, "Shaders/Pipeline/ReSTIRDIVisibility.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDIVisibility(const xiiReSTIRDIVisibilityData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDIVisibility");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReSTIRDIVisibilityPipeline);
    m_Resources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_LocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LocalShadowAtlasDescs", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Reservoir", context.GetTexture(data.m_hReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRDIEmissiveVisibility(xiiReSTIRDIEmissiveVisibilityData& data, xiiRenderGraphBuilder& builder)
{
  auto& lightingPasses = m_Resources.m_LightingPasses;

  data.m_hSceneDepth          = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal       = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangles   = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveLightHeader = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);

  data.m_bUseHardwareRayTracing = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingReSTIRDIEmissiveVisibilityResources();
  data.m_hReservoir             = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoir,
                                                      data.m_bUseHardwareRayTracing ? xiiGALResourceStateFlags::UnorderedAccess : xiiGALResourceStateFlags::ShaderResource);

  if (data.m_bUseHardwareRayTracing)
  {
    data.m_hReservoir           = builder.WriteTexture(data.m_hReservoir, xiiGALResourceStateFlags::UnorderedAccess);
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = lightingPasses.m_pReSTIRDIEmissiveVisibilityPipeline;
    data.m_uiShaderRecordStride = lightingPasses.m_uiReSTIRDIEmissiveVisibilityShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);

    data.m_hShaderBindingTable = builder.ImportBuffer("ReSTIR DI Emissive Visibility Shader Binding Table", lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable,
                                                      lightingPasses.m_pReSTIRDIEmissiveVisibilityShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
  }

  // The pass completes the CPU-visible reservoir history state even when the deterministic
  // non-RT fallback keeps the analytical pass' unoccluded emissive result unchanged.
  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRDIEmissiveVisibility(const xiiReSTIRDIEmissiveVisibilityData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRDIEmissiveVisibility");
  if (data.m_bUseHardwareRayTracing)
  {
    cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
    cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_Reservoir", context.GetTexture(data.m_hReservoir)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
    cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
    cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayAnyHit);
    if (xiiGALBindlessResourceTable::IsInitialized())
    {
      xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", xiiGALShaderType::RayAnyHit);
      xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", xiiGALShaderType::RayAnyHit);
    }
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

    const xiiUInt64            uiStride = data.m_uiShaderRecordStride;
    xiiGALUpdateSBTDescription sbtUpdate;
    sbtUpdate.m_pPipelineState      = data.m_pRayTracingPipeline.Borrow();
    sbtUpdate.m_pShaderBindingTable = context.GetBuffer(data.m_hShaderBindingTable);
    sbtUpdate.m_RayGenerationTable  = {0U, uiStride, uiStride};
    sbtUpdate.m_MissTable           = {uiStride, uiStride, uiStride};
    sbtUpdate.m_HitTable            = {uiStride * 2U, uiStride, uiStride};
    cmd.UpdateSBT(sbtUpdate);

    xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, m_View.GetRenderResolutionWidth(), m_View.GetRenderResolutionHeight());
    trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
    trace.m_MissTable          = sbtUpdate.m_MissTable;
    trace.m_HitTable           = sbtUpdate.m_HitTable;
    cmd.TraceRays(trace);
  }
  m_Resources.m_LightingPasses.m_bDirectReservoirHistoryValid = true;
  cmd.EndDebugGroup();
}

/// Publishes shadow-sampling constants independently of a particular opaque lighting path.
/// Forward+, deferred lighting, and volumetrics can therefore share one authoritative frame setup.

void xiiBuiltinRenderPipelineRuntime::SetupShadowSamplingConstants(xiiShadowSamplingConstantsData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiCloudShadowConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  data.m_hCloudShadowConstants          = builder.WriteBuffer(xiiRGBlackboardKeys::k_CloudShadowConstants, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  constantsDescription.m_uiSize                                 = sizeof(xiiVirtualShadowSamplingConstants);
  data.m_hVirtualShadowSamplingConstants                        = builder.WriteBuffer(xiiRGBlackboardKeys::k_VirtualShadowSamplingConstants, constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
  const xiiVirtualShadowMapSettings& settings                   = xiiVirtualShadowMapManager::GetConfiguration();
  const xiiVirtualShadowMapStats     statistics                 = xiiVirtualShadowMapManager::GetStats();
  data.m_VirtualShadowConstants.VirtualShadowPageTableBaseIndex = 0U;
  data.m_VirtualShadowConstants.VirtualShadowPageTableCapacity  = 0U;
  XII_IGNORE_UNUSED(m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_VirtualShadowTableBaseIndex, data.m_VirtualShadowConstants.VirtualShadowPageTableBaseIndex));
  XII_IGNORE_UNUSED(m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_VirtualShadowTableCapacity, data.m_VirtualShadowConstants.VirtualShadowPageTableCapacity));
  data.m_VirtualShadowConstants.VirtualShadowResolution          = settings.m_uiVirtualResolution;
  data.m_VirtualShadowConstants.VirtualShadowPageSize            = settings.m_uiPageSize;
  data.m_VirtualShadowConstants.VirtualShadowPhysicalAtlasWidth  = statistics.m_uiPhysicalAtlasWidth;
  data.m_VirtualShadowConstants.VirtualShadowPhysicalAtlasHeight = statistics.m_uiPhysicalAtlasHeight;
  data.m_VirtualShadowConstants.VirtualShadowDirectionalLightId  = 0U;
  data.m_VirtualShadowConstants.VirtualShadowEnabled             = 0U;
  if (m_View.m_pExtractedData != nullptr)
  {
    const xiiDirectionalLightRenderData* pMainDirectional = SelectMainDirectionalLight(m_View.m_pExtractedData->GetAllRenderData());
    if (pMainDirectional != nullptr && pMainDirectional->m_bCastShadows)
    {
      data.m_VirtualShadowConstants.VirtualShadowDirectionalLightId = static_cast<xiiUInt32>(pMainDirectional->m_uiSortingKey) & 0x00FFFFFFU;
      data.m_VirtualShadowConstants.VirtualShadowEnabled            = 1U;
    }
  }

  const auto& cloudState                                 = m_Resources.m_LightingPasses.m_CloudShadowState;
  data.m_CloudShadowConstants.LayerOriginAndInvScale     = cloudState.m_vLayerOriginAndInvScale;
  data.m_CloudShadowConstants.ProjectionAxisUAndDetail   = cloudState.m_vProjectionAxisUAndDetail;
  data.m_CloudShadowConstants.ProjectionAxisVAndCoverage = cloudState.m_vProjectionAxisVAndCoverage;
  data.m_CloudShadowConstants.LayerNormalAndOpticalDepth = cloudState.m_vLayerNormalAndOpticalDepth;
  data.m_CloudShadowConstants.WindStrengthAndEnabled     = cloudState.m_vWindStrengthAndEnabled;

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteShadowSamplingConstants(const xiiShadowSamplingConstantsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();
  cmd.BeginDebugGroup("ShadowSamplingConstants");
  {
    xiiGALMapHelper<xiiCloudShadowConstants> cloudConstants(cmd, context.GetBuffer(data.m_hCloudShadowConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *cloudConstants = data.m_CloudShadowConstants;
  }
  {
    xiiGALMapHelper<xiiVirtualShadowSamplingConstants> virtualConstants(cmd, context.GetBuffer(data.m_hVirtualShadowSamplingConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
    *virtualConstants = data.m_VirtualShadowConstants;
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupDirectLighting(xiiDeferredDirectLightingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hGBufferAlbedo                  = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal                  = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial                = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive                = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                     = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalShadowMask       = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hContactShadowTerm              = builder.ReadTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, xiiGALResourceStateFlags::ShaderResource);
  data.m_hShadowCascadeConstants         = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hDirectionalShadowAtlas         = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas               = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors    = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer                = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer               = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveTriangles              = builder.ReadBuffer(m_Resources.m_LightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
  data.m_hEmissiveLightHeader            = builder.ReadBuffer(m_Resources.m_LightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDirectLightReservoir           = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoir, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoirSurface               = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightReservoirSurface, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCloudShadowConstants           = builder.ReadBuffer(xiiRGBlackboardKeys::k_CloudShadowConstants, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hVirtualShadowPageTable         = builder.ReadBuffer(xiiRGBlackboardKeys::k_VirtualShadowPageTable, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVirtualShadowAtlas             = builder.ReadTexture(xiiRGBlackboardKeys::k_VirtualShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVirtualShadowSamplingConstants = builder.ReadBuffer(xiiRGBlackboardKeys::k_VirtualShadowSamplingConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = m_View.GetRenderResolutionWidth();
  description.m_Size.height    = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hDirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pDirectLightingPipeline, "Shaders/Pipeline/DirectLighting.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDirectLighting(const xiiDeferredDirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredDirectLighting");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pDirectLightingPipeline);
    m_Resources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RTShadow", context.GetTexture(data.m_hRayTracedFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ContactShadow", context.GetTexture(data.m_hContactShadowTerm)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hShadowCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiCloudShadowConstants", context.GetBuffer(data.m_hCloudShadowConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVirtualShadowSamplingConstants", context.GetBuffer(data.m_hVirtualShadowSamplingConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VirtualShadowAtlas", context.GetTexture(data.m_hVirtualShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VirtualShadowPageTable", context.GetBuffer(data.m_hVirtualShadowPageTable)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_LocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LocalShadowAtlasDescs", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DirectLightReservoir", context.GetTexture(data.m_hDirectLightReservoir)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DirectOut", context.GetTexture(data.m_hDirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Deferred Indirect Lighting Data //////////
//
// Collects all GPU resources related to deferred indirect lighting.

void xiiBuiltinRenderPipelineRuntime::SetupIndirectLighting(xiiDeferredIndirectLightingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hGBufferAlbedo           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth              = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hStableAmbientOcclusion  = builder.ReadTexture(xiiRGBlackboardKeys::k_StableAOTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut                 = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDDGIIrradiance          = builder.ReadTexture(xiiRGBlackboardKeys::k_DDGIIrradiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSparseVoxelIrradiance   = builder.ReadTexture(xiiRGBlackboardKeys::k_SparseVoxelIrradiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSkyRadiance             = builder.ReadTexture(xiiRGBlackboardKeys::k_SkyRadiance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReflectionProbeData     = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, xiiGALResourceStateFlags::ShaderResource);
  data.m_hReflectionProbeClusters = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, xiiGALResourceStateFlags::ShaderResource);

  auto& reflectionResources = m_Resources.m_LightingPrepPasses;
  XII_ASSERT_DEV(reflectionResources.m_pFallbackReflectionProbeTexture != nullptr, "Reflection probe selection must initialize the fallback cubemap before indirect lighting setup.");
  data.m_hFallbackReflectionProbe = builder.ReadTexture(
    builder.ImportTexture("ReflectionProbeFallback", reflectionResources.m_pFallbackReflectionProbeTexture, reflectionResources.m_pFallbackReflectionProbeTexture->GetResourceState()),
    xiiGALResourceStateFlags::ShaderResource);

  data.m_hReflectionProbeTextures.Reserve(reflectionResources.m_ReflectionProbeTextures.GetCount());
  for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < reflectionResources.m_ReflectionProbeTextures.GetCount(); ++uiProbeIndex)
  {
    xiiStringBuilder sResourceName;
    sResourceName.SetFormat("ReflectionProbeTexture_{0}", uiProbeIndex);
    data.m_hReflectionProbeTextures.PushBack(builder.ReadTexture(
      builder.ImportTexture(sResourceName, reflectionResources.m_ReflectionProbeTextures[uiProbeIndex], reflectionResources.m_ReflectionProbeTextures[uiProbeIndex]->GetResourceState()),
      xiiGALResourceStateFlags::ShaderResource));
  }

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = m_View.GetRenderResolutionWidth();
  description.m_Size.height      = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hIndirectLightingBuffer = builder.WriteTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hEnvironmentSpecular    = builder.WriteTexture(xiiRGBlackboardKeys::k_EnvironmentSpecular, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pIndirectLightingPipeline, "Shaders/Pipeline/IndirectLighting.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteIndirectLighting(const xiiDeferredIndirectLightingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DeferredIndirectLighting");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pIndirectLightingPipeline);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_AOTerm", context.GetTexture(data.m_hStableAmbientOcclusion)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DDGIIr", context.GetTexture(data.m_hDDGIIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SparseVoxelIrradiance", context.GetTexture(data.m_hSparseVoxelIrradiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SkyRadiance", context.GetTexture(data.m_hSkyRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hReflectionProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeClusters", context.GetBuffer(data.m_hReflectionProbeClusters)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);

    xiiHybridArray<xiiGALTextureView*, XII_MAX_REFLECTION_PROBES> reflectionProbeViews;
    reflectionProbeViews.SetCount(XII_MAX_REFLECTION_PROBES);
    xiiGALTextureView* pFallbackView = context.GetTexture(data.m_hFallbackReflectionProbe)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < XII_MAX_REFLECTION_PROBES; ++uiProbeIndex)
    {
      reflectionProbeViews[uiProbeIndex] = uiProbeIndex < data.m_hReflectionProbeTextures.GetCount() ?
        context.GetTexture(data.m_hReflectionProbeTextures[uiProbeIndex])->GetDefaultView(xiiGALTextureViewType::ShaderResource) :
        pFallbackView;
    }
    cmd.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeTextures", 0U, reflectionProbeViews, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_IndirectOut", context.GetTexture(data.m_hIndirectLightingBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_EnvironmentSpecularOut", context.GetTexture(data.m_hEnvironmentSpecular)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Global Illumination Data //////////
//
// Collects all GPU resources related to ray traced global illumination final gather.
