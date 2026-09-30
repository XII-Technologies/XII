/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Lighting/RayTracingScene.h>
#include <GraphicsFoundation/Device/Device.h>

struct xiiRayTracingSceneManager::GeometrySlot
{
  xiiRayTracingGeometryDescription m_Description;
  xiiUInt32                         m_uiGeneration = 1U;
  bool                              m_bAllocated  = false;
  bool                              m_bBLASDirty  = true;
};

struct xiiRayTracingSceneManager::InstanceSlot
{
  xiiRayTracingInstanceDescription m_Description;
  xiiUInt32                         m_uiGeneration = 1U;
  bool                              m_bAllocated  = false;
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
    m_uiGeometryCount = 0U;
    m_uiInstanceCount = 0U;
    ++m_uiSceneRevision;
    m_bInitialized = false;
  }

  xiiDynamicArray<GeometrySlot> m_Geometries;
  xiiDynamicArray<InstanceSlot> m_Instances;
  xiiDynamicArray<xiiUInt32>    m_FreeGeometries;
  xiiDynamicArray<xiiUInt32>    m_FreeInstances;
  xiiRayTracingSceneDescription m_Configuration;
  xiiUInt64                      m_uiSceneRevision = 0U;
  xiiUInt32                      m_uiGeometryCount = 0U;
  xiiUInt32                      m_uiInstanceCount = 0U;
  bool                           m_bEngineStarted = false;
  bool                           m_bHardwareSupported = false;
  bool                           m_bInitialized = false;
};

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
