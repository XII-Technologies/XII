/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Stats.h>

#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Streaming/GpuResidencyManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/Fence.h>
#include <Texture/Utilities/VirtualTextureData.h>
#include <algorithm>
#include <utility>

class xiiGpuResidencyManager::State : public xiiRefCounted
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

  struct DirectTopologyBinding
  {
    xiiGALBindlessResourceHandle m_Buffer;
    xiiUInt64                    m_uiBytes           = 0U;
    xiiUInt32                    m_uiPrimitiveOffset = 0U;
  };

  struct PageIo : public xiiTask
  {
    xiiSharedPtr<State>                 m_Owner;
    xiiSharedPtr<xiiVirtualTextureData> m_Source;
    xiiMeshBufferResourceHandle         m_hMesh;
    xiiUInt32                           m_uiRevision = 0U;
    xiiDynamicArray<xiiUInt8>           m_Data;
    xiiUInt32                           m_uiTile    = 0U;
    xiiResult                           m_Result    = XII_FAILURE;
    xiiAtomicBool                       m_bComplete = false;
    void                                Execute() override
    {
      if (m_hMesh.IsValid())
      {
        xiiResourceLock<xiiMeshBufferResource> mesh(m_hMesh, xiiResourceAcquireMode::PointerOnly);
        m_Result = mesh->ReadGeometryPage(m_uiTile, m_uiRevision, m_Data);
      }
      else
        m_Result = m_Source->ReadTile(m_uiTile, m_Data);
      m_bComplete = true;
    }
    ~PageIo() override { m_Owner->m_uiPendingIo.Decrement(); }
  };
  // Includes completed IO waiting for admission, bounding CPU payload memory as well as worker jobs.
  xiiAtomicInteger32 m_uiPendingIo = 0U;
  struct PhysicalPage
  {
    xiiSharedPtr<xiiGALBuffer>   m_Buffer;
    xiiSharedPtr<xiiGALTexture>  m_Texture;
    xiiGALBindlessResourceHandle m_Handle;
    xiiUInt64                    m_uiBytes = 0U, m_uiRetireFrame = 0U, m_uiPayloadBytes = 0U;
    xiiUInt32                    m_uiClass = 0U;
    bool                         m_bInUse  = true;
  };
  // Geometry uses 4-128 KiB size classes; texture slots separate color spaces. Descriptors remain
  // stable while slots are reused; only completed retired slots enter free lists.
  xiiDynamicArray<PhysicalPage>      m_PhysicalPages;
  xiiDynamicArray<xiiUInt32>         m_FreePhysicalPages[8], m_RetiringPhysicalPages, m_UnusedPhysicalIndices;
  xiiHashTable<xiiUInt32, xiiUInt32> m_PhysicalBuffers, m_PhysicalTextures;
  xiiUInt32                          m_uiPhysicalRetireCursor = 0U, m_uiPhysicalTrimCursor[8] = {};
  xiiUInt64                          m_uiPoolBytes[2] = {}, m_uiPoolReusableBytes = 0U, m_uiPoolRetiringBytes = 0U;
  xiiUInt64                          m_uiPoolPayloadBytes = 0U;
  xiiUInt64                          m_uiPoolReuses = 0U, m_uiPoolAllocations = 0U, m_uiPoolReleasedBytes = 0U;
  xiiUInt64                          m_uiRemainingTextureUploadBytes = 0U;
  xiiUInt32                          AddPhysicalPage(PhysicalPage&& page);
  void                               RetirePhysicalPage(xiiGALBindlessResourceHandle handle, bool texture, xiiUInt64 frame);
  void                               CollectPhysicalPages(xiiUInt64 frame, xiiUInt64 completed);
  void                               ClearPhysicalPages();
  struct TexturePage
  {
    xiiUInt64                    m_uiAdmittedFrame = 0U;
    xiiGALBindlessResourceHandle m_Texture;
    xiiSharedPtr<PageIo>         m_Io;
    bool                         m_bIoFailed = false;
    xiiUInt64                    m_uiDemand = 0U, m_uiTrimFrame = 0U;
    xiiUInt32                    m_uiRequest = 0U;
    xiiUInt64                    m_uiBytes   = 0U;
  };
  struct TextureSource
  {
    xiiUInt64                                     m_uiPageAllocationBytes = xiiVirtualTextureData::s_uiTileBytes;
    xiiSharedPtr<xiiVirtualTextureData>           m_Data;
    xiiDynamicArray<TexturePage>                  m_Pages;
    xiiGALBindlessResourceHandle                  m_Table;
    xiiUInt64                                     m_uiUsed          = 0U;
    bool                                          m_bDirty          = false;
    xiiUInt64                                     m_uiRetiringBytes = 0U;
    xiiDynamicArray<xiiGALBindlessResourceHandle> m_Retiring;
  };
  struct TextureCapture
  {
    xiiSharedPtr<xiiGALBuffer> m_Staging;
    xiiSharedPtr<xiiGALFence>  m_Fence;
    xiiUInt64                  m_uiValue = 0U, m_uiFrame = 0U;
    bool                       m_bPending        = false;
    xiiUInt64                  m_uiConsumedFrame = 0U;
  };
  static constexpr xiiUInt32                                s_uiFeedbackWords = 32768U;
  xiiHashTable<const xiiVirtualTextureData*, TextureSource> m_Textures;
  xiiUInt64                                                 m_uiTextureBytes = 0U;
  xiiSharedPtr<xiiGALBuffer>                                m_TextureFeedback[64];
  xiiGALBindlessResourceHandle                              m_TextureFeedbackUav[64];
  TextureCapture                                            m_TextureCaptures[64];
  xiiSharedPtr<xiiGALFence>                                 m_EndFences[3];
  xiiUInt64                                                 m_uiEndValues[3]      = {};
  xiiUInt32                                                 m_uiGeometryPageCount = 0U, m_uiTexturePageCount = 0U;
  xiiUInt64                                                 m_uiPageRequests = 0U, m_uiPageMisses = 0U;
  xiiUInt64                                                 m_uiGeometryPageRequests = 0U, m_uiGeometryPageUploads = 0U;
  bool                                                      InitializeTextureFeedback();
  AdmissionResult                                           UploadTextureTile(TextureSource& source, xiiUInt32 page, bool optional);
  bool                                                      PublishTexture(TextureSource& source);
  void                                                      ProcessTextures(xiiUInt64 frame, xiiUInt64 completed);
  void                                                      ClearTextures();
  struct Page
  {
    xiiUInt64                    m_uiAdmittedFrame = 0U;
    xiiSharedPtr<PageIo>         m_Io;
    bool                         m_bIoFailed = false;
    xiiGALBindlessResourceHandle m_Buffer;
    xiiUInt32                    m_uiRequest   = 0U;
    xiiUInt64                    m_uiBytes     = 0U;
    xiiUInt64                    m_uiDemand    = 0U;
    xiiUInt32                    m_uiPriority  = 0U;
    bool                         m_bRequired   = false;
    xiiUInt64                    m_uiTrimFrame = 0U;
  };
  struct alignas(16) PageRecord
  {
    xiiVec4U32 m_Mapping = xiiVec4U32(xiiInvalidIndex, 0U, 0U, 0U);
    xiiVec4    m_Bounds  = xiiVec4(0.0f);
    xiiVec4    m_Detail  = xiiVec4(0.0f);
  };
  struct SourceBinding
  {
    xiiMeshBufferResourceHandle                             m_hMesh;
    xiiDynamicArray<Page>                                   m_Pages;
    xiiDynamicArray<xiiUInt32>                              m_RootPages;
    xiiDynamicArray<PageRecord, xiiAlignedAllocatorWrapper> m_PageRecords;
    xiiGALBindlessResourceHandle                            m_PageTable;
    bool                                                    m_bPagesDirty = false;
    xiiDynamicArray<xiiGALBindlessResourceHandle>           m_Retiring;
    xiiUInt64                                               m_uiRetiringBytes = 0U;
    xiiGALBindlessResourceHandle                            m_Vertex;
    xiiGALBindlessResourceHandle                            m_Index;
    DirectTopologyBinding                                   m_DirectTopology;
    xiiUInt64                                               m_uiLastUseFrame   = 0U;
    xiiUInt32                                               m_uiReferenceCount = 0U;
  };

  AdmissionResult UploadDirectTopology(const xiiMeshBufferResource& mesh, xiiGALDevice& device, DirectTopologyBinding& out_binding);

  struct PageRequest
  {
    const void* m_pSource  = nullptr;
    xiiUInt32   m_uiPage   = 0U;
    bool        m_bTexture = false, m_bQueued = false;
    xiiUInt64   m_uiFirstFrame = 0U, m_uiDemand = 0U;
    xiiUInt32   m_uiPriority      = 0U;
    xiiUInt32   m_uiResidentIndex = xiiInvalidIndex;
    xiiUInt64   m_uiCreatedFrame  = 0U;
  };
  struct RetiredPageRequest
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiId;
    xiiUInt64 m_uiFrame;
  };
  xiiDeque<RetiredPageRequest>         m_RetiredPageRequests;
  xiiUInt32                            AllocatePageRequest(const void* source, xiiUInt32 page, bool texture, xiiUInt64 frame);
  void                                 ReleasePageRequest(xiiUInt32 request);
  void                                 AddResidentPage(xiiUInt32 request);
  void                                 RemoveResidentPage(xiiUInt32 index);
  xiiHashTable<xiiUInt32, PageRequest> m_PageRequests;
  struct QueuedPageRequest
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiId;
    xiiUInt64 m_uiCreatedFrame;
  };
  xiiDynamicArray<QueuedPageRequest> m_PageQueue;
  xiiDynamicArray<xiiUInt32>         m_ResidentPages;
  xiiUInt64                          m_uiReusableRequestFrame = 0U, m_uiRequestIdReuses = 0U;
  PageRequest*                       FindQueuedPageRequest(const QueuedPageRequest& key)
  {
    auto* request = m_PageRequests.GetValue(key.m_uiId);
    return request != nullptr && request->m_uiCreatedFrame == key.m_uiCreatedFrame ? request : nullptr;
  }
  xiiUInt32       m_uiEvictionCursor = 0U;
  bool            DemandPage(xiiUInt32 request, xiiUInt64 frame, xiiUInt32 priority);
  void            SchedulePages(xiiUInt64 frame);
  void            TrimPages(xiiUInt64 frame, xiiUInt64 completed);
  xiiUInt32       m_uiNextPageRequest = 1U;
  AdmissionResult UploadPage(SourceBinding& source, xiiUInt32 page, bool optional);
  bool            PublishPages(SourceBinding& source);
  void            ProcessPages(xiiUInt64 frame, xiiUInt64 completed);
  xiiUInt64       m_uiCoverageCacheHits = 0U;
  xiiUInt64       m_uiCoverageQueries = 0U, m_uiCoverageIndexBuilds = 0U, m_uiCoverageMeshlets = 0U;

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
  xiiGpuResidencyStats       m_StreamingStats;

  void ClearGpuState()
  {
    ClearTextures();
    for (Slot& slot : m_Slots)
      xiiGpuResidencyManager::ReleaseBindlessResources(slot, xiiMath::Max(slot.m_uiLastUsedFrame, slot.m_uiRetireFrame));
    XII_ASSERT_DEV(m_SourceBindings.IsEmpty(), "All geometry source bindings must be released with their slots.");
    ClearPhysicalPages();
    m_pTopologyUploadFence.Clear();
    m_uiTopologyUploadValue   = 0U;
    m_uiResidentTopologyBytes = 0U;
    m_uiGeometryPageCount     = 0U;
    m_uiPageRequests = m_uiPageMisses = m_uiGeometryPageRequests = m_uiGeometryPageUploads = 0U;

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
    m_PageRequests.Clear();
    m_RetiredPageRequests.Clear();
    m_uiReusableRequestFrame = m_uiRequestIdReuses = 0U;
    m_PageQueue.Clear();
    m_ResidentPages.Clear();
    m_uiEvictionCursor    = 0U;
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
  xiiHashTable<const void*, SourceBinding>                         m_SourceBindings;
  xiiSharedPtr<xiiGALFence>                                        m_pTopologyUploadFence;
  xiiUInt64                                                        m_uiTopologyUploadValue   = 0U;
  xiiUInt64                                                        m_uiResidentTopologyBytes = 0U;
  xiiUInt32                                                        m_uiFramesInFlight        = 0U;
  xiiUInt64                                                        m_uiAllFrameMask          = 0U;
  xiiUInt64                                                        m_uiBudgetBytes           = 0U;
  xiiUInt64                                                        m_uiResidentBytes         = 0U;
  xiiHashTable<xiiUInt64, UploadHandles>                           m_SetupUploads;
  xiiUInt64                                                        m_uiUploadFrame       = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt64                                                        m_uiLastUploadedBytes = 0U;

  xiiGpuResidencyDescription m_Configuration;
  xiiMutex                   m_InitializationMutex;
  bool                       m_bEngineStarted = false;
  bool                       m_bInitialized   = false;
};

xiiSharedPtr<xiiGpuResidencyManager::State> xiiGpuResidencyManager::s_pState;

namespace
{
  xiiCVarInt          cvar_TextureStreamingUploadBudget("Streaming.TextureUploadMiB", 64, xiiCVarFlags::Save, "Per-device-frame virtual texture upload allowance in MiB (0 pauses optional tiles).");
  xiiCVarInt          cvar_TextureCacheMiB("Streaming.TextureCacheMiB", 512, xiiCVarFlags::Save, "Physical virtual texture tile cache in MiB. Required coarse pages retain priority.");
  xiiCVarInt          cvar_GeometryCacheMiB("Streaming.GeometryCacheMiB", 256, xiiCVarFlags::Save, "Maximum physical geometry page payload in MiB. Required fallback pages retain priority.");
  constexpr float     s_fLodReferenceViewportHeight = 1080.0f;
  constexpr xiiUInt64 s_uiPressureTrimInterval      = 8U;
  constexpr xiiUInt64 s_uiPressureMinimumIdleFrames = 120U;
  constexpr xiiUInt32 s_uiPressureMaxEvictions      = 4U;

  void RetireBufferBinding(xiiGALBindlessResourceHandle handle, xiiUInt64 uiLastUseFrame)
  {
    if (handle.IsValid() && xiiGALBindlessResourceTable::IsInitialized())
      xiiGALBindlessResourceTable::RetireBufferSRV(handle, uiLastUseFrame);
  }

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
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GpuResidencyManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGpuResidencyManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGpuResidencyManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGpuResidencyManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGpuResidencyManager::Shutdown();
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
      XII_MEMBER_PROPERTY("MeshletTopologyBufferIndex", m_uiMeshletTopologyBufferIndex),
      XII_MEMBER_PROPERTY("MeshletPrimitiveByteOffset", m_uiMeshletPrimitiveByteOffset),
      XII_MEMBER_PROPERTY("VertexCount", m_uiVertexCount),
      XII_MEMBER_PROPERTY("IndexCount", m_uiIndexCount),
      XII_MEMBER_PROPERTY("MeshletCount", m_uiMeshletCount),
      XII_MEMBER_PROPERTY("MinimumScreenCoverage", m_fMinimumScreenCoverage),
      XII_MEMBER_PROPERTY("IndexType", m_uiIndexType),
      XII_MEMBER_PROPERTY("MeshletMetadataOffset", m_uiMeshletMetadataOffset),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuGeometryRecord, xiiNoBase, 2, xiiRTTIDefaultAllocator<xiiGpuGeometryRecord>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("BoundsCenterRadius", m_BoundsCenterRadius),
      XII_MEMBER_PROPERTY("BoundsExtents", m_BoundsExtents),
      XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("Lods", GetReflectedLodCount, GetReflectedLod),
      XII_MEMBER_PROPERTY("LodCount", m_uiLodCount),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
      XII_MEMBER_PROPERTY("AvailableLodMask", m_uiAvailableLodMask),
      XII_MEMBER_PROPERTY("Reserved", m_uiReserved),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuResidencyStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuResidencyStats>)
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
      XII_MEMBER_PROPERTY("ResidentTopologyBytes", m_uiResidentTopologyBytes),
      XII_MEMBER_PROPERTY("TopologyUploadBytes", m_uiTopologyUploadBytes),
      XII_MEMBER_PROPERTY("RetiredTopologyBytes", m_uiRetiredTopologyBytes),
      XII_MEMBER_PROPERTY("ReclaimedArenaBytes", m_uiReclaimedArenaBytes),
      XII_MEMBER_PROPERTY("Evictions", m_uiEvictions),
      XII_MEMBER_PROPERTY("PressureEvictions", m_uiPressureEvictions),
      XII_MEMBER_PROPERTY("RemainingUploadBytes", m_uiRemainingUploadBytes),
      XII_MEMBER_PROPERTY("RemainingTextureUploadBytes", m_uiRemainingTextureUploadBytes),
      XII_MEMBER_PROPERTY("LogicalPageCount", m_uiLogicalPageCount),
      XII_MEMBER_PROPERTY("RequestIdHighWater", m_uiRequestIdHighWater),
      XII_MEMBER_PROPERTY("RetiredRequestIds", m_uiRetiredRequestIds),
      XII_MEMBER_PROPERTY("RequestIdReuses", m_uiRequestIdReuses),
      XII_MEMBER_PROPERTY("PoolBackingBytes", m_uiPoolBackingBytes),
      XII_MEMBER_PROPERTY("PoolReusableBytes", m_uiPoolReusableBytes),
      XII_MEMBER_PROPERTY("PoolRetiringBytes", m_uiPoolRetiringBytes),
      XII_MEMBER_PROPERTY("PoolReuses", m_uiPoolReuses),
      XII_MEMBER_PROPERTY("PoolAllocations", m_uiPoolAllocations),
      XII_MEMBER_PROPERTY("PoolReleasedBytes", m_uiPoolReleasedBytes),
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

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuResidencyDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuResidencyDescription>)
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

void xiiGpuResidencyManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "GPU residency manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

xiiResult xiiGpuResidencyManager::Configure(const xiiGpuResidencyDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "GPU residency manager is not started.");
  if (s_pState == nullptr || description.m_uiMaxGeometries == 0U || description.m_uiFramesInFlight == 0U || description.m_uiFramesInFlight > 64U || description.m_uiMaxMeshlets == 0U)
    return XII_FAILURE;

  XII_LOCK(s_pState->m_InitializationMutex);
  const bool bSameConfiguration =
    s_pState->m_Configuration.m_uiMaxGeometries == description.m_uiMaxGeometries &&
    s_pState->m_Configuration.m_uiFramesInFlight == description.m_uiFramesInFlight &&
    s_pState->m_Configuration.m_uiBudgetBytes == description.m_uiBudgetBytes &&
    s_pState->m_Configuration.m_uiUploadBudgetPerFrameBytes == description.m_uiUploadBudgetPerFrameBytes &&
    s_pState->m_Configuration.m_uiMaxMeshlets == description.m_uiMaxMeshlets;
  if (bSameConfiguration && s_pState->m_bInitialized)
    return XII_SUCCESS;

  if (s_pState->m_bInitialized && (GetStats().m_uiGeometryCount != 0U || !s_pState->m_Textures.IsEmpty()))
  {
    XII_ASSERT_DEV(false, "GPU residency cannot be reconfigured while geometry or virtual textures are active.");
    return XII_FAILURE;
  }

  s_pState->m_Configuration = description;
  if (!s_pState->m_bInitialized)
    return XII_SUCCESS;

  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  return pDevice != nullptr ? Initialize(pDevice.Borrow(), s_pState->m_Configuration) : XII_FAILURE;
}

const xiiGpuResidencyDescription& xiiGpuResidencyManager::GetConfiguration()
{
  XII_ASSERT_DEV(s_pState != nullptr, "GPU residency manager is not started.");
  static const xiiGpuResidencyDescription s_DefaultConfiguration;
  return s_pState != nullptr ? s_pState->m_Configuration : s_DefaultConfiguration;
}

bool xiiGpuResidencyManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGpuResidencyManager::IsInitialized()
{
  if (!s_pState)
    return false;
  XII_LOCK(s_pState->m_InitializationMutex);
  return s_pState->m_bInitialized;
}

xiiResult xiiGpuResidencyManager::EnsureInitialized()
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

xiiResult xiiGpuResidencyManager::Initialize(xiiGALDevice* pDevice, const xiiGpuResidencyDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "GPU residency manager is not started.");
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
  desc.m_MemoryCategory       = xiiGALMemoryCategory::StreamingMetadata;
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

void xiiGpuResidencyManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

void xiiGpuResidencyManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede geometry residency engine startup.");
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_InitializationMutex);
  s_pState->m_bEngineStarted = true;
}

void xiiGpuResidencyManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  XII_LOCK(s_pState->m_InitializationMutex);
  s_pState->ClearGpuState();
  s_pState->m_bEngineStarted = false;
}

xiiGeometryHandle xiiGpuResidencyManager::RegisterGeometry(const xiiGeometryDescription& description)
{
  if (EnsureInitialized().Failed())
    return {};
  if (!IsInitialized() || s_pState->m_pMetadataBuffer == nullptr || s_pState->m_GeometryIds.GetCount() >= s_pState->m_Slots.GetCount() || description.m_Lods.IsEmpty() || description.m_Lods.GetCount() > xiiGpuGeometryRecord::s_uiMaxLods)
    return {};

  xiiGeometryId id;
  if (!s_pState->m_GeometryIds.TryInsert(0U, s_pState->m_Slots.GetCount(), id))
    return {};
  const xiiUInt32 uiIndex     = id.m_InstanceIndex;
  s_pState->m_GeometryIds[id] = uiIndex;
  Slot& slot                  = s_pState->m_Slots[uiIndex];
  slot.m_bQueued              = false;
  slot.m_uiLastUsedFrame      = 0U;
  slot.m_uiRetireFrame        = 0U;
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
  slot.m_uiVisibilityPriority = 0U;
  slot.m_uiFirstRequestFrame  = xiiMath::MaxValue<xiiUInt64>();
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaOffset, XII_ARRAY_SIZE(slot.m_uiMeshletArenaOffset));
  xiiMemoryUtils::ZeroFill(slot.m_uiMeshletArenaCount, XII_ARRAY_SIZE(slot.m_uiMeshletArenaCount));

  return xiiGeometryHandle(id);
}

void xiiGpuResidencyManager::UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (!IsValid(handle))
    return;
  Slot& slot = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex];
  if (slot.m_State != xiiGeometryResidencyState::EvictPending)
    s_pState->m_RetireQueue.PushBack(handle.GetInternalID().m_InstanceIndex);
  slot.m_State         = xiiGeometryResidencyState::EvictPending;
  slot.m_uiRetireFrame = xiiMath::Max(slot.m_uiRetireFrame, xiiMath::Max(uiFrameIndex, slot.m_uiLastUsedFrame));
}

xiiGeometryHandle xiiGpuResidencyManager::AcquireMeshGeometry(const xiiMeshResourceHandle& hMesh)
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

  RequestGeometry(hGeometry, 0U);
  return hGeometry;
}

void xiiGpuResidencyManager::ReleaseMeshGeometry(xiiGeometryHandle hGeometry, xiiUInt64 uiFrameIndex)
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

void xiiGpuResidencyManager::RequestGeometry(xiiGeometryHandle handle, xiiUInt64 frame, xiiUInt32 priority)
{
  if (!IsValid(handle))
    return;
  auto& slot = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex];
  if (slot.m_State == xiiGeometryResidencyState::EvictPending)
    return;
  slot.m_uiLastUsedFrame = xiiMath::Max(slot.m_uiLastUsedFrame, frame);
  if (slot.m_State == xiiGeometryResidencyState::Resident)
    return;
  for (const auto& source : slot.m_Description.m_Lods)
    xiiResourceManager::PreloadResource(source.m_hMeshBuffer);
  if (slot.m_State != xiiGeometryResidencyState::Requested && slot.m_State != xiiGeometryResidencyState::Loading)
    slot.m_uiFirstRequestFrame = frame;
  slot.m_uiVisibilityPriority = xiiMath::Max(slot.m_uiVisibilityPriority, priority);
  slot.m_State                = xiiGeometryResidencyState::Requested;
  if (!slot.m_bQueued)
  {
    slot.m_bQueued = true;
    s_pState->m_StreamingQueue.PushBack(handle.GetInternalID().m_InstanceIndex);
  }
}

void xiiGpuResidencyManager::Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex)
{
  if (IsValid(handle))
  {
    auto& lastUsed = s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_uiLastUsedFrame;
    lastUsed       = xiiMath::Max(lastUsed, uiFrameIndex);
  }
}

xiiGpuResidencyManager::AdmissionResult xiiGpuResidencyManager::BuildResidentRecord(Slot& slot, xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  XII_PROFILE_SCOPE("Geometry admission and commit");

  // Planning borrows at most eight source revisions. Keep the leases until commit
  // completes so the metadata, GPU buffers and revision checks describe one snapshot.
  xiiHybridArray<xiiResourceLock<xiiMeshBufferResource>, xiiGpuGeometryRecord::s_uiMaxLods> sources;
  xiiHybridArray<xiiSharedPtr<xiiGALBuffer>, xiiGpuGeometryRecord::s_uiMaxLods>             vertices;
  xiiHybridArray<xiiUInt32, xiiGpuGeometryRecord::s_uiMaxLods>                              newLods;
  xiiGpuGeometryRecord                                                                      record    = slot.m_GpuRecord;
  State::MeshGeometryEntry*                                                                 meshEntry = nullptr;
  const auto                                                                                slotIndex = static_cast<xiiUInt32>(&slot - s_pState->m_Slots.GetData());
  s_pState->m_MeshGeometries.TryGetValue(xiiGeometryHandle(xiiGeometryId(slotIndex, record.m_uiGeneration)), meshEntry);
  // Authored LODs are selection metadata. Payload residency is exclusively page-granular.
  const xiiUInt32 uiLodCount = record.m_uiLodCount;
  const xiiUInt32 uiNextLod = 0U, uiEndLod = uiLodCount;
  xiiUInt64       uiArenaBytes = 0U;
  for (xiiUInt32 i = uiNextLod; i < uiEndLod; ++i)
  {
    if ((record.m_uiAvailableLodMask & XII_BIT(i)) != 0U)
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
    vertices.PushBack(mesh->HasGeometryPages() ? nullptr : mesh->GetVertexBuffer());
    if (!mesh->HasGeometryPages() && vertices.PeekBack() == nullptr)
      return AdmissionResult::Failed;
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
    uiArenaBytes += xiiUInt64(lod.m_uiMeshletCount) * sizeof(xiiMeshlet);
    newLods.PushBack(i);
  }
  // The mesh resource owns source buffers. Admission occupies an arena range
  // and schedules that metadata plus one record update.
  const xiiUInt64 uiUploadBytes = uiArenaBytes + sizeof(xiiGpuGeometryRecord);
  if (uiUploadBytes > s_pState->m_uiRemainingUploadBudget && (s_pState->m_StreamingStats.m_uiAdmittedBytes != 0U || s_pState->m_uiInitialUploadBudget == 0U))
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
    if (uiArenaBytes > s_pState->m_uiBudgetBytes)
      return AdmissionResult::MemoryDeferred;
    EnforceBudget(uiCompletedFrame, uiArenaBytes);
    if (s_pState->m_uiResidentBytes > s_pState->m_uiBudgetBytes - uiArenaBytes)
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
      ReleaseSourceBinding(std::exchange(slot.m_BoundSources[newLods[n]], LodBinding{}), 0U);
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
    }
    ++allocated;
    const auto result = RegisterBindlessResources(slot, record, i, *sources[n].GetPointer(), vertices[n]);
    if (result != AdmissionResult::Admitted)
    {
      rollback();
      return result;
    }
    ++registered;
    if (lod.m_uiMeshletCount != 0U)
    {
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
      if (lod.m_uiMeshletRangesOrdered == 0U && !sources[n]->HasGeometryPages())
      {
        // Sort the admitted copy once. Topology offsets already address the LOD buffer.
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
      if (sources[n]->HasGeometryPages())
        lod.m_uiMeshletRangesOrdered = 0U;
      uploads.PushBack(std::move(upload));
      s_pState->m_StreamingStats.m_uiPreparedMeshlets += lod.m_uiMeshletCount;
    }
    record.m_uiAvailableLodMask |= XII_BIT(i);
  }
  if (!newLods.IsEmpty() && (slot.m_GpuRecord.m_uiAvailableLodMask == 0U || newLods[0] < xiiMath::FirstBitLow(slot.m_GpuRecord.m_uiAvailableLodMask)))
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
  s_pState->m_uiRemainingUploadBudget -= xiiMath::Min(s_pState->m_uiRemainingUploadBudget, uiUploadBytes);
  s_pState->m_StreamingStats.m_uiAdmittedBytes += uiUploadBytes;
  slot.m_uiResidentBytes += uiArenaBytes;
  s_pState->m_uiResidentBytes += uiArenaBytes;
  slot.m_uiAdmissionFrame         = uiFrameIndex;
  slot.m_GpuRecord                = record;
  const xiiUInt32 uiRequestedMask = (1U << uiLodCount) - 1U;
  slot.m_State                    = (record.m_uiAvailableLodMask & uiRequestedMask) == uiRequestedMask ? xiiGeometryResidencyState::Resident : xiiGeometryResidencyState::Requested;
  slot.m_uiDirtyFrameMask         = s_pState->m_uiAllFrameMask;
  slot.m_uiVisibilityPriority     = 0U;
  slot.m_uiFirstRequestFrame      = xiiMath::MaxValue<xiiUInt64>();
  return AdmissionResult::Admitted;
}

bool xiiGpuResidencyManager::State::InitializeTextureFeedback()
{
  auto*                      device = m_pMetadataBuffer->GetDevice().Borrow();
  xiiDynamicArray<xiiUInt32> zeros;
  zeros.SetCount(s_uiFeedbackWords, 0U);
  for (xiiUInt32 slot = 0U; slot < m_uiFramesInFlight; ++slot)
  {
    if (m_TextureFeedbackUav[slot].IsValid())
      continue;
    xiiGALBufferCreationDescription description;
    description.m_MemoryCategory = xiiGALMemoryCategory::StreamingMetadata;
    description.m_uiSize         = zeros.GetCount() * sizeof(xiiUInt32);
    description.m_Usage          = xiiGALResourceUsage::Mutable;
    description.m_Mode           = xiiGALBufferMode::Raw;
    description.m_BindFlags      = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
    xiiGALBufferData data(zeros.GetData(), description.m_uiSize);
    m_TextureFeedback[slot] = device->CreateBuffer(description, &data);
    if (!m_TextureFeedback[slot])
      return false;
    m_TextureFeedbackUav[slot] = xiiGALBindlessResourceTable::RegisterBufferUAV(m_TextureFeedback[slot]->GetDefaultView(xiiGALBufferViewType::UnorderedAccess));
    if (!m_TextureFeedbackUav[slot].IsValid())
      return false;
  }
  return true;
}

xiiUInt32 xiiGpuResidencyManager::State::AllocatePageRequest(const void* source, xiiUInt32 page, bool texture, xiiUInt64 frame)
{
  xiiUInt32 id = 0U;
  if (!m_RetiredPageRequests.IsEmpty() && m_RetiredPageRequests.PeekFront().m_uiFrame < m_uiReusableRequestFrame)
  {
    id = m_RetiredPageRequests.PeekFront().m_uiId;
    m_RetiredPageRequests.PopFront();
    ++m_uiRequestIdReuses;
  }
  else if (m_uiNextPageRequest < s_uiFeedbackWords * 32U)
    id = m_uiNextPageRequest++;
  if (id != 0U)
  {
    PageRequest request{source, page, texture};
    request.m_uiCreatedFrame = frame;
    m_PageRequests.Insert(id, request);
  }
  return id;
}

void xiiGpuResidencyManager::State::AddResidentPage(xiiUInt32 request)
{
  auto* entry = m_PageRequests.GetValue(request);
  XII_ASSERT_DEV(entry != nullptr && entry->m_uiResidentIndex == xiiInvalidIndex, "A page must join the resident ring once.");
  entry->m_uiResidentIndex = m_ResidentPages.GetCount();
  m_ResidentPages.PushBack(request);
}

void xiiGpuResidencyManager::State::RemoveResidentPage(xiiUInt32 index)
{
  if (auto* removed = m_PageRequests.GetValue(m_ResidentPages[index]))
    removed->m_uiResidentIndex = xiiInvalidIndex;
  m_ResidentPages.RemoveAtAndSwap(index);
  if (index < m_ResidentPages.GetCount())
  {
    auto* moved = m_PageRequests.GetValue(m_ResidentPages[index]);
    XII_ASSERT_DEV(moved != nullptr, "Resident ring entries must retain their request owner.");
    moved->m_uiResidentIndex = index;
  }
}

void xiiGpuResidencyManager::State::ReleasePageRequest(xiiUInt32 request)
{
  auto* entry = m_PageRequests.GetValue(request);
  if (entry == nullptr)
    return;
  if (entry->m_uiResidentIndex != xiiInvalidIndex)
    RemoveResidentPage(entry->m_uiResidentIndex);
  m_PageRequests.Remove(request);
  // Each ring slot must capture, clear and consume feedback after withdrawal.
  // Queue completion alone cannot clear bits left in a skipped capture slot.
  m_RetiredPageRequests.PushBack({request, m_uiStreamingFrame});
}

xiiUInt32 xiiGpuResidencyManager::State::AddPhysicalPage(PhysicalPage&& page)
{
  const auto texture    = page.m_Texture != nullptr;
  const auto descriptor = page.m_Handle.m_uiIndex;
  m_uiPoolBytes[texture ? 1U : 0U] += page.m_uiBytes;
  xiiUInt32 index;
  if (m_UnusedPhysicalIndices.IsEmpty())
  {
    index = m_PhysicalPages.GetCount();
    m_PhysicalPages.PushBack(std::move(page));
  }
  else
  {
    index = m_UnusedPhysicalIndices.PeekBack();
    m_UnusedPhysicalIndices.PopBack();
    m_PhysicalPages[index] = std::move(page);
  }
  (texture ? m_PhysicalTextures : m_PhysicalBuffers).Insert(descriptor, index);
  ++m_uiPoolAllocations;
  return index;
}

void xiiGpuResidencyManager::State::RetirePhysicalPage(xiiGALBindlessResourceHandle handle, bool texture, xiiUInt64 frame)
{
  if (!handle.IsValid())
    return;
  const auto* index = (texture ? m_PhysicalTextures : m_PhysicalBuffers).GetValue(handle.m_uiIndex);
  XII_ASSERT_DEV(index != nullptr, "Page must belong to its physical pool.");
  if (index == nullptr)
    return;
  auto& page = m_PhysicalPages[*index];
  XII_ASSERT_DEV(page.m_bInUse && page.m_Handle.m_uiGeneration == handle.m_uiGeneration, "Physical page must retire once with its current descriptor generation.");
  if (!page.m_bInUse || page.m_Handle.m_uiGeneration != handle.m_uiGeneration)
    return;
  m_uiPoolPayloadBytes -= page.m_uiPayloadBytes;
  page.m_uiPayloadBytes = 0U;
  page.m_bInUse         = false;
  page.m_uiRetireFrame  = frame;
  m_uiPoolRetiringBytes += page.m_uiBytes;
  m_RetiringPhysicalPages.PushBack(*index);
}

void xiiGpuResidencyManager::State::CollectPhysicalPages(xiiUInt64 frame, xiiUInt64 completed)
{
  const auto count = xiiMath::Min(512U, m_RetiringPhysicalPages.GetCount());
  for (xiiUInt32 n = 0U; n < count && !m_RetiringPhysicalPages.IsEmpty(); ++n)
  {
    m_uiPhysicalRetireCursor %= m_RetiringPhysicalPages.GetCount();
    const auto  index = m_RetiringPhysicalPages[m_uiPhysicalRetireCursor];
    const auto& page  = m_PhysicalPages[index];
    if (page.m_uiRetireFrame <= completed)
    {
      m_FreePhysicalPages[page.m_uiClass].PushBack(index);
      m_uiPoolRetiringBytes -= page.m_uiBytes;
      m_uiPoolReusableBytes += page.m_uiBytes;
      m_RetiringPhysicalPages.RemoveAtAndSwap(m_uiPhysicalRetireCursor);
    }
    else
      ++m_uiPhysicalRetireCursor;
  }
  const bool      pressure = m_pMetadataBuffer->GetDevice()->IsStreamingMemoryPressured();
  const xiiUInt64 limits[] = {xiiUInt64(xiiMath::Max(0, cvar_GeometryCacheMiB.GetValue())) * 1024U * 1024U,
                              xiiUInt64(xiiMath::Max(0, cvar_TextureCacheMiB.GetValue())) * 1024U * 1024U};
  for (xiiUInt32 bucket = 0U; bucket < 8U; ++bucket)
  {
    auto&      free     = m_FreePhysicalPages[bucket];
    const auto examined = xiiMath::Min(32U, free.GetCount());
    for (xiiUInt32 n = 0U; n < examined && !free.IsEmpty(); ++n)
    {
      auto& cursor = m_uiPhysicalTrimCursor[bucket];
      cursor %= free.GetCount();
      const auto index   = free[cursor];
      auto&      page    = m_PhysicalPages[index];
      const auto texture = page.m_Texture != nullptr;
      if (pressure || m_uiPoolBytes[texture ? 1U : 0U] > limits[texture ? 1U : 0U] || frame - xiiMath::Min(frame, page.m_uiRetireFrame) > 120U)
      {
        if (texture)
          xiiGALBindlessResourceTable::RetireTextureSRV(page.m_Handle, completed);
        else
          RetireBufferBinding(page.m_Handle, completed);
        (texture ? m_PhysicalTextures : m_PhysicalBuffers).Remove(page.m_Handle.m_uiIndex);
        m_uiPoolBytes[texture ? 1U : 0U] -= page.m_uiBytes;
        m_uiPoolReusableBytes -= page.m_uiBytes;
        m_uiPoolReleasedBytes += page.m_uiBytes;
        page = {};
        m_UnusedPhysicalIndices.PushBack(index);
        free.RemoveAtAndSwap(cursor);
      }
      else
        ++cursor;
    }
  }
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStats::SetStat("Renderer/GPU residency/Pool geometry backing bytes", m_uiPoolBytes[0]);
  xiiStats::SetStat("Renderer/GPU residency/Pool texture backing bytes", m_uiPoolBytes[1]);
  xiiStats::SetStat("Renderer/GPU residency/Pool reusable bytes", m_uiPoolReusableBytes);
  xiiStats::SetStat("Renderer/GPU residency/Pool active payload bytes", m_uiPoolPayloadBytes);
  const auto activeCapacity = m_uiPoolBytes[0] + m_uiPoolBytes[1] - m_uiPoolReusableBytes - m_uiPoolRetiringBytes;
  xiiStats::SetStat("Renderer/GPU residency/Pool active padding bytes", activeCapacity - xiiMath::Min(activeCapacity, m_uiPoolPayloadBytes));
  xiiStats::SetStat("Renderer/GPU residency/Pool retiring bytes", m_uiPoolRetiringBytes);
  xiiStats::SetStat("Renderer/GPU residency/Pool reuses", m_uiPoolReuses);
  xiiStats::SetStat("Renderer/GPU residency/Pool allocations", m_uiPoolAllocations);
  xiiStats::SetStat("Renderer/GPU residency/Pool released bytes", m_uiPoolReleasedBytes);
#endif
}

void xiiGpuResidencyManager::State::ClearPhysicalPages()
{
  for (auto& page : m_PhysicalPages)
  {
    if (page.m_Texture)
      xiiGALBindlessResourceTable::RetireTextureSRV(page.m_Handle, m_uiStreamingFrame);
    else if (page.m_Buffer)
      RetireBufferBinding(page.m_Handle, m_uiStreamingFrame);
  }
  m_PhysicalPages.Clear();
  m_PhysicalBuffers.Clear();
  m_PhysicalTextures.Clear();
  m_RetiringPhysicalPages.Clear();
  m_UnusedPhysicalIndices.Clear();
  for (auto& free : m_FreePhysicalPages)
    free.Clear();
  xiiMemoryUtils::ZeroFillArray(m_uiPoolBytes);
  xiiMemoryUtils::ZeroFillArray(m_uiPhysicalTrimCursor);
  m_uiPhysicalRetireCursor = 0U;
  m_uiPoolPayloadBytes = m_uiPoolReusableBytes = m_uiPoolRetiringBytes = 0U;
  m_uiPoolReuses = m_uiPoolAllocations = m_uiPoolReleasedBytes = 0U;
}

xiiGpuResidencyManager::AdmissionResult xiiGpuResidencyManager::State::UploadTextureTile(TextureSource& source, xiiUInt32 page, bool optional)
{
  constexpr auto bytes = xiiVirtualTextureData::s_uiTileBytes;
  if (optional && bytes > m_uiRemainingTextureUploadBytes)
    return AdmissionResult::BudgetDeferred;
  const auto&                    compiled = source.m_Data;
  const xiiUInt32                bucket   = compiled->m_Description.m_Format == xiiGALResourceFormat::RGBA8UNormalizedSRGB ? 7U : 6U;
  auto&                          free     = m_FreePhysicalPages[bucket];
  const auto                     limit    = xiiUInt64(xiiMath::Max(0, cvar_TextureCacheMiB.GetValue())) * 1024U * 1024U;
  auto*                          device   = m_pMetadataBuffer->GetDevice().Borrow();
  xiiGALMemoryCategoryStatistics ownership;
  const auto                     allocated = device->GetMemoryCategoryStatistics(xiiGALMemoryCategory::TexturePages, ownership) ? ownership.m_uiAllocatedBytes : m_uiPoolBytes[1];
  if (optional && free.IsEmpty() && (source.m_uiPageAllocationBytes > limit || allocated > limit - source.m_uiPageAllocationBytes))
  {
    source.m_Pages[page].m_Io.Clear();
    return AdmissionResult::MemoryDeferred;
  }
  auto&                     residency = source.m_Pages[page];
  xiiDynamicArray<xiiUInt8> requiredData;
  xiiUInt8*                 payload = nullptr;
  if (optional)
  {
    if (residency.m_bIoFailed)
      return AdmissionResult::Failed;
    if (!residency.m_Io)
    {
      if (m_uiPendingIo >= 32U)
        return AdmissionResult::NotReady;
      residency.m_Io           = XII_DEFAULT_NEW(PageIo);
      residency.m_Io->m_Owner  = s_pState;
      residency.m_Io->m_Source = compiled;
      residency.m_Io->m_uiTile = page;
      residency.m_Io->ConfigureTask("Virtual texture page IO", xiiTaskNesting::Never);
      m_uiPendingIo.Increment();
      xiiTaskSystem::StartSingleTask(residency.m_Io, xiiTaskPriority::FileAccess);
      return AdmissionResult::NotReady;
    }
    if (!residency.m_Io->m_bComplete)
      return AdmissionResult::NotReady;
    if (residency.m_Io->m_Result.Failed())
    {
      residency.m_bIoFailed = true;
      xiiLog::Warning("Texture page {} failed to load from '{}'; retaining coarse residency.", page, compiled->m_sFile);
      residency.m_Io.Clear();
      return AdmissionResult::Failed;
    }
    payload = residency.m_Io->m_Data.GetData();
  }
  else
  {
    if (compiled->ReadTile(page, requiredData).Failed())
      return AdmissionResult::Failed;
    payload = requiredData.GetData();
  }
  if (!m_pTopologyUploadFence)
    m_pTopologyUploadFence = device->CreateFence({.m_Type = xiiGALFenceType::General});
  if (!m_pTopologyUploadFence)
    return AdmissionResult::Failed;
  auto commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
  if (!commands)
    return AdmissionResult::Failed;
  commands->Begin();
  xiiUInt32 physical;
  if (!free.IsEmpty())
  {
    physical = free.PeekBack();
    free.PopBack();
    m_PhysicalPages[physical].m_bInUse = true;
    m_uiPoolReusableBytes -= m_PhysicalPages[physical].m_uiBytes;
    ++m_uiPoolReuses;
  }
  else
  {
    auto description             = compiled->m_Description;
    description.m_MemoryCategory = xiiGALMemoryCategory::TexturePages;
    description.m_Size           = {xiiVirtualTextureData::s_uiPhysicalSize, xiiVirtualTextureData::s_uiPhysicalSize};
    description.m_uiMipLevels    = 1U;
    description.m_Usage          = xiiGALResourceUsage::Mutable;
    description.m_BindFlags      = xiiGALBindFlags::ShaderResource;
    xiiGALTextureData admission;
    admission.m_bStreamingAllocation          = optional;
    admission.m_uiMaxStreamingAllocationBytes = optional ? limit - xiiMath::Min(limit, allocated) : xiiMath::MaxValue<xiiUInt64>();
    auto texture                              = device->CreateTexture(description, &admission);
    if (!texture)
    {
      commands->End();
      return AdmissionResult::MemoryDeferred;
    }
    const auto handle = xiiGALBindlessResourceTable::RegisterTextureSRV(texture->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    if (!handle.IsValid())
    {
      commands->End();
      return AdmissionResult::Failed;
    }
    PhysicalPage storage;
    storage.m_Handle  = handle;
    storage.m_uiBytes = texture->GetMemoryConsumption();
    storage.m_uiClass = bucket;
    storage.m_Texture = std::move(texture);
    physical          = AddPhysicalPage(std::move(storage));
  }
  auto&            storage = m_PhysicalPages[physical];
  xiiStringBuilder name;
  name.SetFormat("{} Texture page {}", compiled->m_sFile, page);
  storage.m_Texture->SetDebugName(name);
  xiiGALTextureSubResourceData subresource(xiiConstByteBlobPtr(payload, bytes), xiiVirtualTextureData::s_uiPhysicalSize * 4U, bytes);
  const auto                   box = xiiBoundingBoxU32::MakeFromMinMax(xiiVec3U32::MakeZero(), xiiVec3U32(xiiVirtualTextureData::s_uiPhysicalSize, xiiVirtualTextureData::s_uiPhysicalSize, 1U));
  commands->UpdateTexture(storage.m_Texture.Borrow(), {}, box, subresource);
  xiiGALStateTransitionDescription transition{.m_pResource = storage.m_Texture.Borrow(), .m_NewState = xiiGALResourceStateFlags::ShaderResource, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState};
  commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  commands->EnqueueSignal(m_pTopologyUploadFence.Borrow(), ++m_uiTopologyUploadValue);
  commands->End();
  if (device->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(commands.Borrow()) == 0U)
  {
    RetirePhysicalPage(storage.m_Handle, true, m_uiStreamingFrame);
    return AdmissionResult::Failed;
  }
  storage.m_uiPayloadBytes = bytes;
  m_uiPoolPayloadBytes += bytes;
  residency.m_uiAdmittedFrame = m_uiStreamingFrame;
  residency.m_Texture         = storage.m_Handle;
  residency.m_Io.Clear();
  residency.m_uiBytes            = storage.m_uiBytes;
  source.m_uiPageAllocationBytes = xiiMath::Max(source.m_uiPageAllocationBytes, residency.m_uiBytes);
  source.m_bDirty                = true;
  m_uiTextureBytes += residency.m_uiBytes;
  ++m_uiTexturePageCount;
  AddResidentPage(residency.m_uiRequest);
  m_uiRemainingTextureUploadBytes -= xiiMath::Min(m_uiRemainingTextureUploadBytes, xiiUInt64(bytes));
  return AdmissionResult::Admitted;
}

bool xiiGpuResidencyManager::State::PublishTexture(TextureSource& source)
{
  // Header, one offset per mip, then physical descriptor/request pairs.
  xiiDynamicArray<xiiUInt32> table;
  table.SetCount(104U + source.m_Pages.GetCount() * 2U, 0U);
  table[0] = source.m_Data->m_Description.m_Size.width;
  table[1] = source.m_Data->m_Description.m_Size.height;
  table[2] = source.m_Data->m_Description.m_uiMipLevels;
  for (xiiUInt32 slot = 0U; slot < m_uiFramesInFlight; ++slot)
    table[40U + slot] = m_TextureFeedbackUav[slot].m_uiIndex;
  table[36U] = source.m_Data->m_uiAddressFlags;
  for (xiiUInt32 p = 0U; p < source.m_Pages.GetCount(); ++p)
  {
    const auto& tile = source.m_Data->m_Tiles[p];
    if (tile.m_uiX == 0U && tile.m_uiY == 0U)
      table[4U + tile.m_uiMip] = p;
    table[104U + p * 2U] = source.m_Pages[p].m_Texture.IsValid() ? source.m_Pages[p].m_Texture.m_uiIndex : xiiInvalidIndex;
    table[105U + p * 2U] = source.m_Pages[p].m_uiRequest;
  }
  auto*                           device = m_pMetadataBuffer->GetDevice().Borrow();
  xiiGALBufferCreationDescription description;
  description.m_MemoryCategory = xiiGALMemoryCategory::StreamingMetadata;
  description.m_uiSize         = table.GetCount() * sizeof(xiiUInt32);
  description.m_Mode           = xiiGALBufferMode::Raw;
  description.m_Usage          = xiiGALResourceUsage::Immutable;
  description.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  auto commands                = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!commands)
    return false;
  commands->Begin();
  xiiGALBufferData data(table.GetData(), description.m_uiSize, commands.Borrow());
  auto             buffer = device->CreateBuffer(description, &data);
  if (!buffer)
  {
    commands->End();
    return false;
  }
  const auto handle = xiiGALBindlessResourceTable::RegisterBufferSRV(buffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  if (!handle.IsValid())
  {
    commands->End();
    return false;
  }
  if (!m_pTopologyUploadFence)
    m_pTopologyUploadFence = device->CreateFence({.m_Type = xiiGALFenceType::General});
  if (!m_pTopologyUploadFence)
  {
    commands->End();
    RetireBufferBinding(handle, 0U);
    return false;
  }
  xiiGALStateTransitionDescription transition{.m_pResource = buffer.Borrow(), .m_NewState = xiiGALResourceStateFlags::ShaderResource, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState};
  commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  commands->EnqueueSignal(m_pTopologyUploadFence.Borrow(), ++m_uiTopologyUploadValue);
  commands->End();
  if (device->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(commands.Borrow()) == 0U)
  {
    RetireBufferBinding(handle, 0U);
    return false;
  }
  RetireBufferBinding(source.m_Table, m_uiStreamingFrame);
  source.m_Table  = handle;
  source.m_bDirty = false;
  for (auto handle : source.m_Retiring)
    RetirePhysicalPage(handle, true, m_uiStreamingFrame);
  m_uiTextureBytes -= source.m_uiRetiringBytes;
  m_uiTexturePageCount -= source.m_Retiring.GetCount();
  source.m_uiRetiringBytes = 0U;
  source.m_Retiring.Clear();
  return true;
}

void xiiGpuResidencyManager::State::ClearTextures()
{
  for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
  {
    RetireBufferBinding(it.Value().m_Table, m_uiStreamingFrame);
    for (auto handle : it.Value().m_Retiring)
      RetirePhysicalPage(handle, true, m_uiStreamingFrame);
    for (auto& tile : it.Value().m_Pages)
      if (tile.m_Texture.IsValid())
        RetirePhysicalPage(tile.m_Texture, true, m_uiStreamingFrame);
  }
  m_Textures.Clear();
  for (xiiUInt32 slot = 0U; slot < 64U; ++slot)
  {
    if (m_TextureFeedbackUav[slot].IsValid())
      xiiGALBindlessResourceTable::RetireBufferUAV(m_TextureFeedbackUav[slot], m_uiStreamingFrame);
    m_TextureFeedbackUav[slot] = {};
    m_TextureFeedback[slot].Clear();
  }
  for (auto& capture : m_TextureCaptures)
    capture = {};
  for (auto& fence : m_EndFences)
    fence.Clear();
  xiiMemoryUtils::ZeroFillArray(m_uiEndValues);
  m_uiTextureBytes     = 0U;
  m_uiNextPageRequest  = 1U;
  m_uiTexturePageCount = 0U;
}

xiiUInt32 xiiGpuResidencyManager::ResolveVirtualTexture(xiiSharedPtr<xiiVirtualTextureData> data, xiiUInt64 frame)
{
  if (!data || EnsureInitialized().Failed() || !s_pState->InitializeTextureFeedback())
    return xiiInvalidIndex;
  if (s_pState->m_uiStreamingFrame == xiiMath::MaxValue<xiiUInt64>())
  {
    s_pState->m_uiStreamingFrame      = frame;
    s_pState->m_uiInitialUploadBudget = s_pState->m_uiRemainingUploadBudget = s_pState->m_Configuration.m_uiUploadBudgetPerFrameBytes;
    s_pState->m_uiRemainingTextureUploadBytes                               = xiiUInt64(xiiMath::Max(0, cvar_TextureStreamingUploadBudget.GetValue())) * 1024U * 1024U;
  }
  auto* source = s_pState->m_Textures.GetValue(data.Borrow());
  if (!source)
  {
    State::TextureSource fresh;
    if (data->m_Tiles.IsEmpty())
      return xiiInvalidIndex;
    fresh.m_Data = data;
    fresh.m_Pages.SetCount(data->m_Tiles.GetCount());
    bool committed = false;
    XII_SCOPE_EXIT(if (!committed) {
      for (const auto& page : fresh.m_Pages)
        s_pState->ReleasePageRequest(page.m_uiRequest);
    });
    for (xiiUInt32 page = 0U; page < fresh.m_Pages.GetCount(); ++page)
    {
      fresh.m_Pages[page].m_uiRequest = s_pState->AllocatePageRequest(data.Borrow(), page, true, frame);
      if (fresh.m_Pages[page].m_uiRequest == 0U)
        return xiiInvalidIndex;
    }
    const auto last = fresh.m_Pages.GetCount() - 1U;
    if (s_pState->UploadTextureTile(fresh, last, false) != AdmissionResult::Admitted || !s_pState->PublishTexture(fresh))
    {
      if (fresh.m_Pages[last].m_Texture.IsValid())
      {
        s_pState->RetirePhysicalPage(fresh.m_Pages[last].m_Texture, true, frame);
        s_pState->m_uiTextureBytes -= fresh.m_Pages[last].m_uiBytes;
        --s_pState->m_uiTexturePageCount;
      }
      return xiiInvalidIndex;
    }
    committed = true;
    s_pState->m_Textures.Insert(data.Borrow(), std::move(fresh));
    source = s_pState->m_Textures.GetValue(data.Borrow());
  }
  source->m_uiUsed = frame;
  return source->m_Table.m_uiIndex | 0x80000000U | (static_cast<xiiUInt32>(frame % s_pState->m_uiFramesInFlight) << 24U);
}


bool xiiGpuResidencyManager::State::DemandPage(xiiUInt32 request, xiiUInt64 frame, xiiUInt32 priority)
{
  auto* target = m_PageRequests.GetValue(request);
  if (!target || frame < target->m_uiCreatedFrame)
    return false;
  bool      resident = false, required = false;
  xiiUInt64 trimmed = 0U;
  if (target->m_bTexture)
  {
    auto* source = m_Textures.GetValue(static_cast<const xiiVirtualTextureData*>(target->m_pSource));
    if (!source || target->m_uiPage >= source->m_Pages.GetCount())
      return false;
    auto& page = source->m_Pages[target->m_uiPage];
    trimmed    = page.m_uiTrimFrame;
    if (frame <= trimmed)
      return false;
    page.m_uiDemand = xiiMath::Max(page.m_uiDemand, frame);
    resident        = page.m_Texture.IsValid();
    required        = target->m_uiPage + 1U == source->m_Pages.GetCount();
    // Coarse and predictive ancestors become available before costly fine pages.
    priority += source->m_Data->m_Tiles[target->m_uiPage].m_uiMip;
  }
  else
  {
    auto* source = m_SourceBindings.GetValue(target->m_pSource);
    if (!source || target->m_uiPage >= source->m_Pages.GetCount())
      return false;
    auto& page = source->m_Pages[target->m_uiPage];
    trimmed    = page.m_uiTrimFrame;
    if (frame <= trimmed)
      return false;
    page.m_uiDemand = xiiMath::Max(page.m_uiDemand, frame);
    ++m_uiGeometryPageRequests;
    page.m_uiPriority = xiiMath::Max(page.m_uiPriority, priority);
    resident          = page.m_Buffer.IsValid();
    required          = page.m_bRequired;
  }
  ++m_uiPageRequests;
  if (!resident)
    ++m_uiPageMisses;
  target->m_uiDemand   = xiiMath::Max(target->m_uiDemand, frame);
  target->m_uiPriority = xiiMath::Max(target->m_uiPriority, priority);
  if (!resident && !required && !target->m_bQueued && m_PageQueue.GetCount() < 4096U)
  {
    target->m_bQueued      = true;
    target->m_uiFirstFrame = frame;
    m_PageQueue.PushBack({request, target->m_uiCreatedFrame});
  }
  return true;
}

void xiiGpuResidencyManager::State::SchedulePages(xiiUInt64 frame)
{
  // Only missing demanded pages enter this bounded queue. Ties are deterministic across views.
  m_PageQueue.Sort([this, frame](const QueuedPageRequest& a, const QueuedPageRequest& b) {
    const auto* lhs = FindQueuedPageRequest(a);
    const auto* rhs = FindQueuedPageRequest(b);
    if (!lhs || !rhs)
      return lhs != nullptr ? true : rhs != nullptr ? false :
                                                      a.m_uiId < b.m_uiId;
    const auto score = [frame](const PageRequest& request) {
      return xiiUInt64(request.m_uiPriority) + xiiMath::Min<xiiUInt64>(255U, (frame - xiiMath::Min(frame, request.m_uiFirstFrame)) / 8U);
    };
    if (score(*lhs) != score(*rhs))
      return score(*lhs) > score(*rhs);
    if (lhs->m_uiDemand != rhs->m_uiDemand)
      return lhs->m_uiDemand > rhs->m_uiDemand;
    return a.m_uiId < b.m_uiId;
  });
  xiiUInt32 examined = 0U;
  for (xiiUInt32 i = 0U; i < m_PageQueue.GetCount() && examined < 4096U; ++examined)
  {
    const auto      request = m_PageQueue[i];
    auto*           target  = FindQueuedPageRequest(request);
    bool            remove  = target == nullptr;
    AdmissionResult result  = AdmissionResult::NotReady;
    if (target)
    {
      const bool cold = frame - xiiMath::Min(frame, target->m_uiDemand) >= 30U;
      if (target->m_bTexture)
      {
        auto* source = m_Textures.GetValue(static_cast<const xiiVirtualTextureData*>(target->m_pSource));
        if (!source)
          remove = true;
        else
        {
          auto& page = source->m_Pages[target->m_uiPage];
          remove     = cold || page.m_Texture.IsValid() || page.m_bIoFailed;
          if (remove)
            page.m_Io.Clear();
          else if (page.m_uiTrimFrame == 0U || frame - xiiMath::Min(frame, page.m_uiTrimFrame) >= 60U)
            result = UploadTextureTile(*source, target->m_uiPage, true);
        }
      }
      else
      {
        auto* source = m_SourceBindings.GetValue(target->m_pSource);
        if (!source)
          remove = true;
        else
        {
          auto& page = source->m_Pages[target->m_uiPage];
          remove     = cold || page.m_Buffer.IsValid() || page.m_bIoFailed;
          if (remove)
            page.m_Io.Clear();
          else if (page.m_uiTrimFrame == 0U || frame - xiiMath::Min(frame, page.m_uiTrimFrame) >= 60U)
            result = UploadPage(*source, target->m_uiPage, true);
        }
      }
      remove |= result == AdmissionResult::Admitted || result == AdmissionResult::Failed;
      if (result == AdmissionResult::MemoryDeferred)
        ++m_StreamingStats.m_uiMemoryDeferred;
      if (result == AdmissionResult::BudgetDeferred)
        ++m_StreamingStats.m_uiBudgetDeferred;
      if (remove)
        target->m_bQueued = false;
    }
    if (remove)
      m_PageQueue.RemoveAtAndCopy(i);
    else
      ++i;
  }
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStats::SetStat("Renderer/GPU residency/Pending page requests", m_PageQueue.GetCount());
  xiiStats::SetStat("Renderer/GPU residency/Pending page IO", static_cast<xiiUInt32>(m_uiPendingIo));
  xiiStats::SetStat("Renderer/GPU residency/Logical pages", m_PageRequests.GetCount());
  xiiStats::SetStat("Renderer/GPU residency/Request ID high water", m_uiNextPageRequest - 1U);
  xiiStats::SetStat("Renderer/GPU residency/Retired request IDs", m_RetiredPageRequests.GetCount());
  xiiStats::SetStat("Renderer/GPU residency/Request ID reuses", m_uiRequestIdReuses);
  xiiStats::SetStat("Renderer/GPU residency/Page requests", m_uiPageRequests);
  xiiStats::SetStat("Renderer/GPU residency/Page misses", m_uiPageMisses);
#endif
}

void xiiGpuResidencyManager::State::TrimPages(xiiUInt64 frame, xiiUInt64 completed)
{
  const bool pressure      = m_pMetadataBuffer->GetDevice()->IsStreamingMemoryPressured();
  const auto geometryLimit = xiiUInt64(xiiMath::Max(0, cvar_GeometryCacheMiB.GetValue())) * 1024U * 1024U;
  const auto textureLimit  = xiiUInt64(xiiMath::Max(0, cvar_TextureCacheMiB.GetValue())) * 1024U * 1024U;
  const auto examined      = xiiMath::Min(512U, m_ResidentPages.GetCount());
  for (xiiUInt32 n = 0U; n < examined && !m_ResidentPages.IsEmpty(); ++n)
  {
    m_uiEvictionCursor %= m_ResidentPages.GetCount();
    auto* target = m_PageRequests.GetValue(m_ResidentPages[m_uiEvictionCursor]);
    bool  remove = target == nullptr;
    if (target && target->m_bTexture)
    {
      auto* source = m_Textures.GetValue(static_cast<const xiiVirtualTextureData*>(target->m_pSource));
      remove       = source == nullptr;
      if (source)
      {
        auto& page        = source->m_Pages[target->m_uiPage];
        remove            = !page.m_Texture.IsValid();
        const bool excess = pressure || m_uiTextureBytes > textureLimit;
        if (!remove && target->m_uiPage + 1U != source->m_Pages.GetCount() &&
            (frame - xiiMath::Min(frame, page.m_uiDemand) > 120U ||
             (excess && frame - xiiMath::Min(frame, page.m_uiAdmittedFrame) > 60U)))
        {
          source->m_Retiring.PushBack(page.m_Texture);
          source->m_uiRetiringBytes += page.m_uiBytes;
          page.m_Texture     = {};
          page.m_uiTrimFrame = frame;
          source->m_bDirty   = true;
          remove             = true;
          ++m_StreamingStats.m_uiEvictions;
        }
      }
    }
    else if (target)
    {
      auto* source = m_SourceBindings.GetValue(target->m_pSource);
      remove       = source == nullptr;
      if (source)
      {
        auto& page        = source->m_Pages[target->m_uiPage];
        remove            = !page.m_Buffer.IsValid();
        const bool excess = pressure || m_uiResidentTopologyBytes > geometryLimit;
        if (!remove && !page.m_bRequired &&
            (frame - xiiMath::Min(frame, page.m_uiDemand) > 120U ||
             (excess && frame - xiiMath::Min(frame, page.m_uiAdmittedFrame) > 60U)))
        {
          source->m_Retiring.PushBack(page.m_Buffer);
          source->m_uiRetiringBytes += page.m_uiBytes;
          page.m_Buffer                                       = {};
          page.m_uiTrimFrame                                  = frame;
          source->m_PageRecords[target->m_uiPage].m_Mapping.x = xiiInvalidIndex;
          source->m_bPagesDirty                               = true;
          remove                                              = true;
          ++m_StreamingStats.m_uiEvictions;
        }
      }
    }
    if (remove)
      RemoveResidentPage(m_uiEvictionCursor);
    else
      ++m_uiEvictionCursor;
  }
  XII_IGNORE_UNUSED(completed);
}

void xiiGpuResidencyManager::State::ProcessTextures(xiiUInt64 frame, xiiUInt64 completed)
{
  auto* device = m_pMetadataBuffer->GetDevice().Borrow();
  for (auto& capture : m_TextureCaptures)
  {
    if (!capture.m_bPending || capture.m_Fence->GetCompletedValue() < capture.m_uiValue)
      continue;
    auto commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Transfer});
    if (!commands)
      continue;
    commands->Begin();
    void* mapped = nullptr;
    if (commands->MapBuffer(capture.m_Staging.Borrow(), xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, mapped).Succeeded())
    {
      if (mapped)
      {
        const auto* words = static_cast<const xiiUInt32*>(mapped);
        const auto  count = xiiMath::Min(s_uiFeedbackWords, (m_uiNextPageRequest + 31U) / 32U);
        for (xiiUInt32 word = 0U; word < count; ++word)
          for (xiiUInt32 mask = words[word]; mask != 0U; mask &= mask - 1U)
            DemandPage(word * 32U + xiiMath::FirstBitLow(mask), capture.m_uiFrame, 0U);
      }
      commands->UnmapBuffer(capture.m_Staging.Borrow(), xiiGALMapType::Read).AssertSuccess();
      capture.m_bPending        = false;
      capture.m_uiConsumedFrame = capture.m_uiFrame;
    }
    commands->End();
  }
  m_uiReusableRequestFrame = frame;
  for (xiiUInt32 slot = 0U; slot < m_uiFramesInFlight; ++slot)
    m_uiReusableRequestFrame = xiiMath::Min(m_uiReusableRequestFrame, m_TextureCaptures[slot].m_uiConsumedFrame);
  for (auto it = m_Textures.GetIterator(); it.IsValid();)
  {
    auto& source = it.Value();
    if (frame > source.m_uiUsed && frame - source.m_uiUsed > 120U && source.m_uiUsed <= completed)
    {
      RetireBufferBinding(source.m_Table, frame - 1U);
      for (auto handle : source.m_Retiring)
      {
        RetirePhysicalPage(handle, true, frame - 1U);
      }
      m_uiTextureBytes -= source.m_uiRetiringBytes;
      m_uiTexturePageCount -= source.m_Retiring.GetCount();
      source.m_uiRetiringBytes = 0U;
      for (auto& tile : source.m_Pages)
      {
        ReleasePageRequest(tile.m_uiRequest);
        if (tile.m_Texture.IsValid())
        {
          RetirePhysicalPage(tile.m_Texture, true, frame - 1U);
          m_uiTextureBytes -= tile.m_uiBytes;
          --m_uiTexturePageCount;
        }
      }
      it = m_Textures.Remove(it);
      continue;
    }
    if (source.m_bDirty)
      static_cast<void>(PublishTexture(source));
    ++it;
  }
#if XII_ENABLED(XII_USE_PROFILING)
  xiiStats::SetStat("Renderer/GPU residency/Texture tile bytes", m_uiTextureBytes);
  xiiStats::SetStat("Renderer/GPU residency/Virtual textures", m_Textures.GetCount());
#endif
}

void xiiGpuResidencyManager::EndStreamingFrame(xiiUInt64 frame)
{
  if (!IsInitialized() || !s_pState->m_TextureFeedback[0])
    return;
  auto&           state   = *s_pState;
  const xiiUInt32 slot    = static_cast<xiiUInt32>(frame % state.m_uiFramesInFlight);
  auto*           capture = &state.m_TextureCaptures[slot];
  if (capture->m_bPending)
    return;
  auto* device = state.m_pMetadataBuffer->GetDevice().Borrow();
  if (!capture->m_Staging)
  {
    xiiGALBufferCreationDescription description;
    description.m_MemoryCategory = xiiGALMemoryCategory::StreamingMetadata;
    description.m_uiSize         = State::s_uiFeedbackWords * sizeof(xiiUInt32);
    description.m_Usage          = xiiGALResourceUsage::Staging;
    description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Read;
    capture->m_Staging           = device->CreateBuffer(description);
    capture->m_Fence             = device->CreateFence({});
    if (!capture->m_Staging || !capture->m_Fence)
      return;
  }
  constexpr xiiGALCommandQueueFlags::Enum queues[] = {
    xiiGALCommandQueueFlags::Graphics, xiiGALCommandQueueFlags::Compute, xiiGALCommandQueueFlags::Transfer};
  auto commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Transfer});
  if (!commands)
    return;
  commands->Begin();
  for (xiiUInt32 q = 0U; q < 3U; ++q)
  {
    if (!state.m_EndFences[q])
      state.m_EndFences[q] = device->CreateFence({.m_Type = xiiGALFenceType::General});
    if (!state.m_EndFences[q])
    {
      commands->End();
      return;
    }
    auto signal = device->CreateCommandList({.m_QueueFlags = queues[q]});
    if (!signal)
    {
      commands->End();
      return;
    }
    signal->Begin();
    signal->EnqueueSignal(state.m_EndFences[q].Borrow(), ++state.m_uiEndValues[q]);
    signal->End();
    if (device->GetCommandQueue(queues[q])->Submit(signal.Borrow()) == 0U)
    {
      commands->End();
      return;
    }
    commands->DeviceWaitForFence(state.m_EndFences[q].Borrow(), state.m_uiEndValues[q]);
  }
  xiiGALStateTransitionDescription transitions[] = {
    {.m_pResource = state.m_TextureFeedback[slot].Borrow(), .m_NewState = xiiGALResourceStateFlags::CopySource, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState},
    {.m_pResource = capture->m_Staging.Borrow(), .m_NewState = xiiGALResourceStateFlags::CopyDestination, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState}};
  commands->TransitionResourceStates(xiiMakeArrayPtr(transitions));
  commands->CopyBuffer(state.m_TextureFeedback[slot].Borrow(), capture->m_Staging.Borrow());
  transitions[0].m_NewState = xiiGALResourceStateFlags::CopyDestination;
  commands->TransitionResourceStates(xiiMakeArrayPtr(transitions, 1U));
  xiiDynamicArray<xiiUInt8> zeros;
  zeros.SetCount(State::s_uiFeedbackWords * sizeof(xiiUInt32), 0U);
  commands->UpdateBuffer(state.m_TextureFeedback[slot].Borrow(), 0U, zeros);
  transitions[0].m_NewState = xiiGALResourceStateFlags::UnorderedAccess;
  commands->TransitionResourceStates(xiiMakeArrayPtr(transitions, 1U));
  commands->EnqueueSignal(capture->m_Fence.Borrow(), ++capture->m_uiValue);
  commands->End();
  if (device->GetCommandQueue(xiiGALCommandQueueFlags::Transfer)->Submit(commands.Borrow()) != 0U)
  {
    capture->m_uiFrame  = frame;
    capture->m_bPending = true;
  }
}

xiiGpuResidencyManager::AdmissionResult xiiGpuResidencyManager::State::UploadPage(SourceBinding& source, xiiUInt32 index, bool optional)
{
  xiiResourceLock<xiiMeshBufferResource> mesh(source.m_hMesh, xiiResourceAcquireMode::PointerOnly);
  if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return AdmissionResult::NotReady;
  const auto& descriptor = mesh->GetDescriptor();
  const auto& page       = descriptor.m_GeometryPages[index];
  if (page.m_uiSize == 0U || page.m_uiSize > 128U * 1024U)
    return AdmissionResult::Failed;
  xiiUInt32 bucket = 0U, capacity = 4096U;
  while (capacity < page.m_uiSize)
  {
    ++bucket;
    capacity *= 2U;
  }
  // Prefer an exact class, then one larger reusable slot before allocating.
  if (m_FreePhysicalPages[bucket].IsEmpty() && bucket + 1U < 6U && !m_FreePhysicalPages[bucket + 1U].IsEmpty())
  {
    ++bucket;
    capacity *= 2U;
  }
  auto&                          free   = m_FreePhysicalPages[bucket];
  const auto                     limit  = xiiUInt64(xiiMath::Max(0, cvar_GeometryCacheMiB.GetValue())) * 1024U * 1024U;
  auto*                          device = m_pMetadataBuffer->GetDevice().Borrow();
  xiiGALMemoryCategoryStatistics ownership;
  const auto                     allocated = device->GetMemoryCategoryStatistics(xiiGALMemoryCategory::GeometryPages, ownership) ? ownership.m_uiAllocatedBytes : m_uiPoolBytes[0];
  if (optional && free.IsEmpty() && (capacity > limit || allocated > limit - capacity))
  {
    source.m_Pages[index].m_Io.Clear();
    return AdmissionResult::MemoryDeferred;
  }
  if (optional && page.m_uiSize > m_uiRemainingUploadBudget)
    return AdmissionResult::BudgetDeferred;
  auto&                     residency = source.m_Pages[index];
  xiiDynamicArray<xiiUInt8> requiredData;
  xiiUInt8*                 payload = nullptr;
  if (optional)
  {
    if (residency.m_bIoFailed)
      return AdmissionResult::Failed;
    if (!residency.m_Io)
    {
      if (m_uiPendingIo >= 32U)
        return AdmissionResult::NotReady;
      residency.m_Io               = XII_DEFAULT_NEW(PageIo);
      residency.m_Io->m_Owner      = s_pState;
      residency.m_Io->m_hMesh      = source.m_hMesh;
      residency.m_Io->m_uiRevision = mesh->GetCurrentResourceChangeCounter();
      residency.m_Io->m_uiTile     = index;
      residency.m_Io->ConfigureTask("Virtual geometry page IO", xiiTaskNesting::Never);
      m_uiPendingIo.Increment();
      xiiTaskSystem::StartSingleTask(residency.m_Io, xiiTaskPriority::FileAccess);
      return AdmissionResult::NotReady;
    }
    if (!residency.m_Io->m_bComplete)
      return AdmissionResult::NotReady;
    if (residency.m_Io->m_Result.Failed())
    {
      residency.m_bIoFailed = true;
      xiiLog::Warning("Geometry page {} failed to load from '{}'; retaining coarse residency.", index, descriptor.m_sGeometryPageFile);
      residency.m_Io.Clear();
      return AdmissionResult::Failed;
    }
    payload = residency.m_Io->m_Data.GetData();
  }
  else
  {
    if (descriptor.ReadGeometryPage(index, requiredData).Failed())
      return AdmissionResult::Failed;
    payload = requiredData.GetData();
  }
  if (!m_pTopologyUploadFence)
  {
    xiiGALFenceCreationDescription fence;
    fence.m_Type           = xiiGALFenceType::General;
    m_pTopologyUploadFence = device->CreateFence(fence);
    if (!m_pTopologyUploadFence)
      return AdmissionResult::Failed;
  }
  auto commands = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics | xiiGALCommandQueueFlags::Transfer});
  if (!commands)
    return AdmissionResult::Failed;
  commands->Begin();
  xiiUInt32 physical;
  if (!free.IsEmpty())
  {
    physical = free.PeekBack();
    free.PopBack();
    m_PhysicalPages[physical].m_bInUse = true;
    m_uiPoolReusableBytes -= m_PhysicalPages[physical].m_uiBytes;
    ++m_uiPoolReuses;
  }
  else
  {
    xiiGALBufferCreationDescription description;
    description.m_MemoryCategory = xiiGALMemoryCategory::GeometryPages;
    description.m_uiSize         = capacity;
    description.m_BindFlags      = xiiGALBindFlags::ShaderResource;
    description.m_Mode           = xiiGALBufferMode::Raw;
    description.m_Usage          = xiiGALResourceUsage::Mutable;
    bool             deferred    = false;
    xiiGALBufferData admission;
    admission.m_pStreamingDeferred            = optional ? &deferred : nullptr;
    admission.m_uiMaxStreamingAllocationBytes = optional ? limit - xiiMath::Min(limit, allocated) : xiiMath::MaxValue<xiiUInt64>();
    auto buffer                               = device->CreateBuffer(description, &admission);
    if (!buffer)
    {
      commands->End();
      return deferred ? AdmissionResult::MemoryDeferred : AdmissionResult::Failed;
    }
    const auto handle = xiiGALBindlessResourceTable::RegisterBufferSRV(buffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    if (!handle.IsValid())
    {
      commands->End();
      return AdmissionResult::Failed;
    }
    PhysicalPage storage;
    storage.m_Handle  = handle;
    storage.m_uiBytes = buffer->GetMemoryConsumption();
    storage.m_uiClass = bucket;
    storage.m_Buffer  = std::move(buffer);
    physical          = AddPhysicalPage(std::move(storage));
  }
  auto&            storage = m_PhysicalPages[physical];
  const auto       handle  = storage.m_Handle;
  xiiStringBuilder name;
  name.SetFormat("{} Geometry page {}", mesh->GetResourceIdOrDescription(), index);
  storage.m_Buffer->SetDebugName(name);
  commands->UpdateBuffer(storage.m_Buffer.Borrow(), 0U, xiiMakeArrayPtr(payload, page.m_uiSize));
  xiiGALStateTransitionDescription transition{.m_pResource = storage.m_Buffer.Borrow(), .m_NewState = xiiGALResourceStateFlags::ShaderResource, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState};
  commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  commands->EnqueueSignal(m_pTopologyUploadFence.Borrow(), ++m_uiTopologyUploadValue);
  commands->End();
  if (device->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(commands.Borrow()) == 0U)
  {
    RetirePhysicalPage(handle, false, m_uiStreamingFrame);
    return AdmissionResult::Failed;
  }
  storage.m_uiPayloadBytes = page.m_uiSize;
  m_uiPoolPayloadBytes += page.m_uiSize;
  source.m_Pages[index].m_Io.Clear();
  source.m_Pages[index].m_uiAdmittedFrame = m_uiStreamingFrame;
  source.m_Pages[index].m_Buffer          = handle;
  source.m_Pages[index].m_uiBytes         = storage.m_uiBytes;
  source.m_PageRecords[index].m_Mapping.x = handle.m_uiIndex;
  source.m_bPagesDirty                    = true;
  m_uiResidentTopologyBytes += source.m_Pages[index].m_uiBytes;
  ++m_uiGeometryPageCount;
  ++m_uiGeometryPageUploads;
  AddResidentPage(residency.m_uiRequest);
  m_StreamingStats.m_uiTopologyUploadBytes += page.m_uiSize;
  m_StreamingStats.m_uiAdmittedBytes += page.m_uiSize;
  m_uiRemainingUploadBudget -= xiiMath::Min(m_uiRemainingUploadBudget, xiiUInt64(page.m_uiSize));
  return AdmissionResult::Admitted;
}

bool xiiGpuResidencyManager::State::PublishPages(SourceBinding& source)
{
  xiiDynamicArray<xiiUInt8> payload;
  payload.SetCount(272U + source.m_PageRecords.GetCount() * sizeof(PageRecord) + source.m_RootPages.GetCount() * sizeof(xiiUInt32), 0U);
  const xiiUInt32 count = source.m_PageRecords.GetCount();
  const xiiUInt32 roots = source.m_RootPages.GetCount();
  xiiMemoryUtils::Copy(payload.GetData() + 4U, reinterpret_cast<const xiiUInt8*>(&roots), sizeof(roots));
  xiiMemoryUtils::Copy(payload.GetData() + 272U + count * sizeof(PageRecord), reinterpret_cast<const xiiUInt8*>(source.m_RootPages.GetData()), roots * sizeof(xiiUInt32));
  xiiMemoryUtils::Copy(payload.GetData(), reinterpret_cast<const xiiUInt8*>(&count), sizeof(count));
  for (xiiUInt32 slot = 0U; slot < m_uiFramesInFlight; ++slot)
    xiiMemoryUtils::Copy(payload.GetData() + 16U + slot * sizeof(xiiUInt32), reinterpret_cast<const xiiUInt8*>(&m_TextureFeedbackUav[slot].m_uiIndex), sizeof(xiiUInt32));
  xiiMemoryUtils::Copy(payload.GetData() + 272U, reinterpret_cast<const xiiUInt8*>(source.m_PageRecords.GetData()), count * sizeof(PageRecord));
  auto*                           device = m_pMetadataBuffer->GetDevice().Borrow();
  xiiGALBufferCreationDescription description;
  description.m_MemoryCategory = xiiGALMemoryCategory::StreamingMetadata;
  description.m_uiSize         = payload.GetCount();
  description.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  description.m_Mode           = xiiGALBufferMode::Raw;
  description.m_Usage          = xiiGALResourceUsage::Immutable;
  auto commands                = device->CreateCommandList({.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!commands)
    return false;
  commands->Begin();
  xiiGALBufferData data(payload.GetData(), payload.GetCount(), commands.Borrow());
  auto             buffer = device->CreateBuffer(description, &data);
  if (!buffer)
  {
    commands->End();
    return false;
  }
  const auto handle = xiiGALBindlessResourceTable::RegisterBufferSRV(buffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  if (!handle.IsValid())
  {
    commands->End();
    return false;
  }
  xiiGALStateTransitionDescription transition{.m_pResource = buffer.Borrow(), .m_NewState = xiiGALResourceStateFlags::ShaderResource, .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState};
  commands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  commands->EnqueueSignal(m_pTopologyUploadFence.Borrow(), ++m_uiTopologyUploadValue);
  commands->End();
  if (device->GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(commands.Borrow()) == 0U)
  {
    RetireBufferBinding(handle, 0U);
    return false;
  }
  const auto previous  = source.m_PageTable;
  source.m_PageTable   = handle;
  source.m_bPagesDirty = false;
  for (auto handle : source.m_Retiring)
    RetirePhysicalPage(handle, false, m_uiStreamingFrame);
  m_uiResidentTopologyBytes -= source.m_uiRetiringBytes;
  m_uiGeometryPageCount -= source.m_Retiring.GetCount();
  m_StreamingStats.m_uiRetiredTopologyBytes += source.m_uiRetiringBytes;
  source.m_Retiring.Clear();
  source.m_uiRetiringBytes = 0U;
  RetireBufferBinding(previous, m_uiStreamingFrame);
  return true;
}

void xiiGpuResidencyManager::State::ProcessPages(xiiUInt64 frame, xiiUInt64 completed)
{
  for (auto it = m_SourceBindings.GetIterator(); it.IsValid(); ++it)
  {
    auto& source = it.Value();
    if (source.m_Pages.IsEmpty())
      continue;
    if (source.m_bPagesDirty && PublishPages(source))
      for (auto index : m_ActiveSlots)
      {
        auto& slot = m_Slots[index];
        for (xiiUInt32 lod = 0U; lod < slot.m_GpuRecord.m_uiLodCount; ++lod)
          if (slot.m_BoundSources[lod].m_pSource == it.Key())
          {
            slot.m_GpuRecord.m_Lods[lod].m_uiVertexBufferIndex          = source.m_PageTable.m_uiIndex;
            slot.m_GpuRecord.m_Lods[lod].m_uiMeshletTopologyBufferIndex = source.m_PageTable.m_uiIndex;
            slot.m_uiDirtyFrameMask                                     = m_uiAllFrameMask;
          }
      }
  }
}

xiiGpuResidencyManager::AdmissionResult xiiGpuResidencyManager::State::UploadDirectTopology(const xiiMeshBufferResource& mesh, xiiGALDevice& device, DirectTopologyBinding& out_binding)
{
  if (!m_pTopologyUploadFence)
  {
    xiiGALFenceCreationDescription description;
    description.m_Type     = xiiGALFenceType::General;
    m_pTopologyUploadFence = device.CreateFence(description);
    if (!m_pTopologyUploadFence)
      return AdmissionResult::Failed;
    m_pTopologyUploadFence->SetDebugName("Geometry Topology Upload");
  }

  auto pCommands = device.CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
  if (!pCommands)
    return AdmissionResult::Failed;

  const auto                remap      = mesh.GetMeshletVertexRemap().ToByteArray();
  const auto                primitives = mesh.GetMeshletPrimitiveIndices();
  xiiDynamicArray<xiiUInt8> payload;
  const auto                bytes = xiiUInt64(remap.GetCount()) + primitives.GetCount();
  if (bytes > xiiMath::MaxValue<xiiUInt32>() - 3U)
    return AdmissionResult::Failed;
  payload.SetCountUninitialized(xiiMath::RoundUp(static_cast<xiiUInt32>(bytes), 4U));
  xiiMemoryUtils::Copy(payload.GetData(), remap.GetPtr(), remap.GetCount());
  xiiMemoryUtils::Copy(payload.GetData() + remap.GetCount(), primitives.GetPtr(), primitives.GetCount());
  const xiiUInt32 uiContentBytes = remap.GetCount() + primitives.GetCount();
  xiiMemoryUtils::ZeroFill(payload.GetData() + uiContentBytes, payload.GetCount() - uiContentBytes);

  xiiGALBufferCreationDescription description;
  description.m_MemoryCategory = xiiGALMemoryCategory::StreamingMetadata;
  description.m_uiSize         = payload.GetCount();
  description.m_BindFlags      = xiiGALBindFlags::ShaderResource;
  description.m_Mode           = xiiGALBufferMode::Raw;
  description.m_Usage          = xiiGALResourceUsage::Immutable;
  description.m_MemoryCategory = xiiGALMemoryCategory::Geometry;
  xiiGALBufferData initialData(payload.GetData(), payload.GetCount(), pCommands.Borrow());
  pCommands->Begin();
  auto pBuffer = device.CreateBuffer(description, &initialData);
  if (!pBuffer)
  {
    pCommands->End();
    return AdmissionResult::Failed;
  }
  xiiStringBuilder sName(mesh.GetResourceIdOrDescription(), " Direct Deformable Topology");
  pBuffer->SetDebugName(sName);

  const auto hBuffer = xiiGALBindlessResourceTable::RegisterBufferSRV(pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  if (!hBuffer.IsValid())
  {
    pCommands->End();
    return AdmissionResult::Failed;
  }

  xiiGALStateTransitionDescription transition = {
    .m_pResource       = pBuffer.Borrow(),
    .m_NewState        = xiiGALResourceStateFlags::ShaderResource,
    .m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState,
  };
  const xiiUInt64 uiSignal = m_uiTopologyUploadValue + 1U;
  pCommands->TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  pCommands->EnqueueSignal(m_pTopologyUploadFence.Borrow(), uiSignal);
  pCommands->End();
  if (device.GetCommandQueue(xiiGALCommandQueueFlags::Graphics)->Submit(pCommands.Borrow()) == 0U)
  {
    RetireBufferBinding(hBuffer, 0U);
    return AdmissionResult::Failed;
  }

  out_binding.m_Buffer            = hBuffer;
  out_binding.m_uiPrimitiveOffset = remap.GetCount();
  out_binding.m_uiBytes           = pBuffer->GetMemoryConsumption();
  m_uiTopologyUploadValue         = uiSignal;
  const xiiUInt64 uiBytes         = payload.GetCount();
  m_StreamingStats.m_uiTopologyUploadBytes += uiBytes;
  m_StreamingStats.m_uiAdmittedBytes += uiBytes;
  m_uiRemainingUploadBudget -= xiiMath::Min(m_uiRemainingUploadBudget, uiBytes);
  return AdmissionResult::Admitted;
}

xiiGpuResidencyManager::AdmissionResult xiiGpuResidencyManager::RegisterBindlessResources(Slot& slot, xiiGpuGeometryRecord& record, xiiUInt32 uiLod, const xiiMeshBufferResource& mesh, const xiiSharedPtr<xiiGALBuffer>& pVertexBuffer)
{
  if (!xiiGALBindlessResourceTable::IsInitialized() || uiLod >= record.m_uiLodCount)
    return AdmissionResult::Failed;

  if (mesh.HasGeometryPages())
  {
    const void* identity = &mesh;
    auto*       source   = s_pState->m_SourceBindings.GetValue(identity);
    if (source == nullptr)
    {
      if (!s_pState->InitializeTextureFeedback())
        return AdmissionResult::Failed;
      State::SourceBinding fresh;
      fresh.m_hMesh          = slot.m_Description.m_Lods[uiLod].m_hMeshBuffer;
      const auto& descriptor = mesh.GetDescriptor();
      fresh.m_Pages.SetCount(descriptor.m_GeometryPages.GetCount());
      fresh.m_PageRecords.SetCount(fresh.m_Pages.GetCount());
      for (xiiUInt32 p = 0U; p < fresh.m_Pages.GetCount(); ++p)
      {
        auto& page       = fresh.m_Pages[p];
        page.m_bRequired = descriptor.m_GeometryPages[p].m_bRequired;
        page.m_uiRequest = s_pState->AllocatePageRequest(identity, p, false, s_pState->m_uiStreamingFrame);
        if (page.m_uiRequest == 0U)
        {
          for (const auto& allocated : fresh.m_Pages)
            s_pState->ReleasePageRequest(allocated.m_uiRequest);
          return AdmissionResult::MemoryDeferred;
        }
        fresh.m_PageRecords[p].m_Mapping.y = page.m_uiRequest;
        const auto& compiled               = descriptor.m_GeometryPages[p];
        fresh.m_PageRecords[p].m_Mapping.z = compiled.m_uiFirstCluster;
        fresh.m_PageRecords[p].m_Mapping.w = compiled.m_uiClusterCount;
        fresh.m_PageRecords[p].m_Detail    = xiiVec4(compiled.m_fGeometricError,
                                                     compiled.m_uiParent == xiiInvalidIndex ? 0.0f : static_cast<float>(compiled.m_uiParent + 1U),
                                                     compiled.m_uiChildCount == 0U ? 0.0f : static_cast<float>(compiled.m_uiFirstChild + 1U),
                                                     static_cast<float>(compiled.m_uiChildCount));
        if (page.m_bRequired)
          fresh.m_RootPages.PushBack(p);
      }
      for (const auto& cluster : descriptor.m_Meshlets)
      {
        auto&      record = fresh.m_PageRecords[cluster.m_Page.x];
        const auto bounds = cluster.m_Bounds;
        if (record.m_Bounds.w == 0.0f)
          record.m_Bounds = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fRadius);
        else
        {
          auto sphere = xiiBoundingSphere::MakeFromCenterAndRadius(record.m_Bounds.GetAsVec3(), record.m_Bounds.w);
          sphere.ExpandToInclude(bounds);
          record.m_Bounds = xiiVec4(sphere.m_vCenter.x, sphere.m_vCenter.y, sphere.m_vCenter.z, sphere.m_fRadius);
        }
        record.m_Detail.x = xiiMath::Max(record.m_Detail.x, cluster.m_Hierarchy.x);
      }
      s_pState->m_SourceBindings.Insert(identity, std::move(fresh));
      source = s_pState->m_SourceBindings.GetValue(identity);
    }
    bool committed = false;
    XII_SCOPE_EXIT(if (!committed && source->m_uiReferenceCount == 0U) {
      ++source->m_uiReferenceCount;
      ReleaseSourceBinding({identity}, 0U);
    });
    // Required fallback pages complete before any record exposes the hierarchy.
    for (xiiUInt32 p = 0U; p < source->m_Pages.GetCount(); ++p)
      if (source->m_Pages[p].m_bRequired && !source->m_Pages[p].m_Buffer.IsValid())
      {
        const auto result = s_pState->UploadPage(*source, p, false);
        if (result != AdmissionResult::Admitted)
          return result;
      }
    if ((!source->m_PageTable.IsValid() || source->m_bPagesDirty) && !s_pState->PublishPages(*source))
      return AdmissionResult::Failed;
    auto& lod                          = record.m_Lods[uiLod];
    lod.m_uiPadding                    = 1U | (static_cast<xiiUInt32>(s_pState->m_uiStreamingFrame % s_pState->m_uiFramesInFlight) << 1U);
    lod.m_uiIndexBufferIndex           = slot.m_Description.m_Lods[uiLod].m_uiFirstMeshlet;
    lod.m_uiVertexBufferIndex          = source->m_PageTable.m_uiIndex;
    lod.m_uiMeshletTopologyBufferIndex = source->m_PageTable.m_uiIndex;
    lod.m_uiMeshletPrimitiveByteOffset = 0U;
    slot.m_BoundSources[uiLod]         = {identity};
    ++source->m_uiReferenceCount;
    committed = true;
    return AdmissionResult::Admitted;
  }


  // Deformable and mutable geometry retain one direct source topology. It is
  // shared by every authored selection range and has no independent LOD cache.
  const auto*          vertex = pVertexBuffer.Borrow();
  auto*                source = s_pState->m_SourceBindings.GetValue(vertex);
  State::SourceBinding pending;
  XII_SCOPE_EXIT(
    RetireBufferBinding(pending.m_Vertex, 0U);
    RetireBufferBinding(pending.m_Index, 0U);
    RetireBufferBinding(pending.m_DirectTopology.m_Buffer, 0U));
  if (!source)
  {
    const auto indices = mesh.GetIndexBuffer();
    if (!indices)
      return AdmissionResult::Failed;
    pending.m_Vertex = xiiGALBindlessResourceTable::RegisterBufferSRV(pVertexBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    pending.m_Index  = xiiGALBindlessResourceTable::RegisterBufferSRV(indices->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    if (!pending.m_Vertex.IsValid() || !pending.m_Index.IsValid())
      return AdmissionResult::Failed;
    if (!mesh.GetMeshlets().IsEmpty())
    {
      const auto result = s_pState->UploadDirectTopology(mesh, *pVertexBuffer->GetDevice(), pending.m_DirectTopology);
      if (result != AdmissionResult::Admitted)
        return result;
    }
    s_pState->m_uiResidentTopologyBytes += pending.m_DirectTopology.m_uiBytes;
    s_pState->m_SourceBindings.Insert(vertex, std::move(pending));
    pending.m_Vertex         = {};
    pending.m_Index          = {};
    pending.m_DirectTopology = {};
    source                   = s_pState->m_SourceBindings.GetValue(vertex);
  }
  auto& lod                          = record.m_Lods[uiLod];
  lod.m_uiVertexBufferIndex          = source->m_Vertex.m_uiIndex;
  lod.m_uiIndexBufferIndex           = source->m_Index.m_uiIndex;
  lod.m_uiMeshletTopologyBufferIndex = source->m_DirectTopology.m_Buffer.m_uiIndex;
  lod.m_uiMeshletPrimitiveByteOffset = source->m_DirectTopology.m_uiPrimitiveOffset;
  slot.m_BoundSources[uiLod]         = {vertex};
  ++source->m_uiReferenceCount;
  return AdmissionResult::Admitted;
}

void xiiGpuResidencyManager::ReleaseSourceBinding(LodBinding binding, xiiUInt64 uiLastUseFrame)
{
  if (binding.m_pSource == nullptr)
    return;

  auto* pSource = s_pState->m_SourceBindings.GetValue(binding.m_pSource);
  XII_ASSERT_DEV(pSource != nullptr && pSource->m_uiReferenceCount != 0U, "An admitted LOD must own a source lease.");
  pSource->m_uiLastUseFrame = xiiMath::Max(pSource->m_uiLastUseFrame, uiLastUseFrame);
  if (--pSource->m_uiReferenceCount == 0U)
  {
    RetireBufferBinding(pSource->m_DirectTopology.m_Buffer, pSource->m_uiLastUseFrame);
    s_pState->m_uiResidentTopologyBytes -= pSource->m_DirectTopology.m_uiBytes;
    RetireBufferBinding(pSource->m_PageTable, pSource->m_uiLastUseFrame);
    for (auto handle : pSource->m_Retiring)
      s_pState->RetirePhysicalPage(handle, false, pSource->m_uiLastUseFrame);
    s_pState->m_uiResidentTopologyBytes -= pSource->m_uiRetiringBytes;
    s_pState->m_uiGeometryPageCount -= pSource->m_Retiring.GetCount();
    if (!pSource->m_Pages.IsEmpty())
    {
      xiiResourceLock<xiiMeshBufferResource> mesh(pSource->m_hMesh, xiiResourceAcquireMode::PointerOnly);
      xiiUInt32                              pageIndex = 0U;
      for (const auto& page : pSource->m_Pages)
      {
        if (page.m_Buffer.IsValid() && mesh.IsValid())
        {
          s_pState->m_uiResidentTopologyBytes -= page.m_uiBytes;
          --s_pState->m_uiGeometryPageCount;
        }
        ++pageIndex;
        s_pState->ReleasePageRequest(page.m_uiRequest);
        s_pState->RetirePhysicalPage(page.m_Buffer, false, pSource->m_uiLastUseFrame);
      }
    }
    RetireBufferBinding(pSource->m_Vertex, pSource->m_uiLastUseFrame);
    RetireBufferBinding(pSource->m_Index, pSource->m_uiLastUseFrame);
    s_pState->m_SourceBindings.Remove(binding.m_pSource);
  }
}

void xiiGpuResidencyManager::ReleaseBindlessResources(Slot& slot, xiiUInt64 uiLastUseFrame)
{
  for (xiiUInt32 uiLod = 0U; uiLod < xiiGpuGeometryRecord::s_uiMaxLods; ++uiLod)
  {
    ReleaseSourceBinding(std::exchange(slot.m_BoundSources[uiLod], LodBinding{}), uiLastUseFrame);

    xiiGpuGeometryLod& lod             = slot.m_GpuRecord.m_Lods[uiLod];
    lod.m_uiVertexBufferIndex          = xiiInvalidIndex;
    lod.m_uiIndexBufferIndex           = xiiInvalidIndex;
    lod.m_uiMeshletTopologyBufferIndex = xiiInvalidIndex;
    lod.m_uiMeshletPrimitiveByteOffset = 0U;
  }
}

void xiiGpuResidencyManager::EnforceBudget(xiiUInt64 uiCompletedFrame, xiiUInt64 uiRequiredBytes)
{
  if (uiRequiredBytes > s_pState->m_uiBudgetBytes)
    return;
  while (s_pState->m_uiResidentBytes > s_pState->m_uiBudgetBytes - uiRequiredBytes)
    if (!EvictOne(uiCompletedFrame))
      break;
}

bool xiiGpuResidencyManager::EvictOne(xiiUInt64 uiCompletedFrame, xiiUInt64 uiMinimumIdleFrames)
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
    const auto  index     = candidates[s_pState->m_uiNextEviction];
    const auto& candidate = s_pState->m_Slots[index];
    if (!candidate.m_bAllocated || candidate.m_State != xiiGeometryResidencyState::Resident || candidate.m_uiLastUsedFrame > uiCompletedFrame || candidate.m_uiLastUsedFrame >= s_pState->m_uiStreamingFrame || candidate.m_uiAdmissionFrame == s_pState->m_uiStreamingFrame)
    {
      ++s_pState->m_uiNextEviction;
      continue;
    }
    // Leave younger candidates available for ordinary arena eviction later this frame.
    if (s_pState->m_uiStreamingFrame - candidate.m_uiLastUsedFrame < uiMinimumIdleFrames)
      return false;
    ++s_pState->m_uiNextEviction;
    auto& victim = s_pState->m_Slots[index];
    ++s_pState->m_StreamingStats.m_uiEvictions;
    s_pState->m_StreamingStats.m_uiReclaimedArenaBytes += victim.m_uiResidentBytes;
    s_pState->m_uiResidentBytes -= victim.m_uiResidentBytes;
    victim.m_uiResidentBytes                = 0U;
    victim.m_GpuRecord.m_uiAvailableLodMask = 0U;
    ReleaseBindlessResources(victim, victim.m_uiLastUsedFrame);
    ReleaseMeshletAllocations(victim);
    victim.m_State            = xiiGeometryResidencyState::Unloaded;
    victim.m_uiDirtyFrameMask = s_pState->m_uiAllFrameMask;
    return true;
  }
  return false;
}

void xiiGpuResidencyManager::ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes)
{
  if (!IsInitialized())
    return;

  XII_PROFILE_SCOPE("Geometry streaming");
  const auto start     = xiiTime::Now();
  const bool bNewFrame = s_pState->m_uiStreamingFrame != uiFrameIndex;
  if (bNewFrame)
  {
    s_pState->m_uiStreamingFrame      = uiFrameIndex;
    s_pState->m_uiInitialUploadBudget = s_pState->m_uiRemainingUploadBudget = uiUploadBudgetBytes;
    s_pState->m_uiRemainingTextureUploadBytes                               = xiiUInt64(xiiMath::Max(0, cvar_TextureStreamingUploadBudget.GetValue())) * 1024U * 1024U;
    s_pState->CollectPhysicalPages(uiFrameIndex, uiCompletedFrame);
    s_pState->m_StreamingStats = {};
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
  if (bNewFrame && uiFrameIndex % s_uiPressureTrimInterval == 0U && s_pState->m_uiResidentTopologyBytes != 0U &&
      s_pState->m_pMetadataBuffer->GetDevice()->IsStreamingMemoryPressured())
  {
    xiiUInt32 uiRemaining = s_uiPressureMaxEvictions;
    for (; uiRemaining != 0U && EvictOne(uiCompletedFrame, s_uiPressureMinimumIdleFrames); --uiRemaining)
      ++s_pState->m_StreamingStats.m_uiPressureEvictions;
  }
  if (bNewFrame)
    for (auto index : s_pState->m_ActiveSlots)
    {
      auto& slot = s_pState->m_Slots[index];
      for (xiiUInt32 lod = 0U; lod < slot.m_GpuRecord.m_uiLodCount; ++lod)
        if (slot.m_GpuRecord.m_Lods[lod].m_uiPadding != 0U)
        {
          slot.m_GpuRecord.m_Lods[lod].m_uiPadding = 1U | (static_cast<xiiUInt32>(uiFrameIndex % s_pState->m_uiFramesInFlight) << 1U);
          slot.m_uiDirtyFrameMask |= xiiUInt64(1U) << (uiFrameIndex % s_pState->m_uiFramesInFlight);
        }
    }
  if (bNewFrame)
    s_pState->TrimPages(uiFrameIndex, uiCompletedFrame);
  s_pState->ProcessTextures(uiFrameIndex, uiCompletedFrame);
  s_pState->ProcessPages(uiFrameIndex, uiCompletedFrame);
  if (bNewFrame)
    s_pState->SchedulePages(uiFrameIndex);
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
  xiiStats::SetStat("Renderer/Geometry/Admitted upload bytes", s_pState->m_StreamingStats.m_uiAdmittedBytes);
  xiiStats::SetStat("Renderer/Geometry/Prepared meshlets", s_pState->m_StreamingStats.m_uiPreparedMeshlets);
  xiiStats::SetStat("Renderer/Geometry/Arena meshlets", s_pState->m_uiArenaMeshlets);
  xiiStats::SetStat("Renderer/Geometry/Occupied arena bytes", s_pState->m_uiResidentBytes);
  xiiStats::SetStat("Renderer/Geometry/Reclaimed arena bytes", s_pState->m_StreamingStats.m_uiReclaimedArenaBytes);
  xiiStats::SetStat("Renderer/Geometry/Residency evictions", s_pState->m_StreamingStats.m_uiEvictions);
  xiiStats::SetStat("Renderer/Geometry/Pressure evictions", s_pState->m_StreamingStats.m_uiPressureEvictions);
  xiiStats::SetStat("Renderer/Geometry/Resident topology bytes", s_pState->m_uiResidentTopologyBytes);
  xiiStats::SetStat("Renderer/Geometry/Topology upload bytes", s_pState->m_StreamingStats.m_uiTopologyUploadBytes);
  xiiStats::SetStat("Renderer/Geometry/Retired topology bytes", s_pState->m_StreamingStats.m_uiRetiredTopologyBytes);
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

bool xiiGpuResidencyManager::IsValid(xiiGeometryHandle handle)
{
  return s_pState != nullptr && s_pState->m_GeometryIds.Contains(handle.GetInternalID());
}

xiiEnum<xiiGeometryResidencyState> xiiGpuResidencyManager::GetState(xiiGeometryHandle handle)
{
  if (IsValid(handle))
    return s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_State;
  return xiiEnum<xiiGeometryResidencyState>(xiiGeometryResidencyState::Unloaded);
}

const xiiGpuGeometryRecord* xiiGpuResidencyManager::GetGpuRecord(xiiGeometryHandle handle)
{
  return IsValid(handle) ? &s_pState->m_Slots[handle.GetInternalID().m_InstanceIndex].m_GpuRecord : nullptr;
}

xiiResult xiiGpuResidencyManager::GetPrimitiveRangeMeshletCounts(xiiGeometryHandle handle, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount, xiiArrayPtr<xiiUInt32> out_counts)
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

xiiUInt32 xiiGpuResidencyManager::GetMetadataBaseIndex(xiiUInt64 frameIndex)
{
  return s_pState && s_pState->m_uiFramesInFlight ? static_cast<xiiUInt32>(frameIndex % s_pState->m_uiFramesInFlight) * s_pState->m_Slots.GetCount() : 0;
}

xiiSharedPtr<xiiGALBuffer> xiiGpuResidencyManager::GetMetadataBuffer()
{
  return s_pState != nullptr ? s_pState->m_pMetadataBuffer : nullptr;
}

xiiSharedPtr<xiiGALBuffer> xiiGpuResidencyManager::GetMeshletMetadataBuffer()
{
  return s_pState != nullptr ? s_pState->m_pMeshletMetadataBuffer : nullptr;
}

xiiGpuResidencyStats xiiGpuResidencyManager::GetStats()
{
  xiiGpuResidencyStats stats;
  if (s_pState == nullptr)
    return stats;

  stats                                 = s_pState->m_StreamingStats;
  stats.m_uiLogicalPageCount            = s_pState->m_PageRequests.GetCount();
  stats.m_uiRequestIdHighWater          = s_pState->m_uiNextPageRequest - 1U;
  stats.m_uiRetiredRequestIds           = s_pState->m_RetiredPageRequests.GetCount();
  stats.m_uiRequestIdReuses             = s_pState->m_uiRequestIdReuses;
  stats.m_uiGeometryPageBytes           = s_pState->m_uiResidentTopologyBytes;
  stats.m_uiTexturePageBytes            = s_pState->m_uiTextureBytes;
  stats.m_uiGeometryPages               = s_pState->m_uiGeometryPageCount;
  stats.m_uiTexturePages                = s_pState->m_uiTexturePageCount;
  stats.m_uiGeometryPageRequests        = s_pState->m_uiGeometryPageRequests;
  stats.m_uiGeometryPageUploads         = s_pState->m_uiGeometryPageUploads;
  stats.m_uiPendingPageRequests         = s_pState->m_PageQueue.GetCount();
  stats.m_uiPendingPageIO               = s_pState->m_uiPendingIo;
  stats.m_uiPageRequests                = s_pState->m_uiPageRequests;
  stats.m_uiPageMisses                  = s_pState->m_uiPageMisses;
  stats.m_uiRemainingUploadBytes        = s_pState->m_uiRemainingUploadBudget;
  stats.m_uiRemainingTextureUploadBytes = s_pState->m_uiRemainingTextureUploadBytes;
  stats.m_uiPoolBackingBytes            = s_pState->m_uiPoolBytes[0] + s_pState->m_uiPoolBytes[1];
  stats.m_uiPoolReusableBytes           = s_pState->m_uiPoolReusableBytes;
  stats.m_uiPoolRetiringBytes           = s_pState->m_uiPoolRetiringBytes;
  stats.m_uiPoolReuses                  = s_pState->m_uiPoolReuses;
  stats.m_uiPoolAllocations             = s_pState->m_uiPoolAllocations;
  stats.m_uiPoolReleasedBytes           = s_pState->m_uiPoolReleasedBytes;
  stats.m_uiArenaMeshlets               = s_pState->m_uiArenaMeshlets;
  stats.m_uiCoverageCacheHits           = s_pState->m_uiCoverageCacheHits;
  stats.m_uiCoverageQueries             = s_pState->m_uiCoverageQueries;
  stats.m_uiCoverageIndexBuilds         = s_pState->m_uiCoverageIndexBuilds;
  stats.m_uiCoverageMeshlets            = s_pState->m_uiCoverageMeshlets;
  stats.m_uiBudgetBytes                 = s_pState->m_uiBudgetBytes;
  stats.m_uiResidentBytes               = s_pState->m_uiResidentBytes;
  stats.m_uiResidentTopologyBytes       = s_pState->m_uiResidentTopologyBytes;
  stats.m_uiUploadedBytes               = s_pState->m_uiLastUploadedBytes;
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

xiiGpuResidencyManager::UploadHandles xiiGpuResidencyManager::AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
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
        if (slot.m_GpuRecord.m_uiAvailableLodMask & XII_BIT(lod))
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
  // New metadata can expose topology copied on the graphics queue. Forward its
  // signal through the metadata dependency to every queue consuming this graph.
  const xiiUInt64 uiTopologyWait = pState->m_uiTopologyUploadValue != 0U &&
      (!snapshot.m_Uploads.IsEmpty() || pState->m_pTopologyUploadFence->GetCompletedValue() < pState->m_uiTopologyUploadValue) ?
    pState->m_uiTopologyUploadValue :
    0U;
  const auto      epoch          = pState->m_uiEpoch;
  const auto      retainedState  = s_pState;
  auto            pass           = graph.AddPass<UploadPassData>("Geometry Metadata Upload", xiiGALCommandQueueFlags::Transfer, [pState, &snapshot, uiTopologyWait](UploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_Uploads = std::move(snapshot.m_Uploads);
      data.m_Records = std::move(snapshot.m_Records);
      data.m_MeshletUploads = std::move(snapshot.m_MeshletUploads);
      data.m_hGeometryBuffer = builder.ImportBuffer("GPU Geometry Metadata", pState->m_pMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      if (uiTopologyWait != 0U)
        XII_VERIFY(builder.WaitForFence(pState->m_pTopologyUploadFence, uiTopologyWait).Succeeded(), "Topology uploads use a valid GPU fence.");
      if (!data.m_Uploads.IsEmpty() || uiTopologyWait != 0U) data.m_hGeometryBuffer = builder.WriteBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::ShaderResource);
      data.m_hMeshletBuffer = builder.ImportBuffer("GPU Meshlet Metadata", pState->m_pMeshletMetadataBuffer, xiiGALResourceStateFlags::ShaderResource);
      if (!data.m_MeshletUploads.IsEmpty()) data.m_hMeshletBuffer = builder.WriteBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hMeshletBuffer, xiiGALResourceStateFlags::ShaderResource);
      builder.SetPassSideEffects(!data.m_Uploads.IsEmpty() || !data.m_MeshletUploads.IsEmpty() || uiTopologyWait != 0U);
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

bool xiiGpuResidencyManager::AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset)
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

void xiiGpuResidencyManager::FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount)
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

void xiiGpuResidencyManager::ReleaseMeshletAllocations(Slot& slot)
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Streaming_Implementation_GpuResidencyManager);
