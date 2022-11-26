#pragma once

#include <Core/World/Declarations.h>
#include <Physics/Collision/ContactListener.h>

class xiiWorld;
class xiiJoltTriggerComponent;
class xiiJoltContactEvents;

namespace JPH
{
  class SubShapeIDPair;
  class ContactSettings;
  class ContactManifold;
  class Body;
} // namespace JPH

class xiiJoltContactEvents
{
public:
  struct InteractionContact
  {
    xiiVec3                   m_vPosition;
    xiiVec3                   m_vNormal;
    const xiiSurfaceResource* m_pSurface;
    xiiTempHashedString       m_sInteraction;
    float                     m_fImpulseSqr;
    float                     m_fDistanceSqr;
  };

  struct SlideAndRollInfo
  {
    const JPH::Body* m_pBody         = nullptr;
    bool             m_bStillSliding = false;
    bool             m_bStillRolling = false;

    float               m_fDistanceSqr;
    xiiVec3             m_vContactPosition;
    xiiGameObjectHandle m_hSlidePrefab;
    xiiGameObjectHandle m_hRollPrefab;
    xiiHashedString     m_sSlideInteractionPrefab;
    xiiHashedString     m_sRollInteractionPrefab;
  };

  xiiMutex                              m_Mutex;
  xiiWorld*                             m_pWorld              = nullptr;
  xiiVec3                               m_vMainCameraPosition = xiiVec3::ZeroVector();
  xiiHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  xiiHybridArray<SlideAndRollInfo, 4>   m_SlidingOrRollingActors;

  SlideAndRollInfo* FindSlideOrRollInfo(const JPH::Body* pBody, const xiiVec3& vAvgPos);

  void OnContact_SlideReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal);

  void OnContact_RollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal0);

  void OnContact_ImpactReaction(const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal, float fMaxImpactSqr, const xiiSurfaceResource* pSurface1, const xiiSurfaceResource* pSurface2, bool bActor1StaticOrKinematic);
  void OnContact_SlideAndRollReaction(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, xiiBitflags<xiiOnJoltContact> onContact0, xiiBitflags<xiiOnJoltContact> onContact1, const xiiVec3& vAvgPos, const xiiVec3& vAvgNormal, xiiBitflags<xiiOnJoltContact> CombinedContactFlags);

  void SpawnPhysicsImpactReactions();
  void UpdatePhysicsSlideReactions();
  void UpdatePhysicsRollReactions();
};

class xiiJoltContactListener : public JPH::ContactListener
{
public:
  xiiWorld*            m_pWorld = nullptr;
  xiiJoltContactEvents m_ContactEvents;

  struct TriggerObj
  {
    const xiiJoltTriggerComponent* m_pTrigger = nullptr;
    xiiGameObjectHandle            m_hTarget;
  };

  xiiMutex                      m_TriggerMutex;
  xiiMap<xiiUInt64, TriggerObj> m_Trigs;

  virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
  virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

  virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

  void OnContact(const JPH::Body& inBody0, const JPH::Body& inBody1, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings, bool bPersistent);

  bool ActivateTrigger(const JPH::Body& inBody1, const JPH::Body& inBody2, xiiUInt64 uiBody1id, xiiUInt64 uiBody2id);

  void DeactivateTrigger(xiiUInt64 uiBody1id, xiiUInt64 uiBody2id);
};
