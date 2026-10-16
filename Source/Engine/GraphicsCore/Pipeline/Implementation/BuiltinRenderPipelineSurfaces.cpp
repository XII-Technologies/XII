/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;



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
    EnsureFallbackTexture(m_Resources.m_DecalPasses.m_pFallbackDecalAlbedoAtlasTexture, 0xFFFFFFFFU, "FallbackDecalAtlas::Albedo");
    EnsureFallbackTexture(m_Resources.m_DecalPasses.m_pFallbackDecalNormalAtlasTexture, 0xFFFF8080U, "FallbackDecalAtlas::Normal");
    EnsureFallbackTexture(m_Resources.m_DecalPasses.m_pFallbackDecalMaterialAtlasTexture, 0x00FF0080U, "FallbackDecalAtlas::Material");
    EnsureFallbackTexture(m_Resources.m_DecalPasses.m_pFallbackDecalEmissiveAtlasTexture, 0x00000000U, "FallbackDecalAtlas::Emissive");

    if (m_Resources.m_DecalPasses.m_pFallbackDecalAtlasSampler == nullptr)
    {
      xiiGALSamplerCreationDescription samplerDescription    = xiiGALGraphicsUtilities::GetDefaultSamplerDescription();
      samplerDescription.m_AddressU                          = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressV                          = xiiGALTextureAddressMode::Clamp;
      samplerDescription.m_AddressW                          = xiiGALTextureAddressMode::Clamp;
      m_Resources.m_DecalPasses.m_pFallbackDecalAtlasSampler = xiiGALDevice::GetDefaultDevice()->CreateSampler(samplerDescription);
    }
  };

  EnsureFallbackAtlases();

  xiiSharedPtr<xiiGALTexture> pAtlasAlbedo   = m_Resources.m_DecalPasses.m_pFallbackDecalAlbedoAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasNormal   = m_Resources.m_DecalPasses.m_pFallbackDecalNormalAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasMaterial = m_Resources.m_DecalPasses.m_pFallbackDecalMaterialAtlasTexture;
  xiiSharedPtr<xiiGALTexture> pAtlasEmissive = m_Resources.m_DecalPasses.m_pFallbackDecalEmissiveAtlasTexture;

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

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DecalPasses.m_pDecalCullBatchPipeline, "Shaders/Pipeline/DecalClassification.xiiShader");

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

    cmd.SetPipelineState(m_Resources.m_DecalPasses.m_pDecalCullBatchPipeline);

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

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DecalPasses.m_pSSDecalResolvePipeline, "Shaders/Pipeline/DecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteProjectedDecalResolve(const xiiProjectedDecalResolveData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ProjectedDecalResolve");
  {
    cmd.SetPipelineState(m_Resources.m_DecalPasses.m_pSSDecalResolvePipeline);
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

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_DecalPasses.m_pMeshDecalResolvePipeline, "Shaders/Pipeline/MeshDecalResolve.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteMeshDecalDraw(const xiiMeshDecalDrawData& data, xiiRenderGraphPassContext& context)
{
  if (data.m_uiDecalCount == 0U)
    return;

  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("MeshDecalDraw");
  {
    cmd.SetPipelineState(m_Resources.m_DecalPasses.m_pMeshDecalResolvePipeline);
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



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_BuiltinRenderPipelineSurfaces);
