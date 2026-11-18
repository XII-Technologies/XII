/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Components/Lights/DirectionalLightComponent.h>
#include <GraphicsCore/Components/Lights/DiscAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/PointLightComponent.h>
#include <GraphicsCore/Components/Lights/RectangleAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SphereAreaLightComponent.h>
#include <GraphicsCore/Components/Lights/SpotLightComponent.h>
#include <GraphicsCore/Components/Lights/TubeAreaLightComponent.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Lighting/IESProfileResource.h>
#include <GraphicsCore/Lighting/WorldLightData.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderGraphManager.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
namespace
{
  static xiiVec3 NormalizeOrFallback(xiiVec3 vDirection, const xiiVec3& vFallback)
  {
    vDirection.NormalizeIfNotZero(vFallback).IgnoreResult();
    return vDirection;
  }

  static xiiVec4 MakeVec4(const xiiColor& color, float fAlpha)
  {
    return xiiVec4(color.r, color.g, color.b, fAlpha);
  }

  static xiiVec4 MakeVec4(const xiiVec3& vVector, float fW)
  {
    return xiiVec4(vVector.x, vVector.y, vVector.z, fW);
  }

  static float GetSafeRange(float fRange, float fIntensity)
  {
    if (fRange > 0.0f)
    {
      return fRange;
    }

    return xiiLightComponent::CalculateEffectiveRange(0.0f, fIntensity);
  }

  bool Pack(const xiiRenderData* pRenderData, xiiGpuLightData& result, xiiIESProfileResourceHandle& profile)
  {
    const auto finish = [&](xiiGpuLightData value, xiiGpuLightType type) {
      value.m_Metadata.z = xiiUInt32(type);
      result             = value;
      return true;
    };

    if (const xiiDirectionalLightRenderData* pDirectionalLight = xiiDynamicCast<const xiiDirectionalLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const xiiColor lightColor = xiiEvaluateLightColor(pDirectionalLight->m_LightColor, pDirectionalLight->m_uiTemperature);
      const xiiVec3  vDirection = NormalizeOrFallback(pDirectionalLight->m_vDirection, xiiVec3(0.0f, 0.0f, -1.0f));

      lightData.m_PositionAndInvRange   = xiiVec4::MakeZero();
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Directional));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pDirectionalLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(0.0f, pDirectionalLight->m_fRadius, 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pDirectionalLight->m_bCastShadows ? 1.0f : 0.0f, 0.0f, pDirectionalLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = xiiVec4::MakeZero();

      return finish(lightData, xiiGpuLightType::Directional);
    }

    if (const xiiPointLightRenderData* pPointLight = xiiDynamicCast<const xiiPointLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pPointLight->m_fRange, pPointLight->m_fPhotometricIntensity), 0.001f);
      const xiiVec3  vPosition  = pPointLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pPointLight->m_qGlobalRotation * xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(1.0f, 0.0f, 0.0f));
      const xiiVec3  vRight     = NormalizeOrFallback(pPointLight->m_qGlobalRotation * xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f));
      const xiiColor lightColor = xiiEvaluateLightColor(pPointLight->m_LightColor, pPointLight->m_uiTemperature);

      lightData.m_PositionAndInvRange    = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType       = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Point));
      lightData.m_ColorAndIntensity      = MakeVec4(lightColor, pPointLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize     = xiiVec4(fRange, pPointLight->m_fRadius, pPointLight->m_fLength, 0.0f);
      lightData.m_SpotAnglesAndRectSize  = xiiVec4(1.0f, -1.0f, 0.0f, 0.0f);
      lightData.m_ShadowData             = xiiVec4(pPointLight->m_bCastShadows ? 1.0f : 0.0f, pPointLight->m_fShadowFadeOutRange, pPointLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius  = MakeVec4(vPosition, fRange + pPointLight->m_fLength * 0.5f);
      lightData.m_OrientationRightAndIES = MakeVec4(vRight, 0.0f);

      profile = pPointLight->m_hIESProfile;
      return finish(lightData, xiiGpuLightType::Point);
    }

    if (const xiiSpotLightRenderData* pSpotLight = xiiDynamicCast<const xiiSpotLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pSpotLight->m_fRange, pSpotLight->m_fPhotometricIntensity), 0.001f);
      const xiiVec3  vPosition  = pSpotLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pSpotLight->m_qGlobalRotation * xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(1.0f, 0.0f, 0.0f));
      const xiiVec3  vRight     = NormalizeOrFallback(pSpotLight->m_qGlobalRotation * xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f));
      const xiiColor lightColor = xiiEvaluateLightColor(pSpotLight->m_LightColor, pSpotLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Spot));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pSpotLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, pSpotLight->m_fRadius, 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(xiiMath::Cos(pSpotLight->m_InnerSpotAngle * 0.5f), xiiMath::Cos(pSpotLight->m_OuterSpotAngle * 0.5f), 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pSpotLight->m_bCastShadows ? 1.0f : 0.0f, pSpotLight->m_fShadowFadeOutRange, pSpotLight->m_fRadius, 0.0f);

      const xiiBoundingSphere boundingSphere = pSpotLight->m_GlobalBounds.GetSphere();
      lightData.m_BoundsCenterAndRadius      = MakeVec4(boundingSphere.m_vCenter, xiiMath::Max(boundingSphere.m_fRadius, fRange));
      lightData.m_OrientationRightAndIES     = MakeVec4(vRight, 0.0f);

      profile = pSpotLight->m_hIESProfile;
      return finish(lightData, xiiGpuLightType::Spot);
    }

    if (const xiiRectangleAreaLightRenderData* pRectangleLight = xiiDynamicCast<const xiiRectangleAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(pRectangleLight->m_fRadius, 0.001f);
      const xiiVec3  vPosition  = pRectangleLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pRectangleLight->m_qGlobalRotation * xiiVec3(-1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f));
      const xiiVec3  vRight     = NormalizeOrFallback(pRectangleLight->m_qGlobalRotation * xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f));
      const xiiColor lightColor = xiiEvaluateLightColor(pRectangleLight->m_LightColor, pRectangleLight->m_uiTemperature);

      lightData.m_PositionAndInvRange    = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType       = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Rectangle));
      lightData.m_ColorAndIntensity      = MakeVec4(lightColor, pRectangleLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize     = xiiVec4(fRange, 0.0f, 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize  = xiiVec4(1.0f, -1.0f, xiiMath::Max(pRectangleLight->m_vExtents.x, 0.001f), xiiMath::Max(pRectangleLight->m_vExtents.y, 0.001f));
      lightData.m_ShadowData             = xiiVec4(pRectangleLight->m_bCastShadows ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
      lightData.m_BoundsCenterAndRadius  = MakeVec4(vPosition, fRange + pRectangleLight->m_vExtents.GetLength() * 0.5f);
      lightData.m_OrientationRightAndIES = MakeVec4(vRight, 0.0f);

      return finish(lightData, xiiGpuLightType::Rectangle);
    }

    if (const xiiDiscAreaLightRenderData* pDiscLight = xiiDynamicCast<const xiiDiscAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(GetSafeRange(pDiscLight->m_fRange, pDiscLight->m_fPhotometricIntensity), 0.001f);
      const xiiVec3  vPosition  = pDiscLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pDiscLight->m_qGlobalRotation * xiiVec3(-1.0f, 0.0f, 0.0f), xiiVec3(-1.0f, 0.0f, 0.0f));
      const xiiVec3  vRight     = NormalizeOrFallback(pDiscLight->m_qGlobalRotation * xiiVec3(0.0f, 1.0f, 0.0f), xiiVec3(0.0f, 1.0f, 0.0f));
      const xiiColor lightColor = xiiEvaluateLightColor(pDiscLight->m_LightColor, pDiscLight->m_uiTemperature);

      lightData.m_PositionAndInvRange    = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType       = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Disc));
      lightData.m_ColorAndIntensity      = MakeVec4(lightColor, pDiscLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize     = xiiVec4(fRange, xiiMath::Max(pDiscLight->m_fRadius, 0.001f), 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize  = xiiVec4(1.0f, -1.0f, xiiMath::Max(pDiscLight->m_fRadius, 0.001f), 0.0f);
      lightData.m_ShadowData             = xiiVec4(pDiscLight->m_bCastShadows ? 1.0f : 0.0f, pDiscLight->m_fShadowFadeOutRange, pDiscLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius  = MakeVec4(vPosition, fRange + pDiscLight->m_fRadius);
      lightData.m_OrientationRightAndIES = MakeVec4(vRight, 0.0f);

      return finish(lightData, xiiGpuLightType::Disc);
    }

    if (const xiiSphereAreaLightRenderData* pSphereLight = xiiDynamicCast<const xiiSphereAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(pSphereLight->m_fRange, 0.001f);
      const xiiVec3  vPosition  = pSphereLight->m_GlobalTransform.m_vPosition;
      const xiiColor lightColor = xiiEvaluateLightColor(pSphereLight->m_LightColor, pSphereLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(xiiVec3::MakeZero(), static_cast<float>(xiiGpuLightType::Sphere));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pSphereLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, xiiMath::Max(pSphereLight->m_fRadius, 0.001f), 0.0f, 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(1.0f, -1.0f, 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pSphereLight->m_bCastShadows ? 1.0f : 0.0f, pSphereLight->m_fShadowFadeOutRange, pSphereLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = MakeVec4(vPosition, fRange + pSphereLight->m_fRadius);

      return finish(lightData, xiiGpuLightType::Sphere);
    }

    if (const xiiTubeAreaLightRenderData* pTubeLight = xiiDynamicCast<const xiiTubeAreaLightRenderData*>(pRenderData))
    {
      xiiGpuLightData lightData;
      xiiMemoryUtils::ZeroFill(&lightData, 1);

      const float    fRange     = xiiMath::Max(pTubeLight->m_fRange, 0.001f);
      const xiiVec3  vPosition  = pTubeLight->m_GlobalTransform.m_vPosition;
      const xiiVec3  vDirection = NormalizeOrFallback(pTubeLight->m_qGlobalRotation * xiiVec3(1.0f, 0.0f, 0.0f), xiiVec3(1.0f, 0.0f, 0.0f));
      const xiiColor lightColor = xiiEvaluateLightColor(pTubeLight->m_LightColor, pTubeLight->m_uiTemperature);

      lightData.m_PositionAndInvRange   = MakeVec4(vPosition, 1.0f / fRange);
      lightData.m_DirectionAndType      = MakeVec4(vDirection, static_cast<float>(xiiGpuLightType::Tube));
      lightData.m_ColorAndIntensity     = MakeVec4(lightColor, pTubeLight->m_fPhotometricIntensity);
      lightData.m_AttenuationAndSize    = xiiVec4(fRange, xiiMath::Max(pTubeLight->m_fRadius, 0.001f), xiiMath::Max(pTubeLight->m_fLength, 0.001f), 0.0f);
      lightData.m_SpotAnglesAndRectSize = xiiVec4(1.0f, -1.0f, 0.0f, 0.0f);
      lightData.m_ShadowData            = xiiVec4(pTubeLight->m_bCastShadows ? 1.0f : 0.0f, pTubeLight->m_fShadowFadeOutRange, pTubeLight->m_fRadius, 0.0f);
      lightData.m_BoundsCenterAndRadius = MakeVec4(vPosition, fRange + pTubeLight->m_fLength * 0.5f + pTubeLight->m_fRadius);

      return finish(lightData, xiiGpuLightType::Tube);
    }

    return false;
  }
  xiiAtomicInteger64 nextIdentity, nextOwner;
} // namespace
namespace
{
  class WorldLightUploadSnapshot : public xiiRefCounted
  {
  public:
    xiiDynamicArray<xiiGpuLightData> m_Records;
    xiiDynamicArray<float>           m_Profiles;
    xiiVec4U32                       m_Counts = xiiVec4U32::MakeZero();
    WorldLightUploadSnapshot()
    {
      xiiGpuLightData empty;
      xiiMemoryUtils::ZeroFill(&empty, 1);
      m_Records.PushBack(empty);
      m_Profiles.PushBack(0);
    }
  };
} // namespace
class xiiWorldLightDataState : public xiiRefCounted
{
public:
  struct Source
  {
    xiiComponentHandle m_Component;
    xiiUInt32          m_Identity = 0;
  };
  struct Slot : public xiiRefCounted
  {
    xiiSharedPtr<xiiGALBuffer>                   m_Lights, m_Profiles, m_Counts;
    xiiSharedPtr<const WorldLightUploadSnapshot> m_Uploaded;
    xiiUInt64                                    m_LastUse     = 0;
    bool                                         m_Initialized = false;
  };
  explicit xiiWorldLightDataState(const xiiWorld& world) : m_World(world), m_Owner(nextOwner.Increment()), m_Snapshot(XII_DEFAULT_NEW(WorldLightUploadSnapshot)) {}
  const xiiWorld&                              m_World;
  xiiUInt64                                    m_Owner, m_StatsFrame = 0, m_UploadStructure = 0;
  xiiUInt64                                    m_uiUploadContractRevision = 0U;
  xiiUInt32                                    m_uiLightCapacity          = 1U;
  xiiUInt32                                    m_uiProfileCapacity        = 1U;
  xiiWorldLightData::UploadHandles             m_Upload;
  xiiDynamicArray<Source>                      m_Sources;
  xiiHashTable<xiiComponentHandle, xiiUInt32>  m_Indices;
  xiiSharedPtr<const WorldLightUploadSnapshot> m_Snapshot;
  xiiDynamicArray<xiiSharedPtr<Slot>>          m_Slots;
  xiiWorldLightUploadStatistics                m_Stats;
  bool                                         Live(xiiComponentHandle handle) const
  {
    const xiiComponent* component = nullptr;
    return !handle.IsInvalidated() && handle.GetInternalID().m_WorldIndex == m_World.GetIndex() && m_World.TryGetComponent(handle, component) && component->IsActiveAndInitialized() && component->GetOwner()->IsActive();
  }
};
xiiWorldLightData::xiiWorldLightData(const xiiWorld& world) : m_State(XII_DEFAULT_NEW(xiiWorldLightDataState, world)) {}
xiiWorldLightData::~xiiWorldLightData() = default;
xiiArrayPtr<const xiiGpuLightData>   xiiWorldLightData::GetLights() const { return m_State->m_Snapshot->m_Records.GetArrayPtr().GetSubArray(0, m_State->m_Snapshot->m_Counts.x); }
const xiiWorldLightUploadStatistics& xiiWorldLightData::GetUploadStatistics() const { return m_State->m_Stats; }
xiiUInt64                            xiiWorldLightData::GetUploadContractRevision() const { return m_State->m_uiUploadContractRevision; }
xiiUInt32                            xiiWorldLightData::GetIndex(xiiComponentHandle component) const
{
  if (!m_State->Live(component))
    return xiiInvalidIndex;
  xiiUInt32 index = xiiInvalidIndex;
  m_State->m_Indices.TryGetValue(component, index);
  return index;
}
xiiResult xiiWorldLightData::Resolve(xiiUInt32 identity, xiiComponentHandle& component) const
{
  component = {};
  for (const auto& source : m_State->m_Sources)
    if (source.m_Identity == identity && m_State->Live(source.m_Component))
    {
      component = source.m_Component;
      return XII_SUCCESS;
    }
  return XII_FAILURE;
}
xiiResult xiiWorldLightData::UpdateFromExtraction(xiiArrayPtr<xiiRenderData* const> packets)
{
  auto& state = *m_State;
  struct Item
  {
    xiiGpuLightData                m_Record;
    xiiWorldLightDataState::Source m_Source;
    xiiIESProfileResourceHandle    m_Profile;
    xiiUInt32                      m_ProfileHash = 0;
  };
  xiiDynamicArray<Item>                       items;
  xiiHashSet<xiiComponentHandle>              seen;
  xiiHashTable<xiiComponentHandle, xiiUInt32> identities;
  for (const auto& source : state.m_Sources)
    identities.Insert(source.m_Component, source.m_Identity);
  for (const auto* packet : packets)
  {
    if (!packet || !state.Live(packet->m_hOwnerComponent) || seen.Contains(packet->m_hOwnerComponent))
      continue;
    const xiiComponent* component = nullptr;
    if (!state.m_World.TryGetComponent(packet->m_hOwnerComponent, component) || component->GetOwner()->GetHandle() != packet->m_hOwnerObject)
      continue;
    Item item;
    if (!Pack(packet, item.m_Record, item.m_Profile))
      continue;
    if (items.GetCount() >= 1048576)
      return XII_FAILURE;
    seen.Insert(packet->m_hOwnerComponent);
    item.m_Source.m_Component = packet->m_hOwnerComponent;
    if (!identities.TryGetValue(packet->m_hOwnerComponent, item.m_Source.m_Identity))
    {
      const auto token = nextIdentity.Increment();
      if (token >= xiiInvalidIndex)
        return XII_FAILURE;
      item.m_Source.m_Identity = xiiUInt32(token);
    }
    item.m_Record.m_Metadata.x = item.m_Source.m_Identity;
    items.PushBack(item);
  }
  items.Sort([](const Item& a, const Item& b) {
    const bool ad = a.m_Record.m_Metadata.z == xiiUInt32(xiiGpuLightType::Directional), bd = b.m_Record.m_Metadata.z == xiiUInt32(xiiGpuLightType::Directional);
    if (ad != bd)
      return ad;
    if (ad && a.m_Record.m_ColorAndIntensity.w != b.m_Record.m_ColorAndIntensity.w)
      return a.m_Record.m_ColorAndIntensity.w > b.m_Record.m_ColorAndIntensity.w;
    return a.m_Source.m_Component.GetInternalID().m_Data < b.m_Source.m_Component.GetInternalID().m_Data;
  });
  xiiDynamicArray<float>                               profiles;
  xiiDynamicArray<xiiUInt32>                           profileHashes;
  xiiHashTable<xiiIESProfileResourceHandle, xiiUInt32> profileSlots;
  for (auto& item : items)
  {
    if (!item.m_Profile.IsValid())
      continue;
    xiiUInt32 slot = 0;
    if (!profileSlots.TryGetValue(item.m_Profile, slot))
    {
      if (profileSlots.GetCount() >= 4096)
        return XII_FAILURE;
      xiiResourceLock<xiiIESProfileResource> profile(item.m_Profile, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!profile.IsValid() || profile.GetAcquireResult() != xiiResourceAcquireResult::Final || !profile->GetDescriptor().IsValid())
        return XII_FAILURE;
      slot = profileSlots.GetCount();
      profileSlots.Insert(item.m_Profile, slot);
      const auto& values = profile->GetDescriptor().m_NormalizedCandela;
      profiles.PushBackRange(values);
      profileHashes.PushBack(xiiHashingUtils::xxHash32(values.GetData(), values.GetCount() * sizeof(float)));
    }
    item.m_Record.m_OrientationRightAndIES.w = float(slot + 1);
    item.m_ProfileHash                       = profileHashes[slot];
  }
  xiiSharedPtr<WorldLightUploadSnapshot> next = XII_DEFAULT_NEW(WorldLightUploadSnapshot);
  next->m_Records.Clear();
  next->m_Profiles = std::move(profiles);
  if (next->m_Profiles.IsEmpty())
    next->m_Profiles.PushBack(0);
  next->m_Counts = xiiVec4U32(items.GetCount(), 0, profileSlots.GetCount(), 0);
  state.m_Sources.Clear();
  state.m_Indices.Clear();
  for (auto& item : items)
  {
    item.m_Record.m_Metadata.y = next->m_Records.GetCount();
    item.m_Record.m_Metadata.w = xiiHashingUtils::xxHash32(&item.m_Record, offsetof(xiiGpuLightData, m_Metadata), item.m_ProfileHash) & 0xFFFFU;
    state.m_Indices.Insert(item.m_Source.m_Component, next->m_Records.GetCount());
    next->m_Records.PushBack(item.m_Record);
    state.m_Sources.PushBack(item.m_Source);
    if (item.m_Record.m_Metadata.z == xiiUInt32(xiiGpuLightType::Directional))
      ++next->m_Counts.y;
  }
  if (next->m_Records.IsEmpty())
  {
    xiiGpuLightData empty;
    xiiMemoryUtils::ZeroFill(&empty, 1);
    next->m_Records.PushBack(empty);
  }
  if (next->m_Records.GetCount() > state.m_uiLightCapacity || next->m_Profiles.GetCount() > state.m_uiProfileCapacity)
  {
    const xiiUInt32 uiLightCapacity   = xiiMath::PowerOfTwo_Ceil(xiiMath::Max(state.m_uiLightCapacity, next->m_Records.GetCount()));
    const xiiUInt32 uiProfileCapacity = xiiMath::PowerOfTwo_Ceil(xiiMath::Max(state.m_uiProfileCapacity, next->m_Profiles.GetCount()));
    if (uiLightCapacity == 0U || uiProfileCapacity == 0U)
      return XII_FAILURE;

    state.m_uiLightCapacity   = uiLightCapacity;
    state.m_uiProfileCapacity = uiProfileCapacity;
    ++state.m_uiUploadContractRevision;
  }
  const auto previous = state.m_Snapshot;
  if (previous->m_Counts != next->m_Counts || previous->m_Profiles != next->m_Profiles || previous->m_Records.GetCount() != next->m_Records.GetCount() || !xiiMemoryUtils::IsEqual(previous->m_Records.GetData(), next->m_Records.GetData(), next->m_Records.GetCount()))
    state.m_Snapshot = next;
  return XII_SUCCESS;
}

xiiWorldLightData::UploadHandles xiiWorldLightData::DeclareUploadPass(xiiRenderGraph& graph) const
{
  auto state = m_State;
  if (state->m_UploadStructure == graph.GetStructuralGeneration())
    return state->m_Upload;

  struct Range
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_uiFirst, m_uiCount;
  };
  struct Data
  {
    UploadHandles                                m_Handles;
    xiiSharedPtr<const WorldLightUploadSnapshot> m_pSnapshot;
    xiiDynamicArray<Range>                       m_Ranges;
    xiiSharedPtr<xiiWorldLightDataState::Slot>   m_pSlot;
    bool                                         m_bUpdateProfiles = false;
    bool                                         m_bUpdateCounts   = false;
  };
  const auto pass          = graph.AddPass<Data>("World Light Sparse Upload", xiiGALCommandQueueFlags::Transfer, [&](Data& ref_data, xiiRenderGraphBuilder& ref_builder) {
    const auto DeclareBuffer = [&](xiiStringView sName, xiiUInt32 uiStride, xiiUInt32 uiCount) {
      xiiGALBufferCreationDescription description;
      description.m_uiSize = xiiUInt64(uiStride) * uiCount;
      description.m_uiElementByteStride = uiStride;
      description.m_Mode = xiiGALBufferMode::Structured;
      description.m_Usage = xiiGALResourceUsage::Mutable;
      description.m_BindFlags = xiiGALBindFlags::ShaderResource;
      xiiStringBuilder name;
      name.SetFormat("World {} {}", state->m_Owner, sName);
      return ref_builder.WriteBuffer(ref_builder.DeclareImportedBuffer(name, description), xiiGALResourceStateFlags::CopyDestination);
    };
    ref_data.m_Handles.m_Lights = DeclareBuffer("Light Records", sizeof(xiiGpuLightData), state->m_uiLightCapacity);
    ref_data.m_Handles.m_Profiles = DeclareBuffer("IES Profiles", sizeof(float), state->m_uiProfileCapacity);
    ref_data.m_Handles.m_Counts = DeclareBuffer("Light Counts", sizeof(xiiVec4U32), 1U);

    ref_builder.SetPassFramePreparation([state, &ref_data](xiiRenderGraph& ref_graph, const xiiView*, xiiStringBuilder& ref_error) {
      xiiUInt64 uiCompleted = 0U;
      const xiiUInt64 uiFrame = ref_graph.GetFrameIndex();
      if (!xiiGALDevice::HasDefaultDevice() || xiiRenderGraphManager::GetFrameCompletion(uiFrame, uiCompleted).Failed())
      {
        ref_error = "World light uploads require an active shared GPU frame.";
        return XII_FAILURE;
      }

      const auto pDevice = xiiGALDevice::GetDefaultDevice();
      const auto pSnapshot = state->m_Snapshot;
      const auto& lights = *ref_graph.GetBufferDescription(ref_data.m_Handles.m_Lights);
      const auto& profiles = *ref_graph.GetBufferDescription(ref_data.m_Handles.m_Profiles);
      const auto& counts = *ref_graph.GetBufferDescription(ref_data.m_Handles.m_Counts);
      if (pSnapshot->m_Records.GetCount() > lights.m_uiSize / sizeof(xiiGpuLightData) ||
          pSnapshot->m_Profiles.GetCount() > profiles.m_uiSize / sizeof(float))
      {
        ref_error = "The current world light snapshot exceeds its declared upload capacity.";
        return XII_FAILURE;
      }

      xiiSharedPtr<xiiWorldLightDataState::Slot> pSlot;
      for (const auto& pCandidate : state->m_Slots)
      {
        if (pCandidate->m_LastUse == uiFrame || pCandidate->m_LastUse <= uiCompleted)
        {
          pSlot = pCandidate;
          break;
        }
      }
      if (pSlot == nullptr)
      {
        pSlot = XII_DEFAULT_NEW(xiiWorldLightDataState::Slot);
        state->m_Slots.PushBack(pSlot);
      }
      const auto ResolveBuffer = [&](const xiiSharedPtr<xiiGALBuffer>& pBuffer, const xiiGALBufferCreationDescription& description) {
        if (pBuffer != nullptr && pBuffer->GetDescription() == description)
          return pBuffer;
        return pDevice->CreateBuffer(description);
      };
      const auto pLights = ResolveBuffer(pSlot->m_Lights, lights);
      const auto pProfiles = ResolveBuffer(pSlot->m_Profiles, profiles);
      const auto pCounts = ResolveBuffer(pSlot->m_Counts, counts);
      if (pLights == nullptr || pProfiles == nullptr || pCounts == nullptr ||
          ref_graph.BindImportedBuffer(ref_data.m_Handles.m_Lights, pLights, pLights->GetResourceState()).Failed() ||
          ref_graph.BindImportedBuffer(ref_data.m_Handles.m_Profiles, pProfiles, pProfiles->GetResourceState()).Failed() ||
          ref_graph.BindImportedBuffer(ref_data.m_Handles.m_Counts, pCounts, pCounts->GetResourceState()).Failed())
      {
        ref_error = "Could not allocate or bind the declared world light buffers.";
        return XII_FAILURE;
      }

      const auto pUploaded = pSlot->m_Initialized ? pSlot->m_Uploaded : nullptr;
      const bool bResetLights = pSlot->m_Lights != pLights;
      ref_data.m_bUpdateProfiles = pSlot->m_Profiles != pProfiles || pUploaded == nullptr || pUploaded->m_Profiles != pSnapshot->m_Profiles;
      ref_data.m_bUpdateCounts = pSlot->m_Counts != pCounts || pUploaded == nullptr || pUploaded->m_Counts != pSnapshot->m_Counts;
      ref_data.m_Ranges.Clear();
      const auto Changed = [&](xiiUInt32 uiIndex) {
        return bResetLights || pUploaded == nullptr || uiIndex >= pUploaded->m_Records.GetCount() ||
               !xiiMemoryUtils::IsEqual(&pUploaded->m_Records[uiIndex], &pSnapshot->m_Records[uiIndex], 1U);
      };
      for (xiiUInt32 uiIndex = 0U; uiIndex < pSnapshot->m_Records.GetCount();)
      {
        if (!Changed(uiIndex))
        {
          ++uiIndex;
          continue;
        }
        const xiiUInt32 uiFirst = uiIndex++;
        while (uiIndex < pSnapshot->m_Records.GetCount() && Changed(uiIndex))
          ++uiIndex;
        ref_data.m_Ranges.PushBack({uiFirst, uiIndex - uiFirst});
      }

      if (bResetLights || pSlot->m_Profiles != pProfiles || pSlot->m_Counts != pCounts)
        pSlot->m_Initialized = false;
      pSlot->m_Lights = pLights;
      pSlot->m_Profiles = pProfiles;
      pSlot->m_Counts = pCounts;
      pSlot->m_LastUse = uiFrame;
      ref_data.m_pSnapshot = pSnapshot;
      ref_data.m_pSlot = pSlot;
      if (state->m_StatsFrame != uiFrame)
      {
        state->m_StatsFrame = uiFrame;
        state->m_Stats = {};
      }
      return XII_SUCCESS;
    }); }, [state](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands = execute.GetCommandList();
    for (const auto& range : data.m_Ranges)
    {
      const auto bytes = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_pSnapshot->m_Records.GetData() + range.m_uiFirst), range.m_uiCount * sizeof(xiiGpuLightData));
      commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Lights), range.m_uiFirst * sizeof(xiiGpuLightData), bytes);
      state->m_Stats.m_uiBytesUploaded += bytes.GetCount();
      ++state->m_Stats.m_uiRanges;
    }
    if (data.m_bUpdateProfiles)
    {
      const auto bytes = data.m_pSnapshot->m_Profiles.GetByteArrayPtr();
      commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Profiles), 0U, bytes);
      state->m_Stats.m_uiBytesUploaded += bytes.GetCount();
      ++state->m_Stats.m_uiRanges;
    }
    if (data.m_bUpdateCounts)
    {
      const auto bytes = xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_pSnapshot->m_Counts), sizeof(data.m_pSnapshot->m_Counts));
      commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Counts), 0U, bytes);
      state->m_Stats.m_uiBytesUploaded += bytes.GetCount();
      ++state->m_Stats.m_uiRanges;
    }
    if (!data.m_Ranges.IsEmpty() || data.m_bUpdateProfiles || data.m_bUpdateCounts)
      ++state->m_Stats.m_uiUploadPasses;
    data.m_pSlot->m_Uploaded = data.m_pSnapshot;
    data.m_pSlot->m_Initialized = true; });
  state->m_UploadStructure = graph.GetStructuralGeneration();
  state->m_Upload          = pass.first->m_Handles;
  return state->m_Upload;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_WorldLightData);
