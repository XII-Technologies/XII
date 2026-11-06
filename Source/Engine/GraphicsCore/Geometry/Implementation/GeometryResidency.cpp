/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/Stats.h>

#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <algorithm>

class xiiGeometryResidencyManager::State : public xiiRefCounted
{
public:
  struct GeometryHash
  {
    static xiiUInt32 Hash(xiiGeometryHandle handle) { return xiiHashHelper<xiiUInt32>::Hash(handle.GetInternalID().m_Data); }
    static bool      Equal(xiiGeometryHandle a, xiiGeometryHandle b) { return a == b; }
  };

  struct MeshGeometryEntry
  {
    xiiMeshResourceHandle m_hMesh;
    xiiUInt32             m_uiMeshRevision   = 0U;
    xiiUInt32             m_uiBufferRevision = 0U;
    xiiUInt32             m_uiReferenceCount = 0U;
  };

  xiiUInt64 m_uiCoverageCacheHits = 0U;
  xiiUInt64 m_uiCoverageQueries = 0U, m_uiCoverageIndexBuilds = 0U, m_uiCoverageMeshlets = 0U;

  xiiDynamicArray<xiiUInt32> m_ActiveSlots, m_StreamingQueue, m_RetireQueue;
  xiiUInt64                  m_uiStreamingFrame      = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt64                  m_uiInitialUploadBudget = 0U, m_uiRemainingUploadBudget = 0U;
  xiiUInt64                  m_uiEpoch                       = 0U;
  xiiUInt32                  m_uiArenaMeshlets               = 0U;
  xiiUInt32                  m_uiMaximumResidentMeshletCount = 0U;
  bool                       m_bMaximumDirty                 = false;
  xiiDynamicArray<xiiUInt32> m_EvictionCandidates;
  xiiUInt32                  m_uiNextEviction  = 0U;
  xiiUInt64                  m_uiEvictionFrame = xiiMath::MaxValue<xiiUInt64>(), m_uiEvictionCompletedFrame = 0U;
  xiiGeometryResidencyStats  m_StreamingStats;

  void ClearGpuState()
  {
    for (Slot& slot : m_Slots)
      xiiGeometryResidencyManager::ReleaseBindlessResources(slot, xiiMath::Max(slot.m_uiLastUsedFrame, slot.m_uiRetireFrame));

    m_ActiveSlots.Clear();
    m_StreamingQueue.Clear();
    m_RetireQueue.Clear();
    m_EvictionCandidates.Clear();
    m_uiEvictionFrame       = xiiMath::MaxValue<xiiUInt64>();
    m_uiNextEviction        = 0U;
    m_uiStreamingFrame      = xiiMath::MaxValue<xiiUInt64>();
    m_uiInitialUploadBudget = m_uiRemainingUploadBudget = 0U;
    ++m_uiEpoch;
    m_uiArenaMeshlets               = 0U;
    m_StreamingStats                = {};
    m_uiMaximumResidentMeshletCount = 0U;
    m_bMaximumDirty                 = false;
    m_SetupUploads.Clear();
    m_uiUploadFrame = xiiMath::MaxValue<xiiUInt64>();
    m_pMetadataBuffer.Clear();
    m_pMeshletMetadataBuffer.Clear();
    m_GeometryIds.Clear();
    m_Slots.Clear();
    m_FreeMeshletRanges.Clear();
    m_PendingMeshletUploads.Clear();
    m_MeshGeometries.Clear();
    m_CurrentMeshGeometries.Clear();
    m_uiFramesInFlight    = 0U;
    m_uiAllFrameMask      = 0U;
    m_uiBudgetBytes       = 0U;
    m_uiResidentBytes     = 0U;
    m_uiLastUploadedBytes = 0U;
    m_bInitialized        = false;
  }

  xiiIdTable<xiiGeometryId, xiiUInt32>                             m_GeometryIds;
  xiiDynamicArray<Slot, xiiAlignedAllocatorWrapper>                m_Slots;
  xiiSharedPtr<xiiGALBuffer>                                       m_pMetadataBuffer;
  xiiSharedPtr<xiiGALBuffer>                                       m_pMeshletMetadataBuffer;
  xiiDynamicArray<FreeRange>                                       m_FreeMeshletRanges;
  xiiDynamicArray<xiiSharedPtr<MeshletUpload>>                     m_PendingMeshletUploads;
  xiiHashTable<xiiGeometryHandle, MeshGeometryEntry, GeometryHash> m_MeshGeometries;
  xiiHashTable<xiiMeshResourceHandle, xiiGeometryHandle>           m_CurrentMeshGeometries;
  xiiUInt32                                                        m_uiFramesInFlight = 0U;
  xiiUInt64                                                        m_uiAllFrameMask   = 0U;
  xiiUInt64                                                        m_uiBudgetBytes    = 0U;
  xiiUInt64                                                        m_uiResidentBytes  = 0U;
  xiiHashTable<xiiUInt64, UploadHandles>                           m_SetupUploads;
  xiiUInt64                                                        m_uiUploadFrame       = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt64                                                        m_uiLastUploadedBytes = 0U;

  xiiGeometryResidencyDescription m_Configuration;
  xiiMutex                        m_InitializationMutex;
  bool                            m_bEngineStarted = false;
  bool                            m_bInitialized   = false;
};

xiiSharedPtr<xiiGeometryResidencyManager::State> xiiGeometryResidencyManager::s_pState;

namespace
{
  constexpr float s_fLodReferenceViewportHeight = 1080.0f;

  xiiResult BuildMeshGeometryDescription(const xiiMeshResource& mesh, xiiGeometryDescription& out_description)
  {
    const xiiMeshBufferResourceHandle& hMeshBuffer = mesh.GetMeshBuffer();
    if (!hMeshBuffer.IsValid())
      return XII_FAILURE;

    const xiiArrayPtr<const xiiMeshLOD> lods = mesh.GetLODs();
    if (lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
      return XII_FAILURE;

    out_description                       = {};
    out_description.m_uiStreamingPriority = mesh.GetDescriptor().m_uiStreamingGroup;
    out_description.m_bPinned             = !mesh.GetUsageFlags().IsSet(xiiMeshResourceUsageFlags::Streaming);

    if (lods.IsEmpty())
    {
      xiiGeometryLodSource& source = out_description.m_Lods.ExpandAndGetRef();
      source.m_hMeshBuffer         = hMeshBuffer;
      return XII_SUCCESS;
    }

    const xiiBoundingBoxSphere& bounds = mesh.GetBounds();
    for (const xiiMeshLOD& meshLod : lods)
    {
      xiiGeometryLodSource& source = out_description.m_Lods.ExpandAndGetRef();
      source.m_hMeshBuffer         = hMeshBuffer;
      // Source count zero means "to the end". An authored empty LOD must therefore
      // start at the source end, rather than accidentally borrowing the next LOD's meshlets.
      source.m_uiFirstMeshlet = meshLod.m_uiMeshletCount == 0 ? mesh.GetMeshletCount() : meshLod.m_uiFirstMeshlet;
      source.m_uiMeshletCount = meshLod.m_uiMeshletCount;

      if (mesh.GetLodMode() == xiiMeshLodSelectionMode::Distance && meshLod.m_fMaxDistance > 0.0f && bounds.IsValid())
      {
        source.m_fMinimumScreenCoverage = bounds.m_fSphereRadius * s_fLodReferenceViewportHeight / meshLod.m_fMaxDistance;
      }
      else
      {
        // xiiMeshLOD stores normalized screen coverage. GPU visibility works in projected pixels,
        // so normalize authored thresholds against a stable reference height. Actual viewport
        // height remains part of the GPU coverage calculation and preserves resolution scaling.
        source.m_fMinimumScreenCoverage = meshLod.m_fScreenSize * s_fLodReferenceViewportHeight;
      }
    }

    return XII_SUCCESS;
  }
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GeometryResidencyManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGeometryResidencyManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGeometryResidencyManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGeometryResidencyManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGeometryResidencyManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGeometryResidencyState, 1)
  XII_ENUM_CONSTANTS(xiiGeometryResidencyState::Unloaded, xiiGeometryResidencyState::Requested, xiiGeometryResidencyState::Loading)
  XII_ENUM_CONSTANTS(xiiGeometryResidencyState::Resident, xiiGeometryResidencyState::EvictPending, xiiGeometryResidencyState::Failed)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryHandle>)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryLodSource, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryLodSource>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("MeshBuffer", GetMeshBufferResourceId, SetMeshBufferResourceId),
      XII_MEMBER_PROPERTY("MinimumScreenCoverage", m_fMinimumScreenCoverage),
      XII_MEMBER_PROPERTY("FirstMeshlet", m_uiFirstMeshlet),
      XII_MEMBER_PROPERTY("MeshletCount", m_uiMeshletCount),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

xiiString xiiGeometryLodSource::GetMeshBufferResourceId() const
{
  return m_hMeshBuffer.IsValid() ? xiiString(m_hMeshBuffer.GetResourceID()) : xiiString();
}

void xiiGeometryLodSource::SetMeshBufferResourceId(xiiString sResourceId)
{
  m_hMeshBuffer = sResourceId.IsEmpty() ? xiiMeshBufferResourceHandle() : xiiResourceManager::LoadResource<xiiMeshBufferResource>(sResourceId);
}

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ARRAY_MEMBER_PROPERTY("Lods", m_Lods),
      XII_MEMBER_PROPERTY("StreamingPriority", m_uiStreamingPriority),
      XII_MEMBER_PROPERTY("Pinned", m_bPinned),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuGeometryLod, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuGeometryLod>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("VertexBufferIndex", m_uiVertexBufferIndex),
      XII_MEMBER_PROPERTY("IndexBufferIndex", m_uiIndexBufferIndex),
      XII_MEMBER_PROPERTY("MeshletBufferIndex", m_uiMeshletBufferIndex),
      XII_MEMBER_PROPERTY("MeshletRemapBufferIndex", m_uiMeshletRemapBufferIndex),
      XII_MEMBER_PROPERTY("MeshletPrimitiveBufferIndex", m_uiMeshletPrimitiveBufferIndex),
      XII_MEMBER_PROPERTY("VertexCount", m_uiVertexCount),
      XII_MEMBER_PROPERTY("IndexCount", m_uiIndexCount),
      XII_MEMBER_PROPERTY("MeshletCount", m_uiMeshletCount),
      XII_MEMBER_PROPERTY("MinimumScreenCoverage", m_fMinimumScreenCoverage),
      XII_MEMBER_PROPERTY("IndexType", m_uiIndexType),
      XII_MEMBER_PROPERTY("MeshletMetadataOffset", m_uiMeshletMetadataOffset),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuGeometryRecord, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuGeometryRecord>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("BoundsCenterRadius", m_BoundsCenterRadius),
      XII_MEMBER_PROPERTY("BoundsExtents", m_BoundsExtents),
      XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("Lods", GetReflectedLodCount, GetReflectedLod),
      XII_MEMBER_PROPERTY("LodCount", m_uiLodCount),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
      XII_MEMBER_PROPERTY("ResidentLodMask", m_uiResidentLodMask),
      XII_MEMBER_PROPERTY("Flags", m_uiFlags),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryResidencyStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryResidencyStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("GeometryCount", m_uiGeometryCount),
      XII_MEMBER_PROPERTY("ResidentGeometryCount", m_uiResidentGeometryCount),
      XII_MEMBER_PROPERTY("PendingGeometryCount", m_uiPendingGeometryCount),
      XII_MEMBER_PROPERTY("ResidentBytes", m_uiResidentBytes),
      XII_MEMBER_PROPERTY("BudgetBytes", m_uiBudgetBytes),
      XII_MEMBER_PROPERTY("UploadedBytes", m_uiUploadedBytes),
      XII_MEMBER_PROPERTY("AdmittedBytes", m_uiAdmittedBytes),
      XII_MEMBER_PROPERTY("RemainingUploadBytes", m_uiRemainingUploadBytes),
      XII_MEMBER_PROPERTY("PreparedMeshlets", m_uiPreparedMeshlets),
      XII_MEMBER_PROPERTY("Examined", m_uiExamined),
      XII_MEMBER_PROPERTY("Admitted", m_uiAdmitted),
      XII_MEMBER_PROPERTY("BudgetDeferred", m_uiBudgetDeferred),
      XII_MEMBER_PROPERTY("NotReady", m_uiNotReady),
      XII_MEMBER_PROPERTY("ArenaDeferred", m_uiArenaDeferred),
      XII_MEMBER_PROPERTY("MemoryDeferred", m_uiMemoryDeferred),
      XII_MEMBER_PROPERTY("Failed", m_uiFailed),
      XII_MEMBER_PROPERTY("ArenaMeshlets", m_uiArenaMeshlets),
      XII_MEMBER_PROPERTY("CoverageQueries", m_uiCoverageQueries),
      XII_MEMBER_PROPERTY("CoverageCacheHits", m_uiCoverageCacheHits),
      XII_MEMBER_PROPERTY("CoverageIndexBuilds", m_uiCoverageIndexBuilds),
      XII_MEMBER_PROPERTY("CoverageMeshlets", m_uiCoverageMeshlets),
      XII_MEMBER_PROPERTY("StreamingMilliseconds", m_fStreamingMilliseconds),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGeometryResidencyDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGeometryResidencyDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MaxGeometries", m_uiMaxGeometries)->AddAttributes(new xiiDefaultValueAttribute(65536U), new xiiClampValueAttribute(1U, 1048576U)),
      XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(1U, 64U)),
      XII_MEMBER_PROPERTY("BudgetBytes", m_uiBudgetBytes)->AddAttributes(new xiiDefaultValueAttribute(512ULL * 1024ULL * 1024ULL)),
      XII_MEMBER_PROPERTY("UploadBudgetPerFrameBytes", m_uiUploadBudgetPerFrameBytes)->AddAttributes(new xiiDefaultValueAttribute(32ULL * 1024ULL * 1024ULL)),
      XII_MEMBER_PROPERTY("MaxMeshlets", m_uiMaxMeshlets)->AddAttributes(new xiiDefaultValueAttribute(1024U * 1024U), new xiiClampValueAttribute(1U, 64U * 1024U * 1024U)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

void xiiGeometryResidencyManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Geometry residency manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

xiiResult xiiGeometryResidencyManager::Configure(const xiiGeometryResidencyDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Geometry residency manager is not started.");
  if (s_pState == nullptr || description.m_uiMaxGeometries == 0U || description.m_uiFramesInFlight == 0U || description.m_uiFramesInFlight > 64U || description.m_uiMaxMeshlets == 0U)
    return XII_FAILURE;

  XII_LOCK(s_pState->m_InitializationMutex);
  if (s_pState->m_bInitialized && GetStats().m_uiGeometryCount != 0U)
  {
    XII_ASSERT_DEV(false, "Geometry residency cannot be reconfigured while geometry handles are active.");
    return XII_FAILURE;
  }

  const bool bSameConfiguration =
    s_pState->m_Configuration.m_uiMaxGeometries == description.m_uiMaxGeometries &&
    s_pState->m_Configuration.m_uiFramesInFlight == description.m_uiFramesInFlight &&
    s_pState->m_Configuration.m_uiBudgetBytes == description.m_uiBudgetBytes &&
    s_pState->m_Configuration.m_uiUploadBudgetPerFrameBytes == description.m_uiUploadBudgetPerFrameBytes &&
    s_pState->m_Configuration.m_uiMaxMeshlets == description.m_uiMaxMeshlets;
  if (bSameConfiguration && s_pState->m_bInitialized)
    return XII_SUCCESS;

  s_pState->m_Configuration = description;
  if (!s_pState->m_bInitialized)
    return XII_SUCCESS;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  return pDevice != nullptr ? Initialize(pDevice.Borrow(), s_pState->m_Configuration) : XII_FAILURE;
}

const xiiGeometryResidencyDescription& xiiGeometryResidencyManager::GetConfiguration()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Geometry residency manager is not started.");
  static const xiiGeometryResidencyDescription s_DefaultConfiguration;
  return s_pState != nullptr ? s_pState->m_Configuration : s_DefaultConfiguration;
}

bool xiiGeometryResidencyManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGeometryResidencyManager::IsInitialized()
{
  if (!s_pState)
    return false;
  XII_LOCK(s_pState->m_InitializationMutex);
  return s_pState->m_bInitialized;
}

xiiResult xiiGeometryResidencyManager::EnsureInitialized()
{
  if (!s_pState)
    return XII_FAILURE;
  // Different worlds can request residency before acquiring their render-frame scope.
  // Publish one complete allocation and prevent concurrent ClearGpuState/SetCount calls.
  XII_LOCK(s_pState->m_InitializationMutex);
  if (!s_pState->m_bEngineStarted)
    return XII_FAILURE;
  if (s_pState->m_bInitialized)
    return XII_SUCCESS;
  const auto device = xiiGALDevice::GetDefaultDevice();
  return device ? Initialize(device.Borrow(), s_pState->m_Configuration) : XII_FAILURE;
}

xiiResult xiiGeometryResidencyManager::Initialize(xiiGALDevice* pDevice, const xiiGeometryResidencyDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Geometry residency manager is not started.");
  if (s_pState == nullptr)
    return XII_FAILURE;

  s_pState->ClearGpuState();
  const xiiUInt32 uiMaxGeometries  = description.m_uiMaxGeometries;
  const xiiUInt32 uiFramesInFlight = description.m_uiFramesInFlight;
  const xiiUInt64 uiBudgetBytes    = description.m_uiBudgetBytes;
  const xiiUInt32 uiMaxMeshlets    = description.m_uiMaxMeshlets;
  if (pDevice == nullptr || uiMaxGeometries == 0U || uiFramesInFlight == 0U || uiFramesInFlight > 64U || uiMaxMeshlets == 0U || xiiUInt64(uiMaxMeshlets) * sizeof(xiiMeshlet) > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  const xiiUInt64 uiSize = static_cast<xiiUInt64>(sizeof(xiiGpuGeometryRecord)) * uiMaxGeometries * uiFramesInFlight;
  if (uiSize > xiiMath::MaxValue<xiiUInt32>())
    return XII_FAILURE;

  xiiGALBufferCreationDescription desc;
  desc.m_uiSize               = static_cast<xiiUInt32>(uiSize);
  desc.m_uiElementByteStride  = sizeof(xiiGpuGeometryRecord);
  desc.m_BindFlags            = xiiGALBindFlags::ShaderResource;
  desc.m_Mode                 = xiiGALBufferMode::Structured;
  desc.m_Usage                = xiiGALResourceUsage::Mutable;
  s_pState->m_pMetadataBuffer = pDevice->CreateBuffer(desc);
  if (s_pState->m_pMetadataBuffer == nullptr)
    return XII_FAILURE;
  s_pState->m_pMetadataBuffer->SetDebugName("GPU Geometry Metadata");

  desc.m_uiSize                      = uiMaxMeshlets * sizeof(xiiMeshlet);
  desc.m_uiElementByteStride         = sizeof(xiiMeshlet);
  s_pState->m_pMeshletMetadataBuffer = pDevice->CreateBuffer(desc);
  if (s_pState->m_pMeshletMetadataBuffer == nullptr)
  {
    s_pState->m_pMetadataBuffer.Clear();
    return XII_FAILURE;
  }
  s_pState->m_pMeshletMetadataBuffer->SetDebugName("GPU Meshlet Metadata Arena");
  s_pState->m_FreeMeshletRanges.PushBack({0U, uiMaxMeshlets});

  s_pState->m_GeometryIds.Reserve(uiMaxGeometries);
  s_pState->m_Slots.SetCount(uiMaxGeometries);
  s_pState->m_uiFramesInFlight = uiFramesInFlight;
  s_pState->m_uiAllFrameMask   = uiFramesInFlight == 64U ? xiiMath::MaxValue<xiiUInt64>() : (xiiUInt64(1) << uiFramesInFlight) - 1U;
  s_pState->m_uiBudgetBytes    = uiBudgetBytes;
  s_pState->m_bInitialized     = true;
  return XII_SUCCESS;
}

void xiiGeometryResidencyManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

void xiiGeometryResidencyManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede geometry residency engine startup.");
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_InitializationMutex);
  s_pState->m_bEngineStarted = true;
}

void xiiGeometryResidencyManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_InitializationMutex);
  s_pState->ClearGpuState();
  s_pState->m_bEngineStarted = false;
}

xiiGeometryHandle xiiGeometryResidencyManager::RegisterGeometry(const xiiGeometryDescription& description)
{
  if (EnsureInitialized().Failed())
    return {};
  if (!IsInitialized() || s_pState->m_pMetadataBuffer == nullptr || s_pState->m_GeometryIds.GetCount() >= s_pState->m_Slots.GetCount() || description.m_Lods.IsEmpty() || description.m_Lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
    return {};

  const xiiGeometryId id      = s_pState->m_GeometryIds.Insert(0U);
  const xiiUInt32     uiIndex = id.m_InstanceIndex;
  s_pState->m_GeometryIds[id] = uiIndex;
  Slot& slot                  = s_pState->m_Slots[uiIndex];
  slot.m_bQueued              = false;
  slot.m_uiAdmissionFrame     = xiiMath::MaxValue<xiiUInt64>();
  slot.m_uiActiveIndex        = s_pState->m_ActiveSlots.GetCount();
  s_pState->m_ActiveSlots.PushBack(uiIndex);
  slot.m_pCoverage.Clear();
  slot.m_Description              = description;
  slot.m_GpuRecord                = {};
  slot.m_GpuRecord.m_uiGeneration = id.m_Generation;
  slot.m_GpuRecord.m_uiLodCount   = description.m_Lods.GetCount();
  for (xiiUInt32 uiLod = 0U; uiLod < description.m_Lods.GetCount(); ++uiLod)
    slot.m_GpuRecord.m_Lods[uiLod].m_fMinimumScreenCoverage = description.m_Lods[uiLod].m_fMinimumScreenCoverage;
  slot.m_State                = xiiGeometryResidencyState::Unloaded;
  slot.m_bAllocated           = true;
  slot.m_uiDirtyFrameMask     = s_pState->m_uiAllFrameMask;
  slot.m_uiRequestedLod       = description.m_Lods.GetCount() - 1U;
  slot.m_uiVisibilityPriority = 0U;
  slot.m_uiFirstRequestFrame  = xiiMath::MaxValue<xiiUInt64>();
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaOffset, XII_ARRAY_SIZE(slot.m_uiMeshletArenaOffset));
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaCount, XII_ARRAY_SIZE(slot.m_uiMeshletArenaCount));

  return xiiGeometryHandle(id);
}

void xiiGeometryResidencyManager::UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle))
    return;
  Slot& slot = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex];
  if (slot.m_State != xiiGeometryResidencyState::EvictPending)
    s_pState->m_RetireQueue.PushBack(handle.GetInternalID().m_InstanceIndex);
  slot.m_State         = xiiGeometryResidencyState::EvictPending;
  slot.m_uiRetireFrame = xiiMath::Max(slot.m_uiRetireFrame, xiiMath::Max(uiFrameIndex, slot.m_uiLastUsedFrame));
}

xiiGeometryHandle xiiGeometryResidencyManager::AcquireMeshGeometry(const xiiMeshResourceHandle& hMesh)
{
  if (!hMesh.IsValid() || EnsureInitialized().Failed())
    return {};

  xiiResourceLock<xiiMeshResource> mesh(hMesh, xiiResourceAcquireMode::PointerOnly);
  if (mesh->GetLoadingState() != xiiResourceState::Loaded || !mesh->GetMeshBuffer().IsValid())
    return {};
  xiiResourceLock<xiiMeshBufferResource> buffer(mesh->GetMeshBuffer(), xiiResourceAcquireMode::PointerOnly);
  if (buffer->GetLoadingState() != xiiResourceState::Loaded)
    return {};
  const auto meshRevision   = mesh->GetCurrentResourceChangeCounter();
  const auto bufferRevision = buffer->GetCurrentResourceChangeCounter();

  xiiGeometryHandle         cached;
  State::MeshGeometryEntry* pCachedEntry = nullptr;
  if (s_pState->m_CurrentMeshGeometries.TryGetValue(hMesh, cached) && s_pState->m_MeshGeometries.TryGetValue(cached, pCachedEntry))
  {
    if (IsValid(cached) && GetState(cached) != xiiGeometryResidencyState::EvictPending && pCachedEntry->m_uiMeshRevision == meshRevision && pCachedEntry->m_uiBufferRevision == bufferRevision)
    {
      XII_ASSERT_DEV(pCachedEntry->m_uiReferenceCount < xiiMath::MaxValue<xiiUInt32>(), "Mesh geometry reference count overflow.");
      if (pCachedEntry->m_uiReferenceCount == xiiMath::MaxValue<xiiUInt32>())
        return {};

      ++pCachedEntry->m_uiReferenceCount;
      return cached;
    }

    // Existing users own this exact revision. Replacing the current lookup must never
    // redirect their release to the new record or overwrite GPU data still in flight.
  }

  xiiGeometryDescription description;
  if (BuildMeshGeometryDescription(*mesh.GetPointer(), description).Failed())
    return {};

  const xiiGeometryHandle hGeometry = RegisterGeometry(description);
  if (hGeometry.IsInvalidated())
    return {};

  State::MeshGeometryEntry entry;
  entry.m_hMesh            = hMesh;
  entry.m_uiMeshRevision   = meshRevision;
  entry.m_uiBufferRevision = bufferRevision;
  entry.m_uiReferenceCount = 1U;
  s_pState->m_MeshGeometries.Insert(hGeometry, entry);
  s_pState->m_CurrentMeshGeometries.Insert(hMesh, hGeometry);

  // Start with the coarsest authored LOD. Fine LODs are requested on demand by visibility users.
  RequestResidency(hGeometry, description.m_Lods.GetCount() - 1U, 0U);
  return hGeometry;
}

void xiiGeometryResidencyManager::ReleaseMeshGeometry(xiiGeometryHandle hGeometry, xiiUInt64 uiFrameIndex)
{
  if (s_pState == nullptr || hGeometry.IsInvalidated())
    return;

  State::MeshGeometryEntry* pEntry = nullptr;
  if (!s_pState->m_MeshGeometries.TryGetValue(hGeometry, pEntry) || pEntry->m_uiReferenceCount == 0U)
    return;

  --pEntry->m_uiReferenceCount;
  if (pEntry->m_uiReferenceCount != 0U)
    return;

  xiiGeometryHandle current;
  if (s_pState->m_CurrentMeshGeometries.TryGetValue(pEntry->m_hMesh, current) && current == hGeometry)
    s_pState->m_CurrentMeshGeometries.Remove(pEntry->m_hMesh);
  s_pState->m_MeshGeometries.Remove(hGeometry);
  UnregisterGeometry(hGeometry, uiFrameIndex);
}

void xiiGeometryResidencyManager::RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex, xiiUInt32 uiVisibilityPriority)
{
  if (!IsValid(handle))
    return;
  Slot&           slot           = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex];
  const xiiUInt32 uiRequestedLod = xiiMath::Min(uiMinimumLod, slot.m_Description.m_Lods.GetCount() - 1U);
  slot.m_uiLastUsedFrame         = xiiMath::Max(slot.m_uiLastUsedFrame, uiFrameIndex);

  // Once streaming has started, later shadow or reflection requests must not replace a finer
  // main/sensor request. A fresh unloaded slot still accepts its initial coarse request.
  if (slot.m_State == xiiGeometryResidencyState::Unloaded || slot.m_State == xiiGeometryResidencyState::Failed)
    slot.m_uiRequestedLod = uiRequestedLod;
  else
    slot.m_uiRequestedLod = xiiMath::Min(slot.m_uiRequestedLod, uiRequestedLod);

  bool bRequiresStreaming = false;
  for (xiiUInt32 i = slot.m_uiRequestedLod; i < slot.m_Description.m_Lods.GetCount(); ++i)
  {
    if ((slot.m_GpuRecord.m_uiResidentLodMask & XII_BIT(i)) == 0U)
    {
      bRequiresStreaming = true;
      xiiResourceManager::PreloadResource(slot.m_Description.m_Lods[i].m_hMeshBuffer);
    }
  }

  // A resident geometry may receive a finer request later. Keep the existing LODs live while
  // loading the missing range; BuildResidentRecord commits the additions transactionally.
  if (bRequiresStreaming && slot.m_State != xiiGeometryResidencyState::EvictPending)
  {
    if (slot.m_State != xiiGeometryResidencyState::Requested && slot.m_State != xiiGeometryResidencyState::Loading)
      slot.m_uiFirstRequestFrame = uiFrameIndex;
    slot.m_uiVisibilityPriority = xiiMath::Max(slot.m_uiVisibilityPriority, uiVisibilityPriority);
    slot.m_State                = xiiGeometryResidencyState::Requested;
    if (!slot.m_bQueued)
    {
      slot.m_bQueued = true;
      s_pState->m_StreamingQueue.PushBack(handle.GetInternalID().m_InstanceIndex);
    }
  }
}

void xiiGeometryResidencyManager::Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (IsValid(handle))
  {
    auto& lastUsed = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_uiLastUsedFrame;
    lastUsed       = xiiMath::Max(lastUsed, uiFrameIndex);
  }
}

xiiGeometryResidencyManager::AdmissionResult xiiGeometryResidencyManager::BuildResidentRecord(Slot& slot, xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  XII_PROFILE_SCOPE("Geometry admission and commit");

  // Planning borrows at most eight source revisions. Keep the leases until commit
  // completes so the metadata, GPU buffers and revision checks describe one snapshot.
  xiiHybridArray<xiiResourceLock<xiiMeshBufferResource>, xiiGpuGeometryRecord::s_uiMaxLods> sources;
  xiiHybridArray<xiiUInt32, xiiGpuGeometryRecord::s_uiMaxLods>                              newLods;
  xiiGpuGeometryRecord                                                                      record    = slot.m_GpuRecord;
  State::MeshGeometryEntry*                                                                 meshEntry = nullptr;
  const auto                                                                                slotIndex = static_cast<xiiUInt32>(&slot - s_pState->m_Slots.GetData());
  s_pState->m_MeshGeometries.TryGetValue(xiiGeometryHandle(xiiGeometryId(slotIndex, record.m_uiGeneration)), meshEntry);
  xiiUInt64 bytes = 0U;
  for (xiiUInt32 i = slot.m_uiRequestedLod; i < slot.m_Description.m_Lods.GetCount(); ++i)
  {
    if ((record.m_uiResidentLodMask & XII_BIT(i)) != 0U)
      continue;
    const auto& source = slot.m_Description.m_Lods[i];
    if (!source.m_hMeshBuffer.IsValid())
    {
      slot.m_State = xiiGeometryResidencyState::Failed;
      return AdmissionResult::Failed;
    }
    sources.PushBack(xiiResourceLock<xiiMeshBufferResource>(source.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly));
    const auto& mesh = sources.PeekBack();
    if (mesh->GetLoadingState() != xiiResourceState::Loaded || (meshEntry && mesh->GetCurrentResourceChangeCounter() != meshEntry->m_uiBufferRevision))
      return AdmissionResult::NotReady;
    const auto meshlets = mesh->GetMeshlets();
    if (source.m_uiFirstMeshlet > meshlets.GetCount() || source.m_uiMeshletCount > meshlets.GetCount() - source.m_uiFirstMeshlet)
    {
      slot.m_State = xiiGeometryResidencyState::Failed;
      return AdmissionResult::Failed;
    }
    auto& lod                    = record.m_Lods[i];
    lod.m_uiVertexCount          = mesh->GetVertexCount();
    lod.m_uiIndexCount           = mesh->GetIndexCount();
    lod.m_uiMeshletCount         = source.m_uiMeshletCount == 0U ? meshlets.GetCount() - source.m_uiFirstMeshlet : source.m_uiMeshletCount;
    lod.m_fMinimumScreenCoverage = source.m_fMinimumScreenCoverage;
    lod.m_uiIndexType            = mesh->GetIndexType().GetValue();
    bytes += xiiUInt64(mesh->GetVertexCount()) * mesh->GetVertexStride();
    bytes += xiiUInt64(mesh->GetIndexCount()) * (mesh->GetIndexType() == xiiGALValueType::UInt16 ? 2U : 4U);
    bytes += xiiUInt64(lod.m_uiMeshletCount) * sizeof(xiiMeshlet);
    newLods.PushBack(i);
  }
  if (bytes > s_pState->m_uiRemainingUploadBudget && (s_pState->m_StreamingStats.m_uiAdmittedBytes != 0U || s_pState->m_uiInitialUploadBudget == 0U))
    return AdmissionResult::BudgetDeferred;
  xiiUInt64 requiredMeshlets = 0U;
  for (const auto lod : newLods)
    requiredMeshlets += record.m_Lods[lod].m_uiMeshletCount;
  if (requiredMeshlets > s_pState->m_Configuration.m_uiMaxMeshlets)
    return AdmissionResult::ArenaDeferred;
  // Pinned assets may exceed the soft residency budget; ordinary streaming must
  // not evict and immediately readmit newly requested/current-frame geometry.
  if (!slot.m_Description.m_bPinned)
  {
    if (bytes > s_pState->m_uiBudgetBytes)
      return AdmissionResult::MemoryDeferred;
    EnforceBudget(uiCompletedFrame, bytes);
    if (s_pState->m_uiResidentBytes > s_pState->m_uiBudgetBytes - bytes)
      return AdmissionResult::MemoryDeferred;
  }

  // Simulate first-fit with at most eight reservations. No arena mutation or
  // bulk metadata copy occurs for a budget/capacity-deferred record.
  auto hasArenaCapacity = [&]() {
    xiiUInt32 ranges[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    for (xiiUInt32 n = 0; n < newLods.GetCount(); ++n)
    {
      const auto count = record.m_Lods[newLods[n]].m_uiMeshletCount;
      ranges[n]        = xiiInvalidIndex;
      if (count == 0U)
        continue;
      for (xiiUInt32 r = 0; r < s_pState->m_FreeMeshletRanges.GetCount(); ++r)
      {
        xiiUInt64 reserved = 0U;
        for (xiiUInt32 previous = 0; previous < n; ++previous)
          if (ranges[previous] == r)
            reserved += record.m_Lods[newLods[previous]].m_uiMeshletCount;
        if (reserved + count <= s_pState->m_FreeMeshletRanges[r].m_uiCount)
        {
          ranges[n] = r;
          break;
        }
      }
      if (ranges[n] == xiiInvalidIndex)
        return false;
    }
    return true;
  };
  while (!hasArenaCapacity())
    if (!EvictOne(uiCompletedFrame))
      return AdmissionResult::ArenaDeferred;

  xiiHybridArray<xiiSharedPtr<MeshletUpload>, xiiGpuGeometryRecord::s_uiMaxLods> uploads;
  xiiUInt32                                                                      allocated = 0U, registered = 0U;
  auto                                                                           rollback = [&]() {
    for (xiiUInt32 n = 0; n < allocated; ++n)
    {
      const auto& lod = record.m_Lods[newLods[n]];
      FreeMeshlets(lod.m_uiMeshletMetadataOffset, lod.m_uiMeshletCount);
    }
    for (xiiUInt32 n = 0; n < registered; ++n)
      for (auto& resource : slot.m_BindlessResources[newLods[n]])
      {
        xiiGALBindlessResourceTable::RetireBufferSRV(resource, 0U);
        resource = {};
      }
  };
  for (xiiUInt32 n = 0; n < newLods.GetCount(); ++n)
  {
    const auto i   = newLods[n];
    auto&      lod = record.m_Lods[i];
    if (lod.m_uiMeshletCount != 0U)
    {
      if (!AllocateMeshlets(lod.m_uiMeshletCount, lod.m_uiMeshletMetadataOffset))
      {
        rollback();
        return AdmissionResult::ArenaDeferred;
      }
      auto upload                  = XII_DEFAULT_NEW(MeshletUpload);
      upload->m_uiSlotIndex        = slotIndex;
      upload->m_uiLod              = i;
      upload->m_uiOffset           = lod.m_uiMeshletMetadataOffset * sizeof(xiiMeshlet);
      upload->m_Meshlets           = sources[n]->GetMeshlets().GetSubArray(slot.m_Description.m_Lods[i].m_uiFirstMeshlet, lod.m_uiMeshletCount);
      lod.m_uiMeshletRangesOrdered = 1U;
      xiiUInt32 previousStart      = 0U;
      xiiUInt64 previousEnd        = 0U;
      for (auto& meshlet : upload->m_Meshlets)
      {
        meshlet.m_uiLodIndex = static_cast<xiiUInt16>(i);
        const xiiUInt64 end  = xiiUInt64(meshlet.m_uiFirstPrimitive) + meshlet.m_uiPrimitiveCount;
        if (meshlet.m_uiFirstPrimitive < previousStart || end > xiiMath::MaxValue<xiiUInt32>())
          lod.m_uiMeshletRangesOrdered = 0U;
        previousStart             = meshlet.m_uiFirstPrimitive;
        previousEnd               = xiiMath::Max(previousEnd, end);
        meshlet.m_uiGpuPadding[0] = static_cast<xiiUInt32>(previousEnd);
      }
      if (lod.m_uiMeshletRangesOrdered == 0U)
      {
        // Authored section order need not follow primitive order. Arena metadata
        // has no source-index identity: remap/primitive offsets remain absolute.
        // Sort the admitted copy once, preserving source buffers and LOD spans.
        upload->m_Meshlets.Sort([](const xiiMeshlet& a, const xiiMeshlet& b) {
          if (a.m_uiFirstPrimitive != b.m_uiFirstPrimitive)
            return a.m_uiFirstPrimitive < b.m_uiFirstPrimitive;
          if (a.m_uiPrimitiveCount != b.m_uiPrimitiveCount)
            return a.m_uiPrimitiveCount < b.m_uiPrimitiveCount;
          if (a.m_uiSectionIndex != b.m_uiSectionIndex)
            return a.m_uiSectionIndex < b.m_uiSectionIndex;
          return a.m_uiPrimitiveIndexOffset < b.m_uiPrimitiveIndexOffset;
        });
        lod.m_uiMeshletRangesOrdered = 1U;
        previousEnd                  = 0U;
        for (auto& meshlet : upload->m_Meshlets)
        {
          const xiiUInt64 end = xiiUInt64(meshlet.m_uiFirstPrimitive) + meshlet.m_uiPrimitiveCount;
          if (end > xiiMath::MaxValue<xiiUInt32>())
            lod.m_uiMeshletRangesOrdered = 0U;
          previousEnd               = xiiMath::Max(previousEnd, end);
          meshlet.m_uiGpuPadding[0] = static_cast<xiiUInt32>(previousEnd);
        }
      }
      uploads.PushBack(std::move(upload));
      s_pState->m_StreamingStats.m_uiPreparedMeshlets += lod.m_uiMeshletCount;
    }
    ++allocated;
    if (!RegisterBindlessResources(slot, record, i, *sources[n].GetPointerNonConst()))
    {
      rollback();
      return AdmissionResult::Failed;
    }
    ++registered;
    record.m_uiResidentLodMask |= XII_BIT(i);
  }
  if (!newLods.IsEmpty() && (slot.m_GpuRecord.m_uiResidentLodMask == 0U || newLods[0] < xiiMath::FirstBitLow(slot.m_GpuRecord.m_uiResidentLodMask)))
  {
    const auto& bounds          = sources[0]->GetBounds();
    record.m_BoundsCenterRadius = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fSphereRadius);
    record.m_BoundsExtents      = xiiVec4(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z, 0.0f);
  }
  for (const auto i : newLods)
  {
    slot.m_uiMeshletArenaOffset[i]            = record.m_Lods[i].m_uiMeshletMetadataOffset;
    slot.m_uiMeshletArenaCount[i]             = record.m_Lods[i].m_uiMeshletCount;
    s_pState->m_uiMaximumResidentMeshletCount = xiiMath::Max(s_pState->m_uiMaximumResidentMeshletCount, record.m_Lods[i].m_uiMeshletCount);
  }
  xiiUInt32 uploadIndex = 0;
  for (const auto i : newLods)
    if (record.m_Lods[i].m_uiMeshletCount != 0U)
      slot.m_PendingUploads[i] = uploads[uploadIndex++];
  for (auto& upload : uploads)
    s_pState->m_PendingMeshletUploads.PushBack(std::move(upload));
  s_pState->m_uiRemainingUploadBudget -= xiiMath::Min(s_pState->m_uiRemainingUploadBudget, bytes);
  s_pState->m_StreamingStats.m_uiAdmittedBytes += bytes;
  slot.m_uiResidentBytes += bytes;
  s_pState->m_uiResidentBytes += bytes;
  slot.m_uiAdmissionFrame     = uiFrameIndex;
  slot.m_GpuRecord            = record;
  slot.m_State                = xiiGeometryResidencyState::Resident;
  slot.m_uiDirtyFrameMask     = s_pState->m_uiAllFrameMask;
  slot.m_uiVisibilityPriority = 0U;
  slot.m_uiFirstRequestFrame  = xiiMath::MaxValue<xiiUInt64>();
  return AdmissionResult::Admitted;
}

bool xiiGeometryResidencyManager::RegisterBindlessResources(Slot& slot, xiiGpuGeometryRecord& record, xiiUInt32 uiLod, xiiMeshBufferResource& mesh)
{
  if (!xiiGALBindlessResourceTable::IsInitialized() || uiLod >= record.m_uiLodCount)
    return false;

  xiiSharedPtr<xiiGALBuffer> buffers[Slot::s_uiBindlessResourcesPerLod] = {
    mesh.GetVertexBuffer(),
    mesh.GetIndexBuffer(),
    mesh.GetMeshletBuffer(),
    mesh.GetMeshletVertexRemapBuffer(),
    mesh.GetMeshletPrimitiveIndexBuffer(),
  };

  xiiGALBindlessResourceHandle handles[Slot::s_uiBindlessResourcesPerLod] = {};
  for (xiiUInt32 uiResource = 0U; uiResource < Slot::s_uiBindlessResourcesPerLod; ++uiResource)
  {
    if (buffers[uiResource] == nullptr)
      break;

    handles[uiResource] = xiiGALBindlessResourceTable::RegisterBufferSRV(buffers[uiResource]->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    if (!handles[uiResource].IsValid())
      break;
  }

  for (xiiUInt32 uiResource = 0U; uiResource < Slot::s_uiBindlessResourcesPerLod; ++uiResource)
  {
    if (handles[uiResource].IsValid())
      continue;

    for (xiiUInt32 uiRegistered = 0U; uiRegistered < uiResource; ++uiRegistered)
      xiiGALBindlessResourceTable::RetireBufferSRV(handles[uiRegistered], 0U);
    return false;
  }

  for (xiiUInt32 uiResource = 0U; uiResource < Slot::s_uiBindlessResourcesPerLod; ++uiResource)
    slot.m_BindlessResources[uiLod][uiResource] = handles[uiResource];

  xiiGpuGeometryLod& lod              = record.m_Lods[uiLod];
  lod.m_uiVertexBufferIndex           = handles[0].m_uiIndex;
  lod.m_uiIndexBufferIndex            = handles[1].m_uiIndex;
  lod.m_uiMeshletBufferIndex          = handles[2].m_uiIndex;
  lod.m_uiMeshletRemapBufferIndex     = handles[3].m_uiIndex;
  lod.m_uiMeshletPrimitiveBufferIndex = handles[4].m_uiIndex;
  return true;
}

void xiiGeometryResidencyManager::ReleaseBindlessResources(Slot& slot, xiiUInt64 uiLastUseFrame)
{
  for (xiiUInt32 uiLod = 0U; uiLod < xiiGpuGeometryRecord::s_uiMaxLods; ++uiLod)
  {
    for (xiiUInt32 uiResource = 0U; uiResource < Slot::s_uiBindlessResourcesPerLod; ++uiResource)
    {
      xiiGALBindlessResourceHandle& handle = slot.m_BindlessResources[uiLod][uiResource];
      if (handle.IsValid() && xiiGALBindlessResourceTable::IsInitialized())
        xiiGALBindlessResourceTable::RetireBufferSRV(handle, uiLastUseFrame);
      handle = {};
    }

    xiiGpuGeometryLod& lod              = slot.m_GpuRecord.m_Lods[uiLod];
    lod.m_uiVertexBufferIndex           = xiiInvalidIndex;
    lod.m_uiIndexBufferIndex            = xiiInvalidIndex;
    lod.m_uiMeshletBufferIndex          = xiiInvalidIndex;
    lod.m_uiMeshletRemapBufferIndex     = xiiInvalidIndex;
    lod.m_uiMeshletPrimitiveBufferIndex = xiiInvalidIndex;
  }
}

void xiiGeometryResidencyManager::EnforceBudget(xiiUInt64 uiCompletedFrame, xiiUInt64 uiRequiredBytes)
{
  if (uiRequiredBytes > s_pState->m_uiBudgetBytes)
    return;
  while (s_pState->m_uiResidentBytes > s_pState->m_uiBudgetBytes - uiRequiredBytes)
    if (!EvictOne(uiCompletedFrame))
      break;
}

bool xiiGeometryResidencyManager::EvictOne(xiiUInt64 uiCompletedFrame)
{
  auto& candidates = s_pState->m_EvictionCandidates;
  if (s_pState->m_uiEvictionFrame != s_pState->m_uiStreamingFrame || s_pState->m_uiEvictionCompletedFrame < uiCompletedFrame)
  {
    candidates.Clear();
    s_pState->m_uiNextEviction           = 0U;
    s_pState->m_uiEvictionFrame          = s_pState->m_uiStreamingFrame;
    s_pState->m_uiEvictionCompletedFrame = uiCompletedFrame;
    for (const auto index : s_pState->m_ActiveSlots)
    {
      const auto& slot = s_pState->m_Slots[index];
      if (slot.m_uiResidentBytes && slot.m_State == xiiGeometryResidencyState::Resident && !slot.m_Description.m_bPinned &&
          slot.m_uiLastUsedFrame <= uiCompletedFrame && slot.m_uiLastUsedFrame < s_pState->m_uiStreamingFrame && slot.m_uiAdmissionFrame != s_pState->m_uiStreamingFrame)
        candidates.PushBack(index);
    }
    candidates.Sort([](xiiUInt32 a, xiiUInt32 b) {
      const auto af = s_pState->m_Slots[a].m_uiLastUsedFrame, bf = s_pState->m_Slots[b].m_uiLastUsedFrame;
      return af != bf ? af < bf : a < b;
    });
  }
  while (s_pState->m_uiNextEviction < candidates.GetCount())
  {
    const auto  index     = candidates[s_pState->m_uiNextEviction++];
    const auto& candidate = s_pState->m_Slots[index];
    if (!candidate.m_bAllocated || candidate.m_State != xiiGeometryResidencyState::Resident || candidate.m_uiLastUsedFrame > uiCompletedFrame || candidate.m_uiLastUsedFrame >= s_pState->m_uiStreamingFrame || candidate.m_uiAdmissionFrame == s_pState->m_uiStreamingFrame)
      continue;
    auto& victim = s_pState->m_Slots[index];
    s_pState->m_uiResidentBytes -= victim.m_uiResidentBytes;
    victim.m_uiResidentBytes               = 0U;
    victim.m_GpuRecord.m_uiResidentLodMask = 0U;
    ReleaseBindlessResources(victim, victim.m_uiLastUsedFrame);
    ReleaseMeshletAllocations(victim);
    victim.m_State            = xiiGeometryResidencyState::Unloaded;
    victim.m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
    return true;
  }
  return false;
}

void xiiGeometryResidencyManager::ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes)
{
  if (!IsInitialized())
    return;

  XII_PROFILE_SCOPE("Geometry streaming");
  const auto start = xiiTime::Now();
  if (s_pState->m_uiStreamingFrame != uiFrameIndex)
  {
    s_pState->m_uiStreamingFrame      = uiFrameIndex;
    s_pState->m_uiInitialUploadBudget = s_pState->m_uiRemainingUploadBudget = uiUploadBudgetBytes;
    s_pState->m_StreamingStats                                              = {};
  }
  for (xiiUInt32 n = 0; n < s_pState->m_RetireQueue.GetCount();)
  {
    const auto i    = s_pState->m_RetireQueue[n];
    auto&      slot = s_pState->m_Slots[i];
    if (slot.m_uiRetireFrame > uiCompletedFrame)
    {
      ++n;
      continue;
    }
    s_pState->m_uiResidentBytes -= slot.m_uiResidentBytes;
    ReleaseBindlessResources(slot, slot.m_uiRetireFrame);
    ReleaseMeshletAllocations(slot);
    slot.m_bAllocated = false;
    slot.m_Description.m_Lods.Clear();
    slot.m_pCoverage.Clear();
    slot.m_uiResidentBytes = 0U;
    s_pState->m_GeometryIds.Remove(xiiGeometryId(i, slot.m_GpuRecord.m_uiGeneration));
    s_pState->m_ActiveSlots.RemoveAtAndSwap(slot.m_uiActiveIndex);
    if (slot.m_uiActiveIndex < s_pState->m_ActiveSlots.GetCount())
      s_pState->m_Slots[s_pState->m_ActiveSlots[slot.m_uiActiveIndex]].m_uiActiveIndex = slot.m_uiActiveIndex;
    s_pState->m_RetireQueue.RemoveAtAndSwap(n);
  }
  auto& streamingQueue = s_pState->m_StreamingQueue;
  for (xiiUInt32 n = 0; n < streamingQueue.GetCount();)
  {
    auto& slot = s_pState->m_Slots[streamingQueue[n]];
    if (!slot.m_bAllocated || (slot.m_State != xiiGeometryResidencyState::Requested && slot.m_State != xiiGeometryResidencyState::Loading))
    {
      slot.m_bQueued = false;
      streamingQueue.RemoveAtAndSwap(n);
    }
    else
      ++n;
  }

  // Authoring priority remains the primary control. Within that tier, visibility priority favors
  // main/sensor demand and bounded request aging prevents lower-priority views from starving.
  // Recency and slot index retain deterministic tie-breaking for captures and simulation replay.
  if (s_pState->m_uiRemainingUploadBudget > 0U)
    streamingQueue.Sort([uiFrameIndex](xiiUInt32 lhsIndex, xiiUInt32 rhsIndex) {
      const Slot& lhs = s_pState->m_Slots[lhsIndex];
      const Slot& rhs = s_pState->m_Slots[rhsIndex];
      if (lhs.m_Description.m_uiStreamingPriority != rhs.m_Description.m_uiStreamingPriority)
        return lhs.m_Description.m_uiStreamingPriority > rhs.m_Description.m_uiStreamingPriority;
      const xiiUInt64 lhsAge             = lhs.m_uiFirstRequestFrame == xiiMath::MaxValue<xiiUInt64>() ? 0U : uiFrameIndex - xiiMath::Min(uiFrameIndex, lhs.m_uiFirstRequestFrame);
      const xiiUInt64 rhsAge             = rhs.m_uiFirstRequestFrame == xiiMath::MaxValue<xiiUInt64>() ? 0U : uiFrameIndex - xiiMath::Min(uiFrameIndex, rhs.m_uiFirstRequestFrame);
      const xiiUInt64 lhsVisibilityScore = static_cast<xiiUInt64>(lhs.m_uiVisibilityPriority) + xiiMath::Min<xiiUInt64>(lhsAge, 0xFFFFU);
      const xiiUInt64 rhsVisibilityScore = static_cast<xiiUInt64>(rhs.m_uiVisibilityPriority) + xiiMath::Min<xiiUInt64>(rhsAge, 0xFFFFU);
      if (lhsVisibilityScore != rhsVisibilityScore)
        return lhsVisibilityScore > rhsVisibilityScore;
      if (lhs.m_uiLastUsedFrame != rhs.m_uiLastUsedFrame)
        return lhs.m_uiLastUsedFrame > rhs.m_uiLastUsedFrame;
      return lhsIndex < rhsIndex;
    });

  for (const auto index : streamingQueue)
  {
    if (s_pState->m_uiRemainingUploadBudget == 0U)
      break;
    auto& slot   = s_pState->m_Slots[index];
    slot.m_State = xiiGeometryResidencyState::Loading;
    ++s_pState->m_StreamingStats.m_uiExamined;
    switch (BuildResidentRecord(slot, uiFrameIndex, uiCompletedFrame))
    {
      case AdmissionResult::Admitted:
        ++s_pState->m_StreamingStats.m_uiAdmitted;
        break;
      case AdmissionResult::BudgetDeferred:
        ++s_pState->m_StreamingStats.m_uiBudgetDeferred;
        break;
      case AdmissionResult::NotReady:
        ++s_pState->m_StreamingStats.m_uiNotReady;
        break;
      case AdmissionResult::ArenaDeferred:
        ++s_pState->m_StreamingStats.m_uiArenaDeferred;
        break;
      case AdmissionResult::MemoryDeferred:
        ++s_pState->m_StreamingStats.m_uiMemoryDeferred;
        break;
      case AdmissionResult::Failed:
        ++s_pState->m_StreamingStats.m_uiFailed;
        break;
    }
  }
  s_pState->m_StreamingStats.m_fStreamingMilliseconds += (xiiTime::Now() - start).GetMilliseconds();
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStats::SetStat("Renderer/Geometry/Streaming ms", s_pState->m_StreamingStats.m_fStreamingMilliseconds);
  xiiStats::SetStat("Renderer/Geometry/Admitted bytes", s_pState->m_StreamingStats.m_uiAdmittedBytes);
  xiiStats::SetStat("Renderer/Geometry/Prepared meshlets", s_pState->m_StreamingStats.m_uiPreparedMeshlets);
  xiiStats::SetStat("Renderer/Geometry/Arena meshlets", s_pState->m_uiArenaMeshlets);
  xiiStats::SetStat("Renderer/Geometry/Resident bytes", s_pState->m_uiResidentBytes);
  const auto stats = GetStats();
  xiiStats::SetStat("Renderer/Geometry/Examined", stats.m_uiExamined);
  xiiStats::SetStat("Renderer/Geometry/Admitted", stats.m_uiAdmitted);
  xiiStats::SetStat("Renderer/Geometry/Budget deferred", stats.m_uiBudgetDeferred);
  xiiStats::SetStat("Renderer/Geometry/Not ready", stats.m_uiNotReady);
  xiiStats::SetStat("Renderer/Geometry/Arena deferred", stats.m_uiArenaDeferred);
  xiiStats::SetStat("Renderer/Geometry/Memory deferred", stats.m_uiMemoryDeferred);
  xiiStats::SetStat("Renderer/Geometry/Failed", stats.m_uiFailed);
  xiiStats::SetStat("Renderer/Geometry/Resident records", stats.m_uiResidentGeometryCount);
  xiiStats::SetStat("Renderer/Geometry/Pending records", stats.m_uiPendingGeometryCount);
  xiiStats::SetStat("Renderer/Geometry/Coverage queries", stats.m_uiCoverageQueries);
  xiiStats::SetStat("Renderer/Geometry/Coverage index hits", stats.m_uiCoverageCacheHits);
  xiiStats::SetStat("Renderer/Geometry/Coverage index builds", stats.m_uiCoverageIndexBuilds);
#endif
}

bool xiiGeometryResidencyManager::IsValid(xiiGeometryHandle handle)
{
  return s_pState != nullptr && s_pState->m_GeometryIds.Contains(handle.GetInternalID());
}

xiiEnum<xiiGeometryResidencyState> xiiGeometryResidencyManager::GetState(xiiGeometryHandle handle)
{
  if (IsValid(handle))
    return s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_State;
  return xiiEnum<xiiGeometryResidencyState>(xiiGeometryResidencyState::Unloaded);
}

const xiiGpuGeometryRecord* xiiGeometryResidencyManager::GetGpuRecord(xiiGeometryHandle handle)
{
  return IsValid(handle) ? &s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_GpuRecord : nullptr;
}

xiiResult xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(xiiGeometryHandle handle, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount, xiiArrayPtr<xiiUInt32> out_counts)
{
  for (auto& count : out_counts)
    count = 0;
  if (!IsValid(handle) || out_counts.GetCount() != xiiGpuGeometryRecord::s_uiMaxLods || uiPrimitiveCount > xiiMath::MaxValue<xiiUInt32>() - uiFirstPrimitive)
    return XII_FAILURE;

  XII_PROFILE_SCOPE("Geometry coverage");
  ++s_pState->m_uiCoverageQueries;
  auto&                     slot  = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex];
  State::MeshGeometryEntry* entry = nullptr;
  s_pState->m_MeshGeometries.TryGetValue(handle, entry);
  if (!slot.m_pCoverage)
    slot.m_pCoverage = XII_DEFAULT_NEW(CoverageIndex);
  bool      cacheHit                                  = true;
  xiiUInt32 counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
  for (xiiUInt32 lod = 0; lod < slot.m_Description.m_Lods.GetCount(); ++lod)
  {
    const auto&                            source = slot.m_Description.m_Lods[lod];
    xiiResourceLock<xiiMeshBufferResource> buffer(source.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
    if (buffer->GetLoadingState() != xiiResourceState::Loaded || (entry && buffer->GetCurrentResourceChangeCounter() != entry->m_uiBufferRevision))
      return XII_FAILURE;
    auto&      index    = slot.m_pCoverage->m_Lods[lod];
    const auto revision = buffer->GetCurrentResourceChangeCounter();
    if (!index.m_bValid || index.m_uiRevision != revision)
    {
      cacheHit            = false;
      const auto meshlets = buffer->GetMeshlets();
      if (source.m_uiFirstMeshlet > meshlets.GetCount())
        return XII_FAILURE;
      const auto available = meshlets.GetCount() - source.m_uiFirstMeshlet;
      const auto count     = source.m_uiMeshletCount == 0 ? available : source.m_uiMeshletCount;
      if (count > available)
        return XII_FAILURE;
      index.m_Starts.SetCountUninitialized(count);
      index.m_Ends.SetCountUninitialized(count);
      for (xiiUInt32 i = 0; i < count; ++i)
      {
        const auto& meshlet = meshlets[source.m_uiFirstMeshlet + i];
        index.m_Starts[i]   = meshlet.m_uiFirstPrimitive;
        index.m_Ends[i]     = xiiUInt64(meshlet.m_uiFirstPrimitive) + meshlet.m_uiPrimitiveCount;
      }
      index.m_Starts.Sort();
      index.m_Ends.Sort();
      index.m_uiRevision = revision;
      index.m_bValid     = true;
      ++s_pState->m_uiCoverageIndexBuilds;
      s_pState->m_uiCoverageMeshlets += count;
    }
    // Half-open overlap: starts < query end, minus ends <= query start.
    // Sorting endpoints independently also handles unordered and overlapping meshlets.
    counts[lod] = index.m_Starts.GetCount();
    if (uiPrimitiveCount != 0 && !index.m_Starts.IsEmpty())
    {
      const auto starts = std::lower_bound(index.m_Starts.GetData(), index.m_Starts.GetData() + index.m_Starts.GetCount(), uiFirstPrimitive + uiPrimitiveCount);
      const auto ends   = std::upper_bound(index.m_Ends.GetData(), index.m_Ends.GetData() + index.m_Ends.GetCount(), xiiUInt64(uiFirstPrimitive));
      counts[lod]       = static_cast<xiiUInt32>((starts - index.m_Starts.GetData()) - (ends - index.m_Ends.GetData()));
    }
  }
  if (cacheHit)
    ++s_pState->m_uiCoverageCacheHits;
  out_counts.CopyFrom(xiiMakeArrayPtr(counts));
  return XII_SUCCESS;
}

xiiUInt32 xiiGeometryResidencyManager::GetMetadataBaseIndex(xiiUInt64 frameIndex)
{
  return s_pState && s_pState->m_uiFramesInFlight ? static_cast<xiiUInt32>(frameIndex % s_pState->m_uiFramesInFlight) * s_pState->m_Slots.GetCount() : 0;
}

xiiSharedPtr<xiiGALBuffer> xiiGeometryResidencyManager::GetMetadataBuffer()
{
  return s_pState != nullptr ? s_pState->m_pMetadataBuffer : nullptr;
}

xiiSharedPtr<xiiGALBuffer> xiiGeometryResidencyManager::GetMeshletMetadataBuffer()
{
  return s_pState != nullptr ? s_pState->m_pMeshletMetadataBuffer : nullptr;
}

xiiGeometryResidencyStats xiiGeometryResidencyManager::GetStats()
{
  xiiGeometryResidencyStats stats;
  if (s_pState == nullptr)
    return stats;

  stats                          = s_pState->m_StreamingStats;
  stats.m_uiRemainingUploadBytes = s_pState->m_uiRemainingUploadBudget;
  stats.m_uiArenaMeshlets        = s_pState->m_uiArenaMeshlets;
  stats.m_uiCoverageCacheHits    = s_pState->m_uiCoverageCacheHits;
  stats.m_uiCoverageQueries      = s_pState->m_uiCoverageQueries;
  stats.m_uiCoverageIndexBuilds  = s_pState->m_uiCoverageIndexBuilds;
  stats.m_uiCoverageMeshlets     = s_pState->m_uiCoverageMeshlets;
  stats.m_uiBudgetBytes          = s_pState->m_uiBudgetBytes;
  stats.m_uiResidentBytes        = s_pState->m_uiResidentBytes;
  stats.m_uiUploadedBytes        = s_pState->m_uiLastUploadedBytes;
  for (const auto index : s_pState->m_ActiveSlots)
  {
    const auto& slot = s_pState->m_Slots[index];
    if (!slot.m_bAllocated)
      continue;
    ++stats.m_uiGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Resident)
      ++stats.m_uiResidentGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Requested || slot.m_State == xiiGeometryResidencyState::Loading)
      ++stats.m_uiPendingGeometryCount;
  }
  return stats;
}

xiiGeometryResidencyManager::UploadHandles xiiGeometryResidencyManager::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  if (EnsureInitialized().Failed())
    return {};

  State* pState = s_pState.Borrow();
  if (pState->m_uiUploadFrame != uiFrameIndex)
  {
    pState->m_SetupUploads.Clear();
    pState->m_uiUploadFrame       = uiFrameIndex;
    pState->m_uiLastUploadedBytes = 0U;
  }
  UploadHandles existing;
  if (pState->m_SetupUploads.TryGetValue(graph.GetSetupSerial(), existing))
    return existing;
  UploadPassData                snapshot;
  const xiiUInt32               slice = static_cast<xiiUInt32>(uiFrameIndex % pState->m_uiFramesInFlight);
  const xiiUInt64               bit   = xiiUInt64(1) << slice;
  xiiHybridArray<xiiUInt32, 64> dirty;
  if (pState->m_bMaximumDirty)
    pState->m_uiMaximumResidentMeshletCount = 0U;
  for (const auto i : pState->m_ActiveSlots)
  {
    const auto& slot = pState->m_Slots[i];
    if (pState->m_bMaximumDirty)
      for (xiiUInt32 lod = 0; lod < slot.m_GpuRecord.m_uiLodCount; ++lod)
        if (slot.m_GpuRecord.m_uiResidentLodMask & XII_BIT(lod))
          pState->m_uiMaximumResidentMeshletCount = xiiMath::Max(pState->m_uiMaximumResidentMeshletCount, slot.m_GpuRecord.m_Lods[lod].m_uiMeshletCount);
    if (slot.m_uiDirtyFrameMask & bit)
      dirty.PushBack(i);
  }
  pState->m_bMaximumDirty = false;
  dirty.Sort();
  for (const auto i : dirty)
  {
    auto& upload         = snapshot.m_Uploads.ExpandAndGetRef();
    upload.m_uiOffset    = (slice * pState->m_Slots.GetCount() + i) * sizeof(xiiGpuGeometryRecord);
    upload.m_uiSlotIndex = i;
    upload.m_uiFrameBit  = bit;
    snapshot.m_Records.PushBack(pState->m_Slots[i].m_GpuRecord);
  }
  for (const auto& upload : pState->m_PendingMeshletUploads)
    if (!upload->m_bRecorded && !upload->m_bCanceled)
      snapshot.m_MeshletUploads.PushBack(upload);
  const auto epoch         = pState->m_uiEpoch;
  const auto retainedState = s_pState;
  auto       pass          = graph.AddPass<UploadPassData>("Geometry Metadata Upload", xiiGALCommandQueueFlags::Transfer, [pState, &snapshot](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_Uploads = std::move(snapshot.m_Uploads);
      data.m_Records = std::move(snapshot.m_Records);
      data.m_MeshletUploads = std::move(snapshot.m_MeshletUploads);
      data.m_hGeometryBuffer = builder.ImportBuffer("GPU Geometry Metadata", pState->m_pMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      if (!data.m_Uploads.IsEmpty()) data.m_hGeometryBuffer = builder.WriteBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.ImportBuffer("GPU Meshlet Metadata", pState->m_pMeshletMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      if (!data.m_MeshletUploads.IsEmpty()) data.m_hMeshletBuffer = builder.WriteBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(!data.m_Uploads.IsEmpty() || !data.m_MeshletUploads.IsEmpty());
      builder.SetPassAllowMerge(false); }, [retainedState, epoch, uiFrameIndex](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      State* pState = retainedState.Borrow();
      if (pState->m_uiEpoch != epoch)
        return;
      xiiUInt64 recordedBytes = 0;
      const auto needsUpload = [&](xiiUInt32 i) {
        const auto& upload = data.m_Uploads[i];
        const auto& slot = pState->m_Slots[upload.m_uiSlotIndex];
        return slot.m_bAllocated && (slot.m_uiDirtyFrameMask & upload.m_uiFrameBit) != 0 &&
          xiiMemoryUtils::RawByteCompare(&slot.m_GpuRecord, &data.m_Records[i], sizeof(xiiGpuGeometryRecord)) == 0;
      };
      for (xiiUInt32 i = 0; i < data.m_Uploads.GetCount();)
      {
        if (!needsUpload(i)) { ++i; continue; }
        const auto first = i++;
        while (i < data.m_Uploads.GetCount() && needsUpload(i) && data.m_Uploads[i].m_uiOffset == data.m_Uploads[i-1].m_uiOffset + sizeof(xiiGpuGeometryRecord)) ++i;
        const xiiUInt32 bytes = (i - first) * static_cast<xiiUInt32>(sizeof(xiiGpuGeometryRecord));
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hGeometryBuffer), data.m_Uploads[first].m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_Records.GetData() + first), bytes));
        recordedBytes += bytes;
      }
      for (const auto& upload : data.m_MeshletUploads)
      {
        if (upload->m_bRecorded || upload->m_bCanceled)
          continue;
        const xiiUInt32 bytes = upload->m_Meshlets.GetCount() * static_cast<xiiUInt32>(sizeof(xiiMeshlet));
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hMeshletBuffer), upload->m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(upload->m_Meshlets.GetData()), bytes));
        upload->m_bRecorded = true;
        auto& pending = pState->m_Slots[upload->m_uiSlotIndex].m_PendingUploads[upload->m_uiLod];
        if (pending == upload) pending.Clear();
        upload->m_Meshlets.Clear();
        upload->m_Meshlets.Compact();
        recordedBytes += bytes;
      }
      if (pState->m_uiUploadFrame == uiFrameIndex)
        pState->m_uiLastUploadedBytes += recordedBytes;
#if XII_ENABLED(XII_USE_PROFILING)
      xiiStats::SetStat("Renderer/Geometry/Metadata upload bytes", pState->m_uiLastUploadedBytes);
#endif

      // Retire only snapshots that were actually recorded. If a newer CPU revision appeared
      // after graph setup, its byte comparison fails and the frame slice remains dirty.
      for (xiiUInt32 i = 0; i < data.m_Uploads.GetCount(); ++i)
      {
        const auto& upload = data.m_Uploads[i];
        Slot& slot = pState->m_Slots[upload.m_uiSlotIndex];
        if (slot.m_bAllocated && xiiMemoryUtils::RawByteCompare(&slot.m_GpuRecord, &data.m_Records[i], sizeof(xiiGpuGeometryRecord)) == 0)
          slot.m_uiDirtyFrameMask &= ~upload.m_uiFrameBit;
      }

      // Linear compaction retires only recorded/canceled payloads. Retained graphs
      // share completion state and cannot duplicate or resurrect a retired transfer.
      xiiUInt32 keep = 0;
      for (xiiUInt32 i = 0; i < pState->m_PendingMeshletUploads.GetCount(); ++i)
      {
        const auto& upload = pState->m_PendingMeshletUploads[i];
        if (!upload->m_bRecorded && !upload->m_bCanceled)
        {
          if (keep != i) pState->m_PendingMeshletUploads[keep] = std::move(pState->m_PendingMeshletUploads[i]);
          ++keep;
        }
      }
      pState->m_PendingMeshletUploads.SetCount(keep); }, true);


  UploadHandles result;
  result.m_hGeometryMetadata             = pass.first->m_hGeometryBuffer;
  result.m_hMeshletMetadata              = pass.first->m_hMeshletBuffer;
  result.m_uiGeometryBaseIndex           = GetMetadataBaseIndex(uiFrameIndex);
  result.m_uiMaximumResidentMeshletCount = pState->m_uiMaximumResidentMeshletCount;
  pState->m_SetupUploads.Insert(graph.GetSetupSerial(), result);
  return result;
}

bool xiiGeometryResidencyManager::AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset)
{
  for (xiiUInt32 i = 0; i < s_pState->m_FreeMeshletRanges.GetCount(); ++i)
  {
    FreeRange& range = s_pState->m_FreeMeshletRanges[i];
    if (range.m_uiCount < uiCount)
      continue;
    s_pState->m_uiArenaMeshlets += uiCount;
    out_uiOffset = range.m_uiOffset;
    range.m_uiOffset += uiCount;
    range.m_uiCount -= uiCount;
    if (range.m_uiCount == 0U)
      s_pState->m_FreeMeshletRanges.RemoveAtAndCopy(i);
    return true;
  }
  return false;
}

void xiiGeometryResidencyManager::FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount)
{
  if (uiCount == 0U)
    return;
  s_pState->m_uiArenaMeshlets -= uiCount;
  xiiUInt32 uiInsert = 0U;
  while (uiInsert < s_pState->m_FreeMeshletRanges.GetCount() && s_pState->m_FreeMeshletRanges[uiInsert].m_uiOffset < uiOffset)
    ++uiInsert;
  s_pState->m_FreeMeshletRanges.InsertAt(uiInsert, {uiOffset, uiCount});
  if (uiInsert > 0U)
  {
    FreeRange& prev = s_pState->m_FreeMeshletRanges[uiInsert - 1U];
    if (prev.m_uiOffset + prev.m_uiCount == s_pState->m_FreeMeshletRanges[uiInsert].m_uiOffset)
    {
      prev.m_uiCount += s_pState->m_FreeMeshletRanges[uiInsert].m_uiCount;
      s_pState->m_FreeMeshletRanges.RemoveAtAndCopy(uiInsert);
      --uiInsert;
    }
  }
  if (uiInsert + 1U < s_pState->m_FreeMeshletRanges.GetCount())
  {
    FreeRange&       current = s_pState->m_FreeMeshletRanges[uiInsert];
    const FreeRange& next    = s_pState->m_FreeMeshletRanges[uiInsert + 1U];
    if (current.m_uiOffset + current.m_uiCount == next.m_uiOffset)
    {
      current.m_uiCount += next.m_uiCount;
      s_pState->m_FreeMeshletRanges.RemoveAtAndCopy(uiInsert + 1U);
    }
  }
}

void xiiGeometryResidencyManager::ReleaseMeshletAllocations(Slot& slot)
{
  s_pState->m_bMaximumDirty = true;
  for (xiiUInt32 i = 0; i < xiiGpuGeometryRecord::s_uiMaxLods; ++i)
  {
    if (slot.m_PendingUploads[i])
    {
      slot.m_PendingUploads[i]->m_bCanceled = true;
      slot.m_PendingUploads[i].Clear();
    }
    FreeMeshlets(slot.m_uiMeshletArenaOffset[i], slot.m_uiMeshletArenaCount[i]);
    slot.m_uiMeshletArenaOffset[i]                       = 0U;
    slot.m_uiMeshletArenaCount[i]                        = 0U;
    slot.m_GpuRecord.m_Lods[i].m_uiMeshletMetadataOffset = 0U;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Geometry_Implementation_GeometryResidency);
