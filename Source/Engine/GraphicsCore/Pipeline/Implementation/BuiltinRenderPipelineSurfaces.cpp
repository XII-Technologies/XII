/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupOpaqueLightingComposite(xiiOpaqueLightingCompositeData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hDirectLighting       = builder.ReadTexture(xiiRGBlackboardKeys::k_DirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hIndirectLighting     = builder.ReadTexture(xiiRGBlackboardKeys::k_IndirectLightingBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferEmissive      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::ShaderResource);
  data.m_hRayTracedFinalGI     = builder.ReadTexture(xiiRGBlackboardKeys::k_RTFinalGI, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVolumetricScattering = builder.ReadTexture(xiiRGBlackboardKeys::k_VolumetricScattering, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::RGBA16Float;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1U;
  description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hHDRSceneColor     = builder.WriteTexture(xiiRGBlackboardKeys::k_HDRSceneColor, description, xiiGALResourceStateFlags::UnorderedAccess);

  builder.SetPassAllowMerge(false);
  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_ForwardPasses.m_pOpaqueLightingCompositePipeline, "Shaders/Pipeline/OpaqueComposite.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteOpaqueLightingComposite(const xiiOpaqueLightingCompositeData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("OpaqueLightingComposite");
  {
    cmd.SetPipelineState(m_Resources.m_ForwardPasses.m_pOpaqueLightingCompositePipeline);
    BindFrameResources(cmd);
    cmd.ResolveAndSetShaderResourceTextureView("g_DirectLight", context.GetTexture(data.m_hDirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_IndirectLight", context.GetTexture(data.m_hIndirectLighting)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Emissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Volumetric", context.GetTexture(data.m_hVolumetricScattering)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_RayTracedGI", context.GetTexture(data.m_hRayTracedFinalGI)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Hair Rendering Data //////////
//
// Collects all GPU resources related to the hair rendering pass.

void xiiBuiltinRenderPipelineRuntime::SetupHairRendering(xiiHairRenderingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteHairRendering(const xiiHairRenderingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("HairRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_ForwardPasses.m_pHairPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_Resources.m_ForwardPasses.m_pHairPipeline);
      BindFrameResources(cmd);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Water Rendering Data //////////
//
// Collects all GPU resources related to the water rendering pass.

void xiiBuiltinRenderPipelineRuntime::SetupWaterRendering(xiiWaterRenderingData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hPlanarReflectionMap  = builder.ReadTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteWaterRendering(const xiiWaterRenderingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("WaterRendering");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_ForwardPasses.m_pWaterPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_Resources.m_ForwardPasses.m_pWaterPipeline);
      BindFrameResources(cmd);
      if (data.m_hPlanarReflectionMap.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_PlanarRefl", context.GetTexture(data.m_hPlanarReflectionMap)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Pixel);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Subsurface Scattering Data //////////
//
// Collects all GPU resources related to the screen-space subsurface scattering pass.

void xiiBuiltinRenderPipelineRuntime::SetupSubsurfaceScattering(xiiSubsurfaceScatteringData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hSceneDepth      = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferMaterial = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::ShaderResource);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteSubsurfaceScattering(const xiiSubsurfaceScatteringData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SubsurfaceScattering");
  {
    if (m_Resources.m_ForwardPasses.m_pSSSComputePipeline)
    {
      cmd.SetPipelineState(m_Resources.m_ForwardPasses.m_pSSSComputePipeline);
      BindFrameResources(cmd);
      if (data.m_hSceneDepth.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      }
      if (data.m_hGBufferMaterial.IsValid())
      {
        cmd.ResolveAndSetShaderResourceTextureView("g_GBufMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      }
      if (data.m_hHDRSceneColor.IsValid())
      {
        cmd.ResolveAndSetUnorderedAccessTextureView("g_HDRInOut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      }
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

      const xiiUInt32 uiRenderWidth  = m_View.GetRenderResolutionWidth();
      const xiiUInt32 uiRenderHeight = m_View.GetRenderResolutionHeight();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Eye Shader Data //////////
//
// Collects all GPU resources related to the eye shading pass.

void xiiBuiltinRenderPipelineRuntime::SetupEyeShader(xiiEyeShaderData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::RenderTarget), xiiGALResourceStateFlags::RenderTarget);
  data.m_hSceneDepth           = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthWrite), xiiGALResourceStateFlags::DepthWrite);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  builder.SetPassAllowMerge(true);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteEyeShader(const xiiEyeShaderData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("EyeShader");
  {
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(m_View.GetRenderResolutionWidth()), static_cast<float>(m_View.GetRenderResolutionHeight()), 0.0f, 1.0f});

    if (m_Resources.m_ForwardPasses.m_pEyePipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_Resources.m_ForwardPasses.m_pEyePipeline);
      BindFrameResources(cmd);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Particle Simulate Data //////////
//
// Collects all GPU resources related to the particle simulation pass.

static_assert((sizeof(xiiGPUParticleSimulateConstants) % 16U) == 0U);

void xiiBuiltinRenderPipelineRuntime::SetupGPUParticleSimulate(xiiGPUParticleSimulateData& data, xiiRenderGraphBuilder& builder)
{
  constexpr xiiUInt32 uiDefaultParticleCapacity = xiiParticleSystemConstants::s_uiDefaultMaxParticles;

  if (!m_Resources.m_TransparencyPasses.m_pParticleStateBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride = sizeof(xiiParticleGPUState);
    description.m_uiSize              = description.m_uiElementByteStride * uiDefaultParticleCapacity;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Default;

    m_Resources.m_TransparencyPasses.m_pParticleStateBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    m_Resources.m_TransparencyPasses.m_uiParticleCapacity   = uiDefaultParticleCapacity;
  }

  data.m_hParticleState  = builder.ImportBuffer("ParticleState", m_Resources.m_TransparencyPasses.m_pParticleStateBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hParticleState  = builder.WriteBuffer(data.m_hParticleState, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiParticleCount = xiiMath::Max(1U, m_Resources.m_TransparencyPasses.m_uiParticleCapacity);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiGPUParticleSimulateConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Mode           = xiiGALBufferMode::Undefined;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;

  data.m_hParticleConstants = builder.WriteBuffer("ParticleSimConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TransparencyPasses.m_pParticleSimulatePipeline, "Shaders/Pipeline/GPUParticleSimulate.xiiShader");

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteGPUParticleSimulate(const xiiGPUParticleSimulateData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("GPUParticleSimulate");
  {
    {
      xiiGALMapHelper<xiiGPUParticleSimulateConstants> pConstants(cmd, context.GetBuffer(data.m_hParticleConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->m_fDeltaTimeS      = xiiMath::Clamp(static_cast<float>(xiiClock::GetGlobalClock()->GetTimeDiff().GetSeconds()), 0.0f, 1.0f / 15.0f);
      pConstants->m_fGravityScale    = 1.0f;
      pConstants->m_fDragCoefficient = 0.05f;
      pConstants->m_fTurbulenceScale = 0.0f;
      pConstants->m_vWindVelocity    = xiiVec3::MakeZero();
      pConstants->m_uiActiveCount    = data.m_uiParticleCount;
    }

    cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pParticleSimulatePipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("ParticleSimConstants", context.GetBuffer(data.m_hParticleConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_Particles", context.GetBuffer(data.m_hParticleState)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiParticleCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Decal Upload Data //////////
//
// Uploads extracted projected and mesh decal instances into a GPU-visible structured buffer and imports the active atlas set.

static_assert((sizeof(xiiGPUDecalInstance) % 16U) == 0U);

void xiiBuiltinRenderPipelineRuntime::SetupDecalUpload(xiiDecalUploadData& data, xiiRenderGraphBuilder& builder)
{
  auto EnsureFallbackTexture = [&](xiiSharedPtr<xiiGALTexture>& inout_pTexture, xiiUInt32 uiClearValue, xiiStringView sDebugName) {
    if (inout_pTexture != nullptr)
      return;

    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type        = xiiGALResourceDimension::Texture2D;
    textureDescription.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
    textureDescription.m_Size.width  = 1U;
    textureDescription.m_Size.height = 1U;
    textureDescription.m_uiMipLevels = 1U;
    textureDescription.m_BindFlags   = xiiGALBindFlags::ShaderResource;
    textureDescription.m_Usage       = xiiGALResourceUsage::Default;

    xiiUInt32 uiPixel = uiClearValue;

    xiiHybridArray<xiiGALTextureSubResourceData, 1U> initData;
    xiiGALTextureSubResourceData&                    subResourceData = initData.ExpandAndGetRef();
    subResourceData.m_pData                                          = xiiMakeByteBlobPtr(static_cast<const void*>(&uiPixel), sizeof(uiPixel));
    subResourceData.m_uiStride                                       = sizeof(uiPixel);
    subResourceData.m_uiDepthStride                                  = sizeof(uiPixel);

    xiiGALTextureData textureData(initData);
    inout_pTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(textureDescription, &textureData);
    if (inout_pTexture)
    {
      inout_pTexture->SetDebugName(sDebugName);
    }
  };

  auto EnsureFallbackAtlases = [&]() {
    EnsureFallbackTexture(m_Resources.m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture, 0xFFFFFFFFU, "FallbackDecalAtlas::Albedo");
    EnsureFallbackTexture(m_Resources.m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture, 0xFFFF8080U, "FallbackDecalAtlas::Normal");
    EnsureFallbackTexture(m_Resources.m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture, 0x00FF0080U, "FallbackDecalAtlas::Material");
    EnsureFallbackTexture(m_Resources.m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture, 0x00000000U, "FallbackDecalAtlas::Emissive");

    if (m_Resources.m_TransparencyPasses.m_pFallbackDecalAtlasSampler == nullptr)
    {
      xiiGALSamplerCreationDescription samplerDescription           = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
      samplerDescription.m_AddressU                                 = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressV                                 = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressW                                 = xiiGALTextureAddressMode::Clamp;
      m_Resources.m_TransparencyPasses.m_pFallbackDecalAtlasSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(samplerDescription);
    }
  };

  EnsureFallbackAtlases();

  xiiSharedPtr<xiiGALTexture> pAtlasAlbedo   = m_Resources.m_TransparencyPasses.m_pFallbackDecalAlbedoAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasNormal   = m_Resources.m_TransparencyPasses.m_pFallbackDecalNormalAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasMaterial = m_Resources.m_TransparencyPasses.m_pFallbackDecalMaterialAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasEmissive = m_Resources.m_TransparencyPasses.m_pFallbackDecalEmissiveAtlasTexture;

  xiiDecalAtlasResourceHandle hSelectedAtlas;

  const xiiArrayPtr<xiiRenderData* const> renderData = m_View.m_pExtractedData != nullptr ? m_View.m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_Decals.Reserve(renderData.GetCount());

  const xiiRTTI* pDecalType = xiiGetStaticRTTI<xiiDecalRenderData>();

  for (const xiiRenderData* pBaseRenderData : renderData)
  {
    if (pBaseRenderData == nullptr || pBaseRenderData->GetDynamicRTTI() == nullptr || !pBaseRenderData->GetDynamicRTTI()->IsDerivedFrom(pDecalType))
      continue;

    const xiiDecalRenderData* pDecal = static_cast<const xiiDecalRenderData*>(pBaseRenderData);
    if (pDecal->m_fOpacity <= 0.0f || pDecal->m_ChannelMask.IsNoFlagSet())
      continue;

    if (pDecal->m_hAtlas.IsValid() && hSelectedAtlas.IsValid() && pDecal->m_hAtlas != hSelectedAtlas)
      continue;

    if (pDecal->m_hAtlas.IsValid() && !hSelectedAtlas.IsValid())
    {
      xiiResourceLock<xiiDecalAtlasResource> pAtlas(pDecal->m_hAtlas, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAtlas)
      {
        hSelectedAtlas = pDecal->m_hAtlas;
        pAtlasAlbedo   = pAtlas->GetAlbedoTexture() != nullptr ? pAtlas->GetAlbedoTexture() : pAtlasAlbedo;
        pAtlasNormal   = pAtlas->GetNormalTexture() != nullptr ? pAtlas->GetNormalTexture() : pAtlasNormal;
        pAtlasMaterial = pAtlas->GetMaterialTexture() != nullptr ? pAtlas->GetMaterialTexture() : pAtlasMaterial;
        pAtlasEmissive = pAtlas->GetEmissiveTexture() != nullptr ? pAtlas->GetEmissiveTexture() : pAtlasEmissive;
      }
    }

    xiiGPUDecalInstance& gpuDecal = data.m_Decals.ExpandAndGetRef();
    gpuDecal.m_WorldToDecal       = pDecal->m_GlobalTransform.GetInverse().GetAsMat4();
    gpuDecal.m_AtlasUVRect        = pDecal->m_vAtlasUVRect;
    gpuDecal.m_Tint               = xiiVec4(pDecal->m_Tint.r, pDecal->m_Tint.g, pDecal->m_Tint.b, pDecal->m_Tint.a);
    gpuDecal.m_UVOffsetScale      = xiiVec4(pDecal->m_vUVOffset.x, pDecal->m_vUVOffset.y, pDecal->m_vUVScale.x, pDecal->m_vUVScale.y);
    gpuDecal.m_ExtentsOpacity     = xiiVec4(pDecal->m_vExtents.x, pDecal->m_vExtents.y, pDecal->m_vExtents.z, pDecal->m_fOpacity);

    const auto boundsSphere      = pDecal->m_GlobalBounds.GetSphere();
    gpuDecal.m_WorldCenterRadius = xiiVec4(boundsSphere.m_vCenter.x, boundsSphere.m_vCenter.y, boundsSphere.m_vCenter.z, boundsSphere.m_fRadius);
    gpuDecal.m_SurfaceParams     = xiiVec4(pDecal->m_fNormalBlend, pDecal->m_fRoughness, pDecal->m_fMetallic, pDecal->m_fEmissive);
    gpuDecal.m_uiChannelMask     = pDecal->m_ChannelMask.GetValue();
    gpuDecal.m_uiMode            = pDecal->m_Mode.GetValue();
    gpuDecal.m_uiPriority        = pDecal->m_uiPriority;
  }

  data.m_uiDecalCount = data.m_Decals.GetCount();
  m_View.GetBlackboard().Set(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiElementByteStride = sizeof(xiiGPUDecalInstance);
  bufferDescription.m_uiSize              = xiiMath::Max<xiiUInt32>(1U, data.m_uiDecalCount) * bufferDescription.m_uiElementByteStride;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hDecalData                       = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, bufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hAtlasAlbedo   = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, pAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal   = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, pAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, pAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive = builder.ImportTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, pAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDecalUpload(const xiiDecalUploadData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_Decals.IsEmpty())
    return;

  xiiGALBuffer* pDecalBuffer = context.GetBuffer(data.m_hDecalData);
  if (pDecalBuffer == nullptr)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DecalUpload");
  {
    const xiiUInt8* pBytes = reinterpret_cast<const xiiUInt8*>(data.m_Decals.GetData());
    UpdateBufferPreservingState(cmd, pDecalBuffer, 0U, xiiMakeArrayPtr(pBytes, data.m_Decals.GetCount() * sizeof(xiiGPUDecalInstance)));
  }
  cmd.EndDebugGroup();
}

////////// GPU Decal Cull & Batch Data //////////
//
// Performs GPU-side frustum culling and builds projected tile bins plus a compact mesh-decal list.

void xiiBuiltinRenderPipelineRuntime::SetupDecalCullBatch(xiiDecalCullBatchData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData  = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);

  const bool bHasDecalCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasDecalCount);

  const xiiUInt32 uiTileCountX = (m_View.GetRenderResolutionWidth() + k_uiDecalTileSize - 1U) / k_uiDecalTileSize;
  const xiiUInt32 uiTileCountY = (m_View.GetRenderResolutionHeight() + k_uiDecalTileSize - 1U) / k_uiDecalTileSize;
  data.m_uiTileCount           = uiTileCountX * uiTileCountY;

  xiiGALBufferCreationDescription visibleDescription;
  visibleDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  visibleDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(2U, data.m_uiDecalCount + 1U);
  visibleDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  visibleDescription.m_Mode                = xiiGALBufferMode::Structured;
  visibleDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hVisibleList = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalVisibleList, visibleDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription tileDescription;
  tileDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  tileDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(1U, data.m_uiTileCount * k_uiMaxProjectedDecalTileStride);
  tileDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  tileDescription.m_Mode                = xiiGALBufferMode::Structured;
  tileDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hProjectedTileList = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalTileList, tileDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription meshCommandDescription;
  meshCommandDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  meshCommandDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(2U, data.m_uiDecalCount + 1U);
  meshCommandDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  meshCommandDescription.m_Mode                = xiiGALBufferMode::Structured;
  meshCommandDescription.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hMeshDrawCommands = builder.WriteBuffer(xiiRGBlackboardKeys::k_DecalDrawCommands, meshCommandDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TransparencyPasses.m_pDecalCullBatchPipeline, "Shaders/Pipeline/DecalClassification.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDecalCullBatch(const xiiDecalCullBatchData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  auto ClearStructuredUIntBuffer = [&cmd](xiiGALBuffer* pBuffer) {
    const xiiUInt32            uiValueCount = static_cast<xiiUInt32>(pBuffer->GetDescription().m_uiSize / sizeof(xiiUInt32));
    xiiDynamicArray<xiiUInt32> zeroData;
    zeroData.SetCount(uiValueCount);
    for (xiiUInt32& uiValue : zeroData)
    {
      uiValue = 0U;
    }

    const xiiUInt8* pBytes = reinterpret_cast<const xiiUInt8*>(zeroData.GetData());
    UpdateBufferPreservingState(cmd, pBuffer, 0U, xiiMakeArrayPtr(pBytes, zeroData.GetCount() * sizeof(xiiUInt32)));
  };

  xiiGALBuffer* pVisibleList     = context.GetBuffer(data.m_hVisibleList);
  xiiGALBuffer* pProjectedTiles  = context.GetBuffer(data.m_hProjectedTileList);
  xiiGALBuffer* pMeshDrawCommand = context.GetBuffer(data.m_hMeshDrawCommands);

  cmd.BeginDebugGroup("DecalCullBatch");
  {
    ClearStructuredUIntBuffer(pVisibleList);
    ClearStructuredUIntBuffer(pProjectedTiles);
    ClearStructuredUIntBuffer(pMeshDrawCommand);

    cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pDecalCullBatchPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleDecals", pVisibleList->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_TileLists", pProjectedTiles->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_MeshDecalCommands", pMeshDrawCommand->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiDecalCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Projected Decal Resolve Data //////////
//
// Resolves projected deferred decals into the live G-Buffer surfaces.

void xiiBuiltinRenderPipelineRuntime::SetupProjectedDecalResolve(xiiProjectedDecalResolveData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth        = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData         = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hProjectedTileList = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalTileList, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasAlbedo       = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal       = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial     = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive     = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  data.m_hGBufferAlbedo   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferNormal   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferMaterial = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferEmissive = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  const bool bHasProjectedDecalCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasProjectedDecalCount);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TransparencyPasses.m_pSSDecalResolvePipeline, "Shaders/Pipeline/DecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ProjectedDecalResolve");
  {
    cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pSSDecalResolvePipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_TileLists", context.GetBuffer(data.m_hProjectedTileList)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasAlbedo", context.GetTexture(data.m_hAtlasAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasNormal", context.GetTexture(data.m_hAtlasNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasMaterial", context.GetTexture(data.m_hAtlasMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasEmissive", context.GetTexture(data.m_hAtlasEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Mesh Decal Resolve Data //////////
//
// Resolves mesh decals as a distinct GPU pass using the mesh-decal list built during classification.

void xiiBuiltinRenderPipelineRuntime::SetupMeshDecalDraw(xiiMeshDecalDrawData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth       = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hDecalData        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDataBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hMeshDrawCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DecalDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasAlbedo      = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasAlbedo, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasNormal      = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasMaterial    = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasMaterial, xiiGALResourceStateFlags::ShaderResource);
  data.m_hAtlasEmissive    = builder.ReadTexture(xiiRGBlackboardKeys::k_DecalAtlasEmissive, xiiGALResourceStateFlags::ShaderResource);

  data.m_hGBufferAlbedo   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferAlbedo, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferNormal   = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferMaterial = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferMaterial, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hGBufferEmissive = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferEmissive, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  const bool bHasMeshDecalCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_DecalCount, data.m_uiDecalCount);
  XII_IGNORE_UNUSED(bHasMeshDecalCount);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TransparencyPasses.m_pMeshDecalResolvePipeline, "Shaders/Pipeline/MeshDecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MeshDecalDraw");
  {
    cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pMeshDecalResolvePipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);

    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Decals", context.GetBuffer(data.m_hDecalData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_MeshDecalCommands", context.GetBuffer(data.m_hMeshDrawCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasAlbedo", context.GetTexture(data.m_hAtlasAlbedo)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasNormal", context.GetTexture(data.m_hAtlasNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasMaterial", context.GetTexture(data.m_hAtlasMaterial)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_DecalAtlasEmissive", context.GetTexture(data.m_hAtlasEmissive)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferAlbedo", context.GetTexture(data.m_hGBufferAlbedo)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferMaterial", context.GetTexture(data.m_hGBufferMaterial)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_GBufferEmissive", context.GetTexture(data.m_hGBufferEmissive)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Weighted Blended OIT Data //////////
//
// Collects all GPU resources related to weighted blended transparency accumulation and resolve.

void xiiBuiltinRenderPipelineRuntime::SetupWeightedBlendedOIT(xiiWeightedBlendedOITData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = m_View.GetRenderResolutionHeight();

  data.m_hHDRSceneColor        = builder.WriteTexture(builder.ReadTexture(xiiRGBlackboardKeys::k_HDRSceneColor, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hSceneDepth           = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::DepthRead);
  data.m_hDrawIndirectCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiGALResourceStateFlags::IndirectArgument);

  xiiGALTextureCreationDescription accumulateDescription;
  accumulateDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  accumulateDescription.m_Format      = xiiGALResourceFormat::RGBA16Float;
  accumulateDescription.m_Size.width  = uiRenderWidth;
  accumulateDescription.m_Size.height = uiRenderHeight;
  accumulateDescription.m_uiMipLevels = 1U;
  accumulateDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  accumulateDescription.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hOITAccumulate               = builder.WriteTexture(xiiRGBlackboardKeys::k_OITAccumulateBuffer, accumulateDescription, xiiGALResourceStateFlags::RenderTarget);

  xiiGALTextureCreationDescription revealDescription;
  revealDescription.m_Type        = xiiGALResourceDimension::Texture2D;
  revealDescription.m_Format      = xiiGALResourceFormat::R8UNormalized;
  revealDescription.m_Size.width  = uiRenderWidth;
  revealDescription.m_Size.height = uiRenderHeight;
  revealDescription.m_uiMipLevels = 1U;
  revealDescription.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
  revealDescription.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hOITReveal               = builder.WriteTexture(xiiRGBlackboardKeys::k_OITRevealBuffer, revealDescription, xiiGALResourceStateFlags::RenderTarget);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_TransparencyPasses.m_pOITResolvePipeline, "Shaders/Pipeline/OITResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteWeightedBlendedOIT(const xiiWeightedBlendedOITData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = m_View.GetRenderResolutionHeight();

  cmd.BeginDebugGroup("WeightedBlendedOIT");
  {
    ClearRenderTargetPreservingState(cmd, context.GetTexture(data.m_hOITAccumulate)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    ClearRenderTargetPreservingState(cmd, context.GetTexture(data.m_hOITReveal)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor(1.0f, 1.0f, 1.0f, 1.0f));
    cmd.SetViewport({0.0f, 0.0f, static_cast<float>(uiRenderWidth), static_cast<float>(uiRenderHeight), 0.0f, 1.0f});

    if (m_Resources.m_TransparencyPasses.m_pTranslucentPipeline && data.m_hDrawIndirectCommands.IsValid())
    {
      cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pTranslucentPipeline);
      BindFrameResources(cmd);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hDrawIndirectCommands), GetDrawCommandCapacity(m_View.GetBlackboard())});
    }

    if (m_Resources.m_TransparencyPasses.m_pOITResolvePipeline)
    {
      cmd.SetPipelineState(m_Resources.m_TransparencyPasses.m_pOITResolvePipeline);
      BindFrameResources(cmd);
      cmd.ResolveAndSetShaderResourceTextureView("g_OITAccum", context.GetTexture(data.m_hOITAccumulate)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_OITReveal", context.GetTexture(data.m_hOITReveal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_HDROut", context.GetTexture(data.m_hHDRSceneColor)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Screen-Space Global Illumination Data //////////
//
void xiiBuiltinRenderPipelineRuntime::SetupPlanarReflections(xiiPlanarReflectionsData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiRenderHeight = m_View.GetRenderResolutionHeight();

  if (!m_Resources.m_ScreenSpacePasses.m_pPlanarReflectionTarget)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type        = xiiGALResourceDimension::Texture2D;
    description.m_Format      = xiiGALResourceFormat::RGBA16Float;
    description.m_Size.width  = xiiMath::Max(1U, uiRenderWidth / 2U);
    description.m_Size.height = xiiMath::Max(1U, uiRenderHeight / 2U);
    description.m_uiMipLevels = 1U;
    description.m_BindFlags   = xiiGALBindFlags::RenderTarget | xiiGALBindFlags::ShaderResource;
    description.m_Usage       = xiiGALResourceUsage::Default;

    m_Resources.m_ScreenSpacePasses.m_pPlanarReflectionTarget = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hPlanarTarget = builder.ImportTexture(xiiRGBlackboardKeys::k_PlanarReflectionMap, m_Resources.m_ScreenSpacePasses.m_pPlanarReflectionTarget, m_Resources.m_ScreenSpacePasses.m_pPlanarReflectionTarget->GetResourceState());
  data.m_hPlanarTarget = builder.WriteTexture(data.m_hPlanarTarget, xiiGALResourceStateFlags::RenderTarget);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecutePlanarReflections(const xiiPlanarReflectionsData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("PlanarReflections");
  {
    ClearRenderTargetPreservingState(cmd, context.GetTexture(data.m_hPlanarTarget)->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::MakeZero());
    // Secondary view reflection rendering is scheduled by the render world module.
  }
  cmd.EndDebugGroup();
}

////////// GPU Luminance Histogram Data //////////
//
// Collects all GPU resources related to luminance histogram generation.
