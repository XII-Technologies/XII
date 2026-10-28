/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Lighting/Implementation/RayTracingDeformation.h>
#include <GraphicsCore/Lighting/RayTracingResources.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>
#include <GraphicsFoundation/Resources/BottomLevelAS.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <Shaders/Pipeline/Passes/RayTracing/RayTracingMaterialData.h>

struct xiiRayTracingResources::GeometrySlot
{
  xiiRayTracingGeometryDescription  m_Description;
  xiiSharedPtr<xiiGALBuffer>        m_pVertexBuffer;
  xiiSharedPtr<xiiGALBuffer>        m_pIndexBuffer;
  xiiSharedPtr<xiiGALBuffer>        m_pScratchBuffer;
  xiiSharedPtr<xiiGALBottomLevelAS> m_pBottomLevelAS;
  xiiGALBindlessResourceHandle      m_hVertexBufferSRV;
  xiiGALBindlessResourceHandle      m_hIndexBufferSRV;
  xiiUInt64                         m_uiVertexBufferOffset = 0U;
  xiiUInt64                         m_uiVertexStride       = 0U;
  xiiUInt32                         m_uiNormalStride       = 0U;
  xiiUInt32                         m_uiNormalOffset       = xiiInvalidIndex;
  xiiUInt32                         m_uiTangentStride      = 0U;
  xiiUInt32                         m_uiTangentOffset      = xiiInvalidIndex;
  xiiUInt32                         m_uiTexCoordStride     = 0U;
  xiiUInt32                         m_uiTexCoordOffset     = xiiInvalidIndex;
  xiiUInt32                         m_uiIndexStride        = 0U;
  xiiUInt32                         m_uiPrimitiveCount     = 0U;
  xiiRayTracingGeometryId           m_Id;
  xiiUInt32                         m_uiReferences      = 0;
  xiiUInt32                         m_uiResourceVersion = 0;
  bool                              m_bAllocated        = false;
  bool                              m_bBLASDirty        = true;
  xiiUInt64                         m_SetupSerial       = 0;
  xiiRenderGraphBufferHandle        m_BuildDependency;
};

struct xiiRayTracingInstanceInput
{
  xiiRayTracingGeometryHandle                m_hGeometry;
  xiiMaterialGpuHandle                       m_hMaterial;
  xiiMat4                                    m_Transform        = xiiMat4::MakeIdentity();
  xiiUInt32                                  m_uiStableObjectId = 0;
  xiiUInt8                                   m_uiVisibilityMask = 0xFF;
  xiiUInt32                                  m_uiDrawIndex      = 0;
  xiiBitflags<xiiGALRayTracingInstanceFlags> m_Flags;
};

struct xiiRayTracingDynamicBLAS : xiiRefCounted
{
  xiiSharedPtr<xiiGALBottomLevelAS> m_Blas;
  xiiSharedPtr<xiiGALBuffer>        m_Scratch, m_Vertices;
  xiiRayTracingGeometryHandle       m_Source;
  xiiUInt64                         m_ReadyRevision = 0, m_SetupSerial = 0, m_ScheduledRevision = 0;
  xiiRenderGraphBufferHandle        m_Dependency;
};

struct xiiRayTracingGeometryFrame
{
  xiiHashTable<xiiUInt32, xiiSharedPtr<xiiRayTracingDeformedVertices>> m_Vertices;
  xiiHashTable<xiiUInt32, xiiSharedPtr<xiiRayTracingDynamicBLAS>>      m_Blas;
};

struct xiiRayTracingResources::FrameResources
{
  xiiSharedPtr<xiiGALTopLevelAS> m_pTopLevelAS;
  xiiSharedPtr<xiiGALBuffer>     m_pInstanceBuffer;
  xiiSharedPtr<xiiGALBuffer>     m_pMaterialBuffer;
  xiiSharedPtr<xiiGALBuffer>     m_pGeometryBuffer;
  xiiSharedPtr<xiiGALBuffer>     m_pScratchBuffer;
  xiiUInt64                      m_uiBuiltRevision      = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt32                      m_uiBuiltInstanceCount = 0U;
  xiiUInt32                      m_uiCapacity           = 0;
  bool                           m_bReady               = false;
};

class xiiRayTracingFrameResourcesState : public xiiRefCounted
{
public:
  ~xiiRayTracingFrameResourcesState()
  {
    for (const auto& lease : m_Leases)
      xiiRayTracingResources::ReleaseGeometry(lease);
  }
  xiiDynamicArray<xiiRayTracingResources::FrameResources> m_Frames;
  xiiDynamicArray<xiiRayTracingGeometryHandle>            m_Leases;
  xiiDynamicArray<xiiRayTracingGeometryFrame>             m_GeometryFrames;
  xiiSharedPtr<xiiGALComputePipelineState>                m_DeformationPipeline;
  xiiUInt64                                               m_PruneFrame = xiiMath::MaxValue<xiiUInt64>();
  xiiUInt64                                               m_Id         = 0;
};
xiiRayTracingFrameResources::xiiRayTracingFrameResources() : m_pState(XII_DEFAULT_NEW(xiiRayTracingFrameResourcesState))
{
  static xiiAtomicInteger64 nextWorld;
  m_pState->m_Id = nextWorld.Increment();
}
xiiRayTracingFrameResources::~xiiRayTracingFrameResources() = default;

class xiiRayTracingResources::State
{
public:
  void ClearResources()
  {
    if (xiiGALBindlessResourceTable::IsInitialized())
    {
      for (GeometrySlot& geometry : m_Geometries)
      {
        if (geometry.m_hVertexBufferSRV.IsValid())
          xiiGALBindlessResourceTable::RetireBufferSRV(geometry.m_hVertexBufferSRV, m_uiLastFrameIndex);
        if (geometry.m_hIndexBufferSRV.IsValid())
          xiiGALBindlessResourceTable::RetireBufferSRV(geometry.m_hIndexBufferSRV, m_uiLastFrameIndex);
      }
    }
    m_GeometryIds.Clear();
    m_Geometries.Clear();
    m_uiGeometryCount = 0U;
    ++m_uiResourceRevision;
    m_bInitialized = false;
  }

  xiiIdTable<xiiRayTracingGeometryId, xiiUInt32> m_GeometryIds;
  xiiDynamicArray<GeometrySlot>                  m_Geometries;
  xiiRayTracingResourceLimits                    m_Configuration;
  xiiUInt64                                      m_uiResourceRevision = 0U;
  xiiUInt32                                      m_uiGeometryCount    = 0U;
  xiiUInt64                                      m_uiLastFrameIndex   = 0U;
  bool                                           m_bEngineStarted     = false;
  bool                                           m_bHardwareSupported = false;
  bool                                           m_bInitialized       = false;
};

namespace
{
  struct RayTracingBLASBuild
  {
    xiiUInt32                              m_uiGeometryIndex      = xiiInvalidIndex;
    xiiUInt32                              m_uiGeometryGeneration = 0U;
    xiiSharedPtr<xiiGALBottomLevelAS>      m_pBottomLevelAS;
    xiiSharedPtr<xiiGALBuffer>             m_pVertexBuffer;
    xiiSharedPtr<xiiGALBuffer>             m_pIndexBuffer;
    xiiSharedPtr<xiiGALBuffer>             m_pScratchBuffer;
    xiiUInt64                              m_uiVertexBufferOffset = 0U;
    xiiUInt64                              m_uiVertexStride       = 0U;
    xiiUInt32                              m_uiPrimitiveCount     = 0U;
    xiiUInt64                              m_uiIndexBufferOffset  = 0;
    xiiRenderGraphBufferHandle             m_hVertexBuffer;
    xiiRenderGraphBufferHandle             m_hIndexBuffer;
    xiiRenderGraphBufferHandle             m_hScratchBuffer;
    xiiSharedPtr<xiiRayTracingDynamicBLAS> m_Dynamic;
    xiiUInt64                              m_DeformationRevision = 0;
    bool                                   m_Update              = false;
    xiiUInt32                              m_DrawId              = 0;
  };

  struct RayTracingSceneBuildPassData
  {
    xiiDynamicArray<RayTracingBLASBuild>                                m_BLASBuilds;
    xiiDynamicArray<xiiGALTLASInstanceData, xiiAlignedAllocatorWrapper> m_Instances;
    xiiSharedPtr<xiiGALTopLevelAS>                                      m_pTopLevelAS;
    xiiSharedPtr<xiiGALBuffer>                                          m_pInstanceBuffer;
    xiiSharedPtr<xiiGALBuffer>                                          m_pTLASScratchBuffer;
    xiiRenderGraphBufferHandle                                          m_hInstanceBuffer;
    xiiRenderGraphBufferHandle                                          m_hTLASScratchBuffer;
    xiiUInt64                                                           m_uiResourceRevision = 0U;
    xiiUInt32                                                           m_uiFrameSlot        = 0U;
    bool                                                                m_bUpdateTLAS        = false;
    xiiDynamicArray<xiiRenderGraphBufferHandle>                         m_BLASDependencies;
  };

  struct RayTracingHitDataUploadPassData
  {
    xiiDynamicArray<xiiRayTracingMaterialData, xiiAlignedAllocatorWrapper> m_Materials;
    xiiDynamicArray<xiiRayTracingGeometryData, xiiAlignedAllocatorWrapper> m_Geometries;
    xiiSharedPtr<xiiGALBuffer>                                             m_pMaterialBuffer;
    xiiSharedPtr<xiiGALBuffer>                                             m_pGeometryBuffer;
    xiiRenderGraphBufferHandle                                             m_hMaterialBuffer;
    xiiRenderGraphBufferHandle                                             m_hGeometryBuffer;
    xiiRenderGraphBufferHandle                                             m_hSceneDependency;

    struct GeometryDependency
    {
      xiiSharedPtr<xiiGALBuffer> m_pBuffer;
      xiiRenderGraphBufferHandle m_hBuffer;
      xiiUInt32                  m_uiGeometryIndex = xiiInvalidIndex;
      xiiUInt32                  m_uiGeneration    = 0U;
      bool                       m_bIndexBuffer    = false;
    };
    xiiDynamicArray<GeometryDependency> m_GeometryDependencies;
  };

  template <typename T>
  bool TryGetMaterialParameter(const xiiMaterialInstance& material, xiiStringView sName, T& out_value)
  {
    const xiiVariant value = material.GetParameter(xiiMaterialParameterId::Make(sName));
    if (!value.IsValid() || !value.CanConvertTo<T>())
      return false;

    out_value = value.ConvertTo<T>();
    return true;
  }

  void ResolveMaterialColor(const xiiMaterialInstance& material, xiiStringView sName, xiiVec4& inout_value, bool bPreserveAlpha)
  {
    const float fPreviousAlpha = inout_value.w;
    xiiColor    color;
    if (TryGetMaterialParameter(material, sName, color))
    {
      inout_value = xiiVec4(color.r, color.g, color.b, color.a);
      if (bPreserveAlpha)
        inout_value.w = fPreviousAlpha;
      return;
    }

    xiiVec4 vector4;
    if (TryGetMaterialParameter(material, sName, vector4))
    {
      inout_value = vector4;
      if (bPreserveAlpha)
        inout_value.w = fPreviousAlpha;
      return;
    }

    xiiVec3 vector3;
    if (TryGetMaterialParameter(material, sName, vector3))
      inout_value = xiiVec4(vector3.x, vector3.y, vector3.z, inout_value.w);
  }

  xiiUInt32 ResolveMaterialTexture(const xiiMaterialInstanceSnapshot& snapshot, xiiStringView sName, bool bSampler = false)
  {
    if (snapshot.m_pSchema == nullptr)
      return xiiInvalidIndex;

    const xiiUInt32 uiTextureIndex = snapshot.m_pSchema->FindTextureIndex(xiiMaterialParameterId::Make(sName));
    if (uiTextureIndex == xiiInvalidIndex || uiTextureIndex >= snapshot.m_ResourceBindings.GetCount())
      return xiiInvalidIndex;

    const auto& binding = snapshot.m_ResourceBindings[uiTextureIndex];
    return bSampler ? binding.m_uiSamplerIndex : binding.m_uiBindlessIndex;
  }

  xiiRayTracingMaterialData ResolveRayTracingMaterial(const xiiRayTracingInstanceInput& instance)
  {
    xiiRayTracingMaterialData result;
    result.BaseColorOpacity          = xiiVec4(1.0f);
    result.EmissiveColorAndRoughness = xiiVec4(0.0f, 0.0f, 0.0f, 0.5f);
    result.SurfaceParameters         = xiiVec4(0.0f, 0.5f, 0.0f, 1.0f);
    result.Metadata                  = xiiVec4U32(instance.m_hMaterial.GetInternalID().m_InstanceIndex, instance.m_uiStableObjectId, static_cast<xiiUInt32>(xiiMaterialShadingModel::Lit), 0U);
    result.TextureIndices0           = xiiVec4U32(xiiInvalidIndex);
    result.TextureIndices1           = xiiVec4U32(xiiInvalidIndex);
    result.SamplerIndices0           = xiiVec4U32(xiiInvalidIndex);
    result.SamplerIndices1           = xiiVec4U32(xiiInvalidIndex);
    result.LayerParameters           = xiiVec4(0.5f, 1.0f, 0.0f, 0.0f);
    result.Rendering                 = xiiVec4U32(static_cast<xiiUInt32>(xiiMaterialAlphaMode::Opaque), static_cast<xiiUInt32>(xiiMaterialBlendMode::Opaque), 0U, instance.m_uiVisibilityMask);

    if (instance.m_hMaterial.IsInvalidated() || !xiiMaterialManager::IsInitialized())
      return result;

    const xiiSharedPtr<xiiMaterialInstance> material = xiiMaterialManager::GetGpuStorage().GetMaterial(instance.m_hMaterial);
    if (material == nullptr)
      return result;

    ResolveMaterialColor(*material, "BaseColor", result.BaseColorOpacity, false);
    ResolveMaterialColor(*material, "EmissiveColor", result.EmissiveColorAndRoughness, true);
    TryGetMaterialParameter(*material, "Roughness", result.EmissiveColorAndRoughness.w);
    TryGetMaterialParameter(*material, "Metallic", result.SurfaceParameters.x);
    TryGetMaterialParameter(*material, "Specular", result.SurfaceParameters.y);
    TryGetMaterialParameter(*material, "Transmission", result.SurfaceParameters.z);
    TryGetMaterialParameter(*material, "OcclusionStrength", result.SurfaceParameters.w);
    TryGetMaterialParameter(*material, "AlphaCutoff", result.LayerParameters.x);
    TryGetMaterialParameter(*material, "NormalScale", result.LayerParameters.y);
    TryGetMaterialParameter(*material, "ClearCoat", result.LayerParameters.z);
    TryGetMaterialParameter(*material, "ClearCoatRoughness", result.LayerParameters.w);

    const xiiMaterialRuntimeState runtimeState = material->GetRuntimeState();
    result.Metadata.z                          = runtimeState.m_ShadingModel.GetValue();
    result.Metadata.w                          = runtimeState.m_FeatureFlags.GetValue();
    result.Rendering                           = xiiVec4U32(static_cast<xiiUInt32>(runtimeState.IsMasked() ? xiiMaterialAlphaMode::Mask : runtimeState.m_AlphaMode.GetValue()), runtimeState.m_BlendMode.GetValue(), runtimeState.m_uiTextureMask, instance.m_uiVisibilityMask);

    xiiMaterialInstanceSnapshot snapshot;
    material->CreateSnapshot(snapshot);
    result.TextureIndices0 = xiiVec4U32(ResolveMaterialTexture(snapshot, "BaseColorTexture"), ResolveMaterialTexture(snapshot, "NormalTexture"), ResolveMaterialTexture(snapshot, "MetallicRoughnessTexture"), ResolveMaterialTexture(snapshot, "OcclusionTexture"));
    result.TextureIndices1 = xiiVec4U32(ResolveMaterialTexture(snapshot, "EmissiveTexture"), ResolveMaterialTexture(snapshot, "HeightTexture"), ResolveMaterialTexture(snapshot, "ClearCoatTexture"), ResolveMaterialTexture(snapshot, "TransmissionTexture"));
    result.SamplerIndices0 = xiiVec4U32(ResolveMaterialTexture(snapshot, "BaseColorTexture", true), ResolveMaterialTexture(snapshot, "NormalTexture", true), ResolveMaterialTexture(snapshot, "MetallicRoughnessTexture", true), ResolveMaterialTexture(snapshot, "OcclusionTexture", true));
    result.SamplerIndices1 = xiiVec4U32(ResolveMaterialTexture(snapshot, "EmissiveTexture", true), ResolveMaterialTexture(snapshot, "HeightTexture", true), ResolveMaterialTexture(snapshot, "ClearCoatTexture", true), ResolveMaterialTexture(snapshot, "TransmissionTexture", true));
    return result;
  }

} // namespace

xiiUniquePtr<xiiRayTracingResources::State> xiiRayTracingResources::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RayTracingResources)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "MaterialManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiRayTracingResources::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiRayTracingResources::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiRayTracingResources::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiRayTracingResources::Shutdown();
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingGeometryHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingGeometryHandle>)
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingGeometryDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingGeometryDescription>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("MeshBuffer", GetMeshBufferResourceId, SetMeshBufferResourceId),
      XII_MEMBER_PROPERTY("Opaque", m_bOpaque),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingResourceLimits, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingResourceLimits>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("MaxGeometries", m_uiMaxGeometries)->AddAttributes(new xiiDefaultValueAttribute(16384U), new xiiClampValueAttribute(1U, 1048576U)),
      XII_MEMBER_PROPERTY("MaxInstances", m_uiMaxInstances)->AddAttributes(new xiiDefaultValueAttribute(65536U), new xiiClampValueAttribute(1U, 16777215U)),
      XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiDefaultValueAttribute(3U), new xiiClampValueAttribute(1U, 64U)),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiRayTracingResourceStats, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiRayTracingResourceStats>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("GeometryCount", m_uiGeometryCount),
      XII_MEMBER_PROPERTY("PendingBLASCount", m_uiPendingBLASCount),
      XII_MEMBER_PROPERTY("SceneRevision", m_uiResourceRevision),
    } XII_END_PROPERTIES;
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

void xiiRayTracingResources::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Ray-tracing scene manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  ApplyConfiguration().IgnoreResult();
}

void xiiRayTracingResources::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede ray-tracing scene engine startup.");
  if (s_pState == nullptr)
    return;

  s_pState->m_bEngineStarted               = true;
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  s_pState->m_bHardwareSupported           = pDevice != nullptr && pDevice->GetFeatures().m_RayTracing == xiiGALDeviceFeatureState::Enabled;
  if (!s_pState->m_bInitialized)
    ApplyConfiguration().IgnoreResult();
}

void xiiRayTracingResources::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  // Future BLAS/TLAS objects live in these slots and therefore release before the GAL device.
  s_pState->ClearResources();
  s_pState->m_bHardwareSupported = false;
  s_pState->m_bEngineStarted     = false;
}

void xiiRayTracingResources::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiRayTracingResources::ApplyConfiguration()
{
  if (s_pState == nullptr || s_pState->m_Configuration.m_uiMaxGeometries == 0U || s_pState->m_Configuration.m_uiMaxInstances == 0U || s_pState->m_Configuration.m_uiFramesInFlight == 0U || s_pState->m_Configuration.m_uiFramesInFlight > 64U)
    return XII_FAILURE;

  s_pState->ClearResources();
  s_pState->m_GeometryIds.Reserve(s_pState->m_Configuration.m_uiMaxGeometries);
  s_pState->m_Geometries.SetCount(s_pState->m_Configuration.m_uiMaxGeometries);
  s_pState->m_bInitialized = true;
  return XII_SUCCESS;
}

xiiResult xiiRayTracingResources::Configure(const xiiRayTracingResourceLimits& description)
{
  if (s_pState == nullptr || description.m_uiMaxGeometries == 0U || description.m_uiMaxInstances == 0U || description.m_uiFramesInFlight == 0U || description.m_uiFramesInFlight > 64U)
    return XII_FAILURE;
  if (s_pState->m_uiGeometryCount != 0U)
    return XII_FAILURE;

  s_pState->m_Configuration = description;
  return ApplyConfiguration();
}

const xiiRayTracingResourceLimits& xiiRayTracingResources::GetConfiguration()
{
  static const xiiRayTracingResourceLimits s_Default;
  return s_pState != nullptr ? s_pState->m_Configuration : s_Default;
}

bool xiiRayTracingResources::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiRayTracingResources::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

bool xiiRayTracingResources::IsHardwareRayTracingSupported()
{
  return s_pState != nullptr && s_pState->m_bHardwareSupported;
}

xiiRayTracingGeometryHandle xiiRayTracingResources::AcquireGeometry(const xiiRayTracingGeometryDescription& description)
{
  if (!IsInitialized() || !description.m_hMeshBuffer.IsValid())
    return {};

  xiiResourceLock<xiiMeshBufferResource> resource(description.m_hMeshBuffer, xiiResourceAcquireMode::PointerOnly);
  const auto                             version = resource->GetCurrentResourceChangeCounter();
  for (auto& cached : s_pState->m_Geometries)
    if (cached.m_bAllocated && cached.m_Description.m_hMeshBuffer == description.m_hMeshBuffer && cached.m_uiResourceVersion == version && cached.m_Description.m_uiFirstPrimitive == description.m_uiFirstPrimitive && cached.m_Description.m_uiPrimitiveCount == description.m_uiPrimitiveCount && cached.m_Description.m_bOpaque == description.m_bOpaque)
    {
      ++cached.m_uiReferences;
      return xiiRayTracingGeometryHandle(cached.m_Id);
    }
  if (s_pState->m_GeometryIds.GetCount() >= s_pState->m_Geometries.GetCount())
    return {};
  const xiiRayTracingGeometryId id      = s_pState->m_GeometryIds.Insert(0U);
  const xiiUInt32               uiIndex = id.m_InstanceIndex;
  s_pState->m_GeometryIds[id]           = uiIndex;
  GeometrySlot& slot                    = s_pState->m_Geometries[uiIndex];
  slot.m_Description                    = description;
  slot.m_uiReferences                   = 1;
  slot.m_uiResourceVersion              = version;
  slot.m_Id                             = id;
  slot.m_bAllocated                     = true;
  slot.m_bBLASDirty                     = true;
  ++s_pState->m_uiGeometryCount;
  ++s_pState->m_uiResourceRevision;
  return xiiRayTracingGeometryHandle(id);
}

void xiiRayTracingResources::ReleaseGeometry(xiiRayTracingGeometryHandle handle)
{
  if (!IsValid(handle))
    return;

  auto& retained = s_pState->m_Geometries[handle.GetInternalID().m_InstanceIndex];
  if (--retained.m_uiReferences != 0)
    return;
  GeometrySlot& slot = s_pState->m_Geometries[handle.GetInternalID().m_InstanceIndex];
  if (xiiGALBindlessResourceTable::IsInitialized())
  {
    if (slot.m_hVertexBufferSRV.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(slot.m_hVertexBufferSRV, s_pState->m_uiLastFrameIndex);
    if (slot.m_hIndexBufferSRV.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(slot.m_hIndexBufferSRV, s_pState->m_uiLastFrameIndex);
  }
  slot.m_pBottomLevelAS.Clear();
  slot.m_pScratchBuffer.Clear();
  slot.m_pVertexBuffer.Clear();
  slot.m_pIndexBuffer.Clear();
  slot.m_hVertexBufferSRV = {};
  slot.m_hIndexBufferSRV  = {};
  slot.m_Description      = {};
  slot.m_bAllocated       = false;
  slot.m_bBLASDirty       = true;
  s_pState->m_GeometryIds.Remove(slot.m_Id);
  slot.m_Id.Invalidate();
  --s_pState->m_uiGeometryCount;
  ++s_pState->m_uiResourceRevision;
}

bool xiiRayTracingResources::IsValid(xiiRayTracingGeometryHandle handle)
{
  return IsInitialized() && s_pState->m_GeometryIds.Contains(handle.GetInternalID());
}

bool xiiRayTracingResources::PrepareGeometry(xiiUInt32 uiGeometryIndex)
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
  const xiiMeshVertexStream* pNormalStream   = nullptr;
  const xiiMeshVertexStream* pTangentStream  = nullptr;
  const xiiMeshVertexStream* pTexCoordStream = nullptr;
  for (const xiiMeshVertexStream& stream : mesh->GetVertexStreams())
  {
    if (stream.m_Semantic == xiiMeshVertexSemantic::Position)
      pPositionStream = &stream;
    else if (stream.m_Semantic == xiiMeshVertexSemantic::Normal && stream.m_Format == xiiMeshVertexStreamFormat::Float3)
      pNormalStream = &stream;
    else if (stream.m_Semantic == xiiMeshVertexSemantic::Tangent && stream.m_Format == xiiMeshVertexStreamFormat::Float4)
      pTangentStream = &stream;
    else if (stream.m_Semantic == xiiMeshVertexSemantic::TexCoord0 && stream.m_Format == xiiMeshVertexStreamFormat::Float2)
      pTexCoordStream = &stream;
  }
  if (pPositionStream == nullptr || pPositionStream->m_Format != xiiMeshVertexStreamFormat::Float3 || pPositionStream->m_uiStride == 0U)
    return false;

  xiiSharedPtr<xiiGALBuffer> pVertexBuffer = mesh->GetVertexBuffer();
  xiiSharedPtr<xiiGALBuffer> pIndexBuffer  = mesh->GetIndexBuffer();
  if (pVertexBuffer == nullptr || (mesh->GetIndexCount() > 0U && pIndexBuffer == nullptr))
    return false;

  if (!xiiGALBindlessResourceTable::IsInitialized())
    return false;

  const xiiGALBindlessResourceHandle hVertexBufferSRV = xiiGALBindlessResourceTable::RegisterBufferSRV(pVertexBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  if (!hVertexBufferSRV.IsValid())
    return false;

  xiiGALBindlessResourceHandle hIndexBufferSRV;
  if (pIndexBuffer != nullptr)
  {
    hIndexBufferSRV = xiiGALBindlessResourceTable::RegisterBufferSRV(pIndexBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
    if (!hIndexBufferSRV.IsValid())
    {
      xiiGALBindlessResourceTable::RetireBufferSRV(hVertexBufferSRV, s_pState->m_uiLastFrameIndex);
      return false;
    }
  }

  xiiGALBottomLevelASCreationDescription blasDescription;
  blasDescription.m_BuildASFlags          = xiiGALRayTracingBuildASFlags::PreferFastTrace;
  xiiGALBLASTriangleDescription& triangle = blasDescription.m_Triangles.ExpandAndGetRef();
  triangle.m_sGeometryName                = "Mesh";
  triangle.m_uiMaxVertexCount             = mesh->GetVertexCount();
  triangle.m_VertexValueType              = xiiGALValueType::Float32;
  triangle.m_uiVertexComponentCount       = 3U;
  if (slot.m_Description.m_uiFirstPrimitive >= mesh->GetPrimitiveCount())
    return false;
  const auto available = mesh->GetPrimitiveCount() - slot.m_Description.m_uiFirstPrimitive;
  const auto count     = slot.m_Description.m_uiPrimitiveCount == 0 ? available : slot.m_Description.m_uiPrimitiveCount;
  if (count > available)
    return false;
  triangle.m_uiMaxPrimitiveCount = count;
  triangle.m_IndexType           = xiiGALValueType::Undefined;
  if (mesh->GetIndexCount() > 0U)
    triangle.m_IndexType = mesh->GetIndexType();

  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
  {
    xiiGALBindlessResourceTable::RetireBufferSRV(hVertexBufferSRV, s_pState->m_uiLastFrameIndex);
    if (hIndexBufferSRV.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(hIndexBufferSRV, s_pState->m_uiLastFrameIndex);
    return false;
  }

  xiiSharedPtr<xiiGALBottomLevelAS> pBottomLevelAS = pDevice->CreateBottomLevelAS(blasDescription);
  if (pBottomLevelAS == nullptr || pBottomLevelAS->GetScratchBufferSizeDescription().m_uiBuild == 0U)
  {
    xiiGALBindlessResourceTable::RetireBufferSRV(hVertexBufferSRV, s_pState->m_uiLastFrameIndex);
    if (hIndexBufferSRV.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(hIndexBufferSRV, s_pState->m_uiLastFrameIndex);
    return false;
  }

  const xiiUInt32                 uiScratchAlignment = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiScratchBufferAlignment);
  xiiGALBufferCreationDescription scratchDescription;
  scratchDescription.m_uiSize               = xiiMemoryUtils::AlignSize(pBottomLevelAS->GetScratchBufferSizeDescription().m_uiBuild, static_cast<xiiUInt64>(uiScratchAlignment));
  scratchDescription.m_BindFlags            = xiiGALBindFlags::RayTracing;
  scratchDescription.m_Usage                = xiiGALResourceUsage::Mutable;
  scratchDescription.m_Mode                 = xiiGALBufferMode::Raw;
  xiiSharedPtr<xiiGALBuffer> pScratchBuffer = pDevice->CreateBuffer(scratchDescription);
  if (pScratchBuffer == nullptr)
  {
    xiiGALBindlessResourceTable::RetireBufferSRV(hVertexBufferSRV, s_pState->m_uiLastFrameIndex);
    if (hIndexBufferSRV.IsValid())
      xiiGALBindlessResourceTable::RetireBufferSRV(hIndexBufferSRV, s_pState->m_uiLastFrameIndex);
    return false;
  }

  xiiStringBuilder debugName;
  debugName.SetFormat("Ray Tracing BLAS [{}]", uiGeometryIndex);
  pBottomLevelAS->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing BLAS Scratch [{}]", uiGeometryIndex);
  pScratchBuffer->SetDebugName(debugName);

  slot.m_pVertexBuffer        = std::move(pVertexBuffer);
  slot.m_pIndexBuffer         = std::move(pIndexBuffer);
  slot.m_pBottomLevelAS       = std::move(pBottomLevelAS);
  slot.m_pScratchBuffer       = std::move(pScratchBuffer);
  slot.m_hVertexBufferSRV     = hVertexBufferSRV;
  slot.m_hIndexBufferSRV      = hIndexBufferSRV;
  slot.m_uiVertexBufferOffset = pPositionStream->m_uiOffset;
  slot.m_uiVertexStride       = pPositionStream->m_uiStride;
  slot.m_uiNormalStride       = pNormalStream != nullptr ? pNormalStream->m_uiStride : 0U;
  slot.m_uiNormalOffset       = pNormalStream != nullptr ? pNormalStream->m_uiOffset : xiiInvalidIndex;
  slot.m_uiTangentStride      = pTangentStream != nullptr ? pTangentStream->m_uiStride : 0U;
  slot.m_uiTangentOffset      = pTangentStream != nullptr ? pTangentStream->m_uiOffset : xiiInvalidIndex;
  slot.m_uiTexCoordStride     = pTexCoordStream != nullptr ? pTexCoordStream->m_uiStride : 0U;
  slot.m_uiTexCoordOffset     = pTexCoordStream != nullptr ? pTexCoordStream->m_uiOffset : xiiInvalidIndex;
  slot.m_uiIndexStride        = pIndexBuffer == nullptr ? 0U : (mesh->GetIndexType() == xiiGALValueType::UInt32 ? 4U : 2U);
  slot.m_uiPrimitiveCount     = count;
  slot.m_bBLASDirty           = true;
  return true;
}

bool xiiRayTracingResources::PrepareFrameResources(xiiRayTracingFrameResources& resources, xiiUInt32 uiFrameSlot, xiiUInt32 uiInstanceCount)
{
  if (!IsHardwareRayTracingSupported() || uiFrameSlot >= resources.m_pState->m_Frames.GetCount())
    return false;

  FrameResources& frame = resources.m_pState->m_Frames[uiFrameSlot];
  if (frame.m_pTopLevelAS != nullptr && frame.m_uiCapacity >= uiInstanceCount)
    return true;

  frame                              = {};
  const auto capacity                = xiiMath::PowerOfTwo_Ceil(xiiMath::Max(1U, uiInstanceCount));
  frame.m_uiCapacity                 = capacity;
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return false;

  xiiGALTopLevelASCreationDescription tlasDescription;
  tlasDescription.m_uiMaxInstanceCount = capacity;
  tlasDescription.m_Flags              = xiiGALRayTracingBuildASFlags::AllowUpdate | xiiGALRayTracingBuildASFlags::PreferFastTrace;
  frame.m_pTopLevelAS                  = pDevice->CreateTopLevelAS(tlasDescription);
  if (frame.m_pTopLevelAS == nullptr)
    return false;

  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiSize              = static_cast<xiiUInt64>(capacity) * sizeof(xiiGALTLASInstanceData);
  bufferDescription.m_uiElementByteStride = sizeof(xiiGALTLASInstanceData);
  bufferDescription.m_BindFlags           = xiiGALBindFlags::RayTracing;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  frame.m_pInstanceBuffer                 = pDevice->CreateBuffer(bufferDescription);

  bufferDescription.m_uiSize              = static_cast<xiiUInt64>(capacity) * sizeof(xiiRayTracingMaterialData);
  bufferDescription.m_uiElementByteStride = sizeof(xiiRayTracingMaterialData);
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Mutable;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;
  frame.m_pMaterialBuffer                 = pDevice->CreateBuffer(bufferDescription);

  bufferDescription.m_uiSize              = static_cast<xiiUInt64>(capacity) * sizeof(xiiRayTracingGeometryData);
  bufferDescription.m_uiElementByteStride = sizeof(xiiRayTracingGeometryData);
  frame.m_pGeometryBuffer                 = pDevice->CreateBuffer(bufferDescription);

  const xiiGALScratchBufferSizeDescription scratchSizes       = frame.m_pTopLevelAS->GetScratchBufferSizeDescription();
  const xiiUInt32                          uiScratchAlignment = xiiMath::Max(1U, pDevice->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiScratchBufferAlignment);
  bufferDescription                                           = {};
  bufferDescription.m_uiSize                                  = xiiMemoryUtils::AlignSize(xiiMath::Max(scratchSizes.m_uiBuild, scratchSizes.m_uiUpdate), static_cast<xiiUInt64>(uiScratchAlignment));
  bufferDescription.m_BindFlags                               = xiiGALBindFlags::RayTracing;
  bufferDescription.m_Usage                                   = xiiGALResourceUsage::Mutable;
  bufferDescription.m_Mode                                    = xiiGALBufferMode::Raw;
  frame.m_pScratchBuffer                                      = pDevice->CreateBuffer(bufferDescription);
  if (frame.m_pInstanceBuffer == nullptr || frame.m_pMaterialBuffer == nullptr || frame.m_pGeometryBuffer == nullptr || frame.m_pScratchBuffer == nullptr)
  {
    frame = {};
    return false;
  }

  xiiStringBuilder debugName;
  debugName.SetFormat("Ray Tracing TLAS [{}]", uiFrameSlot);
  frame.m_pTopLevelAS->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing Instance Data [{}]", uiFrameSlot);
  frame.m_pInstanceBuffer->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing Material Data [{}]", uiFrameSlot);
  frame.m_pMaterialBuffer->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing Geometry Data [{}]", uiFrameSlot);
  frame.m_pGeometryBuffer->SetDebugName(debugName);
  debugName.SetFormat("Ray Tracing TLAS Scratch [{}]", uiFrameSlot);
  frame.m_pScratchBuffer->SetDebugName(debugName);
  return true;
}

xiiRayTracingResources::BuildHandles xiiRayTracingResources::AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRayTracingFrameResources& worldResources, const xiiGpuInstanceData& instances, const xiiArrayPtr<const xiiUInt32>* selection, xiiRayTracingFrameResources* sharedGeometry)
{
  BuildHandles result;
  if (!IsInitialized() || !IsHardwareRayTracingSupported())
    return result;
  if (instances.GetInstanceCount() == 0U)
  {
    result.m_Result = XII_SUCCESS;
    return result;
  }

  s_pState->m_uiLastFrameIndex = uiFrameIndex;
  auto& world                  = *worldResources.m_pState;
  if (world.m_Frames.IsEmpty())
    world.m_Frames.SetCount(s_pState->m_Configuration.m_uiFramesInFlight);
  auto& geometryOwner = sharedGeometry ? *sharedGeometry->m_pState : world;
  if (geometryOwner.m_GeometryFrames.IsEmpty())
    geometryOwner.m_GeometryFrames.SetCount(s_pState->m_Configuration.m_uiFramesInFlight);
  const auto geometryFrameIndex = xiiUInt32(uiFrameIndex % geometryOwner.m_GeometryFrames.GetCount());
  auto&      dynamicFrame       = geometryOwner.m_GeometryFrames[geometryFrameIndex];
  if (geometryOwner.m_PruneFrame != uiFrameIndex)
  {
    xiiHashSet<xiiUInt32> live;
    for (xiiUInt32 i = 0; i < instances.GetInstanceCount(); ++i)
    {
      const auto& record = instances.GetInstances()[i];
      if ((record.m_uiFlags & xiiGpuInstanceFlags::RayTracingVisible) != 0 && (record.m_uiSkinningCount != 0 || !instances.GetMorphWeights(i).IsEmpty()))
        live.Insert(record.m_uiObjectId);
    }
    for (auto& frame : geometryOwner.m_GeometryFrames)
    {
      for (auto it = frame.m_Vertices.GetIterator(); it.IsValid();)
        if (!live.Contains(it.Key()))
          it = frame.m_Vertices.Remove(it);
        else
          ++it;
      for (auto it = frame.m_Blas.GetIterator(); it.IsValid();)
        if (!live.Contains(it.Key()))
          it = frame.m_Blas.Remove(it);
        else
          ++it;
    }
    geometryOwner.m_PruneFrame = uiFrameIndex;
  }
  xiiDynamicArray<xiiRayTracingInstanceInput>  inputs;
  xiiDynamicArray<xiiRayTracingGeometryHandle> leases;
  for (xiiUInt32 candidate = 0; candidate < (selection ? selection->GetCount() : instances.GetInstanceCount()); ++candidate)
  {
    const auto i = selection ? (*selection)[candidate] : candidate;
    if (i >= instances.GetInstanceCount())
      continue;
    const auto& record = instances.GetInstances()[i];
    if ((record.m_uiFlags & xiiGpuInstanceFlags::RayTracingVisible) == 0)
      continue;
    if (record.m_uiDrawLod != xiiInvalidIndex && record.m_uiDrawLod != (record.m_uiForcedLod == xiiInvalidIndex ? 0U : record.m_uiForcedLod))
      continue;
    xiiRayTracingGeometryDescription description;
    description.m_hMeshBuffer      = instances.GetMeshBufferResource(i);
    description.m_uiFirstPrimitive = record.m_uiFirstPrimitive;
    description.m_uiPrimitiveCount = record.m_uiPrimitiveCount;
    description.m_bOpaque          = (record.m_uiFlags & xiiGpuInstanceFlags::Transparent) == 0;
    const auto geometry            = AcquireGeometry(description);
    if (geometry.IsInvalidated())
      continue;
    leases.PushBack(geometry);
    auto& input              = inputs.ExpandAndGetRef();
    input.m_hGeometry        = geometry;
    input.m_hMaterial        = instances.GetMaterialHandle(i);
    input.m_Transform        = record.m_GlobalTransform;
    input.m_uiStableObjectId = record.m_uiObjectId;
    input.m_uiVisibilityMask = static_cast<xiiUInt8>(record.m_uiVisibilityMask);
    input.m_uiDrawIndex      = i;
  }
  for (const auto& lease : world.m_Leases)
    ReleaseGeometry(lease);
  world.m_Leases = std::move(leases);
  if (inputs.IsEmpty())
  {
    result.m_Result = XII_SUCCESS;
    return result;
  }
  if (inputs.GetCount() > s_pState->m_Configuration.m_uiMaxInstances)
    return result;


  xiiDynamicArray<RayTracingBLASBuild>                                   pendingBLASBuilds;
  xiiDynamicArray<xiiGALTLASInstanceData, xiiAlignedAllocatorWrapper>    instanceData;
  xiiDynamicArray<xiiRayTracingMaterialData, xiiAlignedAllocatorWrapper> materialData;
  xiiDynamicArray<xiiRayTracingGeometryData, xiiAlignedAllocatorWrapper> geometryData;
  instanceData.Reserve(inputs.GetCount());
  materialData.Reserve(inputs.GetCount());
  geometryData.Reserve(inputs.GetCount());
  xiiDynamicArray<xiiRenderGraphBufferHandle>     blasDependencies;
  xiiDynamicArray<xiiRayTracingDeformedVertices*> deformedDependencies;
  xiiUInt64                                       deformationRevision = 0;
  xiiHashSet<xiiUInt32>                           reachable, staticGeometry;
  for (const auto& input : inputs)
  {
    const auto index = input.m_hGeometry.GetInternalID().m_InstanceIndex;
    reachable.Insert(index);
    if (instances.GetInstances()[input.m_uiDrawIndex].m_uiSkinningCount == 0 && instances.GetMorphWeights(input.m_uiDrawIndex).IsEmpty())
      staticGeometry.Insert(index);
  }

  for (xiiUInt32 uiGeometryIndex = 0U; uiGeometryIndex < s_pState->m_Geometries.GetCount(); ++uiGeometryIndex)
  {
    GeometrySlot& geometry = s_pState->m_Geometries[uiGeometryIndex];
    if (!geometry.m_bAllocated || !reachable.Contains(uiGeometryIndex) || !PrepareGeometry(uiGeometryIndex) || !geometry.m_bBLASDirty || !staticGeometry.Contains(uiGeometryIndex))
      continue;
    if (geometry.m_SetupSerial == graph.GetSetupSerial())
    {
      if (geometry.m_BuildDependency.IsValid())
        blasDependencies.PushBack(geometry.m_BuildDependency);
      continue;
    }
    geometry.m_SetupSerial = graph.GetSetupSerial();

    RayTracingBLASBuild& build   = pendingBLASBuilds.ExpandAndGetRef();
    build.m_uiGeometryIndex      = uiGeometryIndex;
    build.m_uiGeometryGeneration = geometry.m_Id.m_Generation;
    build.m_pBottomLevelAS       = geometry.m_pBottomLevelAS;
    build.m_pVertexBuffer        = geometry.m_pVertexBuffer;
    build.m_pIndexBuffer         = geometry.m_pIndexBuffer;
    build.m_pScratchBuffer       = geometry.m_pScratchBuffer;
    build.m_uiVertexBufferOffset = geometry.m_uiVertexBufferOffset;
    if (geometry.m_pIndexBuffer == nullptr)
      build.m_uiVertexBufferOffset += xiiUInt64(geometry.m_Description.m_uiFirstPrimitive) * 3U * geometry.m_uiVertexStride;
    build.m_uiVertexStride      = geometry.m_uiVertexStride;
    build.m_uiPrimitiveCount    = geometry.m_uiPrimitiveCount;
    build.m_uiIndexBufferOffset = xiiUInt64(geometry.m_Description.m_uiFirstPrimitive) * 3U * geometry.m_uiIndexStride;
  }

  for (const auto& instance : inputs)
  {
    if (!IsValid(instance.m_hGeometry))
      continue;

    const GeometrySlot& geometry = s_pState->m_Geometries[instance.m_hGeometry.GetInternalID().m_InstanceIndex];
    if (geometry.m_pBottomLevelAS == nullptr || geometry.m_pBottomLevelAS->GetDeviceAddress() == 0U)
      continue;

    auto                           blas     = geometry.m_pBottomLevelAS;
    xiiRayTracingDeformedVertices* deformed = nullptr;
    const auto&                    record   = instances.GetInstances()[instance.m_uiDrawIndex];
    if (record.m_uiSkinningCount != 0 || !instances.GetMorphWeights(instance.m_uiDrawIndex).IsEmpty())
    {
      const auto representative = record.m_uiLodSelectionIndex == xiiInvalidIndex ? instance.m_uiDrawIndex : record.m_uiLodSelectionIndex;
      const auto sourceId       = instances.GetInstances()[representative].m_uiObjectId;
      auto&      vertices       = dynamicFrame.m_Vertices[sourceId];
      if (!vertices)
        vertices = XII_DEFAULT_NEW(xiiRayTracingDeformedVertices);
      deformed = vertices.Borrow();
      xiiStringBuilder name;
      name.SetFormat("World {} Deformed Vertices [{}:{}]", geometryOwner.m_Id, geometryFrameIndex, sourceId);
      bool dispatched = false;
      if (xiiAddRayTracingDeformation(graph, uiFrameIndex, name, vertices, geometryOwner.m_DeformationPipeline, instances, instance.m_uiDrawIndex, dispatched).Failed())
      {
        xiiLog::Error("Could not deform ray-tracing GPU draw {} from its extracted mesh revision.", sourceId);
        return {};
      }
      result.m_uiDeformationDispatchCount += dispatched ? 1 : 0;
      deformedDependencies.PushBack(deformed);
      deformationRevision = xiiHashingUtils::xxHash64(&deformed->m_Revision, sizeof(deformed->m_Revision), deformationRevision);
      auto& dynamic       = dynamicFrame.m_Blas[record.m_uiObjectId];
      if (!dynamic || dynamic->m_Source != instance.m_hGeometry || dynamic->m_Vertices != deformed->m_Buffer || !dynamic->m_Blas || !dynamic->m_Scratch)
      {
        dynamic                    = XII_DEFAULT_NEW(xiiRayTracingDynamicBLAS);
        dynamic->m_Source          = instance.m_hGeometry;
        dynamic->m_Vertices        = deformed->m_Buffer;
        auto description           = geometry.m_pBottomLevelAS->GetDescription();
        description.m_BuildASFlags = xiiGALRayTracingBuildASFlags::AllowUpdate | xiiGALRayTracingBuildASFlags::PreferFastBuild;
        auto device                = xiiGALDevice::GetDefaultDevice();
        dynamic->m_Blas            = device->CreateBottomLevelAS(description);
        if (!dynamic->m_Blas)
          return {};
        const auto                      scratch = dynamic->m_Blas->GetScratchBufferSizeDescription();
        xiiGALBufferCreationDescription buffer;
        buffer.m_uiSize    = xiiMemoryUtils::AlignSize(xiiMath::Max(scratch.m_uiBuild, scratch.m_uiUpdate), xiiUInt64(xiiMath::Max(1U, device->GetGraphicsDeviceAdapterProperties().m_RayTracingProperties.m_uiScratchBufferAlignment)));
        buffer.m_Mode      = xiiGALBufferMode::Raw;
        buffer.m_BindFlags = xiiGALBindFlags::RayTracing;
        buffer.m_Usage     = xiiGALResourceUsage::Mutable;
        dynamic->m_Scratch = device->CreateBuffer(buffer);
        if (!dynamic->m_Scratch)
          return {};
      }
      blas                 = dynamic->m_Blas;
      const bool scheduled = dynamic->m_SetupSerial == graph.GetSetupSerial() && dynamic->m_ScheduledRevision == deformed->m_Revision;
      if (dynamic->m_ReadyRevision != deformed->m_Revision && !scheduled)
      {
        auto& build                  = pendingBLASBuilds.ExpandAndGetRef();
        build.m_uiGeometryIndex      = instance.m_hGeometry.GetInternalID().m_InstanceIndex;
        build.m_uiGeometryGeneration = instance.m_hGeometry.GetInternalID().m_Generation;
        build.m_DrawId               = record.m_uiObjectId;
        build.m_pBottomLevelAS       = blas;
        build.m_pVertexBuffer        = deformed->m_Buffer;
        build.m_hVertexBuffer        = deformed->m_Handle;
        build.m_pIndexBuffer         = geometry.m_pIndexBuffer;
        build.m_pScratchBuffer       = dynamic->m_Scratch;
        build.m_uiVertexBufferOffset = geometry.m_uiVertexBufferOffset + (geometry.m_pIndexBuffer ? 0 : xiiUInt64(geometry.m_Description.m_uiFirstPrimitive) * 3 * geometry.m_uiVertexStride);
        build.m_uiVertexStride       = geometry.m_uiVertexStride;
        build.m_uiPrimitiveCount     = geometry.m_uiPrimitiveCount;
        build.m_uiIndexBufferOffset  = xiiUInt64(geometry.m_Description.m_uiFirstPrimitive) * 3 * geometry.m_uiIndexStride;
        build.m_Dynamic              = dynamic;
        build.m_DeformationRevision  = deformed->m_Revision;
        build.m_Update               = dynamic->m_ReadyRevision != 0;
        dynamic->m_SetupSerial       = graph.GetSetupSerial();
        dynamic->m_ScheduledRevision = deformed->m_Revision;
        ++result.m_uiDynamicBLASBuildCount;
      }
      else if (scheduled && dynamic->m_Dependency.IsValid())
        blasDependencies.PushBack(dynamic->m_Dependency);
    }

    xiiGALTLASInstanceData& gpuInstance = instanceData.ExpandAndGetRef();
    gpuInstance.SetTransform(instance.m_Transform);
    gpuInstance.SetInstanceID(instanceData.GetCount() - 1U);
    gpuInstance.SetMask(instance.m_uiVisibilityMask);
    gpuInstance.SetHitGroupContribution(0U);
    xiiBitflags<xiiGALRayTracingInstanceFlags> flags           = instance.m_Flags;
    bool                                       bRequiresAnyHit = !geometry.m_Description.m_bOpaque;
    if (!instance.m_hMaterial.IsInvalidated() && xiiMaterialManager::IsInitialized())
    {
      const xiiSharedPtr<xiiMaterialInstance> pMaterial = xiiMaterialManager::GetGpuStorage().GetMaterial(instance.m_hMaterial);
      bRequiresAnyHit                                   = bRequiresAnyHit || (pMaterial != nullptr && pMaterial->GetRuntimeState().IsMasked());
    }
    if (!bRequiresAnyHit)
    {
      flags.Remove(xiiGALRayTracingInstanceFlags::ForceNonOpaque);
      flags.Add(xiiGALRayTracingInstanceFlags::ForceOpaque);
    }
    else
    {
      flags.Remove(xiiGALRayTracingInstanceFlags::ForceOpaque);
      flags.Add(xiiGALRayTracingInstanceFlags::ForceNonOpaque);
    }
    gpuInstance.SetFlags(flags);
    gpuInstance.SetBottomLevelAS(blas.Borrow());
    materialData.PushBack(ResolveRayTracingMaterial(instance));

    xiiRayTracingGeometryData& gpuGeometry = geometryData.ExpandAndGetRef();
    gpuGeometry.BufferIndices              = xiiVec4U32(geometry.m_hVertexBufferSRV.m_uiIndex, geometry.m_hIndexBufferSRV.m_uiIndex, geometry.m_uiIndexStride, geometry.m_uiNormalStride);
    gpuGeometry.VertexLayout               = xiiVec4U32(static_cast<xiiUInt32>(geometry.m_uiVertexStride), static_cast<xiiUInt32>(geometry.m_uiVertexBufferOffset), geometry.m_uiNormalOffset, geometry.m_uiTexCoordOffset);
    gpuGeometry.VertexAttributes           = xiiVec4U32(geometry.m_uiTangentOffset, geometry.m_uiTangentStride, geometry.m_uiTexCoordStride, geometry.m_Description.m_uiFirstPrimitive);
    if (deformed)
      gpuGeometry.BufferIndices.x = deformed->m_Srv.m_uiIndex;
  }

  if (instanceData.IsEmpty())
    return result;

  const xiiUInt32 uiFrameSlot = static_cast<xiiUInt32>(uiFrameIndex % world.m_Frames.GetCount());
  if (!PrepareFrameResources(worldResources, uiFrameSlot, instanceData.GetCount()))
    return result;

  FrameResources& frame       = world.m_Frames[uiFrameSlot];
  State*          pState      = s_pState.Borrow();
  const auto      fingerprint = xiiHashingUtils::xxHash64(instanceData.GetData(), instanceData.GetCount() * sizeof(xiiGALTLASInstanceData), s_pState->m_uiResourceRevision ^ deformationRevision);
  const auto      pWorld      = worldResources.m_pState;
  const bool      bNeedsBuild = !frame.m_bReady || frame.m_uiBuiltRevision != fingerprint || !pendingBLASBuilds.IsEmpty();
  if (bNeedsBuild)
  {
    auto pass = graph.AddPass<RayTracingSceneBuildPassData>("Ray Tracing Scene Build", xiiGALCommandQueueFlags::Compute, [&, uiFrameSlot](RayTracingSceneBuildPassData& data, xiiRenderGraphBuilder& builder) {
        data.m_BLASBuilds         = pendingBLASBuilds;
        data.m_Instances          = instanceData;
        data.m_pTopLevelAS        = frame.m_pTopLevelAS;
        data.m_pInstanceBuffer    = frame.m_pInstanceBuffer;
        data.m_pTLASScratchBuffer = frame.m_pScratchBuffer;
        data.m_uiResourceRevision    = fingerprint;
        data.m_uiFrameSlot        = uiFrameSlot;
        data.m_bUpdateTLAS        = frame.m_bReady && frame.m_uiBuiltInstanceCount == instanceData.GetCount();
        for (const auto dependency : blasDependencies)
          data.m_BLASDependencies.PushBack(builder.ReadBuffer(dependency, xiiGALResourceStateFlags::BuildASRead));

        xiiStringBuilder name;
        for (RayTracingBLASBuild& build : data.m_BLASBuilds)
        {
          name.SetFormat("Ray Tracing Vertex Data [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
          if (!build.m_hVertexBuffer.IsValid())
            build.m_hVertexBuffer = builder.ImportBuffer(name, build.m_pVertexBuffer, build.m_pVertexBuffer->GetResourceState());
          build.m_hVertexBuffer = builder.ReadBuffer(build.m_hVertexBuffer, xiiGALResourceStateFlags::BuildASRead | xiiGALResourceStateFlags::ShaderResource);
          if (build.m_pIndexBuffer != nullptr)
          {
            name.SetFormat("Ray Tracing Index Data [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
            build.m_hIndexBuffer = builder.ImportBuffer(name, build.m_pIndexBuffer, build.m_pIndexBuffer->GetResourceState());
            build.m_hIndexBuffer = builder.ReadBuffer(build.m_hIndexBuffer, xiiGALResourceStateFlags::BuildASRead | xiiGALResourceStateFlags::ShaderResource);
          }
          if (build.m_Dynamic)
            name.SetFormat("World {} Dynamic BLAS Scratch [{}:{}]", geometryOwner.m_Id, geometryFrameIndex, build.m_DrawId);
          else
            name.SetFormat("Ray Tracing BLAS Scratch [{}:{}]", build.m_uiGeometryIndex, build.m_uiGeometryGeneration);
          build.m_hScratchBuffer = builder.ImportBuffer(name, build.m_pScratchBuffer, build.m_pScratchBuffer->GetResourceState());
          build.m_hScratchBuffer = builder.WriteBuffer(build.m_hScratchBuffer, xiiGALResourceStateFlags::BuildASWrite);
        }

        name.SetFormat("World {} Ray Tracing Instance Data [{}]", world.m_Id, uiFrameSlot);
        data.m_hInstanceBuffer = builder.ImportBuffer(name, data.m_pInstanceBuffer, data.m_pInstanceBuffer->GetResourceState());
        data.m_hInstanceBuffer = builder.WriteBuffer(data.m_hInstanceBuffer, xiiGALResourceStateFlags::BuildASRead);
        builder.ExportBuffer(data.m_hInstanceBuffer, xiiGALResourceStateFlags::BuildASRead);

        name.SetFormat("World {} Ray Tracing TLAS Scratch [{}]", world.m_Id, uiFrameSlot);
        data.m_hTLASScratchBuffer = builder.ImportBuffer(name, data.m_pTLASScratchBuffer, data.m_pTLASScratchBuffer->GetResourceState());
        data.m_hTLASScratchBuffer = builder.WriteBuffer(data.m_hTLASScratchBuffer, xiiGALResourceStateFlags::BuildASWrite);
        builder.SetPassSideEffects(true);
        builder.SetPassAllowMerge(false); }, [pState, pWorld](const RayTracingSceneBuildPassData& data, xiiRenderGraphPassContext& context) {
        xiiGALCommandList& commandList = context.GetCommandList();
        commandList.UpdateTLASInstanceBuffer(context.GetBuffer(data.m_hInstanceBuffer), 0U, data.m_Instances.GetArrayPtr());

        for (const RayTracingBLASBuild& build : data.m_BLASBuilds)
        {
          xiiGALBuildBLASDescription description;
          description.m_pBottomLevelAS                 = build.m_pBottomLevelAS.Borrow();
          description.m_pScratchBuffer                 = context.GetBuffer(build.m_hScratchBuffer);
          description.m_bUpdate                        = build.m_Update;
          xiiGALBLASTriangleBuildDescription& triangle = description.m_Triangles.ExpandAndGetRef();
          triangle.m_pVertexBuffer                     = context.GetBuffer(build.m_hVertexBuffer);
          triangle.m_uiVertexBufferOffset              = build.m_uiVertexBufferOffset;
          triangle.m_uiVertexStride                    = build.m_uiVertexStride;
          triangle.m_pIndexBuffer                      = build.m_hIndexBuffer.IsValid() ? context.GetBuffer(build.m_hIndexBuffer) : nullptr;
          triangle.m_uiPrimitiveCount                  = build.m_uiPrimitiveCount;
          triangle.m_uiIndexBufferOffset = build.m_uiIndexBufferOffset;
          commandList.BuildBLAS(description);
        }

        xiiGALBuildTLASDescription tlasDescription;
        tlasDescription.m_pTopLevelAS     = data.m_pTopLevelAS.Borrow();
        tlasDescription.m_pInstanceBuffer = context.GetBuffer(data.m_hInstanceBuffer);
        tlasDescription.m_uiInstanceCount = data.m_Instances.GetCount();
        tlasDescription.m_pScratchBuffer  = context.GetBuffer(data.m_hTLASScratchBuffer);
        tlasDescription.m_bUpdate         = data.m_bUpdateTLAS;
        commandList.BuildTLAS(tlasDescription);

        for (const RayTracingBLASBuild& build : data.m_BLASBuilds)
        {
          if (build.m_Dynamic)
            build.m_Dynamic->m_ReadyRevision = build.m_DeformationRevision;
          if (!build.m_Dynamic && build.m_uiGeometryIndex < pState->m_Geometries.GetCount())
          {
            GeometrySlot& geometry = pState->m_Geometries[build.m_uiGeometryIndex];
            if (geometry.m_bAllocated && geometry.m_Id.m_Generation == build.m_uiGeometryGeneration)
              geometry.m_bBLASDirty = false;
          }
        }
        if (data.m_uiFrameSlot < pWorld->m_Frames.GetCount())
        {
          FrameResources& frame = pWorld->m_Frames[data.m_uiFrameSlot];
          if (frame.m_pTopLevelAS == data.m_pTopLevelAS)
          {
            frame.m_uiBuiltRevision      = data.m_uiResourceRevision;
            frame.m_uiBuiltInstanceCount = data.m_Instances.GetCount();
            frame.m_bReady               = true;
          }
        } }, true);

    result.m_hSceneDependency = pass.first->m_hInstanceBuffer;
    for (const auto& build : pendingBLASBuilds)
      if (build.m_Dynamic)
        build.m_Dynamic->m_Dependency = result.m_hSceneDependency;
      else
        s_pState->m_Geometries[build.m_uiGeometryIndex].m_BuildDependency = result.m_hSceneDependency;
  }

  result.m_pTopLevelAS     = frame.m_pTopLevelAS;
  result.m_uiInstanceCount = instanceData.GetCount();

  auto hitDataUploadPass = graph.AddPass<RayTracingHitDataUploadPassData>("Ray Tracing Hit Data Upload", xiiGALCommandQueueFlags::Compute, [&, uiFrameSlot](RayTracingHitDataUploadPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_Materials       = materialData;
      data.m_Geometries      = geometryData;
      data.m_pMaterialBuffer = frame.m_pMaterialBuffer;
      data.m_pGeometryBuffer = frame.m_pGeometryBuffer;

      xiiStringBuilder name;
      if (!result.m_hSceneDependency.IsValid())
      {
        // An unchanged frame slot still publishes explicit readiness. Its TLAS
        // was built in an earlier completed frame; no rebuild is necessary.
        name.SetFormat("World {} Ray Tracing Instance Data [{}]", world.m_Id, uiFrameSlot);
        result.m_hSceneDependency = builder.ImportBuffer(name, frame.m_pInstanceBuffer, frame.m_pInstanceBuffer->GetResourceState());
      }
      data.m_hSceneDependency = builder.ReadBuffer(result.m_hSceneDependency, xiiGALResourceStateFlags::BuildASRead);

      for (auto* deformed : deformedDependencies)
      {
        auto& dependency     = data.m_GeometryDependencies.ExpandAndGetRef();
        dependency.m_pBuffer = deformed->m_Buffer;
        dependency.m_hBuffer = builder.ReadBuffer(deformed->m_Handle, xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::BuildASRead);
      }
      name.SetFormat("World {} Ray Tracing Material Data [{}]", world.m_Id, uiFrameSlot);
      data.m_hMaterialBuffer = builder.ImportBuffer(name, data.m_pMaterialBuffer, data.m_pMaterialBuffer->GetResourceState());
      data.m_hMaterialBuffer = builder.WriteBuffer(data.m_hMaterialBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hMaterialBuffer, xiiGALResourceStateFlags::ShaderResource);

      name.SetFormat("World {} Ray Tracing Geometry Data [{}]", world.m_Id, uiFrameSlot);
      data.m_hGeometryBuffer = builder.ImportBuffer(name, data.m_pGeometryBuffer, data.m_pGeometryBuffer->GetResourceState());
      data.m_hGeometryBuffer = builder.WriteBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::CopyDestination);
      builder.ExportBuffer(data.m_hGeometryBuffer, xiiGALResourceStateFlags::ShaderResource);

      for (xiiUInt32 uiGeometryIndex = 0U; uiGeometryIndex < pState->m_Geometries.GetCount(); ++uiGeometryIndex)
      {
        const GeometrySlot& geometry = pState->m_Geometries[uiGeometryIndex];
        if (!geometry.m_bAllocated || geometry.m_pBottomLevelAS == nullptr)
          continue;

        RayTracingHitDataUploadPassData::GeometryDependency& vertex = data.m_GeometryDependencies.ExpandAndGetRef();
        vertex.m_pBuffer                                            = geometry.m_pVertexBuffer;
        vertex.m_uiGeometryIndex                                    = uiGeometryIndex;
        vertex.m_uiGeneration                                       = geometry.m_Id.m_Generation;
        name.SetFormat("Ray Tracing Vertex Data [{}:{}]", uiGeometryIndex, geometry.m_Id.m_Generation);
        vertex.m_hBuffer = builder.ImportBuffer(name, vertex.m_pBuffer, vertex.m_pBuffer->GetResourceState());
        vertex.m_hBuffer = builder.ReadBuffer(vertex.m_hBuffer, xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::BuildASRead);

        if (geometry.m_pIndexBuffer != nullptr)
        {
          RayTracingHitDataUploadPassData::GeometryDependency& index = data.m_GeometryDependencies.ExpandAndGetRef();
          index.m_pBuffer                                            = geometry.m_pIndexBuffer;
          index.m_uiGeometryIndex                                    = uiGeometryIndex;
          index.m_uiGeneration                                       = geometry.m_Id.m_Generation;
          index.m_bIndexBuffer                                       = true;
          name.SetFormat("Ray Tracing Index Data [{}:{}]", uiGeometryIndex, geometry.m_Id.m_Generation);
          index.m_hBuffer = builder.ImportBuffer(name, index.m_pBuffer, index.m_pBuffer->GetResourceState());
          index.m_hBuffer = builder.ReadBuffer(index.m_hBuffer, xiiGALResourceStateFlags::ShaderResource | xiiGALResourceStateFlags::BuildASRead);
        }
      }

      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false); }, [](const RayTracingHitDataUploadPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& commandList = context.GetCommandList();
      commandList.UpdateBuffer(context.GetBuffer(data.m_hMaterialBuffer), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_Materials.GetData()), data.m_Materials.GetCount() * sizeof(xiiRayTracingMaterialData)));
      commandList.UpdateBuffer(context.GetBuffer(data.m_hGeometryBuffer), 0U, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(data.m_Geometries.GetData()), data.m_Geometries.GetCount() * sizeof(xiiRayTracingGeometryData))); }, true);

  result.m_hMaterialData = hitDataUploadPass.first->m_hMaterialBuffer;
  result.m_hGeometryData = hitDataUploadPass.first->m_hGeometryBuffer;
  result.m_Result        = XII_SUCCESS;
  return result;
}

xiiRayTracingResourceStats xiiRayTracingResources::GetStats()
{
  xiiRayTracingResourceStats stats;
  if (!IsInitialized())
    return stats;

  stats.m_uiGeometryCount    = s_pState->m_uiGeometryCount;
  stats.m_uiResourceRevision = s_pState->m_uiResourceRevision;
  for (const GeometrySlot& slot : s_pState->m_Geometries)
  {
    if (slot.m_bAllocated && slot.m_bBLASDirty)
      ++stats.m_uiPendingBLASCount;
  }
  return stats;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_RayTracingResources);
