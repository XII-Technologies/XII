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
#include <cmath>
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

  xiiColor EvaluateTemperatureColor(xiiUInt32 uiTemperature)
  {
    const float fTemperature = static_cast<float>(xiiMath::Clamp(uiTemperature, 1000U, 50000U)) / 100.0f;

    float fRed   = 1.0f;
    float fGreen = 1.0f;
    float fBlue  = 1.0f;

    if (fTemperature <= 66.0f)
    {
      fRed   = 1.0f;
      fGreen = xiiMath::Clamp(0.39008158f * static_cast<float>(std::log(fTemperature)) - 0.63184144f, 0.0f, 1.0f);
      fBlue  = fTemperature <= 19.0f ? 0.0f : xiiMath::Clamp(0.5432068f * static_cast<float>(std::log(fTemperature - 10.0f)) - 1.1962541f, 0.0f, 1.0f);
    }
    else
    {
      fRed   = xiiMath::Clamp(1.2929362f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.13320476f)), 0.0f, 1.0f);
      fGreen = xiiMath::Clamp(1.1298909f * static_cast<float>(std::pow(fTemperature - 60.0f, -0.07551485f)), 0.0f, 1.0f);
      fBlue  = 1.0f;
    }

    return xiiColor(fRed, fGreen, fBlue, 1.0f);
  }

  xiiColor EvaluateLightColor(const xiiColorLinearUB& color, xiiUInt32 uiTemperature)
  {
    xiiColor result = color.ToLinearFloat() * EvaluateTemperatureColor(uiTemperature);
    result.a        = 1.0f;
    return result;
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

      const xiiColor lightColor = EvaluateLightColor(pDirectionalLight->m_LightColor, pDirectionalLight->m_uiTemperature);
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
      const xiiColor lightColor = EvaluateLightColor(pPointLight->m_LightColor, pPointLight->m_uiTemperature);

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
      const xiiColor lightColor = EvaluateLightColor(pSpotLight->m_LightColor, pSpotLight->m_uiTemperature);

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
      const xiiColor lightColor = EvaluateLightColor(pRectangleLight->m_LightColor, pRectangleLight->m_uiTemperature);

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
      const xiiColor lightColor = EvaluateLightColor(pDiscLight->m_LightColor, pDiscLight->m_uiTemperature);

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
      const xiiColor lightColor = EvaluateLightColor(pSphereLight->m_LightColor, pSphereLight->m_uiTemperature);

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
      const xiiColor lightColor = EvaluateLightColor(pTubeLight->m_LightColor, pTubeLight->m_uiTemperature);

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
    xiiUInt64                                    m_Id = nextOwner.Increment();
    xiiSharedPtr<xiiGALBuffer>                   m_Lights, m_Profiles, m_Counts;
    xiiSharedPtr<const WorldLightUploadSnapshot> m_Uploaded;
    xiiUInt64                                    m_LastUse     = 0;
    bool                                         m_Initialized = false;
  };
  explicit xiiWorldLightDataState(const xiiWorld& world) : m_World(world), m_Owner(nextOwner.Increment()), m_Snapshot(XII_DEFAULT_NEW(WorldLightUploadSnapshot)) {}
  const xiiWorld&                              m_World;
  xiiUInt64                                    m_Owner, m_StatsFrame = 0, m_UploadSetup = 0;
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
  const auto previous = state.m_Snapshot;
  if (previous->m_Counts != next->m_Counts || previous->m_Profiles != next->m_Profiles || previous->m_Records.GetCount() != next->m_Records.GetCount() || !xiiMemoryUtils::IsEqual(previous->m_Records.GetData(), next->m_Records.GetData(), next->m_Records.GetCount()))
    state.m_Snapshot = next;
  return XII_SUCCESS;
}

xiiWorldLightData::UploadHandles xiiWorldLightData::AddUploadPass(xiiRenderGraph& graph) const
{
  auto state = m_State;
  if (state->m_UploadSetup == graph.GetSetupSerial())
    return state->m_Upload;
  xiiUInt64 completed = 0;
  if (!xiiGALDevice::HasDefaultDevice() || xiiRenderGraphManager::GetFrameCompletion(graph.GetFrameIndex(), completed).Failed())
    return {};
  xiiSharedPtr<xiiWorldLightDataState::Slot> slot;
  for (const auto& candidate : state->m_Slots)
    if (candidate->m_LastUse == graph.GetFrameIndex() || candidate->m_LastUse <= completed)
    {
      slot = candidate;
      break;
    }
  if (!slot)
  {
    slot = XII_DEFAULT_NEW(xiiWorldLightDataState::Slot);
    state->m_Slots.PushBack(slot);
  }
  slot->m_LastUse = graph.GetFrameIndex();
  if (state->m_StatsFrame != graph.GetFrameIndex())
  {
    state->m_StatsFrame = graph.GetFrameIndex();
    state->m_Stats      = {};
  }
  struct Range
  {
    XII_DECLARE_POD_TYPE();
    xiiUInt32 m_First, m_Count;
  };
  struct Data
  {
    UploadHandles                                m_Handles;
    xiiSharedPtr<const WorldLightUploadSnapshot> m_Snapshot;
    xiiDynamicArray<Range>                       m_Ranges;
    xiiSharedPtr<xiiWorldLightDataState::Slot>   m_Slot;
    bool                                         m_UpdateProfiles = false, m_UpdateCounts = false;
  };
  const auto allocate = [&](xiiSharedPtr<xiiGALBuffer>& buffer, xiiUInt32 stride, xiiUInt32 count) -> bool {
    const xiiUInt64 size = xiiUInt64(stride) * xiiMath::Max(count, 1U);
    if (buffer && buffer->GetDescription().m_uiSize == size)
      return true;
    xiiGALBufferCreationDescription description;
    description.m_uiSize              = size;
    description.m_uiElementByteStride = stride;
    description.m_Mode                = xiiGALBufferMode::Structured;
    description.m_Usage               = xiiGALResourceUsage::Mutable;
    description.m_BindFlags           = xiiGALBindFlags::ShaderResource;
    buffer                            = xiiGALDevice::GetDefaultDevice()->CreateBuffer(description);
    return buffer != nullptr;
  };
  const auto oldLights = slot->m_Lights, oldProfiles = slot->m_Profiles, oldCounts = slot->m_Counts;
  if (!allocate(slot->m_Lights, sizeof(xiiGpuLightData), state->m_Snapshot->m_Records.GetCount()) || !allocate(slot->m_Profiles, sizeof(float), state->m_Snapshot->m_Profiles.GetCount()) || !allocate(slot->m_Counts, sizeof(xiiVec4U32), 1))
    return {};
  const bool resetLights = oldLights != slot->m_Lights, resetProfiles = oldProfiles != slot->m_Profiles, resetCounts = oldCounts != slot->m_Counts;
  const auto pass      = graph.AddPass<Data>("World Light Sparse Upload", xiiGALCommandQueueFlags::Transfer, [&](Data& data, xiiRenderGraphBuilder& builder) {
    data.m_Slot=slot; data.m_Snapshot=state->m_Snapshot;
    xiiStringBuilder name;
    name.SetFormat("World {} Light Records [{}]",state->m_Owner,slot->m_Id);
    data.m_Handles.m_Lights=builder.ImportBuffer(name,slot->m_Lights,slot->m_Lights->GetResourceState());
    name.SetFormat("World {} IES Profiles [{}]",state->m_Owner,slot->m_Id);
    data.m_Handles.m_Profiles=builder.ImportBuffer(name,slot->m_Profiles,slot->m_Profiles->GetResourceState());
    name.SetFormat("World {} Light Counts [{}]",state->m_Owner,slot->m_Id);
    data.m_Handles.m_Counts=builder.ImportBuffer(name,slot->m_Counts,slot->m_Counts->GetResourceState());
    const auto changed=[&](xiiUInt32 i){return resetLights || !slot->m_Initialized || !slot->m_Uploaded || i>=slot->m_Uploaded->m_Records.GetCount() || !xiiMemoryUtils::IsEqual(&slot->m_Uploaded->m_Records[i],&data.m_Snapshot->m_Records[i],1);};
    for(xiiUInt32 i=0;i<data.m_Snapshot->m_Records.GetCount();)
    {
      if(!changed(i)) { ++i; continue; }
      const auto first=i++; while(i<data.m_Snapshot->m_Records.GetCount() && changed(i)) ++i;
      data.m_Ranges.PushBack({first,i-first});
    }
    data.m_UpdateProfiles=resetProfiles || !slot->m_Initialized || !slot->m_Uploaded || slot->m_Uploaded->m_Profiles!=data.m_Snapshot->m_Profiles;
    data.m_UpdateCounts=resetCounts || !slot->m_Initialized || !slot->m_Uploaded || slot->m_Uploaded->m_Counts!=data.m_Snapshot->m_Counts;
    if(!data.m_Ranges.IsEmpty()) data.m_Handles.m_Lights=builder.WriteBuffer(data.m_Handles.m_Lights,xiiGALResourceStateFlags::CopyDestination);
    if(data.m_UpdateProfiles) data.m_Handles.m_Profiles=builder.WriteBuffer(data.m_Handles.m_Profiles,xiiGALResourceStateFlags::CopyDestination);
    if(data.m_UpdateCounts) data.m_Handles.m_Counts=builder.WriteBuffer(data.m_Handles.m_Counts,xiiGALResourceStateFlags::CopyDestination);
    builder.SetPassSideEffects(!data.m_Ranges.IsEmpty() || data.m_UpdateProfiles || data.m_UpdateCounts); }, [state](const Data& data, xiiRenderGraphPassContext& execute) {
    auto& commands=execute.GetCommandList();
    for(const auto& range:data.m_Ranges)
    {
      commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Lights),range.m_First*sizeof(xiiGpuLightData),xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Records.GetData()+range.m_First),range.m_Count*sizeof(xiiGpuLightData)));
      state->m_Stats.m_uiBytesUploaded+=range.m_Count*sizeof(xiiGpuLightData); ++state->m_Stats.m_uiRanges;
    }
    if(data.m_UpdateProfiles) { commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Profiles),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(data.m_Snapshot->m_Profiles.GetData()),data.m_Snapshot->m_Profiles.GetCount()*sizeof(float))); state->m_Stats.m_uiBytesUploaded+=data.m_Snapshot->m_Profiles.GetCount()*sizeof(float); ++state->m_Stats.m_uiRanges; }
    if(data.m_UpdateCounts) { commands.UpdateBuffer(execute.GetBuffer(data.m_Handles.m_Counts),0,xiiMakeArrayPtr(reinterpret_cast<const xiiUInt8*>(&data.m_Snapshot->m_Counts),sizeof(data.m_Snapshot->m_Counts))); state->m_Stats.m_uiBytesUploaded+=sizeof(data.m_Snapshot->m_Counts); ++state->m_Stats.m_uiRanges; }
    if(!data.m_Ranges.IsEmpty() || data.m_UpdateProfiles || data.m_UpdateCounts) ++state->m_Stats.m_uiUploadPasses;
    data.m_Slot->m_Uploaded=data.m_Snapshot; data.m_Slot->m_Initialized=true; });
  state->m_UploadSetup = graph.GetSetupSerial();
  state->m_Upload      = pass.first->m_Handles;
  return state->m_Upload;
}
XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lighting_Implementation_WorldLightData);
