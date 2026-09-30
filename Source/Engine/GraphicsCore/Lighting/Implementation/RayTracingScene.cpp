/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BottomLevelAS.h>
#include <GraphicsFoundation/Resources/Buffer.h>

struct xiiRayTracingSceneManager::GeometrySlot
{
  xiiRayTracingGeometryDescription m_Description;
  xiiSharedPtr<xiiGALBuffer>        m_pVertexBuffer;
  xiiSharedPtr<xiiGALBuffer>        m_pIndexBuffer;
  xiiSharedPtr<xiiGALBuffer>        m_pScratchBuffer;
  xiiSharedPtr<xiiGALBottomLevelAS> m_pBottomLevelAS;
  xiiUInt64                         m_uiVertexBufferOffset = 0U;
  xiiUInt64                         m_uiVertexStride       = 0U;
  xiiUInt32                         m_uiPrimitiveCount     = 0U;
  xiiUInt32                         m_uiGeneration         = 1U;
  bool                              m_bAllocated           = false;
  bool                              m_bBLASDirty           = true;
};

struct xiiRayTracingSceneManager::InstanceSlot
{
  xiiRayTracingInstanceDescription m_Description;
  xiiUInt32                         m_uiGeneration = 1U;
  bool                              m_bAllocated  = false;
};

struct xiiRayTracingSceneManager::FrameResources
{
  xiiSharedPtr<xiiGALTopLevelAS> m_pTopLevelAS;
  xiiSharedPtr<xiiGALBuffer>     m_pInstanceBuffer;
  xiiSharedPtr<xiiGALBuffer>     m_pScratchBuffer;
  xiiUInt64                      m_uiBuiltRevision = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32                      m_uiBuiltInstanceCount = 0U;
  bool                           m_bReady          = false;
};

class xiiRayTracingSceneManager::State
{
public:
  void ClearScene()
  {
    m_Geometries.Clear();
    m_Instances.Clear();
    m_FreeGeometries.Clear();
    m_FreeInstances.Clear();
    m_Frames.Clear();
    m_uiGeometryCount = 0U;
    m_uiInstanceCount = 0U;
    ++m_uiSceneRevision;
    m_bInitialized = false;
  }

  xiiDynamicArray<GeometrySlot> m_Geometries;
  xiiDynamicArray<InstanceSlot> m_Instances;
  xiiDynamicArray<xiiUInt32>    m_FreeGeometries;
  xiiDynamicArray<xiiUInt32>    m_FreeInstances;
  xiiDynamicArray<FrameResources> m_Frames;
  xiiRayTracingSceneDescription m_Configuration;
  xiiUInt64                      m_uiSceneRevision = 0U;
  xiiUInt32                      m_uiGeometryCount = 0U;
  xiiUInt32                      m_uiInstanceCount = 0U;
  bool                           m_bEngineStarted = false;
  bool                           m_bHardwareSupported = false;
  bool                           m_bInitialized = false;
};

namespace
{
  struct RayTracingBLASBuild
  {
    xiiUInt32                         m_uiGeometryIndex      = xiiInvalidIndex;
    xiiUInt32                         m_uiGeometryGeneration = 0U;
    xiiSharedPtr<xiiGALBottomLevelAS> m_pBottomLevelAS;
    xiiSharedPtr<xiiGALBuffer>        m_pVertexBuffer;
    xiiSharedPtr<xiiGALBuffer>        m_pIndexBuffer;
    xiiSharedPtr<xiiGALBuffer>        m_pScratchBuffer;
    xiiUInt64                         m_uiVertexBufferOffset = 0U;
    xiiUInt64                         m_uiVertexStride       = 0U;
    xiiUInt32                         m_uiPrimitiveCount     = 0U;
    xiiRenderGraphBufferHandle        m_hVertexBuffer;
    xiiRenderGraphBufferHandle        m_hIndexBuffer;
    xiiRenderGraphBufferHandle        m_hScratchBuffer;
  };

  struct RayTracingSceneBuildPassData
  {
    xiiDynamicArray<RayTracingBLASBuild> m_BLASBuilds;
    xiiDynamicArray<xiiGALTLASInstanceData, xiiAlignedAllocatorWrapper> m_Instances;
    xiiSharedPtr<xiiGALTopLevelAS>       m_pTopLevelAS;
    xiiSharedPtr<xiiGALBuffer>           m_pInstanceBuffer;
    xiiSharedPtr<xiiGALBuffer>           m_pTLASScratchBuffer;
    xiiRenderGraphBufferHandle           m_hInstanceBuffer;
    xiiRenderGraphBufferHandle           m_hTLASScratchBuffer;
    xiiUInt64                            m_uiSceneRevision = 0U;
    xiiUInt32                            m_uiFrameSlot     = 0U;
    bool                                 m_bUpdateTLAS     = false;
  };
}

xiiUniquePtr<xiiRayTracingSceneManager::State> xiiRayTracingSceneManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RayTracingSceneManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiRayTracingSceneManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRayTracingSceneManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRayTracingSceneManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRayTracingSceneManager::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingGeometryHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingGeometryHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingInstanceHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingInstanceHandle>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Index", m_uiIndex),
    XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingGeometryDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingGeometryDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MeshBuffer", GetMeshBufferResourceId, SetMeshBufferResourceId),
    XII_MEMBER_PROPERTY("Opaque", m_bOpaque),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingInstanceDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingInstanceDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Geometry", m_hGeometry),
    XII_MEMBER_PROPERTY("Transform", m_Transform),
    XII_MEMBER_PROPERTY("StableObjectId", m_uiStableObjectId),
    XII_MEMBER_PROPERTY("VisibilityMask", m_uiVisibilityMask),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiGALRayTracingInstanceFlags, m_Flags),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingSceneDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingSceneDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxGeometries", m_uiMaxGeometries)->AddAttributes(new xiiDefaultValueAttribute(16384U), new xiiClampValueAttribute(1U, 1048576U)),
    XII_MEMBER_PROPERTY("MaxInstances", m_uiMaxInstances)->AddAttributes(new xiiDefaultValueAttribute(65536U), new xiiClampValueAttribute(1U, 16777215U)),
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(1U, 64U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingSceneStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingSceneStats>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GeometryCount", m_uiGeometryCount),
    XII_MEMBER_PROPERTY("InstanceCount", m_uiInstanceCount),
    XII_MEMBER_PROPERTY("PendingBLASCount", m_uiPendingBLASCount),
    XII_MEMBER_PROPERTY("SceneRevision", m_uiSceneRevision),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

xiiString xiiRayTracingGeometryDescription::GetMeshBufferResourceId() const
{
  return m_hMeshBuffer.IsValid() ? xiiString(m_hMeshBuffer.GetResourceID()) : xiiString();
}

void xiiRayTracingGeometryDescription::SetMeshBufferResourceId(xiiString sResourceId)
{
  m_hMeshBuffer = sResourceId.IsEmpty() ? xiiMeshBufferResourceHandle() : xiiResourceManager::LoadResource<xiiMeshBufferResource>(sResourceId);
}

void xiiRayTracingSceneManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Ray-tracing scene manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  ApplyConfiguration().IgnoreResult();
}

void xiiRayTracingSceneManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede ray-tracing scene engine startup.");
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted = true;
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  s_pState->m_bHardwareSupported = pDevice != nullptr && pDevice->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled;
  if (!s_pState->m_bInitialized)
    ApplyConfiguration().IgnoreResult();
}

void xiiRayTracingSceneManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  // Future BLAS/TLAS objects live in these slots and therefore release before the GAL device.
  s_pState->ClearScene();
  s_pState->m_bHardwareSupported = false;
  s_pState->m_bEngineStarted = false;
}

void xiiRayTracingSceneManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiRayTracingSceneManager::ApplyConfiguration()
{
  if (s_pState == nullptr || s_pState->m_Configuration.m_uiMaxGeometries == 0U || s_pState->m_Configuration.m_uiMaxInstances == 0U || s_pState->m_Configuration.m_uiFramesInFlight == 0U || s_pState->m_Configuration.m_uiFramesInFlight > 64U)
    return XII_FAILURE;

  s_pState->ClearScene();
  s_pState->m_Geometries.SetCount(s_pState->m_Configuration.m_uiMaxGeometries);
  s_pState->m_Instances.SetCount(s_pState->m_Configuration.m_uiMaxInstances);
  s_pState->m_Frames.SetCount(s_pState->m_Configuration.m_uiFramesInFlight);
  s_pState->m_FreeGeometries.Reserve(s_pState->m_Configuration.m_uiMaxGeometries);
  s_pState->m_FreeInstances.Reserve(s_pState->m_Configuration.m_uiMaxInstances);
  for (xiiUInt32 i = s_pState->m_Configuration.m_uiMaxGeometries; i > 0U; --i)
    s_pState->m_FreeGeometries.PushBack(i - 1U);
  for (xiiUInt32 i = s_pState->m_Configuration.m_uiMaxInstances; i > 0U; --i)
    s_pState->m_FreeInstances.PushBack(i - 1U);
  s_pState->m_bInitialized = true;
  return XII_SUCCESS;
}

xiiResult xiiRayTracingSceneManager::Configure(const xiiRayTracingSceneDescription& description)
{
  if (s_pState == nullptr || description.m_uiMaxGeometries == 0U || description.m_uiMaxInstances == 0U || description.m_uiFramesInFlight == 0U || description.m_uiFramesInFlight > 64U)
    return XII_FAILURE;
  if (s_pState->m_uiGeometryCount != 0U || s_pState->m_uiInstanceCount != 0U)
    return XII_FAILURE;

  s_pState->m_Configuration = description;
  return ApplyConfiguration();
}

const xiiRayTracingSceneDescription& xiiRayTracingSceneManager::GetConfiguration()
{
  static const xiiRayTracingSceneDescription s_Default;
  return s_pState != nullptr ? s_pState->m_Configuration : s_Default;
}

bool xiiRayTracingSceneManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

bool xiiRayTracingSceneManager::IsHardwareRayTracingSupported()
{
  return s_pState != nullptr && s_pState->m_bHardwareSupported;
}

xiiRayTracingGeometryHandle xiiRayTracingSceneManager::RegisterGeometry(const xiiRayTracingGeometryDescription& description)
{
  if (!IsInitialized() || !description.m_hMeshBuffer.IsValid() || s_pState->m_FreeGeometries.IsEmpty())
    return {};

  const xiiUInt32 uiIndex = s_pState->m_FreeGeometries.PeekBack();
  s_pState->m_FreeGeometries.PopBack();
  GeometrySlot& slot = s_pState->m_Geometries[uiIndex];
  slot.m_Description = description;
  slot.m_bAllocated = true;
  slot.m_bBLASDirty = true;
  ++s_pState->m_uiGeometryCount;
  ++s_pState->m_uiSceneRevision;
  return {uiIndex, slot.m_uiGeneration};
}

void xiiRayTracingSceneManager::UnregisterGeometry(xiiRayTracingGeometryHandle handle)
{
  if (!IsValid(handle))
    return;

  // Instances retain a logical dependency on their geometry. Remove them first so a stale
  // handle can never be serialized into a TLAS frame slice.
  for (xiiUInt32 i = 0U; i < s_pState->m_Instances.GetCount(); ++i)
  {
    InstanceSlot& instance = s_pState->m_Instances[i];
    if (instance.m_bAllocated && instance.m_Description.m_hGeometry.m_uiIndex == handle.m_uiIndex && instance.m_Description.m_hGeometry.m_uiGeneration == handle.m_uiGeneration)
      DestroyInstance({i, instance.m_uiGeneration});
  }

  GeometrySlot& slot = s_pState->m_Geometries[handle.m_uiIndex];
  slot.m_pBottomLevelAS.Clear();
  slot.m_pScratchBuffer.Clear();
  slot.m_pVertexBuffer.Clear();
  slot.m_pIndexBuffer.Clear();
  slot.m_Description = {};
  slot.m_bAllocated = false;
  slot.m_bBLASDirty = true;
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeGeometries.PushBack(handle.m_uiIndex);
  --s_pState->m_uiGeometryCount;
  ++s_pState->m_uiSceneRevision;
}

bool xiiRayTracingSceneManager::IsValid(xiiRayTracingGeometryHandle handle)
{
  return IsInitialized() && handle.IsValid() && handle.m_uiIndex < s_pState->m_Geometries.GetCount() && s_pState->m_Geometries[handle.m_uiIndex].m_bAllocated && s_pState->m_Geometries[handle.m_uiIndex].m_uiGeneration == handle.m_uiGeneration;
}

xiiRayTracingInstanceHandle xiiRayTracingSceneManager::CreateInstance(const xiiRayTracingInstanceDescription& description)
{
  if (!IsValid(description.m_hGeometry) || s_pState->m_FreeInstances.IsEmpty() || !description.m_Transform.IsValid())
    return {};

  const xiiUInt32 uiIndex = s_pState->m_FreeInstances.PeekBack();
  s_pState->m_FreeInstances.PopBack();
  InstanceSlot& slot = s_pState->m_Instances[uiIndex];
  slot.m_Description = description;
  slot.m_bAllocated = true;
  ++s_pState->m_uiInstanceCount;
  ++s_pState->m_uiSceneRevision;
  return {uiIndex, slot.m_uiGeneration};
}

void xiiRayTracingSceneManager::DestroyInstance(xiiRayTracingInstanceHandle handle)
{
  if (!IsValid(handle))
    return;

  InstanceSlot& slot = s_pState->m_Instances[handle.m_uiIndex];
  slot.m_Description = {};
  slot.m_bAllocated = false;
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeInstances.PushBack(handle.m_uiIndex);
  --s_pState->m_uiInstanceCount;
  ++s_pState->m_uiSceneRevision;
}

bool xiiRayTracingSceneManager::UpdateInstance(xiiRayTracingInstanceHandle handle, const xiiRayTracingInstanceDescription& description)
{
  if (!IsValid(handle) || !IsValid(description.m_hGeometry) || !description.m_Transform.IsValid())
    return false;

  s_pState->m_Instances[handle.m_uiIndex].m_Description = description;
  ++s_pState->m_uiSceneRevision;
  return true;
}

bool xiiRayTracingSceneManager::IsValid(xiiRayTracingInstanceHandle handle)
{
  return IsInitialized() && handle.IsValid() && handle.m_uiIndex < s_pState->m_Instances.GetCount() && s_pState->m_Instances[handle.m_uiIndex].m_bAllocated && s_pState->m_Instances[handle.m_uiIndex].m_uiGeneration == handle.m_uiGeneration;
}

bool xiiRayTracingSceneManager::PrepareGeometry(xiiUInt32 uiGeometryIndex)
{
  if (!IsHardwareRayTracingSupported() || uiGeometryIndex >= s_pState->m_Geometries.GetCount())
    return false;

  GeometrySlot& slot = s_pState->m_Geometries[uiGeometryIndex];
  if (!slot.m_bAllocated)
    return false;
  if (slot.m_pBottomLevelAS != nullptr)
    return true;

  xiiResourceLock<xiiMeshBufferResource> mesh(slot.m_Description.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
  if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final || mesh->GetTopology() != xiiGALPrimitiveTopology::TriangleList || mesh->GetVertexCount() == 0U || mesh->GetPrimitiveCount() == 0U)
    return false;

  const xiiMeshVertexStream* pPositionStream = nullptr;
  for (const xiiMeshVertexStream& stream : mesh->GetVertexStreams())
  {
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position)
    {
      pPositionStream = &stream;
      break;
    }
  }
  if (pPositionStream == nullptr || pPositionStream->m_Format != xiiMeshVertexStreamFormat::Float3 || pPositionStream->m_uiStride == 0U)
    return false;

  xiiSharedPtr<xiiGALBuffer> pVertexBuffer = mesh->GetVertexBuffer();
  xiiSharedPtr<xiiGALBuffer> pIndexBuffer  = mesh->GetIndexBuffer();
  if (pVertexBuffer == nullptr || (mesh->GetIndexCount() > 0U && pIndexBuffer == nullptr))
    return false;

  xiiGALBottomLevelASCreationDescription blasDescription;
  blasDescription.m_BuildASFlags = xiiGALRayTracingBuildASFlags::PreferFastTrace;
  xiiGALBLASTriangleDescription& triangle = blasDescription.m_Triangles.ExpandAndGetRef();
  triangle.m_sGeometryName        = "Mesh";
  triangle.m_uiMaxVertexCount     = mesh->GetVertexCount();
  triangle.m_VertexValueType      = xiiGALValueType::Float32;
  triangle.m_uiVertexComponentCount = 3U;
  triangle.m_uiMaxPrimitiveCount  = mesh->GetPrimitiveCount();
  triangle.m_IndexType            = xiiGALValueType::Undefined;
  if (mesh->GetIndexCount() > 0U)
    triangle.m_IndexType = mesh->GetIndexType();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return false;

  xiiSharedPtr<xiiGALBottomLevelAS> pBottomLevelAS = pDevice->CreateBottomLevelAS(blasDescription);
  if (pBottomLevelAS == nullptr || pBottomLevelAS->GetScratchBufferSizeDescription().m_uiBuild == 0U)
    return false;

  const xiiUInt32 uiScratchAlignment = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiScratchBufferAlignment);
  xiiGALBufferCreationDescription scratchDescription;
  scratchDescription.m_uiSize    = xiiMemoryUtils::AlignSize(pBottomLevelAS->GetScratchBufferSizeDescription().m_uiBuild, static_cast<xiiUInt64>(uiScratchAlignment));
  scratchDescription.m_BindFlags = xiiGALBindFlags::RayTracing;
  scratchDescription.m_Usage     = xiiGALResourceUsage::Mutable;
  scratchDescription.m_Mode      = xiiGALBufferMode::Raw;
  xiiSharedPtr<xiiGALBuffer> pScratchBuffer = pDevice->CreateBuffer(scratchDescription);
  if (pScratchBuffer == nullptr)
    return false;

  xiiStringBuilder debugName;
  debugName.SetFormat("Ray Tracing BLAS [{}]", uiGeometryIndex);
  pBottomLevelAS->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing BLAS Scratch [{}]", uiGeometryIndex);
  pScratchBuffer->SetDebugName(debugName);

  slot.m_pVertexBuffer        = std::move(pVertexBuffer);
  slot.m_pIndexBuffer         = std::move(pIndexBuffer);
  slot.m_pBottomLevelAS       = std::move(pBottomLevelAS);
  slot.m_pScratchBuffer       = std::move(pScratchBuffer);
  slot.m_uiVertexBufferOffset = pPositionStream->m_uiOffset;
  slot.m_uiVertexStride       = pPositionStream->m_uiStride;
  slot.m_uiPrimitiveCount     = mesh->GetPrimitiveCount();
  slot.m_bBLASDirty           = true;
  return true;
}

bool xiiRayTracingSceneManager::PrepareFrameResources(xiiUInt32 uiFrameSlot)
{
  if (!IsHardwareRayTracingSupported() || uiFrameSlot >= s_pState->m_Frames.GetCount())
    return false;

  FrameResources& frame = s_pState->m_Frames[uiFrameSlot];
  if (frame.m_pTopLevelAS != nullptr)
    return true;

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return false;

  xiiGALTopLevelASCreationDescription tlasDescription;
  tlasDescription.m_uiMaxInstanceCount = s_pState->m_Configuration.m_uiMaxInstances;
  tlasDescription.m_Flags = xiiGALRayTracingBuildASFlags::AllowUpdate | xiiGALRayTracingBuildASFlags::PreferFastTrace;
  frame.m_pTopLevelAS = pDevice->CreateTopLevelAS(tlasDescription);
  if (frame.m_pTopLevelAS == nullptr)
    return false;

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiSize              = static_cast<xiiUInt64>(s_pState->m_Configuration.m_uiMaxInstances) * sizeof(xiiGALTLASInstanceData);
  bufferDescription.m_uiElementByteStride = sizeof(xiiGALTLASInstanceData);
  bufferDescription.m_BindFlags           = xiiGALBindFlags::RayTracing;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  frame.m_pInstanceBuffer = pDevice->CreateBuffer(bufferDescription);

  const xiiGALScratchBufferSizeDescription scratchSizes = frame.m_pTopLevelAS->GetScratchBufferSizeDescription();
  const xiiUInt32 uiScratchAlignment = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiScratchBufferAlignment);
  bufferDescription                    = {};
  bufferDescription.m_uiSize           = xiiMemoryUtils::AlignSize(xiiMath::Max(scratchSizes.m_uiBuild, scratchSizes.m_uiUpdate), static_cast<xiiUInt64>(uiScratchAlignment));
  bufferDescription.m_BindFlags        = xiiGALBindFlags::RayTracing;
  bufferDescription.m_Usage            = xiiGALResourceUsage::Mutable;
  bufferDescription.m_Mode             = xiiGALBufferMode::Raw;
  frame.m_pScratchBuffer = pDevice->CreateBuffer(bufferDescription);
  if (frame.m_pInstanceBuffer == nullptr || frame.m_pScratchBuffer == nullptr)
  {
    frame = {};
    return false;
  }

  xiiStringBuilder debugName;
  debugName.SetFormat("Ray Tracing TLAS [{}]", uiFrameSlot);
  frame.m_pTopLevelAS->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing Instance Data [{}]", uiFrameSlot);
  frame.m_pInstanceBuffer->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing TLAS Scratch [{}]", uiFrameSlot);
  frame.m_pScratchBuffer->SetDebugName(debugName);
  return true;
}

xiiRayTracingSceneManager::BuildHandles xiiRayTracingSceneManager::AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  BuildHandles result;
  if (!IsInitialized() || !IsHardwareRayTracingSupported() || s_pState->m_uiInstanceCount == 0U)
    return result;

  xiiDynamicArray<RayTracingBLASBuild> pendingBLASBuilds;
  xiiDynamicArray<xiiGALTLASInstanceData, xiiAlignedAllocatorWrapper> instanceData;
  instanceData.Reserve(s_pState->m_uiInstanceCount);

  for (xiiUInt32 uiGeometryIndex = 0U; uiGeometryIndex < s_pState->m_Geometries.GetCount(); ++uiGeometryIndex)
  {
    GeometrySlot& geometry = s_pState->m_Geometries[uiGeometryIndex];
    if (!geometry.m_bAllocated || !PrepareGeometry(uiGeometryIndex) || !geometry.m_bBLASDirty)
      continue;

    RayTracingBLASBuild& build = pendingBLASBuilds.ExpandAndGetRef();
    build.m_uiGeometryIndex      = uiGeometryIndex;
    build.m_uiGeometryGeneration = geometry.m_uiGeneration;
    build.m_pBottomLevelAS       = geometry.m_pBottomLevelAS;
    build.m_pVertexBuffer        = geometry.m_pVertexBuffer;
    build.m_pIndexBuffer         = geometry.m_pIndexBuffer;
    build.m_pScratchBuffer       = geometry.m_pScratchBuffer;
    build.m_uiVertexBufferOffset = geometry.m_uiVertexBufferOffset;
    build.m_uiVertexStride       = geometry.m_uiVertexStride;
    build.m_uiPrimitiveCount     = geometry.m_uiPrimitiveCount;
  }

  for (const InstanceSlot& instance : s_pState->m_Instances)
  {
    if (!instance.m_bAllocated || !IsValid(instance.m_Description.m_hGeometry))
      continue;

    const GeometrySlot& geometry = s_pState->m_Geometries[instance.m_Description.m_hGeometry.m_uiIndex];
    if (geometry.m_pBottomLevelAS == nullptr || geometry.m_pBottomLevelAS->GetDeviceAddress() == 0U)
      continue;

    xiiGALTLASInstanceData& gpuInstance = instanceData.ExpandAndGetRef();
    gpuInstance.SetTransform(instance.m_Description.m_Transform);
    gpuInstance.SetInstanceID(instance.m_Description.m_uiStableObjectId & 0x00FFFFFFU);
    gpuInstance.SetMask(instance.m_Description.m_uiVisibilityMask);
    gpuInstance.SetHitGroupContribution(0U);
    xiiBitflags<xiiGALRayTracingInstanceFlags> flags = instance.m_Description.m_Flags;
    if (geometry.m_Description.m_bOpaque)
    {
      flags.Remove(xiiGALRayTracingInstanceFlags::ForceNonOpaque);
      flags.Add(xiiGALRayTracingInstanceFlags::ForceOpaque);
    }
    gpuInstance.SetFlags(flags);
    gpuInstance.m_uiBottomLevelASDeviceAddress = geometry.m_pBottomLevelAS->GetDeviceAddress();
  }

  if (instanceData.IsEmpty())
    return result;

  const xiiUInt32 uiFrameSlot = static_cast<xiiUInt32>(uiFrameIndex % s_pState->m_Frames.GetCount());
  if (!PrepareFrameResources(uiFrameSlot))
    return result;

  FrameResources& frame = s_pState->m_Frames[uiFrameSlot];
  if (frame.m_bReady && frame.m_uiBuiltRevision == s_pState->m_uiSceneRevision && pendingBLASBuilds.IsEmpty())
  {
    result.m_pTopLevelAS    = frame.m_pTopLevelAS;
    result.m_uiInstanceCount = instanceData.GetCount();
    return result;
  }

  State* pState = s_pState.Borrow();
  auto pass = graph.AddPass<RayTracingSceneBuildPassData>(
    "Ray Tracing Scene Build", xiiGALCommandQueueFlags::Compute,
    [&, uiFrameSlot](RayTracingSceneBuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_BLASBuilds       = pendingBLASBuilds;
      data.m_Instances        = instanceData;
      data.m_pTopLevelAS      = frame.m_pTopLevelAS;
      data.m_pInstanceBuffer  = frame.m_pInstanceBuffer;
      data.m_pTLASScratchBuffer = frame.m_pScratchBuffer;
      data.m_uiSceneRevision  = pState->m_uiSceneRevision;
      data.m_uiFrameSlot      = uiFrameSlot;
      data.m_bUpdateTLAS      = frame.m_bReady && frame.m_uiBuiltInstanceCount == instanceData.GetCount();

      xiiStringBuilder name;
      for (RayTracingBLASBuild& build : data.m_BLASBuilds)
      {
        name.SetFormat("Ray Tracing Vertex Data [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
        build.m_hVertexBuffer = builder.ImportBuffer(name, build.m_pVertexBuffer, build.m_pVertexBuffer->GetResourceState());
        build.m_hVertexBuffer = builder.ReadBuffer(build.m_hVertexBuffer, xiiGALResourceStateFlags::BuildASRead);
        if (build.m_pIndexBuffer != nullptr)
        {
          name.SetFormat("Ray Tracing Index Data [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
          build.m_hIndexBuffer = builder.ImportBuffer(name, build.m_pIndexBuffer, build.m_pIndexBuffer->GetResourceState());
          build.m_hIndexBuffer = builder.ReadBuffer(build.m_hIndexBuffer, xiiGALResourceStateFlags::BuildASRead);
        }
        name.SetFormat("Ray Tracing BLAS Scratch [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
        build.m_hScratchBuffer = builder.ImportBuffer(name, build.m_pScratchBuffer, build.m_pScratchBuffer->GetResourceState());
        build.m_hScratchBuffer = builder.WriteBuffer(build.m_hScratchBuffer, xiiGALResourceStateFlags::BuildASWrite);
      }

      name.SetFormat("Ray Tracing Instance Data [{}]", uiFrameSlot);
      data.m_hInstanceBuffer = builder.ImportBuffer(name, data.m_pInstanceBuffer, data.m_pInstanceBuffer->GetResourceState());
      data.m_hInstanceBuffer = builder.WriteBuffer(data.m_hInstanceBuffer, xiiGALResourceStateFlags::BuildASRead);
      builder.ExportBuffer(data.m_hInstanceBuffer, xiiGALResourceStateFlags::BuildASRead);

      name.SetFormat("Ray Tracing TLAS Scratch [{}]", uiFrameSlot);
      data.m_hTLASScratchBuffer = builder.ImportBuffer(name, data.m_pTLASScratchBuffer, data.m_pTLASScratchBuffer->GetResourceState());
      data.m_hTLASScratchBuffer = builder.WriteBuffer(data.m_hTLASScratchBuffer, xiiGALResourceStateFlags::BuildASWrite);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [pState](const RayTracingSceneBuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& commandList = context.GetCommandList();
      commandList.UpdateBuffer(context.GetBuffer(data.m_hInstanceBuffer), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_Instances.GetData()), data.m_Instances.GetCount() * sizeof(xiiGALTLASInstanceData)));

      for (const RayTracingBLASBuild& build : data.m_BLASBuilds)
      {
        xiiGALBuildBLASDescription description;
        description.m_pBottomLevelAS  = build.m_pBottomLevelAS.Borrow();
        description.m_pScratchBuffer  = context.GetBuffer(build.m_hScratchBuffer);
        description.m_BuildFlags      = xiiGALRayTracingBuildASFlags::PreferFastTrace;
        xiiGALBLASTriangleBuildDescription& triangle = description.m_Triangles.ExpandAndGetRef();
        triangle.m_pVertexBuffer        = context.GetBuffer(build.m_hVertexBuffer);
        triangle.m_uiVertexBufferOffset = build.m_uiVertexBufferOffset;
        triangle.m_uiVertexStride       = build.m_uiVertexStride;
        triangle.m_pIndexBuffer         = build.m_hIndexBuffer.IsValid() ? context.GetBuffer(build.m_hIndexBuffer) : nullptr;
        triangle.m_uiPrimitiveCount     = build.m_uiPrimitiveCount;
        commandList.BuildBLAS(description);
      }

      xiiGALBuildTLASDescription tlasDescription;
      tlasDescription.m_pTopLevelAS     = data.m_pTopLevelAS.Borrow();
      tlasDescription.m_pInstanceBuffer = context.GetBuffer(data.m_hInstanceBuffer);
      tlasDescription.m_uiInstanceCount = data.m_Instances.GetCount();
      tlasDescription.m_pScratchBuffer  = context.GetBuffer(data.m_hTLASScratchBuffer);
      tlasDescription.m_BuildFlags      = xiiGALRayTracingBuildASFlags::AllowUpdate | xiiGALRayTracingBuildASFlags::PreferFastTrace;
      tlasDescription.m_bUpdate         = data.m_bUpdateTLAS;
      commandList.BuildTLAS(tlasDescription);

      for (const RayTracingBLASBuild& build : data.m_BLASBuilds)
      {
        if (build.m_uiGeometryIndex < pState->m_Geometries.GetCount())
        {
          GeometrySlot& geometry = pState->m_Geometries[build.m_uiGeometryIndex];
          if (geometry.m_bAllocated && geometry.m_uiGeneration == build.m_uiGeometryGeneration)
            geometry.m_bBLASDirty = false;
        }
      }
      if (data.m_uiFrameSlot < pState->m_Frames.GetCount())
      {
        FrameResources& frame = pState->m_Frames[data.m_uiFrameSlot];
        if (frame.m_pTopLevelAS == data.m_pTopLevelAS)
        {
          frame.m_uiBuiltRevision = data.m_uiSceneRevision;
          frame.m_uiBuiltInstanceCount = data.m_Instances.GetCount();
          frame.m_bReady = true;
        }
      }
    },
    true);

  result.m_pTopLevelAS      = frame.m_pTopLevelAS;
  result.m_hSceneDependency = pass.first->m_hInstanceBuffer;
  result.m_uiInstanceCount  = instanceData.GetCount();
  return result;
}

xiiSharedPtr<xiiGALTopLevelAS> xiiRayTracingSceneManager::GetTopLevelAS(xiiUInt64 uiFrameIndex)
{
  if (!IsInitialized() || s_pState->m_Frames.IsEmpty())
    return nullptr;
  const FrameResources& frame = s_pState->m_Frames[static_cast<xiiUInt32>(uiFrameIndex % s_pState->m_Frames.GetCount())];
  return frame.m_bReady ? frame.m_pTopLevelAS : nullptr;
}

xiiRayTracingSceneStats xiiRayTracingSceneManager::GetStats()
{
  xiiRayTracingSceneStats stats;
  if (!IsInitialized())
    return stats;

  stats.m_uiGeometryCount = s_pState->m_uiGeometryCount;
  stats.m_uiInstanceCount = s_pState->m_uiInstanceCount;
  stats.m_uiSceneRevision = s_pState->m_uiSceneRevision;
  for (const GeometrySlot& slot : s_pState->m_Geometries)
  {
    if (slot.m_bAllocated && slot.m_bBLASDirty)
      ++stats.m_uiPendingBLASCount;
  }
  return stats;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_RayTracingScene);
