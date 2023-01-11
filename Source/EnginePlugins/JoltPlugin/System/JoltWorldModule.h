#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

class xiiJoltCharacterControllerComponent;
class xiiJoltContactListener;

namespace JPH
{
  class Body;
  class TempAllocator;
  class PhysicsSystem;
  class GroupFilter;
} // namespace JPH

class XII_JOLTPLUGIN_DLL xiiJoltWorldModule : public xiiPhysicsWorldModuleInterface
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltWorldModule, xiiPhysicsWorldModuleInterface);

public:
  xiiJoltWorldModule(xiiWorld* pWorld);
  ~xiiJoltWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  JPH::PhysicsSystem*       GetJoltSystem() { return m_pSystem.get(); }
  const JPH::PhysicsSystem* GetJoltSystem() const { return m_pSystem.get(); }

  xiiUInt32 CreateObjectFilterID();
  void      DeleteObjectFilterID(xiiUInt32& uiObjectFilterID);

  xiiUInt32              AllocateUserData(xiiJoltUserData*& out_pUserData);
  void                   DeallocateUserData(xiiUInt32& uiUserDataId);
  const xiiJoltUserData& GetUserData(xiiUInt32 uiUserDataId) const;

  void            SetGravity(const xiiVec3& objectGravity, const xiiVec3& characterGravity);
  virtual xiiVec3 GetGravity() const override { return xiiVec3(0, 0, -10); }
  xiiVec3         GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }

  //////////////////////////////////////////////////////////////////////////
  // xiiPhysicsWorldModuleInterface

  virtual bool Raycast(xiiPhysicsCastResult& out_Result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(xiiPhysicsCastResultArray& out_Results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(xiiPhysicsCastResult& out_Result, float fSphereRadius, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(xiiPhysicsCastResult& out_Result, xiiVec3 vBoxExtends, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(xiiPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_Results, float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(xiiGameObject* pObject, xiiVec3 boxSize) override;

  xiiDeque<xiiComponentHandle> m_RequireUpdate;

  const xiiMap<xiiJoltActorComponent*, xiiUInt32>& GetActiveActors() const { return m_ActiveActors; }

  void QueueBodyToAdd(JPH::Body* pBody, bool bAwake);

  JPH::GroupFilter* GetGroupFilter() const { return m_pGroupFilter; }
  JPH::GroupFilter* GetGroupFilterIgnoreSame() const { return m_pGroupFilterIgnoreSame; }

  void EnableJoinedBodiesCollisions(xiiUInt32 uiObjectFilterID1, xiiUInt32 uiObjectFilterID2, bool bEnable);

  JPH::TempAllocator* GetTempAllocator() const { return m_pTempAllocator.get(); }

  void ActivateCharacterController(xiiJoltCharacterControllerComponent* pCharacter, bool bActivate);

  xiiJoltContactListener* GetContactListener()
  {
    return reinterpret_cast<xiiJoltContactListener*>(m_pContactListener);
  }

private:
  bool SweepTest(xiiPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const;
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const xiiPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const xiiWorldModule::UpdateContext& context);
  void FetchResults(const xiiWorldModule::UpdateContext& context);

  void Simulate();

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void UpdateConstraints();

  xiiTime CalculateUpdateSteps();

  xiiUInt32                  m_uiNextObjectFilterID = 1;
  xiiDynamicArray<xiiUInt32> m_FreeObjectFilterIDs;

  xiiDeque<xiiJoltUserData>  m_AllocatedUserData;
  xiiDynamicArray<xiiUInt32> m_FreeUserData;
  xiiDynamicArray<xiiUInt32> m_FreeUserDataAfterSimulationStep;

  xiiTime m_AccumulatedTimeSinceUpdate;

  xiiJoltSettings m_Settings;

  xiiSharedPtr<xiiTask> m_pSimulateTask;
  xiiTaskGroupID        m_SimulateTaskGroupId;
  xiiTime               m_SimulatedTimeStep;

  std::unique_ptr<JPH::PhysicsSystem> m_pSystem;
  std::unique_ptr<JPH::TempAllocator> m_pTempAllocator;

  xiiJoltObjectToBroadphaseLayer m_ObjectToBroadphase;

  void*                                     m_pContactListener    = nullptr;
  void*                                     m_pActivationListener = nullptr;
  xiiMap<xiiJoltActorComponent*, xiiUInt32> m_ActiveActors;

  JPH::GroupFilter* m_pGroupFilter           = nullptr;
  JPH::GroupFilter* m_pGroupFilterIgnoreSame = nullptr;

  xiiUInt32           m_uiBodiesAddedSinceOptimize = 100;
  xiiDeque<xiiUInt32> m_BodiesToAdd;
  xiiDeque<xiiUInt32> m_BodiesToAddAndActivate;

  xiiHybridArray<xiiTime, 4>                              m_UpdateSteps;
  xiiHybridArray<xiiJoltCharacterControllerComponent*, 4> m_ActiveCharacters;
};
