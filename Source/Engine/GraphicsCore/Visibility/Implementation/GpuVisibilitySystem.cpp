/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Tools/MapHelper.h>

#include <Shaders/Visibility/GpuMeshletDispatchConstants.h>
#include <Shaders/Visibility/GpuSceneCommandBuildConstants.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityView, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiGpuVisibilityView>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("ViewProjectionMatrix", m_ViewProjectionMatrix),
      XII_MEMBER_PROPERTY("OcclusionViewProjectionMatrix", m_OcclusionViewProjectionMatrix),
      XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("FrustumPlanes", GetFrustumPlaneCount, GetFrustumPlane),
      XII_MEMBER_PROPERTY("CameraPosition", m_CameraPosition),
      XII_MEMBER_PROPERTY("ViewportAndHiZ", m_ViewportAndHiZ),
      XII_MEMBER_PROPERTY("LodParameters", m_LodParameters),
      XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
      XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
      XII_MEMBER_PROPERTY("RequiredFlags", m_uiRequiredFlags),
      XII_MEMBER_PROPERTY("ExcludedFlags", m_uiExcludedFlags),
      XII_MEMBER_PROPERTY("GeometryBaseIndex", m_uiGeometryBaseIndex),
      XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGpuVisibilityPurpose, 1)
  XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::MainView),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Shadow),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Reflection),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Sensor),
    XII_ENUM_CONSTANT(xiiGpuVisibilityPurpose::Editor),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityPassDescription, xiiNoBase, 5, xiiRTTIDefaultAllocator<xiiGpuVisibilityPassDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Name", m_sName),
      XII_MEMBER_PROPERTY("ResourceSetName", m_sResourceSetName),
      XII_ENUM_MEMBER_PROPERTY("Purpose", xiiGpuVisibilityPurpose, m_Purpose),
      XII_MEMBER_PROPERTY("LodScreenScale", m_fLodScreenScale),
      XII_MEMBER_PROPERTY("LodHysteresis", m_fLodHysteresis)->AddAttributes(new xiiClampValueAttribute(0.0f, 0.49f)),
      XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets),
      XII_MEMBER_PROPERTY("EnableLodFeedback", m_bEnableLodFeedback),
      XII_MEMBER_PROPERTY("AsyncCompute", m_bAsyncCompute),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityDescription, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiGpuVisibilityDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MaxInstances", m_uiMaxInstances),
      XII_MEMBER_PROPERTY("MaxVisibleMeshlets", m_uiMaxVisibleMeshlets),
      XII_MEMBER_PROPERTY("MaxMeshletsPerGeometry", m_uiMaxMeshletsPerGeometry),
      XII_MEMBER_PROPERTY("MaxDrawCommands", m_uiMaxDrawCommands),
      XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight),
      XII_MEMBER_PROPERTY("MaxVisibilitySets", m_uiMaxVisibilitySets),
      XII_MEMBER_PROPERTY("LodFeedbackCapacity", m_uiLodFeedbackCapacity)->AddAttributes(new xiiDefaultValueAttribute(4096U), new xiiClampValueAttribute(64U, 65536U)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuLodFeedback, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuLodFeedback>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Key", m_uiKey),
      XII_MEMBER_PROPERTY("GeometryGeneration", m_uiGeometryGeneration),
      XII_MEMBER_PROPERTY("DesiredLod", m_uiDesiredLod),
      XII_MEMBER_PROPERTY("SelectedLod", m_uiSelectedLod),
      XII_MEMBER_PROPERTY("PurposeMask", m_uiPurposeMask),
      XII_MEMBER_PROPERTY("ObjectUserData", m_uiObjectUserData),
      XII_MEMBER_PROPERTY("Priority", m_uiPriority),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityStreamingStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityStreamingStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("RawRequestCount", m_uiRawRequestCount),
      XII_MEMBER_PROPERTY("UniqueRequestCount", m_uiUniqueRequestCount),
      XII_MEMBER_PROPERTY("OverflowRequestCount", m_uiOverflowRequestCount),
      XII_MEMBER_PROPERTY("AcceptedRequestCount", m_uiAcceptedRequestCount),
      XII_MEMBER_PROPERTY("StaleRequestCount", m_uiStaleRequestCount),
      XII_MEMBER_PROPERTY("ReadbackLatencyFrames", m_uiReadbackLatencyFrames),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  struct SceneUploadPassData
  {
    xiiRenderGraphBufferHandle           m_hScene;
    xiiDynamicArray<xiiGpuSceneInstance> m_Instances;
    xiiDynamicArray<xiiSceneUploadRange> m_UploadRanges;
    xiiUInt32                            m_uiFrameSlot = 0U;
  };

  struct ViewUploadPassData
  {
    xiiRenderGraphBufferHandle m_hView;
    xiiGpuVisibilityView       m_View;
  };

  struct ResetPassData
  {
    xiiRenderGraphBufferHandle m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle m_hDrawCount;
  };

  struct InstanceCullPassData
  {
    xiiRenderGraphBufferHandle               m_hScene;
    xiiRenderGraphBufferHandle               m_hView;
    xiiRenderGraphBufferHandle               m_hGeometry;
    xiiRenderGraphBufferHandle               m_hVisibleInstances;
    xiiRenderGraphBufferHandle               m_hVisibleCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiInstanceCount = 0U;
  };

  struct MeshletCullPassData
  {
    xiiRenderGraphBufferHandle               m_hScene;
    xiiRenderGraphBufferHandle               m_hView;
    xiiRenderGraphBufferHandle               m_hGeometry;
    xiiRenderGraphBufferHandle               m_hMeshlets;
    xiiRenderGraphBufferHandle               m_hVisibleInstances;
    xiiRenderGraphBufferHandle               m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle               m_hDispatchArguments;
    xiiRenderGraphBufferHandle               m_hPreviousLodHistory;
    xiiRenderGraphBufferHandle               m_hCurrentLodHistory;
    xiiRenderGraphBufferHandle               m_hLodFeedback;
    xiiRenderGraphTextureHandle              m_hHiZ;
    xiiRenderGraphBufferHandle               m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle               m_hVisibleMeshletCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
  };

  struct LodFeedbackClearPassData
  {
    xiiRenderGraphBufferHandle               m_hFeedback;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiEntryCount = 0U;
  };

  struct LodFeedbackReadbackPassData
  {
    xiiRenderGraphBufferHandle m_hFeedback;
    xiiRenderGraphBufferHandle m_hReadback;
    xiiUInt32                  m_uiResourceSlot = 0U;
    xiiUInt64                  m_uiFrameIndex   = 0U;
  };

  struct MeshletDispatchBuildPassData
  {
    xiiRenderGraphBufferHandle               m_hVisibleInstanceCount;
    xiiRenderGraphBufferHandle               m_hDispatchArguments;
    xiiRenderGraphBufferHandle               m_hConstants;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiMaxMeshletGroups = 0U;
  };

  struct HiZOcclusionPassData
  {
    xiiRenderGraphBufferHandle               m_hScene;
    xiiRenderGraphBufferHandle               m_hView;
    xiiRenderGraphBufferHandle               m_hCandidates;
    xiiRenderGraphBufferHandle               m_hCandidateCount;
    xiiRenderGraphTextureHandle              m_hHiZ;
    xiiRenderGraphBufferHandle               m_hVisibleInstances;
    xiiRenderGraphBufferHandle               m_hVisibleCount;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiMaxCandidates = 0U;
  };

  struct CommandBuildPassData
  {
    xiiRenderGraphBufferHandle               m_hVisibleMeshlets;
    xiiRenderGraphBufferHandle               m_hVisibleMeshletCount;
    xiiRenderGraphBufferHandle               m_hCommands;
    xiiRenderGraphBufferHandle               m_hCommandCount;
    xiiRenderGraphBufferHandle               m_hConstants;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiUInt32                                m_uiMaxGroupCountX     = 0U;
    xiiUInt32                                m_uiMaxGroupCountY     = 0U;
    xiiUInt32                                m_uiMaxGroupTotalCount = 0U;
  };

  xiiGALBufferCreationDescription MakeBuffer(xiiUInt32 uiSize, xiiUInt32 uiStride, xiiBitflags<xiiGALBindFlags> bindFlags, xiiEnum<xiiGALBufferMode> mode = xiiGALBufferMode::Structured)
  {
    xiiGALBufferCreationDescription desc;
    desc.m_uiSize              = uiSize;
    desc.m_uiElementByteStride = uiStride;
    desc.m_BindFlags           = bindFlags;
    desc.m_Mode                = mode;
    desc.m_Usage               = xiiGALResourceUsage::Default;
    return desc;
  }
} // namespace

xiiGpuVisibilitySystem::~xiiGpuVisibilitySystem() { Shutdown(); }

xiiResult xiiGpuVisibilitySystem::Initialize(xiiGALDevice* pDevice, const xiiGpuVisibilityDescription& description)
{
  Shutdown();
  if (pDevice == nullptr || description.m_uiMaxInstances == 0U || description.m_uiMaxVisibleMeshlets == 0U || description.m_uiMaxMeshletsPerGeometry == 0U || description.m_uiMaxDrawCommands == 0U || description.m_uiFramesInFlight < 2U || description.m_uiMaxVisibilitySets == 0U || description.m_uiLodFeedbackCapacity > 65536U || !xiiMath::IsPowerOf2(description.m_uiLodFeedbackCapacity))
    return XII_FAILURE;
  m_Description                                    = description;
  m_pDevice                                        = pDevice;
  const xiiGALMeshShaderProperties& meshProperties = pDevice->GetGraphicsDeviceAdapterProperties().m_MeshShaderProperties;
  if (meshProperties.m_uiMaxThreadGroupCountX == 0U || meshProperties.m_uiMaxThreadGroupCountY == 0U || meshProperties.m_uiMaxThreadGroupCountZ == 0U ||
      meshProperties.m_uiMaxThreadGroupTotalCount == 0U || description.m_uiMaxVisibleMeshlets > meshProperties.m_uiMaxThreadGroupTotalCount)
  {
    xiiLog::Error("GPU visibility requires mesh dispatch capacity for {} visible meshlets; device supports {} total groups.",
                  description.m_uiMaxVisibleMeshlets, meshProperties.m_uiMaxThreadGroupTotalCount);
    Shutdown();
    return XII_FAILURE;
  }
  // Choose a fixed row width whose worst-case padded rectangle still fits the total
  // mesh-workgroup limit. Fixed strides let the mesh shader linearize SV_GroupID without
  // requiring backend-specific draw IDs or an extra command metadata lookup.
  m_uiMeshDispatchGroupCountX = xiiMath::Min(meshProperties.m_uiMaxThreadGroupCountX, description.m_uiMaxVisibleMeshlets);
  while (m_uiMeshDispatchGroupCountX > 0U)
  {
    const xiiUInt64 uiRows             = (static_cast<xiiUInt64>(description.m_uiMaxVisibleMeshlets) + m_uiMeshDispatchGroupCountX - 1U) / m_uiMeshDispatchGroupCountX;
    const xiiUInt64 uiPaddedGroupCount = static_cast<xiiUInt64>(m_uiMeshDispatchGroupCountX) * uiRows;
    if (uiRows <= meshProperties.m_uiMaxThreadGroupCountY && uiPaddedGroupCount <= meshProperties.m_uiMaxThreadGroupTotalCount)
    {
      m_uiMeshDispatchGroupCountY = static_cast<xiiUInt32>(uiRows);
      break;
    }
    --m_uiMeshDispatchGroupCountX;
  }
  m_uiMeshDispatchGroupTotalCount = meshProperties.m_uiMaxThreadGroupTotalCount;
  const xiiUInt64 uiTiledCapacity = static_cast<xiiUInt64>(m_uiMeshDispatchGroupCountX) * m_uiMeshDispatchGroupCountY;
  if (m_uiMeshDispatchGroupCountX == 0U || uiTiledCapacity < description.m_uiMaxVisibleMeshlets)
  {
    xiiLog::Error("GPU visibility mesh dispatch tiling supports {} groups, below the requested {} visible meshlets.", uiTiledCapacity, description.m_uiMaxVisibleMeshlets);
    Shutdown();
    return XII_FAILURE;
  }
  const xiiUInt32 uiResourceSlotCount = description.m_uiFramesInFlight * description.m_uiMaxVisibilitySets;
  m_pSceneBuffers.SetCount(description.m_uiFramesInFlight);
  m_pViewBuffers.SetCount(uiResourceSlotCount);
  m_pLodHistoryBuffers.SetCount(uiResourceSlotCount);
  m_pLodFeedbackBuffers.SetCount(uiResourceSlotCount);
  m_pLodFeedbackReadbackBuffers.SetCount(uiResourceSlotCount);
  m_LodFeedbackReadbackFrames.SetCount(uiResourceSlotCount, xiiMath::MaxValue<xiiUInt64>());
  m_LodFeedbackClearFrames.SetCount(uiResourceSlotCount, xiiMath::MaxValue<xiiUInt64>());
  m_SceneBufferMirrors.SetCount(description.m_uiFramesInFlight);

  const auto sceneDescription = MakeBuffer(description.m_uiMaxInstances * sizeof(xiiGpuSceneInstance), sizeof(xiiGpuSceneInstance), xiiGALBindFlags::ShaderResource);
  for (xiiUInt32 uiFrameSlot = 0U; uiFrameSlot < description.m_uiFramesInFlight; ++uiFrameSlot)
  {
    m_pSceneBuffers[uiFrameSlot] = m_pDevice->CreateBuffer(sceneDescription);
    if (m_pSceneBuffers[uiFrameSlot] == nullptr)
    {
      Shutdown();
      return XII_FAILURE;
    }
    xiiStringBuilder debugName;
    debugName.SetFormat("GPU Scene Instances [{}]", uiFrameSlot);
    m_pSceneBuffers[uiFrameSlot]->SetDebugName(debugName);
  }

  m_pInstanceCullPipeline         = LoadComputePipeline("Shaders/Visibility/GpuSceneInstanceCull.xiiShader");
  m_pHiZOcclusionPipeline         = LoadComputePipeline("Shaders/Visibility/GpuSceneHiZOcclusion.xiiShader");
  m_pMeshletDispatchBuildPipeline = LoadComputePipeline("Shaders/Visibility/GpuSceneMeshletDispatchBuild.xiiShader");
  m_pMeshletCullPipeline          = LoadComputePipeline("Shaders/Visibility/GpuSceneMeshletCull.xiiShader");
  m_pMeshletHiZCullPipeline       = LoadComputePipeline("Shaders/Visibility/GpuSceneMeshletHiZCull.xiiShader");
  m_pLodFeedbackClearPipeline     = LoadComputePipeline("Shaders/Visibility/GpuSceneLodFeedbackClear.xiiShader");
  m_pCommandBuildPipeline         = LoadComputePipeline("Shaders/Visibility/GpuSceneCommandBuild.xiiShader");
  xiiGpuHiZPyramidDescription hiZDescription;
  hiZDescription.m_uiFramesInFlight = description.m_uiFramesInFlight;
  const bool bHiZInitialized = m_HiZHistory.Initialize(pDevice, hiZDescription).Succeeded();
  return m_pInstanceCullPipeline != nullptr && m_pHiZOcclusionPipeline != nullptr && m_pMeshletDispatchBuildPipeline != nullptr && m_pMeshletCullPipeline != nullptr && m_pMeshletHiZCullPipeline != nullptr && m_pLodFeedbackClearPipeline != nullptr && m_pCommandBuildPipeline != nullptr && bHiZInitialized ? XII_SUCCESS : XII_FAILURE;
}

void xiiGpuVisibilitySystem::Shutdown()
{
  m_HiZHistory.Shutdown();
  m_pInstanceCullPipeline.Clear();
  m_pHiZOcclusionPipeline.Clear();
  m_pMeshletDispatchBuildPipeline.Clear();
  m_pMeshletCullPipeline.Clear();
  m_pMeshletHiZCullPipeline.Clear();
  m_pLodFeedbackClearPipeline.Clear();
  m_pCommandBuildPipeline.Clear();
  m_pSceneBuffers.Clear();
  m_pViewBuffers.Clear();
  m_pLodHistoryBuffers.Clear();
  m_pLodFeedbackBuffers.Clear();
  m_pLodFeedbackReadbackBuffers.Clear();
  m_LodFeedbackReadbackFrames.Clear();
  m_LodFeedbackClearFrames.Clear();
  m_SceneBufferMirrors.Clear();
  m_VisibilitySetIndices.Clear();
  m_pPreparedGraph                 = nullptr;
  m_pPreparedScene                 = nullptr;
  m_uiPreparedFrame                = xiiMath::MaxValue<xiiUInt64>();
  m_hPreparedScene                 = {};
  m_uiLargestReportedInstanceCount = 0U;
  m_uiLargestReportedMeshletCount  = 0U;
  m_uiMeshDispatchGroupCountX      = 0U;
  m_uiMeshDispatchGroupCountY      = 0U;
  m_uiMeshDispatchGroupTotalCount  = 0U;
  m_StreamingStats                 = {};
  m_pDevice                        = nullptr;
  m_Description                    = {};
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

  const auto viewDescription       = MakeBuffer(sizeof(xiiGpuVisibilityView), sizeof(xiiGpuVisibilityView), xiiGALBindFlags::ShaderResource);
  const auto lodHistoryDescription = MakeBuffer(m_Description.m_uiMaxInstances * sizeof(xiiVec4U32), sizeof(xiiVec4U32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const xiiUInt32 uiLodFeedbackBufferSize = (m_Description.m_uiLodFeedbackCapacity + 1U) * sizeof(xiiGpuLodFeedback);
  const auto lodFeedbackDescription = MakeBuffer(uiLodFeedbackBufferSize, sizeof(xiiGpuLodFeedback), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  xiiGALBufferCreationDescription lodFeedbackReadbackDescription;
  lodFeedbackReadbackDescription.m_uiSize         = uiLodFeedbackBufferSize;
  lodFeedbackReadbackDescription.m_Usage          = xiiGALResourceUsage::Staging;
  lodFeedbackReadbackDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
  xiiDynamicArray<xiiVec4U32> invalidLodHistory;
  invalidLodHistory.SetCount(m_Description.m_uiMaxInstances, xiiVec4U32(xiiInvalidIndex));
  xiiGALBufferData lodHistoryInitialData(invalidLodHistory.GetData(), invalidLodHistory.GetCount() * sizeof(xiiVec4U32));
  for (xiiUInt32 uiFrameSlot = 0U; uiFrameSlot < m_Description.m_uiFramesInFlight; ++uiFrameSlot)
  {
    const xiiUInt32 uiResourceSlot = uiSetIndex * m_Description.m_uiFramesInFlight + uiFrameSlot;
    m_pViewBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(viewDescription);
    if (m_pViewBuffers[uiResourceSlot] == nullptr)
      return xiiInvalidIndex;

    xiiStringBuilder debugName;
    debugName.SetFormat("{} GPU Visibility View [{}]", sName, uiFrameSlot);
    m_pViewBuffers[uiResourceSlot]->SetDebugName(debugName);

    m_pLodHistoryBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(lodHistoryDescription, &lodHistoryInitialData);
    if (m_pLodHistoryBuffers[uiResourceSlot] == nullptr)
      return xiiInvalidIndex;

    debugName.SetFormat("{} GPU LOD History [{}]", sName, uiFrameSlot);
    m_pLodHistoryBuffers[uiResourceSlot]->SetDebugName(debugName);

    m_pLodFeedbackBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(lodFeedbackDescription);
    m_pLodFeedbackReadbackBuffers[uiResourceSlot] = m_pDevice->CreateBuffer(lodFeedbackReadbackDescription);
    if (m_pLodFeedbackBuffers[uiResourceSlot] == nullptr || m_pLodFeedbackReadbackBuffers[uiResourceSlot] == nullptr)
      return xiiInvalidIndex;

    debugName.SetFormat("{} GPU LOD Feedback [{}]", sName, uiFrameSlot);
    m_pLodFeedbackBuffers[uiResourceSlot]->SetDebugName(debugName);
    debugName.SetFormat("{} GPU LOD Feedback Readback [{}]", sName, uiFrameSlot);
    m_pLodFeedbackReadbackBuffers[uiResourceSlot]->SetDebugName(debugName);
  }

  m_VisibilitySetIndices.Insert(name, uiSetIndex);
  return uiSetIndex;
}

void xiiGpuVisibilitySystem::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  m_StreamingStats = {};
  if (m_pDevice == nullptr)
    return;

  xiiGALCommandListCreationDescription commandListDescription;
  commandListDescription.m_QueueFlags = xiiGALCommandQueueFlags::Transfer;
  xiiSharedPtr<xiiGALCommandList> pCommandList = m_pDevice->CreateCommandList(commandListDescription);
  if (pCommandList == nullptr)
    return;

  pCommandList->Begin();
  for (xiiUInt32 uiResourceSlot = 0U; uiResourceSlot < m_LodFeedbackReadbackFrames.GetCount(); ++uiResourceSlot)
  {
    const xiiUInt64 uiFeedbackFrame = m_LodFeedbackReadbackFrames[uiResourceSlot];
    if (uiFeedbackFrame == xiiMath::MaxValue<xiiUInt64>() || uiFeedbackFrame > uiCompletedFrame ||
        uiFrameIndex < uiFeedbackFrame + m_Description.m_uiFramesInFlight || m_pLodFeedbackReadbackBuffers[uiResourceSlot] == nullptr)
      continue;

    void* pMappedData = nullptr;
    if (pCommandList->MapBuffer(m_pLodFeedbackReadbackBuffers[uiResourceSlot], xiiGALMapType::Read, xiiGALMapFlags::None, pMappedData).Failed())
      continue;

    const xiiGpuLodFeedback* pFeedback = static_cast<const xiiGpuLodFeedback*>(pMappedData);
    const xiiGpuLodFeedback& header    = pFeedback[0];
    m_StreamingStats.m_uiRawRequestCount += header.m_uiKey;
    m_StreamingStats.m_uiUniqueRequestCount += xiiMath::Min(header.m_uiGeometryGeneration, m_Description.m_uiLodFeedbackCapacity);
    m_StreamingStats.m_uiOverflowRequestCount += header.m_uiDesiredLod;
    m_StreamingStats.m_uiReadbackLatencyFrames = xiiMath::Max(
      m_StreamingStats.m_uiReadbackLatencyFrames,
      static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(uiFrameIndex - uiFeedbackFrame, xiiMath::MaxValue<xiiUInt32>())));

    for (xiiUInt32 uiEntry = 1U; uiEntry <= m_Description.m_uiLodFeedbackCapacity; ++uiEntry)
    {
      const xiiGpuLodFeedback& feedback = pFeedback[uiEntry];
      if (feedback.m_uiKey == 0U)
        continue;

      const xiiUInt32 uiPackedKey     = feedback.m_uiKey - 1U;
      const xiiUInt32 uiGeometryIndex = uiPackedKey & 0xFFFFFU;
      const xiiUInt32 uiDesiredLod    = (uiPackedKey >> 20U) & 7U;
      xiiGeometryHandle hGeometry;
      hGeometry.m_uiIndex      = uiGeometryIndex;
      hGeometry.m_uiGeneration = feedback.m_uiGeometryGeneration;
      if (feedback.m_uiDesiredLod != uiDesiredLod || !xiiGeometryResidencyManager::IsValid(hGeometry))
      {
        ++m_StreamingStats.m_uiStaleRequestCount;
        continue;
      }

      const xiiUInt32 uiMainOrSensorMask = XII_BIT(xiiGpuVisibilityPurpose::MainView) | XII_BIT(xiiGpuVisibilityPurpose::Sensor);
      const xiiUInt32 uiReflectionOrEditorMask = XII_BIT(xiiGpuVisibilityPurpose::Reflection) | XII_BIT(xiiGpuVisibilityPurpose::Editor);
      const xiiUInt32 uiPurposeTier = (feedback.m_uiPurposeMask & uiMainOrSensorMask) != 0U ? 3U :
                                      (feedback.m_uiPurposeMask & uiReflectionOrEditorMask) != 0U ? 2U : 1U;
      const xiiUInt32 uiPriority = (uiPurposeTier << 30U) | xiiMath::Min(feedback.m_uiPriority, 0x3FFFFFFFU);
      xiiGeometryResidencyManager::RequestResidency(hGeometry, uiDesiredLod, uiFeedbackFrame, uiPriority);
      ++m_StreamingStats.m_uiAcceptedRequestCount;
    }

    pCommandList->UnmapBuffer(m_pLodFeedbackReadbackBuffers[uiResourceSlot], xiiGALMapType::Read).IgnoreResult();
    m_LodFeedbackReadbackFrames[uiResourceSlot] = xiiMath::MaxValue<xiiUInt64>();
  }
  pCommandList->End();
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGpuVisibilitySystem::LoadComputePipeline(xiiStringView sShaderPath)
{
  xiiShaderResourceHandle                        hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
  xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, variables, true);
  xiiResourceLock<xiiShaderPermutationResource>  permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid()) return nullptr;
  xiiGALComputePipelineStateCreationDescription desc;
  desc.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
  desc.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
  return xiiGALPipelineCache::GetPipeline(desc);
}

xiiGpuVisibilityView xiiGpuVisibilitySystem::BuildView(const xiiMat4& viewProjectionMatrix, const xiiFrustum& frustum, const xiiVec3& vCameraPosition, xiiUInt32 uiWidth, xiiUInt32 uiHeight, xiiUInt32 uiHiZMipCount, xiiUInt32 uiInstanceCount, xiiUInt32 uiVisibilityMask)
{
  xiiGpuVisibilityView result;
  result.m_ViewProjectionMatrix = viewProjectionMatrix;
  result.m_OcclusionViewProjectionMatrix = viewProjectionMatrix;
  for (xiiUInt32 i = 0; i < 6U; ++i) result.m_FrustumPlanes[i] = frustum.GetPlane(static_cast<xiiUInt8>(i)).GetAsVec4();
  result.m_CameraPosition   = xiiVec4(vCameraPosition.x, vCameraPosition.y, vCameraPosition.z, 1.0f);
  result.m_ViewportAndHiZ   = xiiVec4(static_cast<float>(uiWidth), static_cast<float>(uiHeight), static_cast<float>(uiHiZMipCount), 0.0005f);
  result.m_uiInstanceCount  = uiInstanceCount;
  result.m_uiVisibilityMask = uiVisibilityMask;
  return result;
}

xiiGpuVisibilityOutputs xiiGpuVisibilitySystem::AddPasses(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiSceneDatabase& scene, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ)
{
  const xiiUInt32 uiSceneInstanceCount     = scene.GetGpuInstances().GetCount();
  const xiiUInt32 uiRequestedInstanceCount = xiiMath::Min(view.m_uiInstanceCount, uiSceneInstanceCount);
  const xiiUInt32 uiInstanceCount          = xiiMath::Min(uiRequestedInstanceCount, m_Description.m_uiMaxInstances);
  if (uiRequestedInstanceCount > m_Description.m_uiMaxInstances && uiRequestedInstanceCount > m_uiLargestReportedInstanceCount)
  {
    xiiLog::Warning("GPU visibility requested {} scene instances, above the configured capacity of {}. Excess instances are omitted to preserve buffer bounds.",
                    uiRequestedInstanceCount, m_Description.m_uiMaxInstances);
    m_uiLargestReportedInstanceCount = uiRequestedInstanceCount;
  }

  const xiiUInt32 uiMeshletGroupCount  = xiiMath::Max((geometry.m_uiMaximumResidentMeshletCount + 63U) / 64U, 1U);
  const xiiUInt32 uiMaximumGroupCountX = m_pDevice->GetGraphicsDeviceAdapterProperties().m_ComputeShaderProperties.m_uiMaxThreadGroupCountX;
  XII_ASSERT_ALWAYS(uiMeshletGroupCount <= uiMaximumGroupCountX,
                    "A resident geometry LOD requires {} meshlet-culling groups, exceeding the device limit of {}.", uiMeshletGroupCount, uiMaximumGroupCountX);
  if (geometry.m_uiMaximumResidentMeshletCount > m_Description.m_uiMaxMeshletsPerGeometry && geometry.m_uiMaximumResidentMeshletCount > m_uiLargestReportedMeshletCount)
  {
    xiiLog::Warning("Resident geometry contains {} meshlets in one LOD, above the configured advisory budget of {}. Dispatch was expanded to preserve correctness.",
                    geometry.m_uiMaximumResidentMeshletCount, m_Description.m_uiMaxMeshletsPerGeometry);
    m_uiLargestReportedMeshletCount = geometry.m_uiMaximumResidentMeshletCount;
  }
  const xiiStringView sResourceSetName    = description.m_sResourceSetName.IsEmpty() ? description.m_sName.GetView() : description.m_sResourceSetName.GetView();
  const xiiUInt32     uiFrameSlot          = static_cast<xiiUInt32>(uiFrameIndex % m_Description.m_uiFramesInFlight);
  const xiiUInt32     uiVisibilitySetIndex = GetOrCreateVisibilitySetIndex(sResourceSetName);
  XII_ASSERT_ALWAYS(uiVisibilitySetIndex != xiiInvalidIndex, "Failed to allocate GPU visibility resources for '{}'.", sResourceSetName);
  const xiiUInt32 uiResourceSlot         = uiVisibilitySetIndex * m_Description.m_uiFramesInFlight + uiFrameSlot;
  const xiiUInt32 uiPreviousFrameSlot    = (uiFrameSlot + m_Description.m_uiFramesInFlight - 1U) % m_Description.m_uiFramesInFlight;
  const xiiUInt32 uiPreviousResourceSlot = uiVisibilitySetIndex * m_Description.m_uiFramesInFlight + uiPreviousFrameSlot;
  const xiiBitflags<xiiGALCommandQueueFlags> computeQueue = description.m_bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  auto                                       makePassName       = [&description](xiiStringView sSuffix) {
    xiiStringBuilder name(description.m_sName);
    name.Append(sSuffix);
    return name;
  };
  auto makeResourceName = [sResourceSetName](xiiStringView sSuffix) {
    xiiStringBuilder name(sResourceSetName);
    name.Append(sSuffix);
    return name;
  };

  if (m_pPreparedGraph != &graph || m_uiPreparedFrame != uiFrameIndex)
  {
    auto sceneUpload = graph.AddPass<SceneUploadPassData>(
      "GPU Scene Upload", xiiGALCommandQueueFlags::Transfer,
      [this, uiFrameSlot](SceneUploadPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hScene = builder.WriteBuffer(builder.ImportBuffer("GPU Scene Instances", m_pSceneBuffers[uiFrameSlot], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::CopyDestination);
        builder.SetPassAllowMerge(false);
      },
      [this](const SceneUploadPassData& data, xiiRenderGraphPassContext& context) {
        for (const xiiSceneUploadRange& range : data.m_UploadRanges)
        {
          const xiiGpuSceneInstance* pFirstInstance = data.m_Instances.GetData() + range.m_uiFirstInstance;
          context.GetCommandList().UpdateBuffer(
            context.GetBuffer(data.m_hScene),
            range.m_uiFirstInstance * sizeof(xiiGpuSceneInstance),
            xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(pFirstInstance), range.m_uiInstanceCount * sizeof(xiiGpuSceneInstance)));
        }

        // The mirror describes GPU contents, so advance it only after this transfer pass was
        // actually recorded. A graph that fails compilation must retry the same ranges later.
        m_SceneBufferMirrors[data.m_uiFrameSlot] = data.m_Instances;
      });
    sceneUpload.first->m_uiFrameSlot = uiFrameSlot;
    // The GPU scene allocation is fixed-size. Keep the upload snapshot within the same bound as
    // every consumer so a release build cannot overwrite the imported buffer when a scene grows.
    const xiiUInt32 uiUploadedInstanceCount           = xiiMath::Min(uiSceneInstanceCount, m_Description.m_uiMaxInstances);
    sceneUpload.first->m_Instances                    = scene.GetGpuInstances().GetSubArray(0U, uiUploadedInstanceCount);
    xiiDynamicArray<xiiGpuSceneInstance>& sceneMirror = m_SceneBufferMirrors[uiFrameSlot];
    if (sceneMirror.GetCount() != sceneUpload.first->m_Instances.GetCount())
    {
      if (!sceneUpload.first->m_Instances.IsEmpty())
        sceneUpload.first->m_UploadRanges.PushBack({0U, sceneUpload.first->m_Instances.GetCount()});
    }
    else
    {
      bool      bRangeOpen   = false;
      xiiUInt32 uiRangeStart = 0U;
      for (xiiUInt32 i = 0U; i < sceneUpload.first->m_Instances.GetCount(); ++i)
      {
        const bool bDirty = xiiMemoryUtils::RawByteCompare(&sceneMirror[i], &sceneUpload.first->m_Instances[i], sizeof(xiiGpuSceneInstance)) != 0;
        if (bDirty && !bRangeOpen)
        {
          bRangeOpen   = true;
          uiRangeStart = i;
        }
        else if (!bDirty && bRangeOpen)
        {
          sceneUpload.first->m_UploadRanges.PushBack({uiRangeStart, i - uiRangeStart});
          bRangeOpen = false;
        }
      }
      if (bRangeOpen)
        sceneUpload.first->m_UploadRanges.PushBack({uiRangeStart, sceneUpload.first->m_Instances.GetCount() - uiRangeStart});
    }
    m_pPreparedGraph  = &graph;
    m_pPreparedScene  = &scene;
    m_uiPreparedFrame = uiFrameIndex;
    m_hPreparedScene  = sceneUpload.first->m_hScene;
  }
  else
  {
    XII_ASSERT_ALWAYS(m_pPreparedScene == &scene, "A visibility system can prepare only one scene snapshot per graph frame.");
  }

  auto viewUpload = graph.AddPass<ViewUploadPassData>(
    makePassName(" GPU View Upload"), xiiGALCommandQueueFlags::Transfer,
    [this, uiResourceSlot, &makeResourceName](ViewUploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hView = builder.WriteBuffer(builder.ImportBuffer(makeResourceName(" GPU Visibility View"), m_pViewBuffers[uiResourceSlot], xiiGALResourceStateFlags::ShaderResource), xiiGALResourceStateFlags::CopyDestination);
    },
    [](const ViewUploadPassData& data, xiiRenderGraphPassContext& context) {
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hView), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&data.m_View), sizeof(data.m_View)));
    });
  viewUpload.first->m_View                        = view;
  viewUpload.first->m_View.m_uiInstanceCount      = uiInstanceCount;
  viewUpload.first->m_View.m_uiGeometryBaseIndex  = geometry.m_uiGeometryBaseIndex;
  viewUpload.first->m_View.m_uiMaxVisibleMeshlets = description.m_uiMaxVisibleMeshlets == 0U ? m_Description.m_uiMaxVisibleMeshlets : xiiMath::Min(description.m_uiMaxVisibleMeshlets, m_Description.m_uiMaxVisibleMeshlets);
  viewUpload.first->m_View.m_ViewportAndHiZ.y *= xiiMath::Max(description.m_fLodScreenScale, 0.001f);
  constexpr xiiUInt32 uiLodHistoryTokenPeriod  = 65535U;
  const xiiUInt32     uiCurrentLodHistoryToken  = static_cast<xiiUInt32>(uiFrameIndex % uiLodHistoryTokenPeriod);
  const xiiUInt32     uiPreviousLodHistoryToken = (uiCurrentLodHistoryToken + uiLodHistoryTokenPeriod - 1U) % uiLodHistoryTokenPeriod;
  viewUpload.first->m_View.m_LodParameters = xiiVec4(
    xiiMath::Clamp(description.m_fLodHysteresis, 0.0f, 0.49f),
    static_cast<float>(uiCurrentLodHistoryToken),
    static_cast<float>(uiPreviousLodHistoryToken),
    description.m_bEnableLodFeedback ? static_cast<float>(XII_BIT(description.m_Purpose.GetValue())) : 0.0f);

  const auto visibleDesc         = MakeBuffer(m_Description.m_uiMaxInstances * sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto countDesc           = MakeBuffer(sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto drawCountDesc       = MakeBuffer(sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);
  const auto meshletDesc         = MakeBuffer(m_Description.m_uiMaxVisibleMeshlets * sizeof(xiiVec2U32), sizeof(xiiVec2U32), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess);
  const auto commandDesc         = MakeBuffer(sizeof(xiiMeshDrawCommand), sizeof(xiiMeshDrawCommand), xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);
  const auto meshletDispatchDesc = MakeBuffer(3U * sizeof(xiiUInt32), sizeof(xiiUInt32), xiiGALBindFlags::UnorderedAccess | xiiGALBindFlags::IndirectDrawArguments);

  auto reset = graph.AddPass<ResetPassData>(
    makePassName(" GPU Visibility Reset"), xiiGALCommandQueueFlags::Transfer,
    [&](ResetPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleInstanceCount = builder.WriteBuffer(makeResourceName(" GPU Visible Instance Count"), countDesc, xiiGALResourceStateFlags::CopyDestination);
      data.m_hVisibleMeshletCount  = builder.WriteBuffer(makeResourceName(" GPU Visible Meshlet Count"), countDesc, xiiGALResourceStateFlags::CopyDestination);
      data.m_hDrawCount            = builder.WriteBuffer(makeResourceName(" GPU Indirect Command Count"), drawCountDesc, xiiGALResourceStateFlags::CopyDestination);
    },
    [](const ResetPassData& data, xiiRenderGraphPassContext& context) {
      const xiiUInt32 zero  = 0U;
      const auto      bytes = xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&zero), sizeof(zero));
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hVisibleInstanceCount), 0U, bytes);
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hVisibleMeshletCount), 0U, bytes);
      context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hDrawCount), 0U, bytes);
    });

  auto instanceCull = graph.AddPass<InstanceCullPassData>(
    makePassName(" GPU Instance Culling"), computeQueue,
    [&](InstanceCullPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hScene            = builder.ReadBuffer(m_hPreparedScene, xiiGALResourceStateFlags::ShaderResource);
      data.m_hView             = builder.ReadBuffer(viewUpload.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry         = builder.ReadBuffer(geometry.m_hGeometryMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstances = builder.WriteBuffer(makeResourceName(" GPU Visible Instances"), visibleDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hVisibleCount     = builder.ReadWriteBuffer(reset.first->m_hVisibleInstanceCount, xiiGALResourceStateFlags::UnorderedAccess);
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
  instanceCull.first->m_pPipeline       = m_pInstanceCullPipeline;
  instanceCull.first->m_uiInstanceCount = uiInstanceCount;

  xiiRenderGraphBufferHandle hVisibleInstances     = instanceCull.first->m_hVisibleInstances;
  xiiRenderGraphBufferHandle hVisibleInstanceCount = instanceCull.first->m_hVisibleCount;
  if (hHiZ.IsValid())
  {
    auto hiZCull = graph.AddPass<HiZOcclusionPassData>(
      makePassName(" GPU Hi-Z Occlusion Culling"), computeQueue,
      [&](HiZOcclusionPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hScene            = builder.ReadBuffer(instanceCull.first->m_hScene, xiiGALResourceStateFlags::ShaderResource);
        data.m_hView             = builder.ReadBuffer(instanceCull.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
        data.m_hCandidates       = builder.ReadBuffer(instanceCull.first->m_hVisibleInstances, xiiGALResourceStateFlags::ShaderResource);
        data.m_hCandidateCount   = builder.ReadBuffer(instanceCull.first->m_hVisibleCount, xiiGALResourceStateFlags::ShaderResource);
        data.m_hHiZ              = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
        data.m_hVisibleInstances = builder.WriteBuffer(makeResourceName(" GPU Hi-Z Visible Instances"), visibleDesc, xiiGALResourceStateFlags::UnorderedAccess);
        data.m_hVisibleCount     = builder.WriteBuffer(makeResourceName(" GPU Hi-Z Visible Instance Count"), countDesc, xiiGALResourceStateFlags::UnorderedAccess);
      },
      [](const HiZOcclusionPassData& data, xiiRenderGraphPassContext& context) {
        xiiGALCommandList& cmd  = context.GetCommandList();
        const xiiUInt32    zero = 0U;
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
    hiZCull.first->m_pPipeline       = m_pHiZOcclusionPipeline;
    hiZCull.first->m_uiMaxCandidates = uiInstanceCount;
    hVisibleInstances                = hiZCull.first->m_hVisibleInstances;
    hVisibleInstanceCount            = hiZCull.first->m_hVisibleCount;
  }

  auto meshletDispatchBuild = graph.AddPass<MeshletDispatchBuildPassData>(
    makePassName(" GPU Meshlet Dispatch Build"), computeQueue,
    [&](MeshletDispatchBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleInstanceCount = builder.ReadBuffer(hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hDispatchArguments    = builder.WriteBuffer(makeResourceName(" GPU Meshlet Dispatch Arguments"), meshletDispatchDesc, xiiGALResourceStateFlags::UnorderedAccess);

      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiGpuMeshletDispatchConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer(makeResourceName(" GPU Meshlet Dispatch Constants"), constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
    },
    [](const MeshletDispatchBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd        = context.GetCommandList();
      xiiGALBuffer*      pConstants = context.GetBuffer(data.m_hConstants);
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
  meshletDispatchBuild.first->m_pPipeline          = m_pMeshletDispatchBuildPipeline;
  meshletDispatchBuild.first->m_uiMaxMeshletGroups = uiMeshletGroupCount;

  xiiRenderGraphBufferHandle hLodFeedback;
  const bool bClearLodFeedback = description.m_bEnableLodFeedback && m_LodFeedbackClearFrames[uiResourceSlot] != uiFrameIndex;
  if (bClearLodFeedback)
  {
    auto clearLodFeedback = graph.AddPass<LodFeedbackClearPassData>(
      makePassName(" GPU LOD Feedback Clear"), computeQueue,
      [&](LodFeedbackClearPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hFeedback = builder.WriteBuffer(builder.ImportBuffer(makeResourceName(" GPU LOD Feedback"), m_pLodFeedbackBuffers[uiResourceSlot], m_pLodFeedbackBuffers[uiResourceSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
      },
      [](const LodFeedbackClearPassData& data, xiiRenderGraphPassContext& context) {
        xiiGALCommandList& cmd = context.GetCommandList();
        cmd.SetPipelineState(data.m_pPipeline.Borrow());
        cmd.ResolveAndSetUnorderedAccessBufferView("g_LodFeedback", context.GetBuffer(data.m_hFeedback)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
        cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).AssertSuccess();
        cmd.DispatchCompute({(data.m_uiEntryCount + 63U) / 64U, 1U, 1U});
      });
    clearLodFeedback.first->m_pPipeline   = m_pLodFeedbackClearPipeline;
    clearLodFeedback.first->m_uiEntryCount = m_Description.m_uiLodFeedbackCapacity + 1U;
    hLodFeedback = clearLodFeedback.first->m_hFeedback;
    m_LodFeedbackClearFrames[uiResourceSlot] = uiFrameIndex;
  }

  const bool bUseMeshletHiZ = hHiZ.IsValid();
  auto meshletCull = graph.AddPass<MeshletCullPassData>(
    makePassName(bUseMeshletHiZ ? " GPU Meshlet Hi-Z Culling" : " GPU Meshlet Culling"), computeQueue,
    [&, hLodFeedback](MeshletCullPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hScene                = builder.ReadBuffer(instanceCull.first->m_hScene, xiiGALResourceStateFlags::ShaderResource);
      data.m_hView                 = builder.ReadBuffer(instanceCull.first->m_hView, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometry             = builder.ReadBuffer(instanceCull.first->m_hGeometry, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshlets             = builder.ReadBuffer(geometry.m_hMeshletMetadata, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstances     = builder.ReadBuffer(hVisibleInstances, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleInstanceCount = builder.ReadBuffer(hVisibleInstanceCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hDispatchArguments    = builder.ReadBuffer(meshletDispatchBuild.first->m_hDispatchArguments, xiiGALResourceStateFlags::IndirectArgument);
      data.m_hPreviousLodHistory   = builder.ReadBuffer(builder.ImportBuffer(makeResourceName(" GPU Previous LOD History"), m_pLodHistoryBuffers[uiPreviousResourceSlot], m_pLodHistoryBuffers[uiPreviousResourceSlot]->GetResourceState()), xiiGALResourceStateFlags::ShaderResource);
      data.m_hCurrentLodHistory    = builder.WriteBuffer(builder.ImportBuffer(makeResourceName(" GPU Current LOD History"), m_pLodHistoryBuffers[uiResourceSlot], m_pLodHistoryBuffers[uiResourceSlot]->GetResourceState()), xiiGALResourceStateFlags::UnorderedAccess);
      const xiiRenderGraphBufferHandle hFeedback = hLodFeedback.IsValid() ? hLodFeedback : builder.ImportBuffer(makeResourceName(" GPU LOD Feedback"), m_pLodFeedbackBuffers[uiResourceSlot], m_pLodFeedbackBuffers[uiResourceSlot]->GetResourceState());
      data.m_hLodFeedback          = builder.ReadWriteBuffer(hFeedback, xiiGALResourceStateFlags::UnorderedAccess);
      if (bUseMeshletHiZ)
        data.m_hHiZ = builder.ReadTexture(hHiZ, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshlets      = builder.WriteBuffer(makeResourceName(" GPU Visible Meshlets"), meshletDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hVisibleMeshletCount  = builder.ReadWriteBuffer(reset.first->m_hVisibleMeshletCount, xiiGALResourceStateFlags::UnorderedAccess);
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
      cmd.ResolveAndSetShaderResourceBufferView("g_PreviousLodHistory", context.GetBuffer(data.m_hPreviousLodHistory)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      if (data.m_hHiZ.IsValid())
        cmd.ResolveAndSetShaderResourceTextureView("g_HiZ", context.GetTexture(data.m_hHiZ)->GetDefaultView(xiiGALTextureViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_CurrentLodHistory", context.GetBuffer(data.m_hCurrentLodHistory)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_LodFeedback", context.GetBuffer(data.m_hLodFeedback)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchComputeIndirect({context.GetBuffer(data.m_hDispatchArguments), xiiGALStateTransitionMode::None});
    });
  meshletCull.first->m_pPipeline = bUseMeshletHiZ ? m_pMeshletHiZCullPipeline : m_pMeshletCullPipeline;

  if (description.m_bEnableLodFeedback)
  {
    auto lodFeedbackReadback = graph.AddPass<LodFeedbackReadbackPassData>(
      makePassName(" GPU LOD Feedback Readback"), xiiGALCommandQueueFlags::Transfer,
      [&, hFeedback = meshletCull.first->m_hLodFeedback](LodFeedbackReadbackPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_hFeedback = builder.ReadBuffer(hFeedback, xiiGALResourceStateFlags::CopySource);
        data.m_hReadback = builder.WriteBuffer(builder.ImportBuffer(makeResourceName(" GPU LOD Feedback Readback"), m_pLodFeedbackReadbackBuffers[uiResourceSlot], m_pLodFeedbackReadbackBuffers[uiResourceSlot]->GetResourceState()), xiiGALResourceStateFlags::CopyDestination);
        builder.ExportBuffer(data.m_hFeedback, xiiGALResourceStateFlags::UnorderedAccess);
        builder.ExportBuffer(data.m_hReadback, xiiGALResourceStateFlags::CopyDestination);
        builder.SetPassSideEffects(true);
        builder.SetPassAllowMerge(false);
      },
      [this](const LodFeedbackReadbackPassData& data, xiiRenderGraphPassContext& context) {
        context.GetCommandList().CopyBuffer(context.GetBuffer(data.m_hFeedback), context.GetBuffer(data.m_hReadback));
        m_LodFeedbackReadbackFrames[data.m_uiResourceSlot] = data.m_uiFrameIndex;
      },
      true);
    lodFeedbackReadback.first->m_uiResourceSlot = uiResourceSlot;
    lodFeedbackReadback.first->m_uiFrameIndex   = uiFrameIndex;
  }

  auto commandBuild = graph.AddPass<CommandBuildPassData>(
    makePassName(" GPU Indirect Command Build"), computeQueue,
    [&](CommandBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hVisibleMeshlets     = builder.ReadBuffer(meshletCull.first->m_hVisibleMeshlets, xiiGALResourceStateFlags::ShaderResource);
      data.m_hVisibleMeshletCount = builder.ReadBuffer(meshletCull.first->m_hVisibleMeshletCount, xiiGALResourceStateFlags::ShaderResource);
      data.m_hCommands            = builder.WriteBuffer(makeResourceName(" GPU Mesh Indirect Commands"), commandDesc, xiiGALResourceStateFlags::UnorderedAccess);
      data.m_hCommandCount        = builder.ReadWriteBuffer(reset.first->m_hDrawCount, xiiGALResourceStateFlags::UnorderedAccess);

      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize         = sizeof(xiiGpuSceneCommandBuildConstants);
      constantsDescription.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants                     = builder.WriteBuffer(makeResourceName(" GPU Mesh Command Constants"), constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
    },
    [](const CommandBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& cmd        = context.GetCommandList();
      xiiGALBuffer*      pConstants = context.GetBuffer(data.m_hConstants);
      {
        xiiGALMapHelper<xiiGpuSceneCommandBuildConstants> constants(cmd, pConstants, xiiGALMapType::Write, xiiGALMapFlags::Discard);
        constants->MaxMeshGroupCountX     = data.m_uiMaxGroupCountX;
        constants->MaxMeshGroupCountY     = data.m_uiMaxGroupCountY;
        constants->MaxMeshGroupTotalCount = data.m_uiMaxGroupTotalCount;
        constants->Padding                = 0U;
      }
      cmd.SetPipelineState(data.m_pPipeline.Borrow());
      cmd.ResolveAndSetConstantBuffer("xiiGpuSceneCommandBuildConstants", pConstants, xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshlets", context.GetBuffer(data.m_hVisibleMeshlets)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetShaderResourceBufferView("g_VisibleMeshletCount", context.GetBuffer(data.m_hVisibleMeshletCount)->GetDefaultView(xiiGALBufferViewType::ShaderResource), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_IndirectCommands", context.GetBuffer(data.m_hCommands)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.ResolveAndSetUnorderedAccessBufferView("g_IndirectCommandCount", context.GetBuffer(data.m_hCommandCount)->GetDefaultView(xiiGALBufferViewType::UnorderedAccess), xiiGALShaderType::Compute);
      cmd.CommitShaderResources(xiiGALStateTransitionMode::Transition).IgnoreResult();
      cmd.DispatchCompute({1U, 1U, 1U});
    });
  commandBuild.first->m_pPipeline            = m_pCommandBuildPipeline;
  commandBuild.first->m_uiMaxGroupCountX     = m_uiMeshDispatchGroupCountX;
  commandBuild.first->m_uiMaxGroupCountY     = m_uiMeshDispatchGroupCountY;
  commandBuild.first->m_uiMaxGroupTotalCount = m_uiMeshDispatchGroupTotalCount;

  xiiGpuVisibilityOutputs outputs;
  outputs.m_hSceneInstances       = instanceCull.first->m_hScene;
  outputs.m_hVisibleInstances     = hVisibleInstances;
  outputs.m_hVisibleInstanceCount = hVisibleInstanceCount;
  outputs.m_hVisibleMeshlets      = meshletCull.first->m_hVisibleMeshlets;
  outputs.m_hVisibleMeshletCount  = meshletCull.first->m_hVisibleMeshletCount;
  outputs.m_hIndirectCommands     = commandBuild.first->m_hCommands;
  outputs.m_hIndirectCommandCount = commandBuild.first->m_hCommandCount;
  return outputs;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuVisibilitySystem);
