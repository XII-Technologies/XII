/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Visibility/GpuMeshletDispatchConstants.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityView, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityView>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("FrustumPlanes", GetFrustumPlaneCount, GetFrustumPlane),
    XII_MEMBER_PROPERTY("CameraPosition", m_CameraPosition),
    XII_MEMBER_PROPERTY("ViewportAndHiZ", m_ViewportAndHiZ),
    XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
    XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
    XII_MEMBER_PROPERTY("RequiredFlags", m_uiRequiredFlags),
    XII_MEMBER_PROPERTY("ExcludedFlags", m_uiExcludedFlags),
    XII_MEMBER_PROPERTY("GeometryBaseIndex", m_uiGeometryBaseIndex),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGpuVisibilityPurpose, 1)
  XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::MainView),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Shadow),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Reflection),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Sensor),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Editor),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityPassDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityPassDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_ENUM_MEMBER_PROPERTY("Purpose", xiiGpuVisibilityPurpose, m_Purpose),
    XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxInstances", m_uiMaxInstances),
    XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets),
    XII_MEMBER_PROPERTY("MaxMeshletsPerGeometry", m_uiMaxMeshletsPerGeometry),
    XII_MEMBER_PROPERTY("MaxDrawCommands", m_uiMaxDrawCommands),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight),
    XII_MEMBER_PROPERTY("MaxVisibilitySets", m_uiMaxVisibilitySets),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  struct UploadPassData
  {
    xiiRenderGraphBufferHandle m_hScene;
    xiiRenderGraphBufferHandle m_hView;
    xiiDynamicArray<xiiGpuSceneInstance> m_Instances;
    xiiDynamicArray<xiiSceneUploadRange> m_UploadRanges;
    xiiGpuVisibilityView m_View;
  };

  struct ResetPassData
  {
    xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle m_hDrawCount;
  };

  struct InstanceCullPassData
  {
    xiiRenderGraphBufferHandle m_hScene;
    xiiRenderGraphBufferHandle m_hView;
    xiiRenderGraphBufferHandle m_hGeometry;
    xiiRenderGraphBufferHandle m_hVisibleInstances;
    xiiRenderGraphBufferHandle m_hVisibleCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32 m_uiInstanceCount = 0U;
  };

  struct MeshletCullPassData
  {
    xiiRenderGraphBufferHandle m_hScene;
    xiiRenderGraphBufferHandle m_hView;
    xiiRenderGraphBufferHandle m_hGeometry;
    xiiRenderGraphBufferHandle m_hMeshlets;
    xiiRenderGraphBufferHandle m_hVisibleInstances;
    xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle m_hDispatchArguments;
    xiiRenderGraphBufferHandle m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
  };

  struct MeshletDispatchBuildPassData
  {
    xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle m_hDispatchArguments;
    xiiRenderGraphBufferHandle m_hConstants;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32 m_uiMaxMeshletGroups = 0U;
  };

  struct HiZOcclusionPassData
  {
    xiiRenderGraphBufferHandle m_hScene;
    xiiRenderGraphBufferHandle m_hView;
    xiiRenderGraphBufferHandle m_hCandidates;
    xiiRenderGraphBufferHandle m_hCandidateCount;
    xiiRenderGraphTextureHandle m_hHiZ;
    xiiRenderGraphBufferHandle m_hVisibleInstances;
    xiiRenderGraphBufferHandle m_hVisibleCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32 m_uiMaxCandidates = 0U;
  };

  struct CommandBuildPassData
  {
    xiiRenderGraphBufferHandle m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle m_hCommands;
    xiiRenderGraphBufferHandle m_hCommandCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32 m_uiMaxMeshlets = 0U;
  };

  xiiGALBufferCreationDescription MakeBuffer(xiiUInt32 uiSize, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiEnum<xiiGALBufferMode> mode = xiiGALBufferMode::Structured)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize = uiSize;
    desc.m_uiElementByteStride = uiStride;
    desc.m_BindFlags = bindFlags;
    desc.m_Mode = mode;
    desc.m_Usage = xiiGALResourceUsage::Default;
    return desc;
  }
}

xiiGpuVisibilitySystem::~xiiGpuVisibilitySystem() { Shutdown(); }

xiiResult xiiGpuVisibilitySystem::Initialize(xiiGALDevice* pDevice, const xiiGpuVisibilityDescription& description)
{
  Shutdown();
  if (pDevice == nullptr || description.m_uiMaxInstances == 0U || description.m_uiMaxVisibleMeshlets == 0U || description.m_uiMaxMeshletsPerGeometry == 0U || description.m_uiMaxDrawCommands == 0U || description.m_uiFramesInFlight == 0U || description.m_uiMaxVisibilitySets == 0U)
    return XII_FAILURE;
  m_Description = description;
  m_pDevice = pDevice;
  const xiiUInt32 uiResourceSlotCount = description.m_uiFramesInFlight * description.m_uiMaxVisibilitySets;
  m_pSceneBuffers.SetCount(uiResourceSlotCount);
  m_pViewBuffers.SetCount(uiResourceSlotCount);
  m_SceneBufferMirrors.SetCount(uiResourceSlotCount);

  m_pInstanceCullPipeline = LoadComputePipeline("Shaders/Visibility/GpuSceneInstanceCull.xiiShader");
  m_pHiZOcclusionPipeline = LoadComputePipeline("Shaders/Visibility/GpuSceneHiZOcclusion.xiiShader");
  m_pMeshletDispatchBuildPipeline = LoadComputePipeline("Shaders/Visibility/GpuSceneMeshletDispatchBuild.xiiShader");
  m_pMeshletCullPipeline  = LoadComputePipeline("Shaders/Visibility/GpuSceneMeshletCull.xiiShader");
  m_pCommandBuildPipeline = LoadComputePipeline("Shaders/Visibility/GpuSceneCommandBuild.xiiShader");
  return m_pInstanceCullPipeline != nullptr && m_pHiZOcclusionPipeline != nullptr && m_pMeshletDispatchBuildPipeline != nullptr && m_pMeshletCullPipeline != nullptr && m_pCommandBuildPipeline != nullptr ? XII_SUCCESS : XII_FAILURE;
}

void xiiGpuVisibilitySystem::Shutdown()
{
  m_pInstanceCullPipeline.Clear();
  m_pHiZOcclusionPipeline.Clear();
  m_pMeshletDispatchBuildPipeline.Clear();
  m_pMeshletCullPipeline.Clear();
  m_pCommandBuildPipeline.Clear();
  m_pSceneBuffers.Clear();
  m_pViewBuffers.Clear();
  m_SceneBufferMirrors.Clear();
  m_VisibilitySetIndices.Clear();
  m_uiLargestReportedMeshletCount = 0U;
  m_pDevice = nullptr;
  m_Description = {};
}

xiiUInt32 xiiGpuVisibilitySystem::GetOrCreateVisibilitySetIndex(xiiStringView sName)
{
  xiiHashedString name;
  name.Assign(sName);

  xiiUInt32 uiSetIndex = xiiInvalidIndex;
  if (m_VisibilitySetIndices.TryGetValue(name, uiSetIndex))
    return uiSetIndex;

  uiSetIndex = m_VisibilitySetIndices.GetCount();
  if (uiSetIndex >= m_Description.m_uiMaxVisibilitySets)
  {
    xiiLog::Error("GPU visibility set capacity ({}) exceeded while creating '{}'.", m_Description.m_uiMaxVisibilitySets, sName);
    return xiiInvalidIndex;
  }

  const auto sceneDescription = MakeBuffer(m_Description.m_uiMaxInstances * sizeof(xiiGpuSceneInstance), sizeof(xiiGpuSceneInstance), xiiGALBindFlags::ShaderResource);
  const auto viewDescription = MakeBuffer(sizeof(xiiGpuVisibilityView), sizeof(xiiGpuVisibilityView), xiiGALBindFlags::ShaderResource);
  for (xiiUInt32 uiFrameSlot = 0U; uiFrameSlot < m_Description.m_uiFramesInFlight; ++uiFrameSlot)
  {
    const xiiUInt32 uiResourceSlot = uiSetIndex * m_Description.m_uiFramesInFlight + uiFrameSlot;
    m_pSceneBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(sceneDescription);
    m_pViewBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(viewDescription);
    if (m_pSceneBuffers[uiResourceSlot] == nullptr || m_pViewBuffers[uiResourceSlot] == nullptr)
      return xiiInvalidIndex;

    xiiStringBuilder debugName;
    debugName.SetFormat("{} GPU Scene Instances [{}]", sName, uiFrameSlot);
    m_pSceneBuffers[uiResourceSlot]->SetDebugName(debugName);
    debugName.SetFormat("{} GPU Visibility View [{}]", sName, uiFrameSlot);
    m_pViewBuffers[uiResourceSlot]->SetDebugName(debugName);
  }

  m_VisibilitySetIndices.Insert(name, uiSetIndex);
  return uiSetIndex;
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGpuVisibilitySystem::LoadComputePipeline(xiiStringView sShaderPath)
{
  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
  xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, variables, true);
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid()) return nullptr;
  xiiGALComputePipelineStateCreationDescription desc;
  desc.m_pComputeShader = permutation->GetGALShader(xiiGALShaderType::Compute);
  desc.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
  return xiiGALPipelineCache::GetPipeline(desc);
}

xiiGpuVisibilityView xiiGpuVisibilitySystem::BuildView(const xiiMat4& viewProjectionMatrix, const xiiFrustum& frustum, const xiiVec3& vCameraPosition, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiHiZMipCount, xiiUInt32 uiInstanceCount, xiiUInt32 uiVisibilityMask)
{
  xiiGpuVisibilityView result;
  result.m_ViewProjectionMatrix = viewProjectionMatrix;
  for (xiiUInt32 i = 0; i < 6U; ++i) result.m_FrustumPlanes[i] = frustum.GetPlane(static_cast<xiiUInt8>(i)).GetAsVec4();
  result.m_CameraPosition = xiiVec4(vCameraPosition.x, vCameraPosition.y, vCameraPosition.z, 1.0f);
  result.m_ViewportAndHiZ = xiiVec4(static_cast<float>(uiWidth), static_cast<float>(uiHeight), static_cast<float>(uiHiZMipCount), 0.0005f);
  result.m_uiInstanceCount = uiInstanceCount;
  result.m_uiVisibilityMask = uiVisibilityMask;
  return result;
}

xiiGpuVisibilityOutputs xiiGpuVisibilitySystem::AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ)
{
  XII_ASSERT_DEV(scene.GetGpuInstances().GetCount() <= m_Description.m_uiMaxInstances, "Scene instance capacity exceeded.");
  const xiiUInt32 uiMeshletGroupCount = (geometry.m_uiMaximumResidentMeshletCount + 63U) / 64U;
  const xiiUInt32 uiMaximumGroupCountX = m_pDevice->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountX;
  XII_ASSERT_ALWAYS(uiMeshletGroupCount <= uiMaximumGroupCountX,
    "A resident geometry LOD requires {} meshlet-culling groups, exceeding the device limit of {}.", uiMeshletGroupCount, uiMaximumGroupCountX);
  if (geometry.m_uiMaximumResidentMeshletCount > m_Description.m_uiMaxMeshletsPerGeometry && geometry.m_uiMaximumResidentMeshletCount > m_uiLargestReportedMeshletCount)
  {
    xiiLog::Warning("Resident geometry contains {} meshlets in one LOD, above the configured advisory budget of {}. Dispatch was expanded to preserve correctness.",
      geometry.m_uiMaximumResidentMeshletCount, m_Description.m_uiMaxMeshletsPerGeometry);
    m_uiLargestReportedMeshletCount = geometry.m_uiMaximumResidentMeshletCount;
  }
  const xiiUInt32 uiFrameSlot = static_cast<xiiUInt32>(uiFrameIndex % m_Description.m_uiFramesInFlight);
  const xiiUInt32 uiVisibilitySetIndex = GetOrCreateVisibilitySetIndex(description.m_sName);
  XII_ASSERT_ALWAYS(uiVisibilitySetIndex != xiiInvalidIndex, "Failed to allocate GPU visibility resources for '{}'.", description.m_sName);
  const xiiUInt32 uiResourceSlot = uiVisibilitySetIndex * m_Description.m_uiFramesInFlight + uiFrameSlot;
  const xiiUInt32 uiInstanceCount = xiiMath::Min(view.m_uiInstanceCount, scene.GetGpuInstances().GetCount());
  const xiiBitflags<xiiGALCommandQueueFlags> computeQueue = description.m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  auto makeName = [&description](xiiStringView sSuffix) {
    xiiStringBuilder name(description.m_sName);
    name.Append(sSuffix);
    return name;
  };

  auto upload = graph.AddPass<UploadPassData>(
    makeName(" GPU Scene Upload"), xiiGALCommandQueueFlags::Transfer,
    [this, uiResourceSlot, &makeName](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hScene = builder.WriteBuffer(builder.ImportBuffer(makeName(" GPU Scene Instances"), m_pSceneBuffers[uiResourceSlot], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::CopyDestination);
      data.m_hView = builder.WriteBuffer(builder.ImportBuffer(makeName(" GPU Visibility View"), m_pViewBuffers[uiResourceSlot], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::CopyDestination);
      builder.SetPassAllowMerge(false);
    },
    [](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      for (const xiiSceneUploadRange& range : data.m_UploadRanges)
      {
        const xiiGpuSceneInstance* pFirstInstance = data.m_Instances.GetData() + range.m_uiFirstInstance;
        context.GetCommandList().UpdateBuffer(
          context.GetBuffer(data.m_hScene),
          range.m_uiFirstInstance * sizeof(xiiGpuSceneInstance),
          xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(pFirstInstance), range.m_uiInstanceCount * sizeof(xiiGpuSceneInstance)));
      }
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hView), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&data.m_View), sizeof(data.m_View)));
    });
  upload.first->m_Instances = scene.GetGpuInstances();
  xiiDynamicArray<xiiGpuSceneInstance>& sceneMirror = m_SceneBufferMirrors[uiResourceSlot];
  if (sceneMirror.GetCount() != upload.first->m_Instances.GetCount())
  {
    if (!upload.first->m_Instances.IsEmpty())
      upload.first->m_UploadRanges.PushBack({0U, upload.first->m_Instances.GetCount()});
  }
  else
  {
    bool bRangeOpen = false;
    xiiUInt32 uiRangeStart = 0U;
    for (xiiUInt32 i = 0U; i < upload.first->m_Instances.GetCount(); ++i)
    {
      const bool bDirty = xiiMemoryUtils::RawByteCompare(&sceneMirror[i], &upload.first->m_Instances[i], sizeof(xiiGpuSceneInstance)) != 0;
      if (bDirty && !bRangeOpen)
      {
        bRangeOpen = true;
        uiRangeStart = i;
      }
      else if (!bDirty && bRangeOpen)
      {
        upload.first->m_UploadRanges.PushBack({uiRangeStart, i - uiRangeStart});
        bRangeOpen = false;
      }
    }
    if (bRangeOpen)
      upload.first->m_UploadRanges.PushBack({uiRangeStart, upload.first->m_Instances.GetCount() - uiRangeStart});
  }
  sceneMirror = upload.first->m_Instances;
  upload.first->m_View = view;
  upload.first->m_View.m_uiInstanceCount = uiInstanceCount;
  upload.first->m_View.m_uiGeometryBaseIndex = geometry.m_uiGeometryBaseIndex;

  const auto visibleDesc = MakeBuffer(m_Description.m_uiMaxInstances * sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto countDesc = MakeBuffer(sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto drawCountDesc = MakeBuffer(sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);
  const auto meshletDesc = MakeBuffer(m_Description.m_uiMaxVisibleMeshlets * sizeof(xiiVec2U32), sizeof(xiiVec2U32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto commandDesc = MakeBuffer(m_Description.m_uiMaxDrawCommands * sizeof(xiiMeshDrawCommand), sizeof(xiiMeshDrawCommand), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);
  const auto meshletDispatchDesc = MakeBuffer(3U * sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);

  auto reset = graph.AddPass<ResetPassData>(
    makeName(" GPU Visibility Reset"), xiiGALCommandQueueFlags::Transfer,
    [&](ResetPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleInstanceCount = builder.WriteBuffer(makeName(" GPU Visible Instance Count"), countDesc, xiiGALResourceStateFlags::CopyDestination);
      data.m_hVisibleMeshletCount = builder.WriteBuffer(makeName(" GPU Visible Meshlet Count"), countDesc, xiiGALResourceStateFlags::CopyDestination);
      data.m_hDrawCount = builder.WriteBuffer(makeName(" GPU Indirect Command Count"), drawCountDesc, xiiGALResourceStateFlags::CopyDestination);
    },
    [](const ResetPassData& data, xiiRenderGraphPassContext& context) {
      const xiiUInt32 zero = 0U;
      const auto bytes = xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&zero), sizeof(zero));
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hVisibleInstanceCount), 0U, bytes);
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hVisibleMeshletCount), 0U, bytes);
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hDrawCount), 0U, bytes);
    });

  auto instanceCull = graph.AddPass<InstanceCullPassData>(
    makeName(" GPU Instance Culling"), computeQueue,
    [&](InstanceCullPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hScene = builder.ReadBuffer(upload.first->m_hScene, xiiGALResourceStateFlags::ShaderResource);
      data.m_hView = builder.ReadBuffer(upload.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstances = builder.WriteBuffer(makeName(" GPU Visible Instances"), visibleDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hVisibleCount = builder.WriteBuffer(reset.first->m_hVisibleInstanceCount, xiiGALResourceStateFlags::UnorderedAccess);
    },
    [](const InstanceCullPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.SetPipelineState(data.m_pPipeline.Borrow());
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hScene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibilityView", context.GetBuffer(data.m_hView)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstances", context.GetBuffer(data.m_hVisibleInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstanceCount", context.GetBuffer(data.m_hVisibleCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({(data.m_uiInstanceCount + 63U) / 64U, 1U, 1U});
    });
  instanceCull.first->m_pPipeline = m_pInstanceCullPipeline;
  instanceCull.first->m_uiInstanceCount = uiInstanceCount;

  xiiRenderGraphBufferHandle hVisibleInstances = instanceCull.first->m_hVisibleInstances;
  xiiRenderGraphBufferHandle hVisibleInstanceCount = instanceCull.first->m_hVisibleCount;
  if (hHiZ.IsValid())
  {
    auto hiZCull = graph.AddPass<HiZOcclusionPassData>(
      makeName(" GPU Hi-Z Occlusion Culling"), computeQueue,
      [&](HiZOcclusionPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hScene = builder.ReadBuffer(instanceCull.first->m_hScene, xiiGALResourceStateFlags::ShaderResource);
        data.m_hView = builder.ReadBuffer(instanceCull.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
        data.m_hCandidates = builder.ReadBuffer(instanceCull.first->m_hVisibleInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_hCandidateCount = builder.ReadBuffer(instanceCull.first->m_hVisibleCount, xiiGALResourceStateFlags::ShaderResource);
        data.m_hHiZ = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVisibleInstances = builder.WriteBuffer(makeName(" GPU Hi-Z Visible Instances"), visibleDesc, xiiGALResourceStateFlags::UnorderedAccess);
        data.m_hVisibleCount = builder.WriteBuffer(makeName(" GPU Hi-Z Visible Instance Count"), countDesc, xiiGALResourceStateFlags::UnorderedAccess);
      },
      [](const HiZOcclusionPassData& data, xiiRenderGraphPassContext& context) {
        xiiGALCommandList& cmd = context.GetCommandList();
        const xiiUInt32 zero = 0U;
        cmd.UpdateBuffer(context.GetBuffer(data.m_hVisibleCount), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&zero), sizeof(zero)));
        cmd.SetPipelineState(data.m_pPipeline.Borrow());
        cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hScene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_VisibilityView", context.GetBuffer(data.m_hView)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_CandidateInstances", context.GetBuffer(data.m_hCandidates)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceBufferView("g_CandidateCount", context.GetBuffer(data.m_hCandidateCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetShaderResourceTextureView("g_HiZ", context.GetTexture(data.m_hHiZ)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
        cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstances", context.GetBuffer(data.m_hVisibleInstances)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleInstanceCount", context.GetBuffer(data.m_hVisibleCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
        cmd.DispatchCompute({(data.m_uiMaxCandidates + 63U) / 64U, 1U, 1U});
      });
    hiZCull.first->m_pPipeline = m_pHiZOcclusionPipeline;
    hiZCull.first->m_uiMaxCandidates = uiInstanceCount;
    hVisibleInstances = hiZCull.first->m_hVisibleInstances;
    hVisibleInstanceCount = hiZCull.first->m_hVisibleCount;
  }

  auto meshletDispatchBuild = graph.AddPass<MeshletDispatchBuildPassData>(
    makeName(" GPU Meshlet Dispatch Build"), computeQueue,
    [&](MeshletDispatchBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleInstanceCount = builder.ReadBuffer(hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hDispatchArguments = builder.WriteBuffer(makeName(" GPU Meshlet Dispatch Arguments"), meshletDispatchDesc, xiiGALResourceStateFlags::UnorderedAccess);

      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize = sizeof(xiiGpuMeshletDispatchConstants);
      constantsDescription.m_BindFlags = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants = builder.WriteBuffer(makeName(" GPU Meshlet Dispatch Constants"), constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
    },
    [](const MeshletDispatchBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      xiiGALBuffer* pConstants = context.GetBuffer(data.m_hConstants);
      {
        xiiGALMapHelper<xiiGpuMeshletDispatchConstants> constants(cmd, pConstants, xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->MaxMeshletGroups = data.m_uiMaxMeshletGroups;
      }
      cmd.SetPipelineState(data.m_pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiGpuMeshletDispatchConstants", pConstants, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleInstanceCount", context.GetBuffer(data.m_hVisibleInstanceCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_MeshletDispatchArguments", context.GetBuffer(data.m_hDispatchArguments)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
      cmd.DispatchCompute({1U, 1U, 1U});
    });
  meshletDispatchBuild.first->m_pPipeline = m_pMeshletDispatchBuildPipeline;
  meshletDispatchBuild.first->m_uiMaxMeshletGroups = uiMeshletGroupCount;

  auto meshletCull = graph.AddPass<MeshletCullPassData>(
    makeName(" GPU Meshlet Culling"), computeQueue,
    [&](MeshletCullPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hScene = builder.ReadBuffer(instanceCull.first->m_hScene, xiiGALResourceStateFlags::ShaderResource);
      data.m_hView = builder.ReadBuffer(instanceCull.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry = builder.ReadBuffer(instanceCull.first->m_hGeometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshlets = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstances = builder.ReadBuffer(hVisibleInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstanceCount = builder.ReadBuffer(hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hDispatchArguments = builder.ReadBuffer(meshletDispatchBuild.first->m_hDispatchArguments, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hVisibleMeshlets = builder.WriteBuffer(makeName(" GPU Visible Meshlets"), meshletDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hVisibleMeshletCount = builder.WriteBuffer(reset.first->m_hVisibleMeshletCount, xiiGALResourceStateFlags::UnorderedAccess);
    },
    [](const MeshletCullPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.SetPipelineState(data.m_pPipeline.Borrow());
      cmd.ResolveAndSetShaderResourceBufferView("g_SceneInstances", context.GetBuffer(data.m_hScene)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibilityView", context.GetBuffer(data.m_hView)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Geometry", context.GetBuffer(data.m_hGeometry)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_Meshlets", context.GetBuffer(data.m_hMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleInstances", context.GetBuffer(data.m_hVisibleInstances)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleInstanceCount", context.GetBuffer(data.m_hVisibleInstanceCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchComputeIndirect({context.GetBuffer(data.m_hDispatchArguments), xiiGALStateTransitionMode::None});
    });
  meshletCull.first->m_pPipeline = m_pMeshletCullPipeline;

  auto commandBuild = graph.AddPass<CommandBuildPassData>(
    makeName(" GPU Indirect Command Build"), computeQueue,
    [&](CommandBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleMeshlets = builder.ReadBuffer(meshletCull.first->m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshletCount = builder.ReadBuffer(meshletCull.first->m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hCommands = builder.WriteBuffer(makeName(" GPU Mesh Indirect Commands"), commandDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hCommandCount = builder.WriteBuffer(reset.first->m_hDrawCount, xiiGALResourceStateFlags::UnorderedAccess);
    },
    [](const CommandBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd = context.GetCommandList();
      cmd.SetPipelineState(data.m_pPipeline.Borrow());
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_IndirectCommands", context.GetBuffer(data.m_hCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_IndirectCommandCount", context.GetBuffer(data.m_hCommandCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({1U, 1U, 1U});
    });
  commandBuild.first->m_pPipeline = m_pCommandBuildPipeline;
  commandBuild.first->m_uiMaxMeshlets = m_Description.m_uiMaxVisibleMeshlets;

  xiiGpuVisibilityOutputs outputs;
  outputs.m_hSceneInstances = instanceCull.first->m_hScene;
  outputs.m_hVisibleInstances = hVisibleInstances;
  outputs.m_hVisibleInstanceCount = hVisibleInstanceCount;
  outputs.m_hVisibleMeshlets = meshletCull.first->m_hVisibleMeshlets;
  outputs.m_hVisibleMeshletCount = meshletCull.first->m_hVisibleMeshletCount;
  outputs.m_hIndirectCommands = commandBuild.first->m_hCommands;
  outputs.m_hIndirectCommandCount = commandBuild.first->m_hCommandCount;
  return outputs;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuVisibilitySystem);
