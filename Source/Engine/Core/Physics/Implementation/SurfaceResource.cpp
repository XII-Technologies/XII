/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSurfaceResource, 1, xiiRTTIDefaultAllocator<xiiSurfaceResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiSurfaceResource);

xiiEvent<const xiiSurfaceResourceEvent&, xiiMutex> xiiSurfaceResource::s_Events;

xiiSurfaceResource::xiiSurfaceResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiSurfaceResource::~xiiSurfaceResource()
{
  xiiSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type     = xiiSurfaceResourceEvent::Type::Destroyed;
  s_Events.Broadcast(e);

  XII_ASSERT_DEV(m_pPhysicsMaterialPhysX == nullptr, "Physics material has not been cleaned up properly");
  XII_ASSERT_DEV(m_pPhysicsMaterialJolt == nullptr, "Physics material has not been cleaned up properly");
}

xiiResourceLoadDescription xiiSurfaceResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiSurfaceResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiSurfaceResource::UpdateContent", GetResourceIdOrDescription());

  m_Interactions.Clear();

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    xiiSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      xiiTempHashedString s(i.m_sInteractionType.GetData());
      auto&               item     = m_Interactions.ExpandAndGetRef();
      item.m_uiInteractionTypeHash = s.GetHash();
      item.m_pInteraction          = &i;
    }

    m_Interactions.Sort([](const SurfInt& lhs, const SurfInt& rhs) -> bool {
      if (lhs.m_uiInteractionTypeHash != rhs.m_uiInteractionTypeHash)
        return lhs.m_uiInteractionTypeHash < rhs.m_uiInteractionTypeHash;

      return lhs.m_pInteraction->m_fImpulseThreshold > rhs.m_pInteraction->m_fImpulseThreshold;
    });
  }

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiSurfaceResource, xiiSurfaceResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  xiiSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type     = xiiSurfaceResourceEvent::Type::Created;
  s_Events.Broadcast(e);

  return res;
}

const xiiSurfaceInteraction* xiiSurfaceResource::FindInteraction(const xiiSurfaceResource* pCurSurf, xiiUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue)
{
  while (true)
  {
    bool bFoundAny = false;

    // try to find a matching interaction
    for (const auto& interaction : pCurSurf->m_Interactions)
    {
      if (interaction.m_uiInteractionTypeHash > uiHash)
        break;

      if (interaction.m_uiInteractionTypeHash == uiHash)
      {
        bFoundAny = true;

        // only use it if the threshold is large enough
        if (fImpulseSqr >= xiiMath::Square(interaction.m_pInteraction->m_fImpulseThreshold))
        {
          const float fImpulse   = xiiMath::Sqrt(fImpulseSqr);
          out_fImpulseParamValue = (fImpulse - interaction.m_pInteraction->m_fImpulseThreshold) * interaction.m_pInteraction->m_fImpulseScale;

          return interaction.m_pInteraction;
        }
      }
    }

    // if we did find something, we just never exceeded the threshold, then do not search in the base surface
    if (bFoundAny)
      break;

    if (pCurSurf->m_Descriptor.m_hBaseSurface.IsValid())
    {
      xiiResourceLock<xiiSurfaceResource> pBase(pCurSurf->m_Descriptor.m_hBaseSurface, xiiResourceAcquireMode::BlockTillLoaded);
      pCurSurf = pBase.GetPointer();
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

bool xiiSurfaceResource::InteractWithSurface(xiiWorld* pWorld, xiiGameObjectHandle hObject, const xiiVec3& vPosition, const xiiVec3& vSurfaceNormal, const xiiVec3& vIncomingDirection, const xiiTempHashedString& sInteraction, const xiiUInt16* pOverrideTeamID, float fImpulseSqr /*= 0.0f*/) const
{
  float                        fImpulseParam = 0;
  const xiiSurfaceInteraction* pIA           = FindInteraction(this, sInteraction.GetHash(), fImpulseSqr, fImpulseParam);

  if (pIA == nullptr)
    return false;

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  xiiResourceLock<xiiPrefabResource> pPrefab(pIA->m_hPrefab, xiiResourceAcquireMode::BlockTillLoaded);

  xiiVec3 vDir;

  switch (pIA->m_Alignment)
  {
    case xiiSurfaceInteractionAlignment::SurfaceNormal:
      vDir = vSurfaceNormal;
      break;

    case xiiSurfaceInteractionAlignment::IncidentDirection:
      vDir = -vIncomingDirection;
      break;

    case xiiSurfaceInteractionAlignment::ReflectedDirection:
      vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;

    case xiiSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vDir = -vSurfaceNormal;
      break;

    case xiiSurfaceInteractionAlignment::ReverseIncidentDirection:
      vDir = vIncomingDirection;
      break;

    case xiiSurfaceInteractionAlignment::ReverseReflectedDirection:
      vDir = -vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;
  }

  vDir.Normalize();
  xiiVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();

  // random rotation around the spawn direction
  {
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleMinMax(0.0, xiiMath::Pi<double>() * 2.0);

    xiiMat3 rotMat = xiiMat3::MakeAxisRotation(vDir, xiiAngle::MakeFromRadian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > xiiAngle::MakeFromRadian(0.0f))
  {
    xiiAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case xiiSurfaceInteractionAlignment::IncidentDirection:
      case xiiSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle     = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = xiiMath::Pi<float>() - xiiMath::ACos(fCosAngle).GetRadian();

        maxDeviation = xiiMath::Min(pIA->m_Deviation, xiiAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      case xiiSurfaceInteractionAlignment::ReflectedDirection:
      case xiiSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle     = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = xiiMath::Pi<float>() - xiiMath::ACos(fCosAngle).GetRadian();

        maxDeviation = xiiMath::Min(pIA->m_Deviation, xiiAngle::MakeFromRadian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const xiiAngle deviation = xiiAngle::MakeFromRadian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    xiiMat3 matTilt = xiiMat3::MakeAxisRotation(vTangent, deviation);

    vDir = matTilt * vDir;
  }

  // finally compute the bi-tangent
  const xiiVec3 vBiTangent = vDir.CrossRH(vTangent);

  xiiMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  xiiTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation = xiiQuat::MakeFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  xiiGameObjectHandle hParent;

  xiiGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t       = xiiTransform::MakeLocalTransform(pObject->GetGlobalTransform(), t);
  }

  xiiHybridArray<xiiGameObject*, 8> rootObjects;

  xiiPrefabInstantiationOptions options;
  options.m_hParent                = hParent;
  options.m_pCreatedRootObjectsOut = &rootObjects;
  options.m_pOverrideTeamID        = pOverrideTeamID;

  pPrefab->InstantiatePrefab(*pWorld, t, options, &pIA->m_Parameters);

  {
    xiiMsgSetFloatParameter msgSetFloat;
    msgSetFloat.m_sParameterName = "Impulse";
    msgSetFloat.m_fValue         = fImpulseParam;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msgSetFloat, xiiTime::MakeZero(), xiiObjectMsgQueueType::AfterInitialized);
    }
  }

  if (pObject != nullptr && pObject->IsDynamic())
  {
    xiiMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, xiiTime::MakeZero(), xiiObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}

bool xiiSurfaceResource::IsBasedOn(const xiiSurfaceResource* pThisOrBaseSurface) const
{
  if (pThisOrBaseSurface == this)
    return true;

  if (m_Descriptor.m_hBaseSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, xiiResourceAcquireMode::BlockTillLoaded);

    return pBase->IsBasedOn(pThisOrBaseSurface);
  }

  return false;
}

bool xiiSurfaceResource::IsBasedOn(const xiiSurfaceResourceHandle hThisOrBaseSurface) const
{
  xiiResourceLock<xiiSurfaceResource> pThisOrBaseSurface(hThisOrBaseSurface, xiiResourceAcquireMode::BlockTillLoaded);

  return IsBasedOn(pThisOrBaseSurface.GetPointer());
}

XII_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResource);
