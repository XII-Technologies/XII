/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupShadowCascadeSetup(xiiShadowCascadeSetupData& data, xiiRenderGraphBuilder& builder)
{
  for (xiiMat4& mCascadeViewProjection : data.m_CascadeViewProjection)
  {
    mCascadeViewProjection = xiiMat4::MakeIdentity();
  }

  xiiVec3                                 vLightDirection                   = xiiVec3(0.0f, 0.0f, -1.0f);
  const xiiArrayPtr<xiiRenderData* const> renderData                        = m_View.m_pExtractedData != nullptr ? m_View.m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  const xiiDirectionalLightRenderData*    pMainDirectional                  = SelectMainDirectionalLight(renderData);
  const bool                              bHasShadowCastingDirectionalLight = pMainDirectional != nullptr && pMainDirectional->m_bCastShadows;
  if (bHasShadowCastingDirectionalLight)
  {
    vLightDirection = pMainDirectional->m_vDirection;
  }

  xiiStaticArray<xiiShadowCascadeDescription, 4> cascades;
  xiiShadowCascadeSettings                       settings;
  settings.m_uiShadowMapResolution = k_uiDirectionalShadowAtlasWidth;
  const float fAspectRatio         = static_cast<float>(m_View.GetRenderResolutionWidth()) / static_cast<float>(xiiMath::Max(m_View.GetRenderResolutionHeight(), 1U));
  if (bHasShadowCastingDirectionalLight && xiiShadowCascadeUtils::Build(*m_View.m_pCamera, fAspectRatio, vLightDirection, settings, cascades).Succeeded())
  {
    data.m_uiActiveCascades = cascades.GetCount();
    for (xiiUInt32 uiCascade = 0U; uiCascade < cascades.GetCount(); ++uiCascade)
    {
      data.m_CascadeViewProjection[uiCascade]         = cascades[uiCascade].m_mViewProjection;
      data.m_vCascadeSplitDepths.GetData()[uiCascade] = cascades[uiCascade].m_fSplitFar;
      data.m_vCascadeWorldRadii.GetData()[uiCascade]  = cascades[uiCascade].m_fWorldRadius;
    }
  }
  m_Resources.m_ShadowPasses.m_uiActiveCascadeCount = data.m_uiActiveCascades;

  // GPU constant buffer: ShadowCascadeConstants (float4x4[4] + float4 + uint + pad3).
  // Keep the resource type consistent with DECLARE_CONSTANT_BUFFER_AUTO so the
  // same reflected declaration can be consumed by raster and compute passes.
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 0U;
  description.m_uiSize              = sizeof(xiiShadowCascadeConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hCascadeMatrices           = builder.WriteBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, description, xiiGALResourceStateFlags::ConstantBuffer);

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteShadowCascadeSetup(const xiiShadowCascadeSetupData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowCascadeSetup");
  {
    {
      xiiGALMapHelper<xiiShadowCascadeConstants> pConstants(cmd, context.GetBuffer(data.m_hCascadeMatrices), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pConstants->ActiveCascadeCount = data.m_uiActiveCascades;

      pConstants->CascadeSplitDepths = data.m_vCascadeSplitDepths;
      pConstants->CascadeWorldRadii  = data.m_vCascadeWorldRadii;
      for (xiiUInt32 i = 0; i < 4; ++i)
      {
        pConstants->CascadeViewProjection[i] = data.m_CascadeViewProjection[i];
      }
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Local Shadow Atlas Allocation //////////
//
// Allocates deterministic atlas descriptors for all shadow-casting local lights.

void xiiBuiltinRenderPipelineRuntime::SetupLocalShadowAtlasAllocation(xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiLocalShadowAtlasData);
  description.m_uiSize              = description.m_uiElementByteStride * m_Resources.m_LightingSystem.GetSettings().m_uiMaxActiveLights;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hLocalShadowAtlasDescriptors = builder.WriteBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, description, xiiGALResourceStateFlags::CopyDestination);

  data.m_ShadowData.PushBackRange(m_Resources.m_LightingSystem.GetLocalShadowData());

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLocalShadowAtlasAllocation(const xiiLocalShadowAtlasAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LocalShadowAtlasUpload");
  {
    if (!data.m_ShadowData.IsEmpty())
    {
      UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hLocalShadowAtlasDescriptors), 0U,
                                  xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_ShadowData.GetData()), data.m_ShadowData.GetCount() * sizeof(xiiLocalShadowAtlasData)));
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Directional Shadow Data //////////
//
// Collects all GPU resources related to directional shadow rendering for the current frame, including cascade matrices, shadow caster lists, and shadow atlases.

void xiiBuiltinRenderPipelineRuntime::SetupDirectionalShadowData(xiiDirectionalShadowData& data, xiiRenderGraphBuilder& builder)
{
  // Persistent directional shadow atlas (D32 float array of 4 slices).
  if (!m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas)
  {
    xiiGALTextureCreationDescription description;
    description.m_Type                                   = xiiGALResourceDimension::Texture2DArray;
    description.m_Format                                 = xiiGALResourceFormat::D32Float;
    description.m_Size.width                             = k_uiDirectionalShadowAtlasWidth;
    description.m_Size.height                            = k_uiDirectionalShadowAtlasHeight;
    description.m_uiArraySizeOrDepth                     = 4U;
    description.m_uiMipLevels                            = 1U;
    description.m_BindFlags                              = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                                  = xiiGALResourceUsage::Default;
    m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);

    XII_ASSERT_DEV(m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas != nullptr, "Failed to create the directional shadow atlas.");
    for (xiiUInt32 uiCascade = 0U; uiCascade < 4U; ++uiCascade)
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType                  = xiiGALTextureViewType::DepthStencil;
      viewDescription.m_uiFirstArrayOrDepthSlice  = uiCascade;
      viewDescription.m_uiArrayOrDepthSlicesCount = 1U;

      m_Resources.m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade] = m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas->CreateView(viewDescription);
      XII_ASSERT_DEV(m_Resources.m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade] != nullptr, "Failed to create directional shadow cascade view {}.", uiCascade);
    }
  }

  data.m_hCascadeMatrices        = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hShadowCasterCommands   = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hDirectionalShadowAtlas = builder.ImportTexture(xiiRGBlackboardKeys::k_DirectionalShadowAtlas, m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas, m_Resources.m_ShadowPasses.m_pDirectionalShadowAtlas->GetResourceState());
  data.m_hDirectionalShadowAtlas = builder.WriteTexture(data.m_hDirectionalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_uiActiveCascades        = m_Resources.m_ShadowPasses.m_uiActiveCascadeCount;

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDirectionalShadowData(const xiiDirectionalShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DirectionalShadowMaps");
  {
    xiiStringBuilder sb;
    for (xiiUInt32 uiCascade = 0; uiCascade < data.m_uiActiveCascades; ++uiCascade)
    {
      sb.SetFormat("DirectionalShadowCascade{}", uiCascade);
      xiiGALScopedDebugGroup debugGroup(cmd, sb);

      xiiGALTextureView* pCascadeDepthView = m_Resources.m_ShadowPasses.m_pDirectionalShadowCascadeViews[uiCascade];
      XII_ASSERT_DEV(pCascadeDepthView != nullptr, "Directional shadow cascade view {} is unavailable.", uiCascade);

      // Clear and render only the current array slice. Clearing the default
      // array DSV here would erase every previously rendered cascade.
      cmd.SetViewport({0.0f, 0.0f, static_cast<float>(k_uiDirectionalShadowAtlasWidth), static_cast<float>(k_uiDirectionalShadowAtlasHeight), 0.0f, 1.0f});
      ClearDepthPreservingState(cmd, pCascadeDepthView, true, false, 0.0f, 0U);

      if (m_Resources.m_ShadowPasses.m_pShadowDepthPipeline)
      {
        cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pShadowDepthPipeline);
        BindFrameResources(cmd);

        xiiGALDrawIndexedIndirectDescription indexedIndirectDrawDescription;
        indexedIndirectDrawDescription.m_IndexType             = xiiGALValueType::UInt32;
        indexedIndirectDrawDescription.m_pBuffer               = context.GetBuffer(data.m_hShadowCasterCommands);
        const xiiUInt32 uiShadowCommandCapacity                = GetShadowCommandCapacity(m_View.GetBlackboard());
        indexedIndirectDrawDescription.m_uiDrawArgumentOffset  = static_cast<xiiUInt64>(uiCascade + 1U) * uiShadowCommandCapacity * sizeof(xiiExtractedDrawCommand);
        indexedIndirectDrawDescription.m_uiDrawArgumentStride  = 20U; // stride per cascade DrawIndexedIndirectArguments (uint index count, uint instance count, uint start index location, int base vertex location, uint start instance location).
        indexedIndirectDrawDescription.m_uiDrawCount           = uiShadowCommandCapacity;
        indexedIndirectDrawDescription.m_BufferStateTransition = xiiGALStateTransitionMode::Transition;

        // Cascade index uploaded via push constant / cbuffer update.
        cmd.DrawIndexedIndirect(indexedIndirectDrawDescription);
      }
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Spot Shadow Data //////////
//
// Collects all GPU resources related to spot shadow rendering for the current frame, including shadow caster lists and shadow atlases.

void xiiBuiltinRenderPipelineRuntime::SetupSpotShadowData(xiiSpotShadowData& data, xiiRenderGraphBuilder& builder)
{
  if (!m_Resources.m_ShadowPasses.m_pLocalShadowAtlas)
  {
    const xiiUInt32                  uiAtlasSize = m_Resources.m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;
    xiiGALTextureCreationDescription description;
    description.m_Type                             = xiiGALResourceDimension::Texture2D;
    description.m_Format                           = xiiGALResourceFormat::D32Float;
    description.m_Size.width                       = uiAtlasSize;
    description.m_Size.height                      = uiAtlasSize;
    description.m_uiMipLevels                      = 1U;
    description.m_BindFlags                        = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                            = xiiGALResourceUsage::Default;
    m_Resources.m_ShadowPasses.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hShadowCasterCommands        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ImportTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, m_Resources.m_ShadowPasses.m_pLocalShadowAtlas, m_Resources.m_ShadowPasses.m_pLocalShadowAtlas->GetResourceState());
  data.m_hLocalShadowAtlas            = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_View.m_pExtractedData != nullptr ? m_View.m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiSpotLightCount                            = CountRenderDataByTypeName(renderData, "xiiSpotLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteSpotShadowData(const xiiSpotShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("SpotLightShadows");
  {
    xiiGALTexture* pAtlas = context.GetTexture(data.m_hLocalShadowAtlas);

    // The atlas is persistent and must be initialized every frame even before
    // a local shadow raster pipeline is available. Reversed-Z far depth is
    // zero, so an unpopulated allocation deterministically evaluates as lit.
    ClearDepthPreservingState(cmd, pAtlas->GetDefaultView(xiiGALTextureViewType::DepthStencil), true, false, 0.0f, 0U);

    if (data.m_uiSpotLightCount > 0U && m_Resources.m_ShadowPasses.m_pShadowDepthPipeline)
    {
      // For each spot light, render into its collision-free atlas tile.
      cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pShadowDepthPipeline);
      BindFrameResources(cmd);
      const float fAtlasSize = static_cast<float>(m_Resources.m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize);
      cmd.SetViewport({0.0f, 0.0f, fAtlasSize, fAtlasSize, 0.0f, 1.0f});
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands), GetShadowCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Point Shadow Data //////////
//
// Collects all GPU resources related to point shadow rendering for the current frame, including shadow caster lists and shadow atlases.

void xiiBuiltinRenderPipelineRuntime::SetupPointShadowData(xiiPointShadowData& data, xiiRenderGraphBuilder& builder)
{
  if (!m_Resources.m_ShadowPasses.m_pLocalShadowAtlas)
  {
    const xiiUInt32                  uiAtlasSize = m_Resources.m_LightingSystem.GetSettings().m_uiLocalShadowAtlasSize;
    xiiGALTextureCreationDescription description;
    description.m_Type                             = xiiGALResourceDimension::Texture2D;
    description.m_Format                           = xiiGALResourceFormat::D32Float;
    description.m_Size.width                       = uiAtlasSize;
    description.m_Size.height                      = uiAtlasSize;
    description.m_uiMipLevels                      = 1U;
    description.m_BindFlags                        = xiiGALBindFlags::DepthStencil | xiiGALBindFlags::ShaderResource;
    description.m_Usage                            = xiiGALResourceUsage::Default;
    m_Resources.m_ShadowPasses.m_pLocalShadowAtlas = xiiGALDevice::GetDefaultDevice()->CreateTexture(description);
  }

  data.m_hShadowCasterCommands        = builder.ReadBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, xiiGALResourceStateFlags::IndirectArgument);
  data.m_hLocalShadowAtlasDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_LocalShadowAtlasDescs, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLocalShadowAtlas            = builder.ReadTexture(xiiRGBlackboardKeys::k_LocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);
  data.m_hLocalShadowAtlas            = builder.WriteTexture(data.m_hLocalShadowAtlas, xiiGALResourceStateFlags::DepthWrite);

  const xiiArrayPtr<xiiRenderData* const> renderData = m_View.m_pExtractedData != nullptr ? m_View.m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_uiPointLightCount                           = CountRenderDataByTypeName(renderData, "xiiPointLightRenderData");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecutePointShadowData(const xiiPointShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  if (data.m_uiPointLightCount == 0U || !m_Resources.m_ShadowPasses.m_pShadowDepthPipeline)
    return;

  cmd.BeginDebugGroup("PointLightCubeShadows");
  {
    cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pShadowDepthPipeline);
    BindFrameResources(cmd);

    // Each point light: 6 draw calls placing results into 6 atlas tiles.
    for (xiiUInt32 uiFace = 0; uiFace < data.m_uiPointLightCount * 6U; ++uiFace)
    {
      cmd.DrawIndexedIndirect({xiiGALValueType::UInt32, context.GetBuffer(data.m_hShadowCasterCommands), GetShadowCommandCapacity(m_View.GetBlackboard())});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Ray-Traced Shadow Data //////////
//
// Collects all GPU resources related to ray-traced shadow rendering for the current frame, including raw shadow masks and scene depth.

void xiiBuiltinRenderPipelineRuntime::SetupRayTracedShadowData(xiiRayTracedShadowData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSceneDepth    = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription description;
  description.m_Type        = xiiGALResourceDimension::Texture2D;
  description.m_Format      = xiiGALResourceFormat::R8UNormalized;
  description.m_Size.width  = m_View.GetRenderResolutionWidth();
  description.m_Size.height = m_View.GetRenderResolutionHeight();
  description.m_uiMipLevels = 1u;
  description.m_BindFlags   = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Usage       = xiiGALResourceUsage::Default;
  data.m_hRTRawShadowMask   = builder.WriteTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  auto&       shadowPasses      = m_Resources.m_ShadowPasses;
  const auto& lightingPasses    = m_Resources.m_LightingPasses;
  data.m_bUseHardwareRayTracing = lightingPasses.m_pRayTracingScene != nullptr && EnsureRayTracingShadowResources();
  if (data.m_bUseHardwareRayTracing)
  {
    data.m_pTopLevelAS          = lightingPasses.m_pRayTracingScene;
    data.m_pRayTracingPipeline  = shadowPasses.m_pRayTracedShadowPipeline;
    data.m_uiShaderRecordStride = shadowPasses.m_uiRayTracedShadowShaderRecordStride;
    if (lightingPasses.m_hRayTracingSceneDependency.IsValid())
      data.m_hSceneDependency = builder.ReadBuffer(lightingPasses.m_hRayTracingSceneDependency, xiiGALResourceStateFlags::BuildASRead);
    if (lightingPasses.m_hRayTracingMaterialData.IsValid())
      data.m_hMaterialData = builder.ReadBuffer(lightingPasses.m_hRayTracingMaterialData, xiiGALResourceStateFlags::ShaderResource);
    if (lightingPasses.m_hRayTracingGeometryData.IsValid())
      data.m_hGeometryData = builder.ReadBuffer(lightingPasses.m_hRayTracingGeometryData, xiiGALResourceStateFlags::ShaderResource);
    data.m_hShaderBindingTable = builder.ImportBuffer("RT Shadow Shader Binding Table", shadowPasses.m_pRayTracedShadowShaderBindingTable, shadowPasses.m_pRayTracedShadowShaderBindingTable->GetResourceState());
    data.m_hShaderBindingTable = builder.ReadBuffer(data.m_hShaderBindingTable, xiiGALResourceStateFlags::RayTracing);
  }
  else
  {
    xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(shadowPasses.m_pRayTracedShadowFallbackPipeline, "Shaders/Pipeline/RTShadowFallback.xiiShader");
  }

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteRayTracedShadowData(const xiiRayTracedShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd            = context.GetCommandList();
  const xiiUInt32    uiRenderWidth  = m_View.GetRenderResolutionWidth();
  const xiiUInt32    uiRenderHeight = m_View.GetRenderResolutionHeight();

  cmd.BeginDebugGroup("Ray-Traced Shadows");
  {
    if (data.m_bUseHardwareRayTracing)
    {
      cmd.SetPipelineState(data.m_pRayTracingPipeline.Borrow());
      BindFrameResources(cmd);
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::AllRayTracing);
      cmd.ResolveAndSetAccelerationStructure("g_RayTracingScene", data.m_pTopLevelAS.Borrow(), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::RayGeneration);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::RayGeneration);
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

      xiiGALTraceRaysDescription trace(sbtUpdate.m_pShaderBindingTable, uiRenderWidth, uiRenderHeight);
      trace.m_RayGenerationTable = sbtUpdate.m_RayGenerationTable;
      trace.m_MissTable          = sbtUpdate.m_MissTable;
      trace.m_HitTable           = sbtUpdate.m_HitTable;
      cmd.TraceRays(trace);
    }
    else
    {
      cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pRayTracedShadowFallbackPipeline);
      BindFrameResources(cmd);
      m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessTextureView("g_RTShadowOut", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(uiRenderWidth + 7U) / 8U, (uiRenderHeight + 7U) / 8U, 1U});
    }
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Denoise Data //////////
//
// Collects all GPU resources related to shadow denoising for the current frame, including raw shadow masks and final shadow masks.

void xiiBuiltinRenderPipelineRuntime::SetupShadowDenoiseData(xiiShadowDenoiseData& data, xiiRenderGraphBuilder& builder)
{
  const xiiUInt32 uiWidth        = m_View.GetRenderResolutionWidth();
  const xiiUInt32 uiHeight       = m_View.GetRenderResolutionHeight();
  const xiiUInt32 uiCurrentSlot  = m_Resources.m_ShadowPasses.m_uiRayTracedShadowHistoryWriteIndex;
  data.m_uiHistoryWriteIndex     = uiCurrentSlot;
  const xiiUInt32 uiPreviousSlot = (uiCurrentSlot + 1U) & 1U;
  auto&           resources      = m_Resources.m_ShadowPasses;

  if (EnsureTemporalHistoryTextures(resources.m_pRayTracedShadowHistory, xiiGALResourceFormat::R8UNormalized, uiWidth, uiHeight))
  {
    resources.m_bRayTracedShadowHistoryValid = false;
  }

  data.m_bHistoryValid       = resources.m_bRayTracedShadowHistoryValid;
  data.m_hRTRawShadowMask    = builder.ReadTexture(xiiRGBlackboardKeys::k_RTRawShadowMask, xiiGALResourceStateFlags::ShaderResource);
  data.m_hSceneDepth         = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);
  data.m_hGBufferNormal      = builder.ReadTexture(xiiRGBlackboardKeys::k_GBufferNormal, xiiGALResourceStateFlags::ShaderResource);
  data.m_hVelocity           = builder.ReadTexture(xiiRGBlackboardKeys::k_DilatedVelocityBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousSurface    = builder.ReadTexture("ReSTIRDISurfacePrevious", xiiGALResourceStateFlags::ShaderResource);
  data.m_hPreviousShadowMask = builder.ReadTexture(builder.ImportTexture("RT Shadow Previous History", resources.m_pRayTracedShadowHistory[uiPreviousSlot], resources.m_pRayTracedShadowHistory[uiPreviousSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
  data.m_hRTFinalShadowMask  = builder.WriteTexture(builder.ImportTexture(xiiRGBlackboardKeys::k_RTFinalShadowMask, resources.m_pRayTracedShadowHistory[uiCurrentSlot], resources.m_pRayTracedShadowHistory[uiCurrentSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hConstants          = CreateTemporalDenoiseConstants(builder, "RT Shadow Temporal Denoise Constants");

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(resources.m_pShadowDenoisePipeline, "Shaders/Pipeline/TemporalDenoiseMask.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteShadowDenoiseData(const xiiShadowDenoiseData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowDenoise");
  {
    {
      xiiGALMapHelper<xiiTemporalDenoiseConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->HistoryWeight   = 0.88f;
      pConstants->DepthThreshold  = 0.05f;
      pConstants->NormalThreshold = 0.90f;
      pConstants->SpatialWeight   = 0.25f;
      pConstants->HistoryValid    = data.m_bHistoryValid ? 1U : 0U;
      pConstants->SignalMode      = 2U;
      pConstants->_Padding        = xiiVec2::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pShadowDenoisePipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiTemporalDenoiseConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_CurrentSignal", context.GetTexture(data.m_hRTRawShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSignal", context.GetTexture(data.m_hPreviousShadowMask)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_GBufferNormal", context.GetTexture(data.m_hGBufferNormal)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_Velocity", context.GetTexture(data.m_hVelocity)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_PreviousSurface", context.GetTexture(data.m_hPreviousSurface)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_DenoisedOutput", context.GetTexture(data.m_hRTFinalShadowMask)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
    m_Resources.m_ShadowPasses.m_bRayTracedShadowHistoryValid = true;
    m_Resources.m_ShadowPasses.m_uiRayTracedShadowHistoryWriteIndex = 1U - data.m_uiHistoryWriteIndex;
  }
  cmd.EndDebugGroup();
}

////////// GPU Contact Shadow Data //////////
//
// Collects all GPU resources related to contact shadow rendering for the current frame, including scene depth and contact shadow masks.

void xiiBuiltinRenderPipelineRuntime::SetupContactShadowData(xiiContactShadowData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hSceneDepth = builder.ReadTexture(xiiRGBlackboardKeys::k_SceneDepthTexture, xiiGALResourceStateFlags::ShaderResource);

  xiiGALTextureCreationDescription desc;
  desc.m_Type           = xiiGALResourceDimension::Texture2D;
  desc.m_Format         = xiiGALResourceFormat::R8UNormalized;
  desc.m_Size.width     = m_View.GetRenderResolutionWidth();
  desc.m_Size.height    = m_View.GetRenderResolutionHeight();
  desc.m_uiMipLevels    = 1U;
  desc.m_BindFlags      = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  desc.m_Usage          = xiiGALResourceUsage::Default;
  data.m_hContactShadow = builder.WriteTexture(xiiRGBlackboardKeys::k_ContactShadowTerm, desc, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_ShadowPasses.m_pContactShadowPipeline, "Shaders/Pipeline/ContactShadows.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteContactShadowData(const xiiContactShadowData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ContactShadows");
  {
    cmd.SetPipelineState(m_Resources.m_ShadowPasses.m_pContactShadowPipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceTextureView("g_SceneDepth", context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessTextureView("g_ContactShadowOut", context.GetTexture(data.m_hContactShadow)->GetDefaultView(xiiGALTextureViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(m_View.GetRenderResolutionWidth() + 7U) / 8U, (m_View.GetRenderResolutionHeight() + 7U) / 8U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Depth Prepass Data //////////
//
// Collects all GPU resources related to depth prepass rendering for the current frame, including the scene depth target and indirect draw commands.
