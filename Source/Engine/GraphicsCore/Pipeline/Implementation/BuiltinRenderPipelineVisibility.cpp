/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Implementation/BuiltinRenderPipelineHelpers.h>

using namespace xiiBuiltinPipelineUtils;

void xiiBuiltinRenderPipelineRuntime::SetupLightingDataUpload(xiiLightingDataUploadData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U;
  description.m_uiSize              = 4U;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;

  data.m_hLightingDataReady = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightingDataReady, description, xiiGALResourceStateFlags::UnorderedAccess);

  const auto* lighting = m_Resources.m_LightingSystem.BorrowSystem();
  const auto& camera   = lighting->GetCameraConstantsBuffer();
  const auto& light    = lighting->GetLightConstantsBuffer();
  const auto& global   = lighting->GetGlobalConstantsBuffer();
  data.m_hCamera       = builder.WriteBuffer(builder.ImportBuffer("PerFrameCamera", camera, camera->GetResourceState()), xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hLight        = builder.WriteBuffer(builder.ImportBuffer("PerFrameLight", light, light->GetResourceState()), xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hGlobal       = builder.WriteBuffer(builder.ImportBuffer("PerFrameGlobal", global, global->GetResourceState()), xiiGALResourceStateFlags::ConstantBuffer);

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLightingDataUpload(const xiiLightingDataUploadData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightingDataUpload");
  {
    m_Resources.m_LightingSystem.UploadFrameData(cmd);

    const xiiUInt32 uiReadyToken = 1U;
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hLightingDataReady), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&uiReadyToken), sizeof(uiReadyToken)));
  }
  cmd.EndDebugGroup();
}

////////// GPU occlusion readback //////////
//
// Reads the oldest staging buffer in the 3-frame ring (from 2 frames ago).
// This provides the GPU frame time used by the PID (the result has already been applied by RunDynamicResolutionPID before BeginSetup, so this pass simply keeps the readback ring rotating).

void xiiBuiltinRenderPipelineRuntime::SetupOcclusionReadback(xiiOcclusionReadbackData& data, xiiRenderGraphBuilder& builder)
{
  data.m_uiReadSlot = (m_Resources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % Resources::VisibilityPasses::s_uiReadbackRingSize;

  builder.SetPassSideEffects(true);
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteOcclusionReadback(const xiiOcclusionReadbackData& data, xiiRenderGraphPassContext& context)
{
  xiiSharedPtr<xiiGALBuffer>& pStaging = m_Resources.m_VisibilityPasses.m_pOcclusionReadbackRing[data.m_uiReadSlot];

  if (pStaging == nullptr)
  {
    // Buffer not yet populated - skip.
    // Advance the write slot so the next stage can write into it next frame.
    m_Resources.m_VisibilityPasses.m_uiReadbackWriteSlot = (m_Resources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % Resources::VisibilityPasses::s_uiReadbackRingSize;
    return;
  }

  // Map the staging buffer (CPU readable, GPU wrote 2 frames ago).
  xiiGALCommandList& cmd   = context.GetCommandList();
  void*              pData = nullptr;
  if (cmd.MapBuffer(pStaging, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, pData).Succeeded())
  {
    // The staging buffer holds a single float: total GPU frame time in nanoseconds.
    // (Written by the previous frame's FrameTotal sentinel query readback.)
    // We don't store it here, the profiler's GetPassDurationMs("FrameTotal") path already does it.
    xiiLog::Debug("Occlusion readback: GPU frame time from 2 frames ago = {0} ms", *static_cast<float*>(pData) / 1'000'000.0f);
    cmd.UnmapBuffer(pStaging, xiiGALMapType::Read).AssertSuccess("Failed to unmap occlusion readback buffer.");
  }

  // Advance ring.
  m_Resources.m_VisibilityPasses.m_uiReadbackWriteSlot = (m_Resources.m_VisibilityPasses.m_uiReadbackWriteSlot + 1U) % Resources::VisibilityPasses::s_uiReadbackRingSize;
}

////////// GPU Frustum Culling //////////
//
// Culls the current frame's extracted mesh packets against the view frustum on the GPU.
// The compact output is consumed by LOD selection, instance update, and indirect draw construction.

static_assert(sizeof(xiiFrustumCullInstanceBounds) == 32U);

void xiiBuiltinRenderPipelineRuntime::SetupFrustumCull(xiiFrustumCullData& data, xiiRenderGraphBuilder& builder)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiArrayPtr<xiiRenderData* const> renderData = m_View.m_pExtractedData != nullptr ? m_View.m_pExtractedData->GetAllRenderData() : xiiArrayPtr<xiiRenderData* const>();
  data.m_InstanceBounds.Reserve(xiiMath::Min(renderData.GetCount(), k_uiMaxInstances));
  for (const xiiRenderData* pRenderData : renderData)
  {
    const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
    if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
      continue;

    xiiFrustumCullInstanceBounds& bounds = data.m_InstanceBounds.ExpandAndGetRef();
    bounds.m_CenterRadius                = xiiVec4(pMesh->m_GlobalBounds.m_vCenter.x, pMesh->m_GlobalBounds.m_vCenter.y, pMesh->m_GlobalBounds.m_vCenter.z, pMesh->m_GlobalBounds.m_fSphereRadius);
    bounds.m_Extents                     = xiiVec4(pMesh->m_GlobalBounds.m_vBoxHalfExtents.x, pMesh->m_GlobalBounds.m_vBoxHalfExtents.y, pMesh->m_GlobalBounds.m_vBoxHalfExtents.z, 0.0f);

    if (data.m_InstanceBounds.GetCount() == k_uiMaxInstances)
      break;
  }
  data.m_uiInstanceCount = data.m_InstanceBounds.GetCount();
  m_View.GetBlackboard().Set(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);

  // Ensure persistent instance bounds buffer exists.
  if (!m_Resources.m_VisibilityPasses.m_pInstanceBoundsBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                      = 32U; // float3 center + float radius + float3 extents + float pad.
    description.m_uiSize                                   = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                     = xiiGALBufferMode::Structured;
    description.m_Usage                                    = xiiGALResourceUsage::Default;
    m_Resources.m_VisibilityPasses.m_pInstanceBoundsBuffer = pDevice->CreateBuffer(description);
  }

  // Import persistent instance bounds as read-only SRV. Their contents are refreshed before dispatch.
  data.m_hInstanceBounds = builder.ImportBuffer("InstanceBoundsIn", m_Resources.m_VisibilityPasses.m_pInstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds = builder.ReadBuffer(data.m_hInstanceBounds, xiiGALResourceStateFlags::ShaderResource);

  // Transient visible candidate buffer.
  xiiGALBufferCreationDescription visibleCandidateBufferDescription;
  visibleCandidateBufferDescription.m_uiElementByteStride = 4U;                         // uint
  visibleCandidateBufferDescription.m_uiSize              = 4U + 4U * k_uiMaxInstances; // [0]=count + indices
  visibleCandidateBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  visibleCandidateBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  visibleCandidateBufferDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hVisibleCandidates                               = builder.WriteBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, visibleCandidateBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiFrustumCullingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiFrustumCullingConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  if (m_View.GetCullingCamera() != nullptr)
  {
    xiiFrustum frustum;
    m_View.ComputeCullingFrustum(frustum);
    data.m_Constants.FrustumPlane0 = frustum.GetPlane(xiiFrustum::NearPlane).GetAsVec4();
    data.m_Constants.FrustumPlane1 = frustum.GetPlane(xiiFrustum::LeftPlane).GetAsVec4();
    data.m_Constants.FrustumPlane2 = frustum.GetPlane(xiiFrustum::RightPlane).GetAsVec4();
    data.m_Constants.FrustumPlane3 = frustum.GetPlane(xiiFrustum::FarPlane).GetAsVec4();
    data.m_Constants.FrustumPlane4 = frustum.GetPlane(xiiFrustum::BottomPlane).GetAsVec4();
    data.m_Constants.FrustumPlane5 = frustum.GetPlane(xiiFrustum::TopPlane).GetAsVec4();
  }
  else
  {
    data.m_uiInstanceCount = 0U;
  }
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pFrustumCullPipeline, "Shaders/Pipeline/CoarseFrustumCulling.xiiShader");

  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteFrustumCull(const xiiFrustumCullData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("FrustumCulling");
  {
    if (!data.m_InstanceBounds.IsEmpty())
    {
      const xiiUInt8* pBoundsBytes = reinterpret_cast<const xiiUInt8*>(data.m_InstanceBounds.GetData());
      UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hInstanceBounds), 0U, xiiMakeArrayPtr(pBoundsBytes, data.m_InstanceBounds.GetCount() * sizeof(xiiFrustumCullInstanceBounds)));
    }

    const xiiUInt32 uiZero = 0U;
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hVisibleCandidates), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    {
      xiiGALMapHelper<xiiFrustumCullingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pFrustumCullPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiFrustumCullingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Bounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleOut", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU LOD Selection //////////
//
// Resolves the LOD selected during mesh extraction into a GPU-visible metadata stream. The
// residency-aware xiiGpuVisibilitySystem performs fully GPU-driven LOD selection for GPU scenes.

void xiiBuiltinRenderPipelineRuntime::SetupLODSelect(xiiLODSelectData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hVisibleCandidates = builder.ReadBuffer(xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);

  const bool bHasExtractedMeshCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  data.m_PreferredLOD.Reserve(data.m_uiInstanceCount);
  if (m_View.m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_View.m_pExtractedData->GetAllRenderData())
    {
      const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
      if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
        continue;

      const xiiUInt32 uiLod   = xiiMath::Min(pMesh->m_uiLODIndex, 0xFFU);
      const xiiUInt32 uiFlags = pMesh->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) ? XII_BIT(0) : 0U;
      data.m_PreferredLOD.PushBack(uiLod | (uiFlags << 16U));
      if (data.m_PreferredLOD.GetCount() == data.m_uiInstanceCount)
        break;
    }
  }
  data.m_uiInstanceCount         = data.m_PreferredLOD.GetCount();
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiGALBufferCreationDescription preferredDescription;
  preferredDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  preferredDescription.m_uiSize              = sizeof(xiiUInt32) * xiiMath::Max(1U, data.m_uiInstanceCount);
  preferredDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  preferredDescription.m_Mode                = xiiGALBufferMode::Structured;
  preferredDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hPreferredLOD                       = builder.WriteBuffer("ExtractedLODMetadata", preferredDescription, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 4U; // packed uint: LOD level + meshlet offset
  description.m_uiSize              = description.m_uiElementByteStride * k_uiMaxInstances;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceLOD               = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiLODSelectionConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiLODSelectionConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pLODSelectPipeline, "Shaders/Pipeline/LodSelection.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLODSelect(const xiiLODSelectData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LODSelection");
  {
    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hPreferredLOD), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_PreferredLOD.GetData()), data.m_PreferredLOD.GetCount() * sizeof(xiiUInt32)));
    {
      xiiGALMapHelper<xiiLODSelectionConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pLODSelectPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiLODSelectionConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VisibleCandidates", context.GetBuffer(data.m_hVisibleCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_PreferredLOD", context.GetBuffer(data.m_hPreferredLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_InstanceLODOut", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Instance Update //////////
//
// Compacts extracted transforms and bounds into GPU rendering streams using the visible packet list.

static_assert(sizeof(xiiInstanceUpdateSource) == 80U);

void xiiBuiltinRenderPipelineRuntime::SetupInstanceUpdate(xiiInstanceUpdateData& data, xiiRenderGraphBuilder& builder)
{
  xiiUInt32  uiExtractedMeshCount   = 0U;
  const bool bHasExtractedMeshCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, uiExtractedMeshCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  data.m_InstanceSource.Reserve(uiExtractedMeshCount);
  if (m_View.m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_View.m_pExtractedData->GetAllRenderData())
    {
      const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
      if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
        continue;

      xiiInstanceUpdateSource& source = data.m_InstanceSource.ExpandAndGetRef();
      source.m_GlobalTransform        = pMesh->m_GlobalTransform;
      source.m_CenterRadius           = xiiVec4(pMesh->m_GlobalBounds.m_vCenter.x, pMesh->m_GlobalBounds.m_vCenter.y, pMesh->m_GlobalBounds.m_vCenter.z, pMesh->m_GlobalBounds.m_fSphereRadius);
      source.m_Extents                = xiiVec4(pMesh->m_GlobalBounds.m_vBoxHalfExtents.x, pMesh->m_GlobalBounds.m_vBoxHalfExtents.y, pMesh->m_GlobalBounds.m_vBoxHalfExtents.z, 0.0f);

      if (data.m_InstanceSource.GetCount() == uiExtractedMeshCount)
        break;
    }
  }
  data.m_uiInstanceCount         = data.m_InstanceSource.GetCount();
  data.m_Constants.InstanceCount = data.m_uiInstanceCount;

  xiiGALBufferCreationDescription sourceDescription;
  sourceDescription.m_uiElementByteStride = sizeof(xiiInstanceUpdateSource);
  sourceDescription.m_uiSize              = sizeof(xiiInstanceUpdateSource) * xiiMath::Max(1U, data.m_uiInstanceCount);
  sourceDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  sourceDescription.m_Mode                = xiiGALBufferMode::Structured;
  sourceDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hInstanceSource                  = builder.WriteBuffer("ExtractedInstanceSource", sourceDescription, xiiGALResourceStateFlags::ShaderResource);

  // Ensure persistent matrix buffer.
  if (!m_Resources.m_VisibilityPasses.m_pInstanceMatrixBuffer)
  {
    xiiGALBufferCreationDescription description;
    description.m_uiElementByteStride                      = 48U; // float4x3 (3 rows x 4 floats)
    description.m_uiSize                                   = description.m_uiElementByteStride * k_uiMaxInstances;
    description.m_BindFlags                                = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    description.m_Mode                                     = xiiGALBufferMode::Structured;
    description.m_Usage                                    = xiiGALResourceUsage::Default;
    m_Resources.m_VisibilityPasses.m_pInstanceMatrixBuffer = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
  }

  data.m_hInstanceMatrices = builder.ImportBuffer("InstanceWorldMatrices", m_Resources.m_VisibilityPasses.m_pInstanceMatrixBuffer, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_hInstanceMatrices = builder.WriteBuffer(data.m_hInstanceMatrices, xiiGALResourceStateFlags::UnorderedAccess);

  // Output AABB buffer for HiZ culling.
  xiiGALBufferCreationDescription boundsBufferDescription;
  boundsBufferDescription.m_uiElementByteStride = 32U;
  boundsBufferDescription.m_uiSize              = boundsBufferDescription.m_uiElementByteStride * k_uiMaxInstances;
  boundsBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  boundsBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hInstanceBoundsOut                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, boundsBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiInstanceUpdateConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiInstanceUpdateConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pInstanceUpdatePipeline, "Shaders/Pipeline/InstanceUpdate.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteInstanceUpdate(const xiiInstanceUpdateData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("InstanceUpdate");
  {
    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hInstanceSource), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_InstanceSource.GetData()), data.m_InstanceSource.GetCount() * sizeof(xiiInstanceUpdateSource)));
    {
      xiiGALMapHelper<xiiInstanceUpdateConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pInstanceUpdatePipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiInstanceUpdateConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceSource", context.GetBuffer(data.m_hInstanceSource)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_MatricesOut", context.GetBuffer(data.m_hInstanceMatrices)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_BoundsOut", context.GetBuffer(data.m_hInstanceBoundsOut)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Draw Command Build //////////
//
// Compacts exact draw ranges extracted from mesh resources into an indexed-indirect stream.

static_assert(sizeof(xiiExtractedDrawCommand) == 20U);

void xiiBuiltinRenderPipelineRuntime::SetupDrawBuildResources(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder, xiiStringView sCandidateBuffer, xiiStringView sCommandBuffer, xiiStringView sCountBuffer, bool bUploadSource)
{
  data.m_hSurvivors    = builder.ReadBuffer(sCandidateBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceLOD  = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceLODBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_bUploadSource = bUploadSource;

  xiiUInt32  uiExtractedMeshCount   = 0U;
  const bool bHasExtractedMeshCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, uiExtractedMeshCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);

  if (bUploadSource)
  {
    data.m_SourceCommands.Reserve(uiExtractedMeshCount);
    if (m_View.m_pExtractedData != nullptr)
    {
      for (const xiiRenderData* pRenderData : m_View.m_pExtractedData->GetAllRenderData())
      {
        const xiiMeshRenderData* pMesh = xiiDynamicCast<const xiiMeshRenderData*>(pRenderData);
        if (pMesh == nullptr || !pMesh->m_GlobalBounds.IsValid())
          continue;

        xiiExtractedDrawCommand& command  = data.m_SourceCommands.ExpandAndGetRef();
        command.m_uiIndexCountPerInstance = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(static_cast<xiiUInt64>(pMesh->m_uiPrimitiveCount) * 3ULL, xiiMath::MaxValue<xiiUInt32>()));
        command.m_uiInstanceCount         = 1U;
        command.m_uiStartIndexLocation    = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(static_cast<xiiUInt64>(pMesh->m_uiFirstPrimitive) * 3ULL, xiiMath::MaxValue<xiiUInt32>()));
        command.m_iBaseVertexLocation     = 0;
        command.m_uiStartInstanceLocation = data.m_SourceCommands.GetCount() - 1U;

        if (data.m_SourceCommands.GetCount() == uiExtractedMeshCount)
          break;
      }
    }
    data.m_uiInstanceCount = data.m_SourceCommands.GetCount();
  }
  else
  {
    data.m_uiInstanceCount = uiExtractedMeshCount;
  }

  data.m_Constants.InstanceCount   = data.m_uiInstanceCount;
  data.m_Constants.MaxCommandCount = data.m_uiInstanceCount;
  m_View.GetBlackboard().Set(xiiRGBlackboardKeys::k_DrawCommandCapacity, data.m_uiInstanceCount);

  if (bUploadSource)
  {
    xiiGALBufferCreationDescription sourceDescription;
    sourceDescription.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
    sourceDescription.m_uiSize              = sizeof(xiiExtractedDrawCommand) * xiiMath::Max(1U, data.m_uiInstanceCount);
    sourceDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    sourceDescription.m_Mode                = xiiGALBufferMode::Structured;
    sourceDescription.m_Usage               = xiiGALResourceUsage::Default;
    data.m_hSourceCommands                  = builder.WriteBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, sourceDescription, xiiGALResourceStateFlags::ShaderResource);
  }
  else
  {
    data.m_hSourceCommands = builder.ReadBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  }

  xiiGALBufferCreationDescription commandDescription;
  commandDescription.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
  commandDescription.m_uiSize              = sizeof(xiiExtractedDrawCommand) * xiiMath::Max(1U, data.m_uiInstanceCount);
  commandDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  commandDescription.m_Mode                = xiiGALBufferMode::Structured;
  commandDescription.m_Usage               = xiiGALResourceUsage::Default;
  data.m_hDrawCommands                     = builder.WriteBuffer(sCommandBuffer, commandDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription counterBufferDescription;
  counterBufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
  counterBufferDescription.m_uiSize              = sizeof(xiiUInt32);
  counterBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  counterBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hDrawCounts                             = builder.WriteBuffer(sCountBuffer, counterBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiDrawCommandBuildConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiDrawCommandBuildConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pDrawBuildPipeline, "Shaders/Pipeline/DrawCommandBuild.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::SetupCoarseDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder)
{
  SetupDrawBuildResources(data, builder, xiiRGBlackboardKeys::k_VisibleCandidateBuffer, xiiRGBlackboardKeys::k_CoarseDrawIndirectCommands, xiiRGBlackboardKeys::k_CoarseDrawCountBuffer, true);
}

void xiiBuiltinRenderPipelineRuntime::SetupDrawBuild(xiiDrawBuildData& data, xiiRenderGraphBuilder& builder)
{
  SetupDrawBuildResources(data, builder, xiiRGBlackboardKeys::k_SurvivingInstanceBuffer, xiiRGBlackboardKeys::k_DrawIndirectCommands, xiiRGBlackboardKeys::k_DrawCountBuffer, false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteDrawBuild(const xiiDrawBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("DrawCommandBuild");
  {
    const xiiUInt32 uiZero = 0U;
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hDrawCounts), 0U, xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&uiZero), sizeof(uiZero)));

    xiiDynamicArray<xiiExtractedDrawCommand> zeroCommands;
    zeroCommands.SetCount(xiiMath::Max(1U, data.m_uiInstanceCount));
    xiiMemoryUtils::ZeroFill(zeroCommands.GetData(), zeroCommands.GetCount());
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hDrawCommands), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCommands.GetData()), zeroCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    if (data.m_bUploadSource)
    {
      UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hSourceCommands), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_SourceCommands.GetData()), data.m_SourceCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));
    }
    {
      xiiGALMapHelper<xiiDrawCommandBuildConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pDrawBuildPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiDrawCommandBuildConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Survivors", context.GetBuffer(data.m_hSurvivors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceLOD", context.GetBuffer(data.m_hInstanceLOD)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SourceCommands", context.GetBuffer(data.m_hSourceCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawArgs", context.GetBuffer(data.m_hDrawCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_DrawCounts", context.GetBuffer(data.m_hDrawCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63u) / 64u, 1u, 1u});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Caster List Build //////////
//
// Builds one global caster command block for local lights and one culled block per cascade.

void xiiBuiltinRenderPipelineRuntime::SetupShadowCasterBuild(xiiShadowCasterBuildData& data, xiiRenderGraphBuilder& builder)
{
  data.m_hSourceCommands   = builder.ReadBuffer(xiiRGBlackboardKeys::k_ExtractedDrawCommands, xiiGALResourceStateFlags::ShaderResource);
  data.m_hInstanceBounds   = builder.ReadBuffer(xiiRGBlackboardKeys::k_InstanceBoundsBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hCascadeConstants = builder.ReadBuffer(xiiRGBlackboardKeys::k_ShadowCascadeMatrices, xiiGALResourceStateFlags::ConstantBuffer);

  const bool bHasExtractedMeshCount = m_View.GetBlackboard().TryGet(xiiRGBlackboardKeys::k_ExtractedMeshCount, data.m_uiInstanceCount);
  XII_IGNORE_UNUSED(bHasExtractedMeshCount);
  data.m_Constants.InstanceCount             = data.m_uiInstanceCount;
  data.m_Constants.MaxCommandCount           = data.m_uiInstanceCount;
  data.m_Constants.CullingActiveCascadeCount = m_Resources.m_ShadowPasses.m_uiActiveCascadeCount;
  m_View.GetBlackboard().Set(xiiRGBlackboardKeys::k_ShadowCommandCapacity, data.m_uiInstanceCount);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiExtractedDrawCommand);
  description.m_uiSize              = description.m_uiElementByteStride * xiiMath::Max(1U, data.m_uiInstanceCount) * 5U; // Global block followed by four cascade blocks.
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hShadowCasterCommands      = builder.WriteBuffer(xiiRGBlackboardKeys::k_DrawShadowCasterCommands, description, xiiGALResourceStateFlags::UnorderedAccess);

  description.m_uiElementByteStride = sizeof(xiiUInt32);
  description.m_uiSize              = sizeof(xiiUInt32) * 5U;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hShadowCasterCounts        = builder.WriteBuffer("ShadowCasterCommandCounts", description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALBufferCreationDescription constantsDescription;
  constantsDescription.m_uiSize         = sizeof(xiiShadowCasterCullingConstants);
  constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hConstants                     = builder.WriteBuffer("xiiShadowCasterCullingConstants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pShadowCasterBuildPipeline, "Shaders/Pipeline/ShadowCasterCulling.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteShadowCasterBuild(const xiiShadowCasterBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ShadowCasterListBuild");
  {
    xiiDynamicArray<xiiExtractedDrawCommand> zeroCommands;
    zeroCommands.SetCount(xiiMath::Max(1U, data.m_uiInstanceCount) * 5U);
    xiiMemoryUtils::ZeroFill(zeroCommands.GetData(), zeroCommands.GetCount());
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hShadowCasterCommands), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCommands.GetData()), zeroCommands.GetCount() * sizeof(xiiExtractedDrawCommand)));

    xiiUInt32 zeroCounts[5] = {};
    UpdateBufferPreservingState(cmd, context.GetBuffer(data.m_hShadowCasterCounts), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(zeroCounts), sizeof(zeroCounts)));

    if (data.m_uiInstanceCount == 0U)
    {
      cmd.EndDebugGroup();
      return;
    }

    {
      xiiGALMapHelper<xiiShadowCasterCullingConstants> pConstants(cmd, context.GetBuffer(data.m_hConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      *pConstants = data.m_Constants;
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pShadowCasterBuildPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCasterCullingConstants", context.GetBuffer(data.m_hConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiShadowCascadeConstants", context.GetBuffer(data.m_hCascadeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_SourceCommands", context.GetBuffer(data.m_hSourceCommands)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_InstanceBounds", context.GetBuffer(data.m_hInstanceBounds)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawArgs", context.GetBuffer(data.m_hShadowCasterCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ShadowDrawCounts", context.GetBuffer(data.m_hShadowCasterCounts)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Cluster Build Data //////////
//
// Builds a cluster grid for clustered shading on the GPU, using the depth buffer from the current frame's Depth Pre-Pass.
// This is a compute pass that writes out a structured buffer of cluster descriptors, which is then consumed by the main lighting pass for light culling and shading.

void xiiBuiltinRenderPipelineRuntime::SetupClusterBuild(xiiClusterBuildData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hLightingDataReady = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightingDataReady, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiTotalClusters = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 32U; // float4 min + float4 max per cluster AABB
  description.m_uiSize              = description.m_uiElementByteStride * uiTotalClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hClusterDescriptors        = builder.WriteBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pClusterBuildPipeline, "Shaders/Pipeline/ClusterGridBuild.xiiShader");

  description.m_uiElementByteStride = 0;
  description.m_uiSize              = sizeof(xiiLightClusteringConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;

  data.m_hClusterConstants = builder.WriteBuffer("xiiLightClusteringConstants", description, xiiGALResourceStateFlags::ConstantBuffer);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteClusterBuild(const xiiClusterBuildData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ClusterGridBuild");
  {
    xiiUInt32 uiClusterCountX = xiiMath::Max(m_Resources.m_LightingSystem.GetClusterCountX(), 1U);
    xiiUInt32 uiClusterCountY = xiiMath::Max(m_Resources.m_LightingSystem.GetClusterCountY(), 1U);
    xiiUInt32 uiClusterCountZ = xiiMath::Max(m_Resources.m_LightingSystem.GetClusterCountZ(), 1U);
    xiiUInt32 uiTotalClusters = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);

    {
      xiiGALMapHelper<xiiLightClusteringConstants> pClusteringConstants(cmd, context.GetBuffer(data.m_hClusterConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);

      pClusteringConstants->ClusterCountX       = uiClusterCountX;
      pClusteringConstants->ClusterCountY       = uiClusterCountY;
      pClusteringConstants->ClusterCountZ       = uiClusterCountZ;
      pClusteringConstants->TotalClusters       = uiTotalClusters;
      pClusteringConstants->NearPlane           = m_View.m_pCamera->GetNearPlane();
      pClusteringConstants->FarPlane            = m_View.m_pCamera->GetFarPlane();
      pClusteringConstants->LogFarOverNear      = xiiMath::Log2(pClusteringConstants->FarPlane / pClusteringConstants->NearPlane);
      pClusteringConstants->TilePixelsX         = m_Resources.m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->TilePixelsY         = m_Resources.m_LightingSystem.GetSettings().m_uiClusterTileSize;
      pClusteringConstants->MaxLightsPerCluster = m_Resources.m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster;
      pClusteringConstants->ActiveLightCount    = m_Resources.m_LightingSystem.GetActiveLightCount();
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pClusterBuildPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ClustersOut", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();

    const xiiUInt32 uiGroups = (uiTotalClusters + 63U) / 64U;
    cmd.DispatchCompute({uiGroups, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Light List Build Data //////////
//
// Builds light lists for clustered shading on the GPU, using the cluster grid from this frame's Cluster Build pass and the list of active lights from extraction.
// This is a compute pass that writes out structured buffers of light indices per cluster, which are then consumed by the main lighting pass for light culling and shading.

void xiiBuiltinRenderPipelineRuntime::SetupLightListClear(xiiLightListClearData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterConstants = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);

  const xiiUInt32 uiMaxClusters = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = 8U; // uint2 (fixed list offset, atomic count) per cluster
  description.m_uiSize              = description.m_uiElementByteStride * uiMaxClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightGridBuffer           = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiTotalClusters            = uiMaxClusters;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pLightListClearPipeline, "Shaders/Pipeline/LightListClear.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLightListClear(const xiiLightListClearData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListClear");
  {
    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pLightListClearPipeline);
    BindFrameResources(cmd);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusters + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

void xiiBuiltinRenderPipelineRuntime::SetupLightListBuild(xiiLightListData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterConstants   = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);

  const xiiUInt32 uiMaxClusters    = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);
  const xiiUInt32 uiMaxLightsPerCl = xiiMath::Max(m_Resources.m_LightingSystem.GetSettings().m_uiMaxLightsPerCluster, 1U);

  xiiGALBufferCreationDescription indexBufferDescription;
  indexBufferDescription.m_uiElementByteStride = 4U;
  indexBufferDescription.m_uiSize              = 4U * uiMaxClusters * uiMaxLightsPerCl;
  indexBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  indexBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightIndexBuffer                     = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, indexBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  data.m_hLightGridBuffer = builder.WriteBuffer(builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  data.m_uiActiveLightCount = m_Resources.m_LightingSystem.GetActiveLightCount();

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pLightListPipeline, "Shaders/Pipeline/LightListBuild.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLightListBuild(const xiiLightListData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightListBuild");
  {
    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pLightListPipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindLightData(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(xiiMath::Max(data.m_uiActiveLightCount, 1U) + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Analytical Light Sampling Data //////////
//
// Converts each bounded cluster light list into a quantized emitted-power CDF. ReSTIR samples this
// distribution with a logarithmic lookup and uses the exact quantized interval as its proposal PDF.

void xiiBuiltinRenderPipelineRuntime::SetupLightSampling(xiiLightSamplingData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterConstants = builder.ReadBuffer("xiiLightClusteringConstants", xiiGALResourceStateFlags::ConstantBuffer);
  data.m_hLightGridBuffer  = builder.ReadBuffer(xiiRGBlackboardKeys::k_LightGridBuffer, xiiGALResourceStateFlags::ShaderResource);
  data.m_hLightIndexBuffer = builder.WriteBuffer(builder.ReadBuffer(xiiRGBlackboardKeys::k_LightIndexBuffer, xiiGALResourceStateFlags::UnorderedAccess), xiiGALResourceStateFlags::UnorderedAccess);

  const xiiUInt32 uiTotalClusters = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(float);
  description.m_uiSize              = static_cast<xiiUInt64>(sizeof(float)) * uiTotalClusters;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hLightSamplingTotals       = builder.WriteBuffer(xiiRGBlackboardKeys::k_LightSamplingTotals, description, xiiGALResourceStateFlags::UnorderedAccess);
  data.m_uiTotalClusters            = uiTotalClusters;

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pLightSamplingPipeline, "Shaders/Pipeline/LightSamplingBuild.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteLightSampling(const xiiLightSamplingData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("LightSamplingBuild");
  {
    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pLightSamplingPipeline);
    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    m_Resources.m_LightingSystem.BindLightData(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiLightClusteringConstants", context.GetBuffer(data.m_hClusterConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_LightGrid", context.GetBuffer(data.m_hLightGridBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightIndex", context.GetBuffer(data.m_hLightIndexBuffer)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_LightSamplingTotals", context.GetBuffer(data.m_hLightSamplingTotals)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusters + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Reflection Probe Select Data //////////
//
// Selects relevant reflection probes for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of reflection probe indices and a bitmask of which probes affect which instances, which are then consumed by the main lighting pass for reflection probe sampling.

void xiiBuiltinRenderPipelineRuntime::SetupReflectionProbeSelect(xiiReflectionProbeSelectData& data, xiiRenderGraphBuilder& builder)
{
  builder.SetPassAllowMerge(false);

  data.m_hClusterDescriptors = builder.ReadBuffer(xiiRGBlackboardKeys::k_ClusterDescriptors, xiiGALResourceStateFlags::ShaderResource);

  auto& reflectionResources = m_Resources.m_LightingPrepPasses;
  reflectionResources.m_ReflectionProbeTextures.Clear();

  if (reflectionResources.m_pFallbackReflectionProbeTexture == nullptr)
  {
    xiiGALTextureCreationDescription textureDescription;
    textureDescription.m_Type               = xiiGALResourceDimension::TextureCube;
    textureDescription.m_Format             = xiiGALResourceFormat::RGBA8UNormalized;
    textureDescription.m_Size.width         = 1U;
    textureDescription.m_Size.height        = 1U;
    textureDescription.m_uiArraySizeOrDepth = 6U;
    textureDescription.m_uiMipLevels        = 1U;
    textureDescription.m_BindFlags          = xiiGALBindFlags::ShaderResource;
    textureDescription.m_Usage              = xiiGALResourceUsage::Immutable;

    const xiiUInt32                                  uiBlackPixel = 0xFF000000U;
    xiiHybridArray<xiiGALTextureSubResourceData, 6U> initialData;
    for (xiiUInt32 uiFace = 0U; uiFace < 6U; ++uiFace)
    {
      xiiGALTextureSubResourceData& faceData = initialData.ExpandAndGetRef();
      faceData.m_pData                       = xiiMakeByteBlobPtr(static_cast<const void*>(&uiBlackPixel), sizeof(uiBlackPixel));
      faceData.m_uiStride                    = sizeof(uiBlackPixel);
      faceData.m_uiDepthStride               = sizeof(uiBlackPixel);
    }

    xiiGALTextureData textureData(initialData);
    reflectionResources.m_pFallbackReflectionProbeTexture = xiiGALDevice::GetDefaultDevice()->CreateTexture(textureDescription, &textureData);
    if (reflectionResources.m_pFallbackReflectionProbeTexture != nullptr)
      reflectionResources.m_pFallbackReflectionProbeTexture->SetDebugName("ReflectionProbe::FallbackBlackCube");
  }

  xiiHybridArray<const xiiReflectionCaptureRenderData*, XII_MAX_REFLECTION_PROBES> candidates;
  if (m_View.m_pExtractedData != nullptr)
  {
    for (const xiiRenderData* pRenderData : m_View.m_pExtractedData->GetAllRenderData())
    {
      if (const xiiReflectionCaptureRenderData* pProbe = xiiDynamicCast<const xiiReflectionCaptureRenderData*>(pRenderData))
        candidates.PushBack(pProbe);
    }
  }

  candidates.Sort([](const xiiReflectionCaptureRenderData* pLeft, const xiiReflectionCaptureRenderData* pRight) {
    if (pLeft->m_iPriority != pRight->m_iPriority)
      return pLeft->m_iPriority > pRight->m_iPriority;
    return pLeft->m_uiSortingKey < pRight->m_uiSortingKey;
  });

  data.m_Probes.Reserve(xiiMath::Min(candidates.GetCount(), XII_MAX_REFLECTION_PROBES));
  for (const xiiReflectionCaptureRenderData* pProbe : candidates)
  {
    if (data.m_Probes.GetCount() >= XII_MAX_REFLECTION_PROBES)
      break;

    xiiResourceLock<xiiTextureCubeResource> reflectionMap(pProbe->m_hReflectionMap, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!reflectionMap.IsValid())
      continue;

    xiiSharedPtr<xiiGALTexture> pTexture = reflectionMap->GetGALTexture();
    if (pTexture == nullptr)
      continue;

    const xiiVec3 vAbsoluteScale   = pProbe->m_GlobalTransform.m_vScale.Abs();
    const float   fMaximumScale    = xiiMath::Max(vAbsoluteScale.x, xiiMath::Max(vAbsoluteScale.y, vAbsoluteScale.z));
    const float   fInfluenceRadius = pProbe->m_InfluenceShape == xiiReflectionProbeInfluenceShape::Sphere ?
        pProbe->m_fSphereRadius * fMaximumScale :
        pProbe->m_vHalfExtents.CompMul(vAbsoluteScale).GetLength();

    xiiGPUReflectionProbe& gpuProbe = data.m_Probes.ExpandAndGetRef();
    gpuProbe.WorldToProbe           = pProbe->m_GlobalTransform.GetInverse().GetAsMat4();
    gpuProbe.PositionAndRadius      = xiiVec4(pProbe->m_GlobalTransform.m_vPosition, xiiMath::Max(fInfluenceRadius, 0.01f));
    gpuProbe.HalfExtentsAndBlend    = xiiVec4(pProbe->m_vHalfExtents, pProbe->m_fBlendDistance);
    gpuProbe.ProbeParameters        = xiiVec4(pProbe->m_fSphereRadius, pProbe->m_fIntensity, pProbe->m_fSaturation, static_cast<float>(pProbe->m_InfluenceShape.GetValue()));
    gpuProbe.Metadata               = xiiVec4U32(static_cast<xiiUInt32>(pProbe->m_uiSortingKey), data.m_Probes.GetCount() - 1U, static_cast<xiiUInt32>(pProbe->m_iPriority), pProbe->m_bParallaxCorrected ? 1U : 0U);

    reflectionResources.m_ReflectionProbeTextures.PushBack(std::move(pTexture));
  }

  xiiGALBufferCreationDescription description;
  description.m_uiElementByteStride = sizeof(xiiGPUReflectionProbe);
  description.m_uiSize              = sizeof(xiiGPUReflectionProbe) * XII_MAX_REFLECTION_PROBES;
  description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Dynamic;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hProbeData                 = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeData, description, xiiGALResourceStateFlags::ShaderResource);

  description.m_uiElementByteStride = 0U;
  description.m_uiSize              = sizeof(xiiReflectionProbeConstants);
  description.m_BindFlags           = xiiGALBindFlags::UniformBuffer;
  description.m_Mode                = xiiGALBufferMode::Undefined;
  data.m_hProbeConstants            = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeConstants, description, xiiGALResourceStateFlags::ConstantBuffer);

  data.m_uiTotalClusterCount        = xiiMath::Max(m_Resources.m_LightingSystem.GetTotalClusterCount(), 1U);
  description.m_uiElementByteStride = sizeof(xiiVec2U32);
  description.m_uiSize              = sizeof(xiiVec2U32) * data.m_uiTotalClusterCount;
  description.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  description.m_Mode                = xiiGALBufferMode::Structured;
  description.m_Usage               = xiiGALResourceUsage::Default;
  description.m_CPUAccessFlags      = xiiGALCPUAccessFlag::None;
  data.m_hProbeClusters             = builder.WriteBuffer(xiiRGBlackboardKeys::k_ReflectionProbeMask, description, xiiGALResourceStateFlags::UnorderedAccess);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pProbeSelectPipeline, "Shaders/Pipeline/ReflectionProbeSelection.xiiShader");
}

void xiiBuiltinRenderPipelineRuntime::ExecuteReflectionProbeSelect(const xiiReflectionProbeSelectData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("ReflectionProbeSelection");
  {
    {
      xiiGALMapHelper<xiiUInt8> pProbeBytes(cmd, context.GetBuffer(data.m_hProbeData), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      xiiMemoryUtils::ZeroFill(pProbeBytes.GetMappedData(), sizeof(xiiGPUReflectionProbe) * XII_MAX_REFLECTION_PROBES);
      if (!data.m_Probes.IsEmpty())
      {
        const xiiArrayPtr<const xiiUInt8> probeBytes = data.m_Probes.GetByteArrayPtr();
        xiiMemoryUtils::Copy(pProbeBytes.GetMappedData(), probeBytes.GetPtr(), probeBytes.GetCount());
      }
    }
    {
      xiiGALMapHelper<xiiReflectionProbeConstants> pConstants(cmd, context.GetBuffer(data.m_hProbeConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ActiveProbeCount        = data.m_Probes.GetCount();
      pConstants->TotalClusterCount       = data.m_uiTotalClusterCount;
      pConstants->_ReflectionProbePadding = xiiVec2U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pProbeSelectPipeline);

    BindFrameResources(cmd);
    cmd.ResolveAndSetShaderResourceBufferView("g_Clusters", context.GetBuffer(data.m_hClusterDescriptors)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_ReflectionProbeData", context.GetBuffer(data.m_hProbeData)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiReflectionProbeConstants", context.GetBuffer(data.m_hProbeConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_ReflectionProbeClusters", context.GetBuffer(data.m_hProbeClusters)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(data.m_uiTotalClusterCount + 63U) / 64U, 1U, 1U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Froxel Allocation Data //////////
//
// Allocates froxels for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of froxel metadata and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.

void xiiBuiltinRenderPipelineRuntime::SetupFroxelAllocation(xiiFroxelAllocationData& data, xiiRenderGraphBuilder& builder)
{
  xiiGALBufferCreationDescription froxelMetadataBufferDescription;
  froxelMetadataBufferDescription.m_uiElementByteStride = 32U;                                                                      // scattering/extinction + emission/phase
  froxelMetadataBufferDescription.m_uiSize              = froxelMetadataBufferDescription.m_uiElementByteStride * 128U * 72U * 64U; // froxel volume
  froxelMetadataBufferDescription.m_BindFlags           = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  froxelMetadataBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  data.m_hFroxelMetadata                                = builder.WriteBuffer(xiiRGBlackboardKeys::k_FroxelMetadataBuffer, froxelMetadataBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  xiiGALTextureCreationDescription scatteringBufferDescription;
  scatteringBufferDescription.m_Type               = xiiGALResourceDimension::Texture3D;
  scatteringBufferDescription.m_Format             = xiiGALResourceFormat::RGBA16Float;
  scatteringBufferDescription.m_Size.width         = 128U;
  scatteringBufferDescription.m_Size.height        = 72U;
  scatteringBufferDescription.m_uiArraySizeOrDepth = 64U;
  scatteringBufferDescription.m_uiMipLevels        = 1U;
  scatteringBufferDescription.m_BindFlags          = xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::ShaderResource;
  scatteringBufferDescription.m_Usage              = xiiGALResourceUsage::Default;
  data.m_hFroxelScattering                         = builder.WriteTexture(xiiRGBlackboardKeys::k_FroxelScatteringBuffer, scatteringBufferDescription, xiiGALResourceStateFlags::UnorderedAccess);

  if (xiiVolumetricMediumManager::IsSubsystemInitialized())
  {
    const xiiVec3 vViewPosition = m_View.m_pCamera != nullptr ? m_View.m_pCamera->GetCenterPosition() : xiiVec3::MakeZero();
    xiiVolumetricMediumManager::GatherGpuMedia(vViewPosition, data.m_Media);
  }

  data.m_uiMediumBufferCapacity = xiiMath::Max(data.m_Media.GetCount(), 1U);
  xiiGALBufferCreationDescription mediumBufferDescription;
  mediumBufferDescription.m_uiElementByteStride = sizeof(xiiGpuVolumetricMedium);
  mediumBufferDescription.m_uiSize              = sizeof(xiiGpuVolumetricMedium) * data.m_uiMediumBufferCapacity;
  mediumBufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  mediumBufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  mediumBufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  mediumBufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  data.m_hVolumetricMedia                       = builder.WriteBuffer("Volumetric Media", mediumBufferDescription, xiiGALResourceStateFlags::ShaderResource);

  xiiGALBufferCreationDescription constantBufferDescription;
  constantBufferDescription.m_uiSize         = sizeof(xiiVolumetricMediumConstants);
  constantBufferDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  constantBufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
  constantBufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  data.m_hVolumetricConstants                = builder.WriteBuffer("Volumetric Medium Constants", constantBufferDescription, xiiGALResourceStateFlags::ConstantBuffer);

  xiiBuiltinRenderPipelineRuntime::EnsureComputePipeline(m_Resources.m_VisibilityPasses.m_pFroxelSetupPipeline, "Shaders/Pipeline/FroxelSetup.xiiShader");
  builder.SetPassAllowMerge(false);
}

void xiiBuiltinRenderPipelineRuntime::ExecuteFroxelAllocation(const xiiFroxelAllocationData& data, xiiRenderGraphPassContext& context)
{
  xiiGALCommandList& cmd = context.GetCommandList();

  cmd.BeginDebugGroup("VolumetricGridAllocation");
  {
    {
      xiiGALMapHelper<xiiUInt8> pMediumBytes(cmd, context.GetBuffer(data.m_hVolumetricMedia), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      xiiMemoryUtils::ZeroFill(pMediumBytes.GetMappedData(), sizeof(xiiGpuVolumetricMedium) * data.m_uiMediumBufferCapacity);
      if (!data.m_Media.IsEmpty())
      {
        const xiiArrayPtr<const xiiUInt8> mediumBytes = data.m_Media.GetByteArrayPtr();
        xiiMemoryUtils::Copy(pMediumBytes.GetMappedData(), mediumBytes.GetPtr(), mediumBytes.GetCount());
      }
    }
    {
      xiiGALMapHelper<xiiVolumetricMediumConstants> pConstants(cmd, context.GetBuffer(data.m_hVolumetricConstants), xiiGALMapType::Write, xiiGALMapFlags::Discard);
      pConstants->ActiveMediumCount        = data.m_Media.GetCount();
      pConstants->_VolumetricMediumPadding = xiiVec3U32::MakeZero();
    }

    cmd.SetPipelineState(m_Resources.m_VisibilityPasses.m_pFroxelSetupPipeline);

    BindFrameResources(cmd);
    m_Resources.m_LightingSystem.BindFrameConstants(cmd, xiiGALShaderType::Compute);
    cmd.ResolveAndSetShaderResourceBufferView("g_VolumetricMedia", context.GetBuffer(data.m_hVolumetricMedia)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
    cmd.ResolveAndSetConstantBuffer("xiiVolumetricMediumConstants", context.GetBuffer(data.m_hVolumetricConstants), xiiGALShaderType::Compute);
    cmd.ResolveAndSetUnorderedAccessBufferView("g_FroxelMetadata", context.GetBuffer(data.m_hFroxelMetadata)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
    cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
    cmd.DispatchCompute({(128u + 7U) / 8U, (72U + 7U) / 8U, 64U});
  }
  cmd.EndDebugGroup();
}

////////// GPU Shadow Cascade Setup Data //////////
//
// Sets up shadow cascades for the current frame on the GPU, using the visible instance list from the current frame's Frustum Culling pass and instance bounds from the previous frame's Instance Update pass.
// This is a compute pass that writes out a structured buffer of cascade matrices and a texture of froxel scattering, which are then consumed by the main lighting pass for froxel-based lighting.
