#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/CVar.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>

xiiCVarInt  cvar_PhysicsReactionsMaxImpacts("Jolt.Reactions.MaxImpacts", 4, xiiCVarFlags::Default, "Maximum number of impact reactions to spawn per frame.");
xiiCVarInt  cvar_PhysicsReactionsMaxSlidesOrRolls("Jolt.Reactions.MaxSlidesOrRolls", 4, xiiCVarFlags::Default, "Maximum number of active slide or roll reactions.");
xiiCVarBool cvar_PhysicsReactionsVisImpacts("Jolt.Reactions.VisImpacts", false, xiiCVarFlags::Default, "Visualize where impact reactions are spawned.");
xiiCVarBool cvar_PhysicsReactionsVisDiscardedImpacts("Jolt.Reactions.VisDiscardedImpacts", false, xiiCVarFlags::Default, "Visualize where impact reactions were NOT spawned.");
xiiCVarBool cvar_PhysicsReactionsVisSlides("Jolt.Reactions.VisSlides", false, xiiCVarFlags::Default, "Visualize active slide reactions.");
xiiCVarBool cvar_PhysicsReactionsVisRolls("Jolt.Reactions.VisRolls", false, xiiCVarFlags::Default, "Visualize active roll reactions.");

void xiiJoltContactListener::RemoveTrigger(const xiiJoltTriggerComponent* pTrigger)
{
  XII_LOCK(m_TriggerMutex);

  for (auto it = m_Trigs.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_pTrigger == pTrigger)
    {
      it = m_Trigs.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void xiiJoltContactListener::OnContactAdded(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
  const xiiUInt64 uiBody0id = inBody0.GetID().GetIndexAndSequenceNumber();
  const xiiUInt64 uiBody1id = inBody1.GetID().GetIndexAndSequenceNumber();

  if (ActivateTrigger(inBody0, inBody1, uiBody0id, uiBody1id))
    return;

  OnContact(inBody0, inBody1, inManifold, ioSettings, false);
}

void xiiJoltContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
{
  OnContact(inBody1, inBody2, inManifold, ioSettings, true);
}

void xiiJoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
  const xiiUInt64 uiBody1id = inSubShapePair.GetBody1ID().GetIndexAndSequenceNumber();
  const xiiUInt64 uiBody2id = inSubShapePair.GetBody2ID().GetIndexAndSequenceNumber();

  DeactivateTrigger(uiBody1id, uiBody2id);
}

void xiiJoltContactListener::OnContact(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings, bool bPersistent)
{
  // compute per-material friction and restitution
  {
    const xiiJoltMaterial* pMat0 = static_cast<const xiiJoltMaterial*>(inBody0.GetShape()->GetMaterial(inManifold.mSubShapeID1));
    const xiiJoltMaterial* pMat1 = static_cast<const xiiJoltMaterial*>(inBody1.GetShape()->GetMaterial(inManifold.mSubShapeID2));

    if (pMat0 && pMat1)
    {
      ioSettings.mCombinedRestitution = xiiMath::Max(pMat0->m_fRestitution, pMat1->m_fRestitution);
      ioSettings.mCombinedFriction    = xiiMath::Sqrt(pMat0->m_fFriction * pMat1->m_fFriction);
    }
  }

  m_ContactEvents.m_pWorld = m_pWorld;

  const xiiJoltDynamicActorComponent* pActor0 = xiiJoltUserData::GetDynamicActorComponent(reinterpret_cast<const void*>(inBody0.GetUserData()));
  const xiiJoltDynamicActorComponent* pActor1 = xiiJoltUserData::GetDynamicActorComponent(reinterpret_cast<const void*>(inBody1.GetUserData()));

  if (pActor0 || pActor1)
  {
    const xiiBitflags<xiiOnJoltContact> ContactFlags0 = pActor0 ? pActor0->m_OnContact : xiiOnJoltContact::None;
    const xiiBitflags<xiiOnJoltContact> ContactFlags1 = pActor1 ? pActor1->m_OnContact : xiiOnJoltContact::None;

    xiiBitflags<xiiOnJoltContact> CombinedContactFlags;
    CombinedContactFlags.SetValue(ContactFlags0.GetValue() | ContactFlags1.GetValue());

    // bSendContactReport = bSendContactReport || CombinedContactFlags.IsSet(xiiOnJoltContact::SendReportMsg);

    if (CombinedContactFlags.IsAnySet(xiiOnJoltContact::AllReactions))
    {
      xiiVec3       vAvgPos(0);
      const xiiVec3 vAvgNormal = xiiJoltConversionUtils::ToVec3(inManifold.mWorldSpaceNormal);

      const float fImpactSqr = (inBody0.GetLinearVelocity() - inBody1.GetLinearVelocity()).LengthSq();

      for (xiiUInt32 uiContactPointIndex = 0; uiContactPointIndex < inManifold.mRelativeContactPointsOn1.size(); ++uiContactPointIndex)
      {
        vAvgPos += xiiJoltConversionUtils::ToVec3(inManifold.GetWorldSpaceContactPointOn1(uiContactPointIndex));
        vAvgPos -= vAvgNormal * inManifold.mPenetrationDepth;
      }

      vAvgPos /= (float)inManifold.mRelativeContactPointsOn1.size();

      if (bPersistent)
      {
        m_ContactEvents.OnContact_SlideAndRollReaction(inBody0, inBody1, inManifold, ContactFlags0, ContactFlags1, vAvgPos, vAvgNormal, CombinedContactFlags);
      }
      else if (fImpactSqr >= 1.0f && CombinedContactFlags.IsAnySet(xiiOnJoltContact::ImpactReactions))
      {
        const xiiJoltMaterial* pMat1 = static_cast<const xiiJoltMaterial*>(inBody0.GetShape()->GetMaterial(inManifold.mSubShapeID1));
        const xiiJoltMaterial* pMat2 = static_cast<const xiiJoltMaterial*>(inBody1.GetShape()->GetMaterial(inManifold.mSubShapeID2));

        if (pMat1 == nullptr)
          pMat1 = static_cast<const xiiJoltMaterial*>(xiiJoltMaterial::sDefault.GetPtr());
        if (pMat2 == nullptr)
          pMat2 = static_cast<const xiiJoltMaterial*>(xiiJoltMaterial::sDefault.GetPtr());

        m_ContactEvents.OnContact_ImpactReaction(vAvgPos, vAvgNormal, fImpactSqr, pMat1->m_pSurface, pMat2->m_pSurface, inBody0.IsStatic() || inBody0.IsKinematic());
      }
    }
  }

  //   if (bSendContactReport)
  //   {
  //     SendContactReport(pairHeader, pairs, nbPairs);
  //   }
}

bool xiiJoltContactListener::ActivateTrigger(const JPH::Body& inBody1, const JPH::Body& inBody2, xiiUInt64 uiBody1id, xiiUInt64 uiBody2id)
{
  if (!inBody1.IsSensor() && !inBody2.IsSensor())
    return false;

  const xiiJoltTriggerComponent* pTrigger   = nullptr;
  const xiiComponent*            pComponent = nullptr;

  if (inBody1.IsSensor())
  {
    pTrigger   = xiiJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(inBody1.GetUserData()));
    pComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(inBody2.GetUserData()));
  }
  else
  {
    pTrigger   = xiiJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(inBody2.GetUserData()));
    pComponent = xiiJoltUserData::GetComponent(reinterpret_cast<const void*>(inBody1.GetUserData()));
  }

  if (pTrigger && pComponent)
  {
    pTrigger->PostTriggerMessage(pComponent->GetOwner()->GetHandle(), xiiTriggerState::Activated);

    XII_LOCK(m_TriggerMutex);

    const xiiUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);
    auto&           trig      = m_Trigs[uiStoreID];
    trig.m_pTrigger           = pTrigger;
    trig.m_hTarget            = pComponent->GetOwner()->GetHandle();
  }

  // one of the bodies is a trigger
  return true;
}

void xiiJoltContactListener::DeactivateTrigger(xiiUInt64 uiBody1id, xiiUInt64 uiBody2id)
{
  XII_LOCK(m_TriggerMutex);

  const xiiUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);
  auto            itTrig    = m_Trigs.Find(uiStoreID);

  if (itTrig.IsValid())
  {
    itTrig.Value().m_pTrigger->PostTriggerMessage(itTrig.Value().m_hTarget, xiiTriggerState::Deactivated);
    m_Trigs.Remove(itTrig);
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiJoltContactEvents::SpawnPhysicsImpactReactions()
{
  XII_PROFILE_SCOPE("SpawnPhysicsImpactReactions");

  XII_LOCK(m_Mutex);

  xiiUInt32 uiMaxPrefabsToSpawn = cvar_PhysicsReactionsMaxImpacts;

  for (const auto& ic : m_InteractionContacts)
  {
    if (ic.m_pSurface != nullptr)
    {
      if (uiMaxPrefabsToSpawn > 0 && ic.m_pSurface->InteractWithSurface(m_pWorld, xiiGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr))
      {
        --uiMaxPrefabsToSpawn;

        if (cvar_PhysicsReactionsVisImpacts)
        {
          xiiDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, xiiColor::LightGreen, xiiTransform(ic.m_vPosition), xiiTime::Seconds(3));
        }
      }
      else
      {
        if (cvar_PhysicsReactionsVisDiscardedImpacts)
        {
          xiiDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, xiiColor::DarkGray, xiiTransform(ic.m_vPosition), xiiTime::Seconds(1));
        }
      }
    }
  }

  m_InteractionContacts.Clear();
}

void xiiJoltContactEvents::UpdatePhysicsSlideReactions()
{
  XII_PROFILE_SCOPE("UpdatePhysicsSlideReactions");

  XII_LOCK(m_Mutex);

  for (auto& slideInfo : m_SlidingOrRollingActors)
  {
    if (slideInfo.m_pBody == nullptr)
      continue;

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        xiiPrefabResourceHandle            hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(slideInfo.m_sSlideInteractionPrefab);
        xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == xiiResourceAcquireResult::Final)
        {
          xiiHybridArray<xiiGameObject*, 8> created;

          xiiPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic          = true;

          pPrefab->InstantiatePrefab(*m_pWorld, xiiTransform(slideInfo.m_vContactPosition), options);
          slideInfo.m_hSlidePrefab = created[0]->GetHandle();
        }
      }
      else
      {
        xiiGameObject* pObject;
        if (m_pWorld->TryGetObject(slideInfo.m_hSlidePrefab, pObject))
        {
          pObject->SetGlobalPosition(slideInfo.m_vContactPosition);
        }
        else
        {
          slideInfo.m_hSlidePrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisSlides)
      {
        xiiDebugRenderer::DrawLineBox(m_pWorld, xiiBoundingBox(xiiVec3(-0.5f), xiiVec3(0.5f)), xiiColor::BlueViolet, xiiTransform(slideInfo.m_vContactPosition));
      }

      slideInfo.m_bStillSliding = false;
    }
    else
    {
      if (!slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(slideInfo.m_hSlidePrefab);
        slideInfo.m_hSlidePrefab.Invalidate();
      }
    }
  }
}

void xiiJoltContactEvents::UpdatePhysicsRollReactions()
{
  XII_PROFILE_SCOPE("UpdatePhysicsRollReactions");

  XII_LOCK(m_Mutex);

  for (auto& rollInfo : m_SlidingOrRollingActors)
  {
    if (rollInfo.m_pBody == nullptr)
      continue;

    if (rollInfo.m_bStillRolling)
    {
      if (rollInfo.m_hRollPrefab.IsInvalidated())
      {
        xiiPrefabResourceHandle            hPrefab = xiiResourceManager::LoadResource<xiiPrefabResource>(rollInfo.m_sRollInteractionPrefab);
        xiiResourceLock<xiiPrefabResource> pPrefab(hPrefab, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == xiiResourceAcquireResult::Final)
        {
          xiiHybridArray<xiiGameObject*, 8> created;

          xiiPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic          = true;

          pPrefab->InstantiatePrefab(*m_pWorld, xiiTransform(rollInfo.m_vContactPosition), options);
          rollInfo.m_hRollPrefab = created[0]->GetHandle();
        }
      }
      else
      {
        xiiGameObject* pObject;
        if (m_pWorld->TryGetObject(rollInfo.m_hRollPrefab, pObject))
        {
          pObject->SetGlobalPosition(rollInfo.m_vContactPosition);
        }
        else
        {
          rollInfo.m_hRollPrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisRolls)
      {
        xiiDebugRenderer::DrawLineCapsuleZ(m_pWorld, 0.4f, 0.2f, xiiColor::GreenYellow, xiiTransform(rollInfo.m_vContactPosition));
      }

      rollInfo.m_bStillRolling = false;
      rollInfo.m_bStillSliding = false; // ensures that no slide reaction is spawned as well
    }
    else
    {
      if (!rollInfo.m_hRollPrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(rollInfo.m_hRollPrefab);
        rollInfo.m_hRollPrefab.Invalidate();
      }
    }
  }
}

void xiiJoltContactEvents::OnContact_ImpactReaction(const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal, float fMaxImpactSqr, const xiiSurfaceResource* pSurface1, const xiiSurfaceResource* pSurface2, bool bActor1StaticOrKinematic)
{
  const float fDistanceSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  XII_LOCK(m_Mutex);

  InteractionContact* ic = nullptr;

  if (m_InteractionContacts.GetCount() < (xiiUInt32)cvar_PhysicsReactionsMaxImpacts * 2)
  {
    ic                 = &m_InteractionContacts.ExpandAndGetRef();
    ic->m_pSurface     = nullptr;
    ic->m_fDistanceSqr = xiiMath::HighValue<float>();
  }
  else
  {
    // compute a score, which contact point is best to replace
    // * prefer to replace points that are farther away than the new one
    // * prefer to replace points that have a lower impact strength than the new one

    float     fBestScore  = 0;
    xiiUInt32 uiBestScore = 0xFFFFFFFFu;

    for (xiiUInt32 i = 0; i < m_InteractionContacts.GetCount(); ++i)
    {
      float fScore = 0;
      fScore += m_InteractionContacts[i].m_fDistanceSqr - fDistanceSqr;
      fScore += 2.0f * (fMaxImpactSqr - m_InteractionContacts[i].m_fImpulseSqr);

      if (fScore > fBestScore)
      {
        fBestScore  = fScore;
        uiBestScore = i;
      }
    }

    if (uiBestScore == 0xFFFFFFFFu)
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        xiiDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, xiiColor::DimGrey, xiiTransform(vAvgPos), xiiTime::Seconds(3));
      }

      return;
    }
    else
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        xiiDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, xiiColor::DimGrey, xiiTransform(m_InteractionContacts[uiBestScore].m_vPosition), xiiTime::Seconds(3));
      }
    }

    // this is the best candidate to replace
    ic = &m_InteractionContacts[uiBestScore];
  }

  if (pSurface1 || pSurface2)
  {
    // if one of the objects doesn't have a surface configured, use the other one
    if (pSurface1 == nullptr)
      pSurface1 = pSurface2;
    if (pSurface2 == nullptr)
      pSurface2 = pSurface1;

    ic->m_fDistanceSqr = fDistanceSqr;
    ic->m_vPosition    = vAvgPos;
    ic->m_vNormal      = vAvgNormal;
    ic->m_vNormal.NormalizeIfNotZero(xiiVec3(0, 0, 1)).IgnoreResult();
    ic->m_fImpulseSqr = fMaxImpactSqr;

    // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
    if (bActor1StaticOrKinematic)
    {
      ic->m_pSurface     = pSurface1;
      ic->m_sInteraction = pSurface2->GetDescriptor().m_sOnCollideInteraction;
    }
    else
    {
      ic->m_pSurface     = pSurface2;
      ic->m_sInteraction = pSurface1->GetDescriptor().m_sOnCollideInteraction;
    }

    return;
  }

  if (cvar_PhysicsReactionsVisDiscardedImpacts)
  {
    xiiDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, xiiColor::DarkOrange, xiiTransform(vAvgPos), xiiTime::Seconds(10));
  }
}

xiiJoltContactEvents::SlideAndRollInfo* xiiJoltContactEvents::FindSlideOrRollInfo(const JPH::Body* pBody, const xiiVec3& vAvgPos)
{
  SlideAndRollInfo* pUnused = nullptr;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (info.m_pBody == pBody)
      return &info;

    if (info.m_pBody == nullptr)
      pUnused = &info;
  }

  const float fDistSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody        = pBody;
    return pUnused;
  }

  if (m_SlidingOrRollingActors.GetCount() < (xiiUInt32)cvar_PhysicsReactionsMaxSlidesOrRolls)
  {
    pUnused                 = &m_SlidingOrRollingActors.ExpandAndGetRef();
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody        = pBody;
    return pUnused;
  }

  float fBestDist = 0.0f;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (!info.m_hRollPrefab.IsInvalidated() || !info.m_hSlidePrefab.IsInvalidated())
      continue;

    // this slot is not yet really in use, so can be replaced by a better match

    if (fDistSqr < info.m_fDistanceSqr && info.m_fDistanceSqr > fBestDist)
    {
      fBestDist = info.m_fDistanceSqr;
      pUnused   = &info;
    }
  }

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody        = pBody;
    return pUnused;
  }

  return nullptr;
}

void xiiJoltContactEvents::OnContact_RollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal0)
{
  // only consider something 'rolling' when it turns faster than this (per second)
  constexpr xiiAngle rollThreshold = xiiAngle::Degree(45);

  xiiBitflags<xiiOnJoltContact> contactFlags[2] = {onContact0, onContact1};
  const JPH::Body*              bodies[2]       = {&inBody0, &inBody1};
  const JPH::SubShapeID         shapeIds[2]     = {inManifold.mSubShapeID1, inManifold.mSubShapeID2};

  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    if (contactFlags[i].IsAnySet(xiiOnJoltContact::AllRollReactions))
    {
      const xiiVec3 vAngularVel = xiiJoltConversionUtils::ToVec3(bodies[i]->GetRotation().InverseRotate(bodies[i]->GetAngularVelocity()));

      if ((contactFlags[i].IsSet(xiiOnJoltContact::RollXReactions) && xiiMath::Abs(vAngularVel.x) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(xiiOnJoltContact::RollYReactions) && xiiMath::Abs(vAngularVel.y) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(xiiOnJoltContact::RollZReactions) && xiiMath::Abs(vAngularVel.z) > rollThreshold.GetRadian()))
      {
        const xiiJoltMaterial* pMaterial = static_cast<const xiiJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

        if (pMaterial && pMaterial->m_pSurface)
        {
          if (!pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab.IsEmpty())
          {
            XII_LOCK(m_Mutex);

            if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
            {
              pInfo->m_bStillRolling          = true;
              pInfo->m_vContactPosition       = vAvgPos;
              pInfo->m_sRollInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab;
            }
          }
        }
      }
    }
  }
}

void xiiJoltContactEvents::OnContact_SlideReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal0)
{
  xiiVec3 vVelocity[2] = {xiiVec3::ZeroVector(), xiiVec3::ZeroVector()};

  {
    vVelocity[0] = xiiJoltConversionUtils::ToVec3(inBody0.GetLinearVelocity());

    if (!vVelocity[0].IsValid())
      vVelocity[0].SetZero();
  }

  {
    vVelocity[1] = xiiJoltConversionUtils::ToVec3(inBody1.GetLinearVelocity());

    if (!vVelocity[1].IsValid())
      vVelocity[1].SetZero();
  }

  const xiiVec3 vRelativeVelocity = vVelocity[1] - vVelocity[0];

  if (!vRelativeVelocity.IsZero(0.0001f))
  {
    const xiiVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

    xiiVec3 vAvgNormal = vAvgNormal0;
    vAvgNormal.NormalizeIfNotZero(xiiVec3::UnitZAxis()).IgnoreResult();

    // an object is only 'sliding' if it moves at roughly 90 degree along another object
    constexpr float slideAngle = 0.17f; // xiiMath ::Cos(xiiAngle::Degree(80));

    if (xiiMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < slideAngle)
    {
      constexpr float slideSpeedThreshold = 0.5f; // in meters per second

      if (vRelativeVelocity.GetLengthSquared() > xiiMath::Square(slideSpeedThreshold))
      {
        xiiBitflags<xiiOnJoltContact> contactFlags[2] = {onContact0, onContact1};
        const JPH::Body*              bodies[2]       = {&inBody0, &inBody1};
        const JPH::SubShapeID         shapeIds[2]     = {inManifold.mSubShapeID1, inManifold.mSubShapeID2};

        for (xiiUInt32 i = 0; i < 2; ++i)
        {
          if (contactFlags[i].IsAnySet(xiiOnJoltContact::SlideReactions))
          {
            const xiiJoltMaterial* pMaterial = static_cast<const xiiJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

            if (pMaterial && pMaterial->m_pSurface)
            {
              if (!pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab.IsEmpty())
              {
                XII_LOCK(m_Mutex);

                if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
                {
                  if (!pInfo->m_bStillRolling)
                  {
                    pInfo->m_bStillSliding           = true;
                    pInfo->m_vContactPosition        = vAvgPos;
                    pInfo->m_sSlideInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void xiiJoltContactEvents::OnContact_SlideAndRollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal, xiiBitflags<xiiOnJoltContact> CombinedContactFlags)
{
  if (inManifold.mRelativeContactPointsOn1.size() >= 2 && CombinedContactFlags.IsAnySet(xiiOnJoltContact::SlideReactions))
  {
    OnContact_SlideReaction(inBody0, inBody1, inManifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }

  if (CombinedContactFlags.IsAnySet(xiiOnJoltContact::AllRollReactions))
  {
    OnContact_RollReaction(inBody0, inBody1, inManifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltContacts);
