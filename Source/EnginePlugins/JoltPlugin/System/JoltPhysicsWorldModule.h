/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/UniquePtr.h>

namespace JPH
{
  class BodyID;
  class BodyInterface;
  class Shape;
} // namespace JPH

/// Queries require world read access. Simulation and configuration require write access.
class XII_JOLTPLUGIN_DLL xiiJoltPhysicsWorldModule final : public xiiPhysicsWorldModuleInterface
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltPhysicsWorldModule, xiiPhysicsWorldModuleInterface);

public:
  explicit xiiJoltPhysicsWorldModule(xiiWorld* pWorld);
  ~xiiJoltPhysicsWorldModule() override;

  /// Sets the fixed step and per-update catch-up limit. A new configuration resets accumulated time.
  [[nodiscard]] xiiResult SetSimulationStep(xiiTime fixedStep, xiiUInt32 uiMaxCatchUpSteps);

  /// Sets a symmetric relationship between two collision layers. The native world must be empty.
  [[nodiscard]] xiiResult SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled);

  xiiUInt32 GetCollisionLayerByName(xiiStringView sName) const override;
  xiiUInt8  GetWeightCategoryByName(xiiStringView sName) const override;
  xiiUInt8  GetImpulseTypeByName(xiiStringView sName) const override;
  bool      Raycast(xiiPhysicsCastResult& out_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;
  bool      RaycastAll(xiiPhysicsCastResultArray& out_results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const override;
  bool      SweepTestSphere(xiiPhysicsCastResult& out_result, float fSphereRadius, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;
  bool      SweepTestBox(xiiPhysicsCastResult& out_result, const xiiVec3& vBoxExtents, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;
  bool      SweepTestCapsule(xiiPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;
  bool      SweepTestCylinder(xiiPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection = xiiPhysicsHitCollection::Closest) const override;
  bool      OverlapTestSphere(float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const override;
  bool      OverlapTestBox(const xiiVec3& vBoxExtents, const xiiVec3& vPosition, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  bool      OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  bool      OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  void      QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_results, float fSphereRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const override;
  void      QueryShapesInBox(xiiPhysicsOverlapResultArray& out_results, const xiiVec3& vBoxExtents, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  void      QueryShapesInCapsule(xiiPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  void      QueryShapesInCylinder(xiiPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const override;
  xiiVec3   GetGravity() const override;

  void                 AddStaticCollisionBox(xiiGameObject* pOwner, xiiVec3 vBoxSize) override;
  xiiBoundingBoxSphere GetWorldSpaceBounds(xiiGameObject* pOwner, xiiUInt32 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const override;

private:
  friend class xiiJoltActorComponent;
  friend class xiiJoltDynamicActorComponent;

  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);
  void QueueBodyUpdate(xiiComponentHandle hActor);
  void SetBodyContactReporting(xiiComponentHandle hActor, bool bEnabled);
  void RemoveBody(xiiComponentHandle hActor);
  void RebuildBody(class xiiJoltActorComponent& actor);
  void SynchronizeDynamicBodies();
  void DispatchContacts(const UpdateContext& context);
  void QueueImpulse(xiiComponentHandle hActor, const xiiMsgPhysicsAddImpulse& msg) const;
  void ApplyQueuedImpulses();

  bool SweepShape(xiiPhysicsCastResult& out_result, const JPH::Shape& shape, const xiiTransform& pose, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const;
  bool OverlapShape(xiiPhysicsOverlapResultArray* pResults, const JPH::Shape& shape, const xiiTransform& pose, const xiiPhysicsQueryParameters& params) const;

  JPH::BodyInterface* FindDynamicBody(xiiComponentHandle hActor, JPH::BodyID& out_bodyId) const;

  struct NativeWorld;
  xiiUniquePtr<NativeWorld> m_pNativeWorld;
  xiiTime                   m_FixedStep = xiiTime::MakeFromSeconds(1.0 / 60.0);
  xiiTime                   m_AccumulatedTime;
  xiiUInt32                 m_uiMaxCatchUpSteps = 4;
};
