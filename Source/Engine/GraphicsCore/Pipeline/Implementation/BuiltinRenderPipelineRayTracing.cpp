/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupRayTracedGlobalIllumination(xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type                     = xiiGALResourceDimension::Texture2D;
  description.m_Format                   = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width               = m_View.GetRenderResolutionWidth();
  description.m_Size.height              = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels              = 1U;
  description.m_BindFlags                = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage                    = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawGlobalIllumination = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto& lightingPasses                     = m_Resources.m_LightingPasses;
  data.m_bUseHardwareRayTracing            = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingGlobalIlluminationResources();
  lightingPasses.m_bRTGIAvailableThisFrame = data.m_bUseHardwareRayTracing;
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS                 = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline         = lightingPasses.m_pRTGIPipeline;
    data.m_uiShaderRecordStride        = lightingPasses.m_uiRTGIShaderRecordStride;
    data.m_hEmissiveTriangles          = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveLightHeader        = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveTriangleAliases    = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangleAliases, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveBlockAliases       = builder.ReadBuffer(lightingPasses.m_hEmissiveBlockAliases, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveAliasHeader        = builder.ReadBuffer(lightingPasses.m_hEmissiveAliasHeader, xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeData        = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeConstants   = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeConstants, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
    data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
    data.m_hAtmosphereConstants        = CreateAtmosphereConstantsBuffer(builder, "RTGIAtmosphereConstants");
    data.m_AtmosphereConstants         = MakeAtmosphereConstants(m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT);

    auto& reflectionResources = m_Resources.m_LightingPrepPasses;
    XII_ASSERT_DEV(reflectionResources.m_pFallbackReflectionProbeTexture != nullptr, "Reflection probe selection must initialize the fallback cubemap before RTGI setup.");
    data.m_hFallbackReflectionProbe = builder.ReadTexture(builder.ImportTexture("ReflectionProbeFallback", reflectionResources.m_pFallbackReflectionProbeTexture, reflectionResources.m_pFallbackReflectionProbeTexture->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeTextures.Reserve(reflectionResources.m_ReflectionProbeTextures.GetCount());
    for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < reflectionResources.m_ReflectionProbeTextures.GetCount(); ++uiProbeIndex)
    {
      xiiStringBuilder sResourceName;
      sResourceName.SetFormat("ReflectionProbeTexture_{0}", uiProbeIndex);
      data.m_hReflectionProbeTextures.PushBack(builder.ReadTexture(builder.ImportTexture(sResourceName, reflectionResources.m_ReflectionProbeTextures[uiProbeIndex], reflectionResources.m_ReflectionProbeTextures[uiProbeIndex]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource));
    }
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT GI Shader Binding Table", lightingPasses.m_pRTGIShaderBindingTable, lightingPasses.m_pRTGIShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
    builder.SetPassAllowMerge(false);
  }
  else
  {
    xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(lightingPasses.m_pRTGIFallbackPipeline, "Shaders/Pipeline/RTGIFallback.xiiShader");
  }
}

void xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedGlobalIllumination(const xiiRayTracedGlobalIlluminationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTGIFinalGather");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hAtmosphereConstants), data.m_AtmosphereConstants);
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      BindFrameResources(cmd);
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      m_Resources.m_LightingSystem.BindLightData(cmd, xiiGALShaderType::RayClosestHit);
      m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hAtmosphereConstants), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetConstantBuffer("xiiReflectionProbeConstants", context.GetBuffer(data.m_hReflectionProbeConstants), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hAtmosphereTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hAtmosphereMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hReflectionProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayMiss);
      xiiHybridArray<xiiGALTextureView*, XII_MAX_REFLECTION_PROBES> reflectionProbeViews;
      reflectionProbeViews.SetCount(XII_MAX_REFLECTION_PROBES);
      xiiSharedPtr<xiiGALTextureView> pFallbackProbeView = context.GetTexture(data.m_hFallbackReflectionProbe)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < XII_MAX_REFLECTION_PROBES; ++uiProbeIndex)
      {
        reflectionProbeViews[uiProbeIndex] = uiProbeIndex < data.m_hReflectionProbeTextures.GetCount() ?
          context.GetTexture(data.m_hReflectionProbeTextures[uiProbeIndex])->GetDefaultView(xiiGALTextureViewType::ShaderResource) :
          pFallbackProbeView;
      }
      cmd.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeTextures", 0U, reflectionProbeViews, xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIRaw", context.GetTexture(data.m_hRayTracedRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      const xiiBitflags<xiiGALShaderType> hitStages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangleAliases", context.GetBuffer(data.m_hEmissiveTriangleAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveBlockAliases", context.GetBuffer(data.m_hEmissiveBlockAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveAliasHeader", context.GetBuffer(data.m_hEmissiveAliasHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", hitStages);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", hitStages);
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
      cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pRTGIFallbackPipeline);
      BindFrameResources(cmd);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTGIRaw", context.GetTexture(data.m_hRayTracedRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// ReSTIR Global Illumination Temporal Reuse //////////

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRGITemporal(xiiReSTIRGITemporalData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = m_View.GetRenderResolutionHeight();
  const xiiUInt32 uiPreviousSlot = 1U - m_Resources.m_LightingPasses.m_uiRTGIReservoirHistoryWriteIndex;
  auto&           resources      = m_Resources.m_LightingPasses;


  data.m_bHistoryValid    = resources.m_bRTGIReservoirHistoryValid && resources.m_bRTGIAvailableThisFrame;
  data.m_hCurrentRadiance = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity        = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSample  = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous Sample", resources.m_pRTGIReservoirSampleHistory[uiPreviousSlot], resources.m_pRTGIReservoirSampleHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousState   = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous State", resources.m_pRTGIReservoirStateHistory[uiPreviousSlot], resources.m_pRTGIReservoirStateHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface = builder.ReadTexture(builder.ImportTexture("ReSTIR GI Previous Surface", resources.m_pRTGIReservoirSurfaceHistory[uiPreviousSlot], resources.m_pRTGIReservoirSurfaceHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hTemporalSample    = builder.WriteTexture("ReSTIR GI Temporal Sample", description, xiiGALResourceStateFlags::UnorderedAccess);

  description.m_Format  = xiiGALResourceFormat::RG32Float;
  data.m_hTemporalState = builder.WriteTexture("ReSTIR GI Temporal State", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("ReSTIR GI Temporal Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pReSTIRGITemporalPipeline, "Shaders/Pipeline/ReSTIRGITemporal.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRGITemporal(const xiiReSTIRGITemporalData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRGITemporal");
  {
    {
      xiiGALMapHelper<xiiReSTIRGIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid   = data.m_bHistoryValid ? 1U : 0U;
      pConstants->MaxTemporalM   = 20U;
      pConstants->SpatialSamples = 4U;
      pConstants->RadianceClamp  = 64.0f;
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReSTIRGITemporalPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentRadiance", context.GetTexture(data.m_hCurrentRadiance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSample", context.GetTexture(data.m_hPreviousSample)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousState", context.GetTexture(data.m_hPreviousState)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalSample", context.GetTexture(data.m_hTemporalSample)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_TemporalState", context.GetTexture(data.m_hTemporalState)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// ReSTIR Global Illumination Spatial Reuse //////////

void xiiBuiltinRenderPipelineRuntime::SetupReSTIRGISpatial(xiiReSTIRGISpatialData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth       = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiHeight      = m_View.GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot = m_Resources.m_LightingPasses.m_uiRTGIReservoirHistoryWriteIndex;
  data.m_uiHistoryWriteIndex    = uiCurrentSlot;
  auto& resources               = m_Resources.m_LightingPasses;

  data.m_hSceneDepth       = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal    = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo    = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalSample   = builder.ReadTexture("ReSTIR GI Temporal Sample", xiiGALResourceStateFlags::ShaderResource);
  data.m_hTemporalState    = builder.ReadTexture("ReSTIR GI Temporal State", xiiGALResourceStateFlags::ShaderResource);
  data.m_hReservoirSample  = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current Sample", resources.m_pRTGIReservoirSampleHistory[uiCurrentSlot], resources.m_pRTGIReservoirSampleHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirState   = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current State", resources.m_pRTGIReservoirStateHistory[uiCurrentSlot], resources.m_pRTGIReservoirStateHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hReservoirSurface = builder.WriteTexture(builder.ImportTexture("ReSTIR GI Current Surface", resources.m_pRTGIReservoirSurfaceHistory[uiCurrentSlot], resources.m_pRTGIReservoirSurfaceHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = uiWidth;
  description.m_Size.height = uiHeight;
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hResampledRadiance = builder.WriteTexture(xiiRGBlackboardKeys::k_RTResampledGI, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiReSTIRGIConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("ReSTIR GI Spatial Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pReSTIRGISpatialPipeline, "Shaders/Pipeline/ReSTIRGISpatial.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReSTIRGISpatial(const xiiReSTIRGISpatialData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReSTIRGISpatial");
  {
    {
      xiiGALMapHelper<xiiReSTIRGIConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryValid   = 1U;
      pConstants->MaxTemporalM   = 20U;
      pConstants->SpatialSamples = 4U;
      pConstants->RadianceClamp  = 64.0f;
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReSTIRGISpatialPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReSTIRGIConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalSample", context.GetTexture(data.m_hTemporalSample)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_TemporalState", context.GetTexture(data.m_hTemporalState)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirSampleOut", context.GetTexture(data.m_hReservoirSample)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirStateOut", context.GetTexture(data.m_hReservoirState)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ReservoirSurfaceOut", context.GetTexture(data.m_hReservoirSurface)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResampledRadianceOut", context.GetTexture(data.m_hResampledRadiance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPasses.m_bRTGIReservoirHistoryValid       = m_Resources.m_LightingPasses.m_bRTGIAvailableThisFrame;
    m_Resources.m_LightingPasses.m_uiRTGIReservoirHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Global Illumination Denoise Data //////////

void xiiBuiltinRenderPipelineRuntime::SetupRayTracedGlobalIlluminationDenoise(xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiRTGIHistoryWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_LightingPasses;

  data.m_bSignalValid                = resources.m_bRTGIAvailableThisFrame;
  data.m_bHistoryValid               = resources.m_bRTGIHistoryValid && data.m_bSignalValid;
  data.m_hRawGlobalIllumination      = builder.ReadTexture(xiiRGBlackboardKeys::k_RTResampledGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                 = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal              = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                   = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface            = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousGlobalIllumination = builder.ReadTexture(builder.ImportTexture("RTGI Previous History", resources.m_pRTGIHistory[uiPreviousSlot], resources.m_pRTGIHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hFinalGlobalIllumination    = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalGI, resources.m_pRTGIHistory[uiCurrentSlot], resources.m_pRTGIHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hConstants = CreateTemporalDenoiseConstants(builder, "RTGI Temporal Denoise Constants");

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pRTGITemporalDenoisePipeline, "Shaders/Pipeline/TemporalDenoise.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedGlobalIlluminationDenoise(const xiiRayTracedGlobalIlluminationDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTGITemporalDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.94f;
      pConstants->DepthThreshold  = 0.06f;
      pConstants->NormalThreshold = 0.86f;
      pConstants->SpatialWeight   = 0.20f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 1U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pRTGITemporalDenoisePipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRawGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hFinalGlobalIllumination)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPasses.m_bRTGIHistoryValid       = data.m_bSignalValid;
    m_Resources.m_LightingPasses.m_uiRTGIHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Data //////////
//
// Collects all GPU resources related to ray traced reflections.

void xiiBuiltinRenderPipelineRuntime::SetupRayTracedReflections(xiiRayTracedReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial   = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo     = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut           = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type              = xiiGALResourceDimension::Texture2D;
  description.m_Format            = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width        = m_View.GetRenderResolutionWidth();
  description.m_Size.height       = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels       = 1U;
  description.m_BindFlags         = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage             = xiiGALResourceUsage::Default;
  data.m_hRayTracedRawReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawReflections, description, xiiGALResourceStateFlags::UnorderedAccess);

  description.m_Format         = xiiGALResourceFormat::R16Float;
  data.m_hRayTracedHitDistance = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawReflectionHitDistance, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto& lightingPasses                             = m_Resources.m_LightingPasses;
  data.m_bUseHardwareRayTracing                    = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingReflectionResources();
  lightingPasses.m_bRTReflectionAvailableThisFrame = data.m_bUseHardwareRayTracing;
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS                 = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline         = lightingPasses.m_pRTReflectionPipeline;
    data.m_uiShaderRecordStride        = lightingPasses.m_uiRTReflectionShaderRecordStride;
    data.m_hEmissiveTriangles          = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangles, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveLightHeader        = builder.ReadBuffer(lightingPasses.m_hEmissiveLightHeader, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveTriangleAliases    = builder.ReadBuffer(lightingPasses.m_hEmissiveTriangleAliases, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveBlockAliases       = builder.ReadBuffer(lightingPasses.m_hEmissiveBlockAliases, xiiGALResourceStateFlags::ShaderResource);
    data.m_hEmissiveAliasHeader        = builder.ReadBuffer(lightingPasses.m_hEmissiveAliasHeader, xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeData        = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeConstants   = builder.ReadBuffer(xiiRGBlackboardKeys::k_ReflectionProbeConstants, xiiGALResourceStateFlags::ConstantBuffer);
    data.m_hAtmosphereTransmittanceLUT = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereTransmittanceLUT, xiiGALResourceStateFlags::ShaderResource);
    data.m_hAtmosphereMultiScatterLUT  = builder.ReadTexture(xiiRGBlackboardKeys::k_AtmosphereMultiScatterLUT, xiiGALResourceStateFlags::ShaderResource);
    data.m_hAtmosphereConstants        = CreateAtmosphereConstantsBuffer(builder, "RTReflectionAtmosphereConstants");
    data.m_AtmosphereConstants         = MakeAtmosphereConstants(m_Resources.m_LightingPrepPasses.m_hAtmosphereLUT);

    auto& reflectionResources = m_Resources.m_LightingPrepPasses;
    XII_ASSERT_DEV(reflectionResources.m_pFallbackReflectionProbeTexture != nullptr, "Reflection probe selection must initialize the fallback cubemap before RT reflection setup.");
    data.m_hFallbackReflectionProbe = builder.ReadTexture(builder.ImportTexture("ReflectionProbeFallback", reflectionResources.m_pFallbackReflectionProbeTexture, reflectionResources.m_pFallbackReflectionProbeTexture->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
    data.m_hReflectionProbeTextures.Reserve(reflectionResources.m_ReflectionProbeTextures.GetCount());
    for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < reflectionResources.m_ReflectionProbeTextures.GetCount(); ++uiProbeIndex)
    {
      xiiStringBuilder sResourceName;
      sResourceName.SetFormat("ReflectionProbeTexture_{0}", uiProbeIndex);
      data.m_hReflectionProbeTextures.PushBack(builder.ReadTexture(builder.ImportTexture(sResourceName, reflectionResources.m_ReflectionProbeTextures[uiProbeIndex], reflectionResources.m_ReflectionProbeTextures[uiProbeIndex]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource));
    }
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT Reflection Shader Binding Table", lightingPasses.m_pRTReflectionShaderBindingTable, lightingPasses.m_pRTReflectionShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
  }
  else
  {
    // Preserve the SSR/probe fallback contract on devices or frames without a valid TLAS.
    xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(lightingPasses.m_pRTReflectionFallbackPipeline, "Shaders/Pipeline/RTReflectionFallback.xiiShader");
  }
}

void xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedReflections(const xiiRayTracedReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTReflections");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      UploadAtmosphereConstants(cmd, context.GetBuffer(data.m_hAtmosphereConstants), data.m_AtmosphereConstants);
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      BindFrameResources(cmd);
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      m_Resources.m_LightingSystem.BindLightData(cmd, xiiGALShaderType::RayClosestHit);
      m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration | xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetConstantBuffer("xiiAtmosphereConstants", context.GetBuffer(data.m_hAtmosphereConstants), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetConstantBuffer("xiiReflectionProbeConstants", context.GetBuffer(data.m_hReflectionProbeConstants), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceTextureView("g_Transmittance", context.GetTexture(data.m_hAtmosphereTransmittanceLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceTextureView("g_MultiScatter", context.GetTexture(data.m_hAtmosphereMultiScatterLUT)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hReflectionProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayMiss);
      xiiHybridArray<xiiGALTextureView*, XII_MAX_REFLECTION_PROBES> reflectionProbeViews;
      reflectionProbeViews.SetCount(XII_MAX_REFLECTION_PROBES);
      xiiSharedPtr<xiiGALTextureView> pFallbackProbeView = context.GetTexture(data.m_hFallbackReflectionProbe)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
      for (xiiUInt32 uiProbeIndex = 0U; uiProbeIndex < XII_MAX_REFLECTION_PROBES; ++uiProbeIndex)
      {
        reflectionProbeViews[uiProbeIndex] = uiProbeIndex < data.m_hReflectionProbeTextures.GetCount() ?
          context.GetTexture(data.m_hReflectionProbeTextures[uiProbeIndex])->GetDefaultView(xiiGALTextureViewType::ShaderResource) :
          pFallbackProbeView;
      }
      cmd.ResolveAndSetShaderResourceTextureViews("g_ReflectionProbeTextures", 0U, reflectionProbeViews, xiiGALShaderType::RayMiss);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflRaw", context.GetTexture(data.m_hRayTracedRawReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflHitDistance", context.GetTexture(data.m_hRayTracedHitDistance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
      const xiiBitflags<xiiGALShaderType> hitStages = xiiGALShaderType::RayClosestHit | xiiGALShaderType::RayAnyHit;
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingMaterials", context.GetBuffer(data.m_hMaterialData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_RayTracingGeometry", context.GetBuffer(data.m_hGeometryData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), hitStages);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangles", context.GetBuffer(data.m_hEmissiveTriangles)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveHeader", context.GetBuffer(data.m_hEmissiveLightHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveTriangleAliases", context.GetBuffer(data.m_hEmissiveTriangleAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveBlockAliases", context.GetBuffer(data.m_hEmissiveBlockAliases)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      cmd.ResolveAndSetShaderResourceBufferView("g_EmissiveAliasHeader", context.GetBuffer(data.m_hEmissiveAliasHeader)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::RayClosestHit);
      if (xiiGALBindlessResourceTable::IsInitialized())
      {
        xiiGALBindlessResourceTable::BindBufferSRVs(cmd, "g_RayTracingBuffers", hitStages);
        xiiGALBindlessResourceTable::BindTextureSRVs(cmd, "g_RayTracingTextures", hitStages);
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
      cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pRTReflectionFallbackPipeline);
      BindFrameResources(cmd);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflRaw", context.GetTexture(data.m_hRayTracedRawReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTReflHitDistance", context.GetTexture(data.m_hRayTracedHitDistance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray Traced Reflections Denoise Data //////////

void xiiBuiltinRenderPipelineRuntime::SetupRayTracedReflectionsDenoise(xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiRTReflectionHistoryWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_LightingPasses;


  data.m_bSignalValid               = resources.m_bRTReflectionAvailableThisFrame;
  data.m_bHistoryValid              = resources.m_bRTReflectionHistoryValid && data.m_bSignalValid;
  data.m_hRawReflections            = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRawHitDistance            = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawReflectionHitDistance, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal             = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferObjectId           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferObjectId, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                  = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface           = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReflections       = builder.ReadTexture(builder.ImportTexture("RT Reflection Previous History", resources.m_pRTReflectionHistory[uiPreviousSlot], resources.m_pRTReflectionHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousHitDistance       = builder.ReadTexture(builder.ImportTexture("RT Reflection Previous Hit Distance", resources.m_pRTReflectionHitDistanceHistory[uiPreviousSlot], resources.m_pRTReflectionHitDistanceHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousMoments           = builder.ReadTexture(builder.ImportTexture("RT Reflection Previous Moments", resources.m_pRTReflectionMomentsHistory[uiPreviousSlot], resources.m_pRTReflectionMomentsHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousIdentitySignature = builder.ReadTexture(builder.ImportTexture("RT Reflection Previous Identity Signature", resources.m_pRTReflectionIdentityHistory[uiPreviousSlot], resources.m_pRTReflectionIdentityHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hFinalReflections          = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalReflections, resources.m_pRTReflectionHistory[uiCurrentSlot], resources.m_pRTReflectionHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCurrentHitDistance        = builder.WriteTexture(builder.ImportTexture("RT Reflection Current Hit Distance", resources.m_pRTReflectionHitDistanceHistory[uiCurrentSlot], resources.m_pRTReflectionHitDistanceHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCurrentMoments            = builder.WriteTexture(builder.ImportTexture("RT Reflection Current Moments", resources.m_pRTReflectionMomentsHistory[uiCurrentSlot], resources.m_pRTReflectionMomentsHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCurrentIdentitySignature  = builder.WriteTexture(builder.ImportTexture("RT Reflection Current Identity Signature", resources.m_pRTReflectionIdentityHistory[uiCurrentSlot], resources.m_pRTReflectionIdentityHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiRTReflectionDenoiseConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiRTReflectionDenoiseConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pRTReflectionTemporalDenoisePipeline, "Shaders/Pipeline/RTReflectionDenoise.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedReflectionsDenoise(const xiiRayTracedReflectionsDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("RTReflectionTemporalDenoise");
  {
    {
      xiiGALMapHelper<xiiRTReflectionDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      const xiiLightingSystemSettings&                 settings = m_Resources.m_LightingSystem.GetSettings();
      pConstants->HistoryWeight                                 = xiiMath::Clamp(settings.m_fRTReflectionHistoryWeight, 0.0f, 0.99f);
      pConstants->MomentsHistoryWeight                          = xiiMath::Clamp(settings.m_fRTReflectionMomentsHistoryWeight, 0.0f, 0.99f);
      pConstants->ConfidenceDecay                               = xiiMath::Clamp(settings.m_fRTReflectionConfidenceDecay, 0.0f, 1.0f);
      pConstants->SpatialWeight                                 = xiiMath::Clamp(settings.m_fRTReflectionSpatialWeight, 0.0f, 1.0f);
      pConstants->DepthThreshold                                = xiiMath::Clamp(settings.m_fRTReflectionDepthThreshold, 0.001f, 0.25f);
      pConstants->NormalThreshold                               = xiiMath::Clamp(settings.m_fRTReflectionNormalThreshold, 0.0f, 1.0f);
      pConstants->HitDistanceRelativeThreshold                  = xiiMath::Clamp(settings.m_fRTReflectionHitDistanceRelativeThreshold, 0.0f, 1.0f);
      pConstants->HitDistanceAbsoluteThreshold                  = xiiMath::Max(settings.m_fRTReflectionHitDistanceAbsoluteThreshold, 0.0f);
      pConstants->VarianceGamma                                 = xiiMath::Clamp(settings.m_fRTReflectionVarianceGamma, 0.5f, 8.0f);
      pConstants->HistoryValid                                  = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding                                      = xiiVec2U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pRTReflectionTemporalDenoisePipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiRTReflectionDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentReflection", context.GetTexture(data.m_hRawReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentHitDistance", context.GetTexture(data.m_hRawHitDistance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousReflection", context.GetTexture(data.m_hPreviousReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousHitDistance", context.GetTexture(data.m_hPreviousHitDistance)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousMoments", context.GetTexture(data.m_hPreviousMoments)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousIdentitySignature", context.GetTexture(data.m_hPreviousIdentitySignature)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferObjectId", context.GetTexture(data.m_hGBufferObjectId)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResolvedReflection", context.GetTexture(data.m_hFinalReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResolvedHitDistance", context.GetTexture(data.m_hCurrentHitDistance)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResolvedMoments", context.GetTexture(data.m_hCurrentMoments)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_CurrentIdentitySignature", context.GetTexture(data.m_hCurrentIdentitySignature)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPasses.m_bRTReflectionHistoryValid       = data.m_bSignalValid;
    m_Resources.m_LightingPasses.m_uiRTReflectionHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen Space Reflections Data //////////
//
// Collects all GPU resources related to screen-space reflections.

void xiiBuiltinRenderPipelineRuntime::SetupScreenSpaceReflections(xiiScreenSpaceReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth         = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial    = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferAlbedo      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hBRDFLut            = builder.ReadTexture(xiiRGBlackboardKeys::k_BRDFLut, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHiZClosestPyramid  = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZClosestPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHiZFarthestPyramid = builder.ReadTexture(xiiRGBlackboardKeys::k_HiZPyramid, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor      = builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type             = xiiGALResourceDimension::Texture2D;
  description.m_Format           = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width       = m_View.GetRenderResolutionWidth();
  description.m_Size.height      = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels      = 1U;
  description.m_BindFlags        = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage            = xiiGALResourceUsage::Default;
  data.m_hScreenSpaceReflections = builder.WriteTexture(xiiRGBlackboardKeys::k_SSRRawTexture, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiUInt32 uiMipWidth  = m_View.GetRenderResolutionWidth();
  xiiUInt32 uiMipHeight = m_View.GetRenderResolutionHeight();
  xiiUInt32 uiMipCount  = 1U;
  while (uiMipWidth > 1U || uiMipHeight > 1U)
  {
    uiMipWidth  = xiiMath::Max(uiMipWidth >> 1U, 1U);
    uiMipHeight = xiiMath::Max(uiMipHeight >> 1U, 1U);
    ++uiMipCount;
  }

  data.m_Constants.MaxSteps      = 96U;
  data.m_Constants.Thickness     = 0.0025f;
  data.m_Constants.MaxRoughness  = 0.72f;
  data.m_Constants.StrideZCutoff = 0.25f;
  data.m_Constants.InvResolution = xiiVec2(1.0f / static_cast<float>(m_View.GetRenderResolutionWidth()), 1.0f / static_cast<float>(m_View.GetRenderResolutionHeight()));
  data.m_Constants.HiZMipCount   = uiMipCount;
  data.m_Constants._Padding      = 0.0f;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pSSRPipeline, "Shaders/Pipeline/SSR.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteScreenSpaceReflections(const xiiScreenSpaceReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSR");
  {
    {
      xiiGALMapHelper<xiiSSRConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pSSRPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_BRDFLut", context.GetTexture(data.m_hBRDFLut)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HiZClosest", context.GetTexture(data.m_hHiZClosestPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HiZFarthest", context.GetTexture(data.m_hHiZFarthestPyramid)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_SSROut", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen Space Reflections Temporal Data //////////
//
// Reprojects SSR radiance through velocity and rejects history when the
// receiver surface, packed G-buffer material signature, or object identity changes.

void xiiBuiltinRenderPipelineRuntime::SetupScreenSpaceReflectionsTemporal(xiiScreenSpaceReflectionsTemporalData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiSSRHistoryWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_LightingPasses;


  data.m_bHistoryValid              = resources.m_bSSRHistoryValid;
  data.m_hCurrentReflection         = builder.ReadTexture(xiiRGBlackboardKeys::k_SSRRawTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth                = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal             = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferObjectId           = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferObjectId, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity                  = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface           = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousReflection        = builder.ReadTexture(builder.ImportTexture("SSR Previous History", resources.m_pSSRHistory[uiPreviousSlot], resources.m_pSSRHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousIdentitySignature = builder.ReadTexture(builder.ImportTexture("SSR Previous Identity Signature", resources.m_pSSRIdentityHistory[uiPreviousSlot], resources.m_pSSRIdentityHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hResolvedReflection        = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_SSRTexture, resources.m_pSSRHistory[uiCurrentSlot], resources.m_pSSRHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hCurrentIdentitySignature  = builder.WriteTexture(builder.ImportTexture("SSR Current Identity Signature", resources.m_pSSRIdentityHistory[uiCurrentSlot], resources.m_pSSRIdentityHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiSSRTemporalConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiSSRTemporalConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pSSRTemporalPipeline, "Shaders/Pipeline/SSRTemporal.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteScreenSpaceReflectionsTemporal(const xiiScreenSpaceReflectionsTemporalData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SSRTemporal");
  {
    {
      xiiGALMapHelper<xiiSSRTemporalConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.92f;
      pConstants->ConfidenceDecay = 0.96f;
      pConstants->DepthThreshold  = 0.02f;
      pConstants->NormalThreshold = 0.95f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding        = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pSSRTemporalPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiSSRTemporalConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentReflection", context.GetTexture(data.m_hCurrentReflection)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousReflection", context.GetTexture(data.m_hPreviousReflection)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferObjectId", context.GetTexture(data.m_hGBufferObjectId)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousIdentitySignature", context.GetTexture(data.m_hPreviousIdentitySignature)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ResolvedReflection", context.GetTexture(data.m_hResolvedReflection)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_CurrentIdentitySignature", context.GetTexture(data.m_hCurrentIdentitySignature)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPasses.m_bSSRHistoryValid       = true;
    m_Resources.m_LightingPasses.m_uiSSRHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// Hybrid Reflection Composite Data //////////
//
// Replaces the probe/sky specular contribution already present in the base
// opaque scene with the highest-confidence reflection tier available.

void xiiBuiltinRenderPipelineRuntime::SetupHybridReflectionComposite(xiiHybridReflectionCompositeData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hEnvironmentSpecular    = builder.ReadTexture(xiiRGBlackboardKeys::k_EnvironmentSpecular, xiiGALResourceStateFlags::ShaderResource);
  data.m_hScreenSpaceReflections = builder.ReadTexture(xiiRGBlackboardKeys::k_SSRTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedReflections   = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalReflections, xiiGALResourceStateFlags::ShaderResource);
  data.m_hHDRSceneColor          = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pReflectionCompositePipeline, "Shaders/Pipeline/ReflectionComposite.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteHybridReflectionComposite(const xiiHybridReflectionCompositeData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HybridReflectionComposite");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pReflectionCompositePipeline);
    BindFrameResources(cmd);
    cmd.ResolveAndSetShaderResourceTextureView("g_EnvironmentSpecular", context.GetTexture(data.m_hEnvironmentSpecular)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_ScreenSpaceReflection", context.GetTexture(data.m_hScreenSpaceReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedReflection", context.GetTexture(data.m_hRayTracedReflections)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRScene", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Light Injection Data //////////
//
// Evaluates shadowed directional and clustered local lighting once per froxel.

void xiiBuiltinRenderPipelineRuntime::SetupVolumetricLightInjection(xiiVolumetricLightInjectionData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelScattering            = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hFroxelMetadata              = builder.ReadBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightGridBuffer             = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer            = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hShadowCascadeConstants      = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hDirectionalShadowAtlas      = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCloudShadowConstants        = builder.ReadBuffer(xiiRGBlackboardKeys::k_CloudShadowConstants, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pVolumetricLightInjectionPipeline, "Shaders/Pipeline/VolumetricLightInjection.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricLightInjection(const xiiVolumetricLightInjectionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricLightInjection");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pVolumetricLightInjectionPipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindLightingResources(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindIESProfiles(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_FroxelMeta", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hShadowCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VolumetricDirectionalShadowAtlas", context.GetTexture(data.m_hDirectionalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_VolumetricLocalShadowAtlas", context.GetTexture(data.m_hLocalShadowAtlas)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VolumetricLocalShadowData", context.GetBuffer(data.m_hLocalShadowAtlasDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiCloudShadowConstants", context.GetBuffer(data.m_hCloudShadowConstants), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 8U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Integration Data //////////
//
// Collects all GPU resources related to volumetric fog integration.

void xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogIntegration(xiiVolumetricFogIntegrationData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelScatteringBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type               = xiiGALResourceDimension::Texture3D;
  description.m_Format             = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width         = 128U;
  description.m_Size.height        = 72U;
  description.m_uiArraySizeOrDepth = 64U;
  description.m_uiMipLevels        = 1U;
  description.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hFroxelIntegratedBuffer   = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelIntegratedBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pVolumetricIntegratePipeline, "Shaders/Pipeline/VolumetricLightIntegration.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogIntegration(const xiiVolumetricFogIntegrationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogIntegrate");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pVolumetricIntegratePipeline);
    BindFrameResources(cmd);

    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelScattering", context.GetTexture(data.m_hFroxelScatteringBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelIntegratedOut", context.GetTexture(data.m_hFroxelIntegratedBuffer)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({16U, 9U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Resolve Data //////////
//
// Samples the integrated froxel prefix at the opaque surface depth.

void xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogResolve(xiiVolumetricFogResolveData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hFroxelIntegratedBuffer = builder.ReadTexture(xiiRGBlackboardKeys::k_FroxelIntegratedBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth             = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type           = xiiGALResourceDimension::Texture2D;
  description.m_Format         = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width     = m_View.GetRenderResolutionWidth();
  description.m_Size.height    = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels    = 1U;
  description.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hVolumetricScattering = builder.WriteTexture(xiiRGBlackboardKeys::k_VolumetricScatteringRaw, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_LightingPasses.m_pVolumetricResolvePipeline, "Shaders/Pipeline/VolumetricFogResolve.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogResolve(const xiiVolumetricFogResolveData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogResolve");
  {
    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pVolumetricResolvePipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelIntegrated", context.GetTexture(data.m_hFroxelIntegratedBuffer)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_VolumetricOut", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Volumetric Fog Temporal Reprojection Data //////////
//
// Collects all GPU resources related to volumetric fog temporal reprojection.

void xiiBuiltinRenderPipelineRuntime::SetupVolumetricFogTemporalReprojection(xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_LightingPasses.m_uiVolumetricHistoryWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_LightingPasses;


  const auto& depthPasses       = m_Resources.m_DepthPasses;
  data.m_bHistoryValid          = resources.m_bVolumetricHistoryValid && depthPasses.m_bMotionHistoryValid;
  data.m_PreviousViewProjection = depthPasses.m_bMotionHistoryValid ? depthPasses.m_PreviousViewProjectionMatrix : m_View.GetViewProjectionMatrix(xiiCameraEye::Left);
  data.m_hCurrentScattering     = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScatteringRaw, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousScattering    = builder.ReadTexture(builder.ImportTexture("Volumetric Scattering Previous", resources.m_pVolumetricHistory[uiPreviousSlot], resources.m_pVolumetricHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth            = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal         = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity              = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface       = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_VolumetricScattering, resources.m_pVolumetricHistory[uiCurrentSlot], resources.m_pVolumetricHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiVolumetricTemporalConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("Volumetric Temporal Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pVolumetricTemporalPipeline, "Shaders/Pipeline/VolumetricFogTemporalRep.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteVolumetricFogTemporalReprojection(const xiiVolumetricFogTemporalReprojectionData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricFogTemporalRep");
  {
    {
      xiiGALMapHelper<xiiVolumetricTemporalConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->PreviousViewProjectionMatrix = data.m_PreviousViewProjection;
      pConstants->HistoryWeight                = 0.92f;
      pConstants->DepthThreshold               = 0.08f;
      pConstants->NormalThreshold              = 0.80f;
      pConstants->SpatialWeight                = 0.10f;
      pConstants->HistoryValid                 = data.m_bHistoryValid ? 1U : 0U;
      pConstants->_Padding                     = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_LightingPasses.m_pVolumetricTemporalPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVolumetricTemporalConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelCurrent", context.GetTexture(data.m_hCurrentScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_FroxelHistory", context.GetTexture(data.m_hPreviousScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_FroxelBlended", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_LightingPasses.m_bVolumetricHistoryValid       = true;
    m_Resources.m_LightingPasses.m_uiVolumetricHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// GPU Atmosphere Composite Data //////////
//
// Collects all GPU resources related to atmosphere compositing.

////////// Calibrated Sensor Output Data //////////

////////// GPU Opaque Lighting Composite Data //////////
//
// Collects all GPU resources related to the forward opaque pass.
