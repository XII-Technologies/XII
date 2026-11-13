/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Actors/JoltKinematicActorComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/System/JoltDebugDraw.h>

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
  /// Reloads project layer settings transactionally for empty worlds and stopped worlds with generated static collision only.
  [[nodiscard]] xiiStatus ReloadCollisionLayerConfig();

  /// Sets debug modes for this world. Call with world write access.
  void                          SetDebugModes(xiiBitflags<xiiJoltDebugMode> modes);
  xiiBitflags<xiiJoltDebugMode> GetDebugModes() const;
  /// Native body statistics walk the body registry. Call at a synchronous world update boundary.
  xiiJoltSimulationDiagnostics GetSimulationDiagnostics() const;
  static bool                  IsDebugDrawingSupported();
  /// True once native bodies or virtual characters exist, including while their simulation is paused.
  bool HasSimulationState() const;
  /// Reuses the runtime shape constructors, scaling validation, and native collision resource snapshots.
  /// Cached local geometry is rebuilt only when its dimensions or resource snapshot change.
  [[nodiscard]] static xiiResult UpdateShapePreview(const class xiiJoltShapeComponent& shape, xiiJoltShapePreview& inout_preview);
  /// Collects once per world update and mode selection. Contact samples begin with the next step.
  /// Borrowed data is invalidated by the next collection or structural change.
  /// Call between simulation updates with world write access. Returns null when native drawing is unavailable.
  const xiiJoltDebugDrawData* GetDebugDrawData(xiiBitflags<xiiJoltDebugMode> modes);

  /// Releases the process-wide Jolt adapter after all native worlds have been removed.
  static void ShutdownDebugRenderer();

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
  void                 AddFixedJointComponent(xiiGameObject* pOwner, const FixedJointConfig& cfg) override;
  xiiBoundingBoxSphere GetWorldSpaceBounds(xiiGameObject* pOwner, xiiUInt32 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const override;

private:
  friend class xiiJoltActorComponent;
  friend class xiiJoltDynamicActorComponent;
  friend class xiiJoltKinematicActorComponent;
  friend class xiiJoltCharacterControllerComponent;
  friend class xiiJoltJointComponent;
  friend class xiiJoltShapeComponent;
  friend class xiiJoltStaticCollisionComponent;

  void Initialize() override;
  void Deinitialize() override;
  void OnSimulationStarted() override;
  void WorldClear() override;

  void Simulate(const UpdateContext& context);
  void QueueJointUpdate(xiiComponentHandle hJoint, bool bRecreate);
  void RemoveJoint(xiiComponentHandle hJoint);
  void UpdateJoints();
  void CheckJointBreakage(float fStepSeconds);
  void UpdateMaterials();
  void RegisterShape(xiiComponentHandle hShape);
  void UnregisterShape(xiiComponentHandle hShape);
  void UpdateShapes();
  void QueueStaticCollision(xiiComponentHandle hGroup);
  void RemoveStaticCollision(xiiComponentHandle hGroup);
  void InvalidateStaticCollisionDescendants(const xiiGameObject& object);
  void UpdateStaticCollision(const UpdateContext& context);
  void QueueBodyUpdate(xiiComponentHandle hActor);
  void SetBodyContactReporting(xiiComponentHandle hActor, bool bEnabled);
  void SetBodySurface(xiiComponentHandle hActor, const xiiSurfaceResourceHandle& hSurface);
  void RemoveBody(xiiComponentHandle hActor);
  void RebuildBody(class xiiJoltActorComponent& actor);
  void SynchronizeMovedObjects();
  void ApplyKinematicTeleports();
  void CaptureKinematicTargets();
  void UpdateKinematicBodies(float fTimeToTarget);
  bool GetKinematicState(const xiiJoltKinematicActorComponent& actor, xiiJoltKinematicActorComponent::BodyState& out_state) const;
  void QueueCharacterUpdate(xiiComponentHandle hCharacter);
  void RemoveCharacter(xiiComponentHandle hCharacter);
  void UpdateCharacterConfigurations();
  void StepCharacters(float fStep);
  bool GetCharacterState(xiiComponentHandle hCharacter, xiiJoltCharacterControllerComponent::State& out_state) const;
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
