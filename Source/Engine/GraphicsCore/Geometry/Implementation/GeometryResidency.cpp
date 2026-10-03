/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

class xiiGeometryResidencyManager::State
{
public:
  struct MeshGeometryEntry
  {
    xiiGeometryHandle m_hGeometry;
    xiiUInt32         m_uiReferenceCount = 0U;
  };

  void ClearGpuState()
  {
    for (Slot& slot : m_Slots)
      xiiGeometryResidencyManager::ReleaseBindlessResources(slot, xiiMath::Max(slot.m_uiLastUsedFrame, slot.m_uiRetireFrame));

    m_pMetadataBuffer.Clear();
    m_pMeshletMetadataBuffer.Clear();
    m_Slots.Clear();
    m_FreeSlots.Clear();
    m_FreeMeshletRanges.Clear();
    m_PendingMeshletUploads.Clear();
    m_MeshGeometries.Clear();
    m_uiFramesInFlight      = 0U;
    m_uiAllFrameMask        = 0U;
    m_uiBudgetBytes         = 0U;
    m_uiResidentBytes       = 0U;
    m_uiLastUploadedBytes   = 0U;
    m_uiNextMeshletUploadId = 1U;
    m_bInitialized          = false;
  }

  xiiDynamicArray<Slot, xiiAlignedAllocatorWrapper>       m_Slots;
  xiiDynamicArray<xiiUInt32>                              m_FreeSlots;
  xiiSharedPtr<xiiGALBuffer>                              m_pMetadataBuffer;
  xiiSharedPtr<xiiGALBuffer>                              m_pMeshletMetadataBuffer;
  xiiDynamicArray<FreeRange>                              m_FreeMeshletRanges;
  xiiDynamicArray<UploadPassData::MeshletUpload>          m_PendingMeshletUploads;
  xiiHashTable<xiiMeshResourceHandle, MeshGeometryEntry> m_MeshGeometries;
  xiiUInt32                                               m_uiFramesInFlight      = 0U;
  xiiUInt64                                               m_uiAllFrameMask        = 0U;
  xiiUInt64                                               m_uiBudgetBytes         = 0U;
  xiiUInt64                                               m_uiResidentBytes       = 0U;
  xiiUInt64                                               m_uiLastUploadedBytes   = 0U;
  xiiUInt64                                               m_uiNextMeshletUploadId = 1U;
  xiiGeometryResidencyDescription                         m_Configuration;
  bool                                                    m_bEngineStarted = false;
  bool                                                    m_bInitialized   = false;
};

xiiUniquePtr<xiiGeometryResidencyManager::State> xiiGeometryResidencyManager::s_pState;

namespace
{
  constexpr float s_fLodReferenceViewportHeight = 1080.0f;

  [[nodiscard]] xiiResult BuildMeshGeometryDescription(const xiiMeshResource& mesh, xiiGeometryDescription& out_description)
  {
    const xiiMeshBufferResourceHandle& hMeshBuffer = mesh.GetMeshBuffer();
    if (!hMeshBuffer.IsValid())
      return XII_FAILURE;

    const xiiArrayPtr<const xiiMeshLOD> lods = mesh.GetLODs();
    if (lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
      return XII_FAILURE;

    out_description                        = {};
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
      source.m_uiFirstMeshlet      = meshLod.m_uiFirstMeshlet;
      source.m_uiMeshletCount      = meshLod.m_uiMeshletCount;

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
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
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
  if (!s_pState->m_bEngineStarted)
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
  return s_pState != nullptr && s_pState->m_bInitialized;
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
  if (pDevice == nullptr || uiMaxGeometries == 0U || uiFramesInFlight == 0U || uiFramesInFlight > 64U || uiMaxMeshlets == 0U)
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

  s_pState->m_Slots.SetCount(uiMaxGeometries);
  s_pState->m_FreeSlots.Reserve(uiMaxGeometries);
  for (xiiUInt32 i = uiMaxGeometries; i > 0U; --i)
    s_pState->m_FreeSlots.PushBack(i - 1U);
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

  s_pState->m_bEngineStarted               = true;
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice != nullptr)
    Initialize(pDevice.Borrow(), s_pState->m_Configuration).IgnoreResult();
}

void xiiGeometryResidencyManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->ClearGpuState();
  s_pState->m_bEngineStarted = false;
}

xiiGeometryHandle xiiGeometryResidencyManager::RegisterGeometry(const xiiGeometryDescription& description)
{
  if (!IsInitialized() || s_pState->m_pMetadataBuffer == nullptr || s_pState->m_FreeSlots.IsEmpty() || description.m_Lods.IsEmpty() || description.m_Lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
    return {};

  const xiiUInt32 uiIndex = s_pState->m_FreeSlots.PeekBack();
  s_pState->m_FreeSlots.PopBack();
  Slot& slot                      = s_pState->m_Slots[uiIndex];
  slot.m_Description              = description;
  slot.m_GpuRecord                = {};
  slot.m_GpuRecord.m_uiGeneration = slot.m_uiGeneration;
  slot.m_GpuRecord.m_uiLodCount   = description.m_Lods.GetCount();
  slot.m_State                    = xiiGeometryResidencyState::Unloaded;
  slot.m_bAllocated               = true;
  slot.m_uiDirtyFrameMask         = s_pState->m_uiAllFrameMask;
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaOffset, XII_ARRAY_SIZE(slot.m_uiMeshletArenaOffset));
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaCount, XII_ARRAY_SIZE(slot.m_uiMeshletArenaCount));

  xiiGeometryHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiGeometryResidencyManager::UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle)) return;
  Slot& slot           = s_pState->m_Slots[handle.m_uiIndex];
  slot.m_State         = xiiGeometryResidencyState::EvictPending;
  slot.m_uiRetireFrame = uiFrameIndex;
}

xiiGeometryHandle xiiGeometryResidencyManager::AcquireMeshGeometry(const xiiMeshResourceHandle& hMesh)
{
  if (!IsInitialized() || !hMesh.IsValid())
    return {};

  State::MeshGeometryEntry* pCachedEntry = nullptr;
  if (s_pState->m_MeshGeometries.TryGetValue(hMesh, pCachedEntry))
  {
    if (IsValid(pCachedEntry->m_hGeometry))
    {
      XII_ASSERT_DEV(pCachedEntry->m_uiReferenceCount < xiiMath::MaxValue<xiiUInt32>(), "Mesh geometry reference count overflow.");
      if (pCachedEntry->m_uiReferenceCount == xiiMath::MaxValue<xiiUInt32>())
        return {};

      ++pCachedEntry->m_uiReferenceCount;
      return pCachedEntry->m_hGeometry;
    }

    s_pState->m_MeshGeometries.Remove(hMesh);
  }

  xiiResourceLock<xiiMeshResource> mesh(hMesh, xiiResourceAcquireMode::PointerOnly);
  if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return {};

  xiiGeometryDescription description;
  if (BuildMeshGeometryDescription(*mesh.GetPointer(), description).Failed())
    return {};

  const xiiGeometryHandle hGeometry = RegisterGeometry(description);
  if (!hGeometry.IsValid())
    return {};

  State::MeshGeometryEntry entry;
  entry.m_hGeometry        = hGeometry;
  entry.m_uiReferenceCount = 1U;
  s_pState->m_MeshGeometries.Insert(hMesh, entry);

  // Start with the coarsest authored LOD. Fine LODs are requested on demand by visibility users.
  RequestResidency(hGeometry, description.m_Lods.GetCount() - 1U, 0U);
  return hGeometry;
}

void xiiGeometryResidencyManager::ReleaseMeshGeometry(const xiiMeshResourceHandle& hMesh, xiiUInt64 uiFrameIndex)
{
  if (s_pState == nullptr || !hMesh.IsValid())
    return;

  State::MeshGeometryEntry* pEntry = nullptr;
  if (!s_pState->m_MeshGeometries.TryGetValue(hMesh, pEntry) || pEntry->m_uiReferenceCount == 0U)
    return;

  --pEntry->m_uiReferenceCount;
  if (pEntry->m_uiReferenceCount != 0U)
    return;

  const xiiGeometryHandle hGeometry = pEntry->m_hGeometry;
  s_pState->m_MeshGeometries.Remove(hMesh);
  UnregisterGeometry(hGeometry, uiFrameIndex);
}

void xiiGeometryResidencyManager::RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle)) return;
  Slot& slot             = s_pState->m_Slots[handle.m_uiIndex];
  slot.m_uiLastUsedFrame = uiFrameIndex;
  slot.m_uiRequestedLod  = xiiMath::Min(uiMinimumLod, slot.m_Description.m_Lods.GetCount() - 1U);

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
    slot.m_State = xiiGeometryResidencyState::Requested;
}

void xiiGeometryResidencyManager::Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (IsValid(handle)) s_pState->m_Slots[handle.m_uiIndex].m_uiLastUsedFrame = uiFrameIndex;
}

bool xiiGeometryResidencyManager::BuildResidentRecord(Slot& slot, xiiUInt64& inout_uiUploadBudget)
{
  xiiGpuGeometryRecord record     = slot.m_GpuRecord;
  xiiUInt64            uiNewBytes = 0U;

  struct PendingAllocation
  {
    xiiUInt32 m_uiLod    = 0U;
    xiiUInt32 m_uiOffset = 0U;
    xiiUInt32 m_uiCount  = 0U;
  };
  xiiHybridArray<PendingAllocation, xiiGpuGeometryRecord::s_uiMaxLods>             allocations;
  xiiHybridArray<UploadPassData::MeshletUpload, xiiGpuGeometryRecord::s_uiMaxLods> uploads;
  xiiHybridArray<xiiUInt32, xiiGpuGeometryRecord::s_uiMaxLods>                     newLods;
  xiiUInt32                                                                        uiBoundsLod = record.m_uiResidentLodMask != 0U ? xiiMath::FirstBitLow(record.m_uiResidentLodMask) : xiiInvalidIndex;

  auto rollback = [&]() {
    for (const PendingAllocation& allocation : allocations)
      FreeMeshlets(allocation.m_uiOffset, allocation.m_uiCount);
  };

  for (xiiUInt32 i = slot.m_uiRequestedLod; i < slot.m_Description.m_Lods.GetCount(); ++i)
  {
    if ((record.m_uiResidentLodMask & XII_BIT(i)) != 0U)
      continue;

    const xiiGeometryLodSource&            source = slot.m_Description.m_Lods[i];
    xiiResourceLock<xiiMeshBufferResource> mesh(source.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
    if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      rollback();
      return false;
    }

    const xiiArrayPtr<const xiiMeshlet> sourceMeshlets = mesh->GetMeshlets();
    if (source.m_uiFirstMeshlet > sourceMeshlets.GetCount())
    {
      rollback();
      slot.m_State = xiiGeometryResidencyState::Failed;
      return false;
    }

    const xiiUInt32 uiAvailableMeshlets = sourceMeshlets.GetCount() - source.m_uiFirstMeshlet;
    const xiiUInt32 uiMeshletCount      = source.m_uiMeshletCount == 0U ? uiAvailableMeshlets : source.m_uiMeshletCount;
    if (uiMeshletCount > uiAvailableMeshlets)
    {
      rollback();
      slot.m_State = xiiGeometryResidencyState::Failed;
      return false;
    }

    xiiGpuGeometryLod& lod       = record.m_Lods[i];
    lod.m_uiVertexCount          = mesh->GetVertexCount();
    lod.m_uiIndexCount           = mesh->GetIndexCount();
    lod.m_uiMeshletCount         = uiMeshletCount;
    lod.m_fMinimumScreenCoverage = source.m_fMinimumScreenCoverage;
    lod.m_uiIndexType            = mesh->GetIndexType().GetValue();
    uiNewBytes += static_cast<xiiUInt64>(mesh->GetVertexCount()) * mesh->GetVertexStride();
    uiNewBytes += static_cast<xiiUInt64>(mesh->GetIndexCount()) * (mesh->GetIndexType() == xiiGALValueType::UInt16 ? 2U : 4U);
    uiNewBytes += static_cast<xiiUInt64>(uiMeshletCount) * sizeof(xiiMeshlet);
    record.m_uiResidentLodMask |= XII_BIT(i);
    newLods.PushBack(i);

    if (lod.m_uiMeshletCount > 0U)
    {
      xiiUInt32 uiArenaOffset = 0U;
      if (!AllocateMeshlets(lod.m_uiMeshletCount, uiArenaOffset))
      {
        rollback();
        return false;
      }
      lod.m_uiMeshletMetadataOffset = uiArenaOffset;

      allocations.PushBack({i, uiArenaOffset, lod.m_uiMeshletCount});
      UploadPassData::MeshletUpload& upload = uploads.ExpandAndGetRef();
      upload.m_uiUploadId                   = s_pState->m_uiNextMeshletUploadId++;
      if (s_pState->m_uiNextMeshletUploadId == 0U)
        s_pState->m_uiNextMeshletUploadId = 1U;
      upload.m_uiOffset = uiArenaOffset * sizeof(xiiMeshlet);
      upload.m_Meshlets = sourceMeshlets.GetSubArray(source.m_uiFirstMeshlet, uiMeshletCount);
      for (xiiMeshlet& meshlet : upload.m_Meshlets)
        meshlet.m_uiLodIndex = static_cast<xiiUInt16>(i);
    }

    if (i < uiBoundsLod)
    {
      const xiiBoundingBoxSphere& bounds = mesh->GetBounds();
      record.m_BoundsCenterRadius        = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fSphereRadius);
      record.m_BoundsExtents             = xiiVec4(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z, 0.0f);
      uiBoundsLod                        = i;
    }
  }

  if (uiNewBytes > inout_uiUploadBudget)
  {
    rollback();
    return false;
  }

  xiiHybridArray<xiiUInt32, xiiGpuGeometryRecord::s_uiMaxLods> registeredLods;
  for (xiiUInt32 uiLod : newLods)
  {
    xiiResourceLock<xiiMeshBufferResource> mesh(slot.m_Description.m_Lods[uiLod].m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
    if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final || !RegisterBindlessResources(slot, record, uiLod, *mesh.GetPointerNonConst()))
    {
      for (xiiUInt32 uiRegisteredLod : registeredLods)
      {
        for (xiiUInt32 uiResource = 0U; uiResource < Slot::s_uiBindlessResourcesPerLod; ++uiResource)
        {
          xiiGALBindlessResourceTable::RetireBufferSRV(slot.m_BindlessResources[uiRegisteredLod][uiResource], 0U);
          slot.m_BindlessResources[uiRegisteredLod][uiResource] = {};
        }
      }
      rollback();
      return false;
    }
    registeredLods.PushBack(uiLod);
  }

  for (const PendingAllocation& allocation : allocations)
  {
    slot.m_uiMeshletArenaOffset[allocation.m_uiLod] = allocation.m_uiOffset;
    slot.m_uiMeshletArenaCount[allocation.m_uiLod]  = allocation.m_uiCount;
  }
  for (UploadPassData::MeshletUpload& upload : uploads)
    s_pState->m_PendingMeshletUploads.PushBack(std::move(upload));

  inout_uiUploadBudget -= uiNewBytes;
  slot.m_uiResidentBytes += uiNewBytes;
  s_pState->m_uiResidentBytes += uiNewBytes;
  slot.m_GpuRecord        = record;
  slot.m_State            = xiiGeometryResidencyState::Resident;
  slot.m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
  return true;
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

  xiiGpuGeometryLod& lod               = record.m_Lods[uiLod];
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

    xiiGpuGeometryLod& lod               = slot.m_GpuRecord.m_Lods[uiLod];
    lod.m_uiVertexBufferIndex           = xiiInvalidIndex;
    lod.m_uiIndexBufferIndex            = xiiInvalidIndex;
    lod.m_uiMeshletBufferIndex          = xiiInvalidIndex;
    lod.m_uiMeshletRemapBufferIndex     = xiiInvalidIndex;
    lod.m_uiMeshletPrimitiveBufferIndex = xiiInvalidIndex;
  }
}

void xiiGeometryResidencyManager::EnforceBudget(xiiUInt64 uiCompletedFrame)
{
  while (s_pState->m_uiResidentBytes > s_pState->m_uiBudgetBytes)
  {
    xiiUInt32 uiVictim = xiiInvalidIndex;
    xiiUInt64 uiOldest = xiiMath::MaxValue<xiiUInt64>();
    for (xiiUInt32 i = 0; i < s_pState->m_Slots.GetCount(); ++i)
    {
      const Slot& slot = s_pState->m_Slots[i];
      if (slot.m_bAllocated && slot.m_State == xiiGeometryResidencyState::Resident && !slot.m_Description.m_bPinned && slot.m_uiLastUsedFrame <= uiCompletedFrame && slot.m_uiLastUsedFrame < uiOldest)
      {
        uiOldest = slot.m_uiLastUsedFrame;
        uiVictim = i;
      }
    }
    if (uiVictim == xiiInvalidIndex) break;
    Slot& victim = s_pState->m_Slots[uiVictim];
    s_pState->m_uiResidentBytes -= victim.m_uiResidentBytes;
    victim.m_uiResidentBytes               = 0U;
    victim.m_GpuRecord.m_uiResidentLodMask = 0U;
    ReleaseBindlessResources(victim, victim.m_uiLastUsedFrame);
    ReleaseMeshletAllocations(victim);
    victim.m_State            = xiiGeometryResidencyState::Unloaded;
    victim.m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
  }
}

void xiiGeometryResidencyManager::ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes)
{
  if (!IsInitialized())
    return;

  xiiDynamicArray<xiiUInt32> streamingQueue;
  for (xiiUInt32 i = 0; i < s_pState->m_Slots.GetCount(); ++i)
  {
    Slot& slot = s_pState->m_Slots[i];
    if (!slot.m_bAllocated) continue;
    if (slot.m_State == xiiGeometryResidencyState::Requested || slot.m_State == xiiGeometryResidencyState::Loading)
      streamingQueue.PushBack(i);
    if (slot.m_State == xiiGeometryResidencyState::EvictPending && slot.m_uiRetireFrame <= uiCompletedFrame)
    {
      s_pState->m_uiResidentBytes -= slot.m_uiResidentBytes;
      ReleaseBindlessResources(slot, slot.m_uiRetireFrame);
      ReleaseMeshletAllocations(slot);
      slot.m_bAllocated = false;
      slot.m_Description.m_Lods.Clear();
      slot.m_uiResidentBytes = 0U;
      ++slot.m_uiGeneration;
      if (slot.m_uiGeneration == 0U) slot.m_uiGeneration = 1U;
      s_pState->m_FreeSlots.PushBack(i);
    }
  }

  // Streaming priority is the primary authoring control. Recency breaks equal-priority ties so
  // actively visible content wins a constrained upload budget, while the slot index keeps the
  // schedule deterministic for captures and simulation replay.
  streamingQueue.Sort([](xiiUInt32 lhsIndex, xiiUInt32 rhsIndex) {
    const Slot& lhs = s_pState->m_Slots[lhsIndex];
    const Slot& rhs = s_pState->m_Slots[rhsIndex];
    if (lhs.m_Description.m_uiStreamingPriority != rhs.m_Description.m_uiStreamingPriority)
      return lhs.m_Description.m_uiStreamingPriority > rhs.m_Description.m_uiStreamingPriority;
    if (lhs.m_uiLastUsedFrame != rhs.m_uiLastUsedFrame)
      return lhs.m_uiLastUsedFrame > rhs.m_uiLastUsedFrame;
    return lhsIndex < rhsIndex;
  });

  for (xiiUInt32 uiSlotIndex : streamingQueue)
  {
    Slot& slot   = s_pState->m_Slots[uiSlotIndex];
    slot.m_State = xiiGeometryResidencyState::Loading;
    BuildResidentRecord(slot, uiUploadBudgetBytes);
  }

  EnforceBudget(uiCompletedFrame);
  XII_IGNORE_UNUSED(uiFrameIndex);
}

bool xiiGeometryResidencyManager::IsValid(xiiGeometryHandle handle)
{
  return s_pState != nullptr && handle.IsValid() && handle.m_uiIndex < s_pState->m_Slots.GetCount() && s_pState->m_Slots[handle.m_uiIndex].m_bAllocated && s_pState->m_Slots[handle.m_uiIndex].m_uiGeneration == handle.m_uiGeneration;
}

xiiEnum<xiiGeometryResidencyState> xiiGeometryResidencyManager::GetState(xiiGeometryHandle handle)
{
  if (IsValid(handle))
    return s_pState->m_Slots[handle.m_uiIndex].m_State;
  return xiiEnum<xiiGeometryResidencyState>(xiiGeometryResidencyState::Unloaded);
}

const xiiGpuGeometryRecord* xiiGeometryResidencyManager::GetGpuRecord(xiiGeometryHandle handle)
{
  return IsValid(handle) ? &s_pState->m_Slots[handle.m_uiIndex].m_GpuRecord : nullptr;
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

  stats.m_uiBudgetBytes   = s_pState->m_uiBudgetBytes;
  stats.m_uiResidentBytes = s_pState->m_uiResidentBytes;
  stats.m_uiUploadedBytes = s_pState->m_uiLastUploadedBytes;
  for (const Slot& slot : s_pState->m_Slots)
  {
    if (!slot.m_bAllocated) continue;
    ++stats.m_uiGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Resident) ++stats.m_uiResidentGeometryCount;
    if (slot.m_State == xiiGeometryResidencyState::Requested || slot.m_State == xiiGeometryResidencyState::Loading) ++stats.m_uiPendingGeometryCount;
  }
  return stats;
}

xiiGeometryResidencyManager::UploadHandles xiiGeometryResidencyManager::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized())
    return {};

  State* pState = s_pState.Borrow();
  auto   pass   = graph.AddPass<UploadPassData>(
    "Geometry Metadata Upload", xiiGALCommandQueueFlags::Transfer,
    [pState](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hGeometryBuffer = builder.ImportBuffer("GPU Geometry Metadata", pState->m_pMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hGeometryBuffer = builder.WriteBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.ImportBuffer("GPU Meshlet Metadata", pState->m_pMeshletMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.WriteBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [pState](const UploadPassData& data, xiiRenderGraphPassContext& context) {
      for (const Upload& upload : data.m_Uploads)
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hGeometryBuffer), upload.m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(&upload.m_Record), sizeof(upload.m_Record)));
      for (const UploadPassData::MeshletUpload& upload : data.m_MeshletUploads)
        context.GetCommandList().UpdateBuffer(context.GetBuffer(data.m_hMeshletBuffer), upload.m_uiOffset, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(upload.m_Meshlets.GetData()), upload.m_Meshlets.GetCount() * sizeof(xiiMeshlet)));

      // Retire only snapshots that were actually recorded. If a newer CPU revision appeared
      // after graph setup, its byte comparison fails and the frame slice remains dirty.
      for (const Upload& upload : data.m_Uploads)
      {
        if (upload.m_uiSlotIndex >= pState->m_Slots.GetCount())
          continue;
        Slot& slot = pState->m_Slots[upload.m_uiSlotIndex];
        if (slot.m_bAllocated && xiiMemoryUtils::RawByteCompare(&slot.m_GpuRecord, &upload.m_Record, sizeof(upload.m_Record)) == 0)
          slot.m_uiDirtyFrameMask &= ~upload.m_uiFrameBit;
      }

      // Pending meshlet payloads remain manager-owned until this transfer pass executes. This
      // makes graph compile failure retryable instead of dropping the only copy of an upload.
      for (const UploadPassData::MeshletUpload& uploaded : data.m_MeshletUploads)
      {
        for (xiiUInt32 i = 0U; i < pState->m_PendingMeshletUploads.GetCount(); ++i)
        {
          if (pState->m_PendingMeshletUploads[i].m_uiUploadId == uploaded.m_uiUploadId)
          {
            pState->m_PendingMeshletUploads.RemoveAtAndCopy(i);
            break;
          }
        }
      }
    },
    true);

  pState->m_uiLastUploadedBytes                 = 0U;
  xiiUInt32       uiMaximumResidentMeshletCount = 0U;
  const xiiUInt32 uiFrameSlice                  = static_cast<xiiUInt32>(uiFrameIndex % pState->m_uiFramesInFlight);
  const xiiUInt64 uiFrameBit                    = xiiUInt64(1) << uiFrameSlice;
  for (xiiUInt32 i = 0; i < pState->m_Slots.GetCount(); ++i)
  {
    Slot& slot = pState->m_Slots[i];
    if (!slot.m_bAllocated) continue;

    for (xiiUInt32 uiLod = 0U; uiLod < slot.m_GpuRecord.m_uiLodCount; ++uiLod)
    {
      if ((slot.m_GpuRecord.m_uiResidentLodMask & XII_BIT(uiLod)) != 0U)
        uiMaximumResidentMeshletCount = xiiMath::Max(uiMaximumResidentMeshletCount, slot.m_GpuRecord.m_Lods[uiLod].m_uiMeshletCount);
    }

    if ((slot.m_uiDirtyFrameMask & uiFrameBit) == 0U) continue;
    Upload& upload       = pass.first->m_Uploads.ExpandAndGetRef();
    upload.m_uiOffset    = (uiFrameSlice * pState->m_Slots.GetCount() + i) * sizeof(xiiGpuGeometryRecord);
    upload.m_uiSlotIndex = i;
    upload.m_uiFrameBit  = uiFrameBit;
    upload.m_Record      = slot.m_GpuRecord;
    pState->m_uiLastUploadedBytes += sizeof(xiiGpuGeometryRecord);
  }
  pass.first->m_MeshletUploads = pState->m_PendingMeshletUploads;
  for (const UploadPassData::MeshletUpload& upload : pass.first->m_MeshletUploads)
    pState->m_uiLastUploadedBytes += upload.m_Meshlets.GetCount() * sizeof(xiiMeshlet);

  UploadHandles result;
  result.m_hGeometryMetadata             = pass.first->m_hGeometryBuffer;
  result.m_hMeshletMetadata              = pass.first->m_hMeshletBuffer;
  result.m_uiGeometryBaseIndex           = uiFrameSlice * pState->m_Slots.GetCount();
  result.m_uiMaximumResidentMeshletCount = uiMaximumResidentMeshletCount;
  return result;
}

bool xiiGeometryResidencyManager::AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset)
{
  for (xiiUInt32 i = 0; i < s_pState->m_FreeMeshletRanges.GetCount(); ++i)
  {
    FreeRange& range = s_pState->m_FreeMeshletRanges[i];
    if (range.m_uiCount < uiCount) continue;
    out_uiOffset = range.m_uiOffset;
    range.m_uiOffset += uiCount;
    range.m_uiCount -= uiCount;
    if (range.m_uiCount == 0U) s_pState->m_FreeMeshletRanges.RemoveAtAndCopy(i);
    return true;
  }
  return false;
}

void xiiGeometryResidencyManager::FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount)
{
  if (uiCount == 0U) return;
  xiiUInt32 uiInsert = 0U;
  while (uiInsert < s_pState->m_FreeMeshletRanges.GetCount() && s_pState->m_FreeMeshletRanges[uiInsert].m_uiOffset < uiOffset) ++uiInsert;
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
  for (xiiUInt32 i = 0; i < xiiGpuGeometryRecord::s_uiMaxLods; ++i)
  {
    FreeMeshlets(slot.m_uiMeshletArenaOffset[i], slot.m_uiMeshletArenaCount[i]);
    slot.m_uiMeshletArenaOffset[i]                       = 0U;
    slot.m_uiMeshletArenaCount[i]                        = 0U;
    slot.m_GpuRecord.m_Lods[i].m_uiMeshletMetadataOffset = 0U;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Geometry_Implementation_GeometryResidency);
