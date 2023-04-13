#pragma once

#include <Foundation/Communication/Message.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>

namespace JPH
{
  class SixDOFConstraint;
}

using xiiJoltGrabObjectComponentManager = xiiComponentManagerSimple<class xiiJoltGrabObjectComponent, xiiComponentUpdateType::WhenSimulating, xiiBlockStorageType::Compact>;

/// \brief Used to 'grab' physical objects and attach them to an object. For player objects to pick up objects.
///
/// The component does a raycast along its X axis to detect nearby physics objects. If it finds a non-kinematic xiiJoltDynamicActor
/// it connects a dedicated object with the picked object through a 6DOF joint, which is set up to drag the picked object towards its
/// position and rotation.
/// The grabbed object can be dropped or thrown away.
///
/// If the picked object has a xiiGrabbableItemComponent, the custom grab points are used to determine how to grab the object.
class XII_JOLTPLUGIN_DLL xiiJoltGrabObjectComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltGrabObjectComponent, xiiComponent, xiiJoltGrabObjectComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltGrabObjectComponent

public:
  xiiJoltGrabObjectComponent();
  ~xiiJoltGrabObjectComponent();

  /// \brief Checks whether there is an object nearby. Note that this function reports static and dynamic objects that are within reach.
  /// Whether these objects are interact able or not is up to the caller.
  bool FindNearbyObject(xiiGameObject*& out_pObject, xiiTransform& out_localGrabPoint) const;

  /// \brief Grabs the given object at the given grab point if possible.
  bool GrabObject(xiiGameObject* pObjectToGrab, const xiiTransform& localGrabPoint);

  /// \brief Tries to find an object to pick up and do so.
  bool GrabNearbyObject(); // [ scriptable ]

  /// \brief Returns whether an object is currently being held.
  bool HasObjectGrabbed() const; // [ scriptable ]

  /// \brief The grabbed object is dropped in place.
  void DropGrabbedObject(); // [ scriptable ]

  /// \brief Throws the held object away.
  void ThrowGrabbedObject(const xiiVec3& vRelativeDir); // [ scriptable ]

  /// \brief Similar to DropGrabbedObject() but additionally posts the event message xiiMsgPhysicsJointBroke.
  ///
  /// This can be used to inform other code that the object was ripped from the hands of the player.
  void BreakObjectGrab(); // [ scriptable ]

  /// If the held actor is pushed out of the hands farther than this, the 'joint' breaks. Set to zero to disable this feature.
  float m_fBreakDistance = 0.5f; // [ property ]

  /// The stiffness of the joint to pull the object towards the player's hands.
  /// Careful, too large values mean the held object can push objects that the player itself cannot push.
  float m_fSpringStiffness = 50.0f; // [ property ]

  /// The damping of the joint, to prevent oscillation when moving around.
  float m_fSpringDamping = 10.0f; // [ property ]

  /// How far grab points are allowed to be away to pick them
  float m_fMaxGrabPointDistance = 2.0f; // [ property ]

  /// The collision layer to use for the raycast.
  xiiUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// If non-zero, the player can pick up objects that have no xiiGrabbableItemComponent, if their bounding box extents are below this value.
  float m_fAllowGrabAnyObjectWithSize = 0.75f; // [ property ]

  void SetAttachToReference(const char* szReference); // [ property ]

  /// Which other game object to attach the grabbed object to.
  /// It is expected to hold a kinematic xiiJoltDynamicActorComponent that an xiiJoltJointComponent can be attached to.
  xiiGameObjectHandle m_hAttachTo;

protected:
  void Update();
  void ReleaseGrabbedObject();

  xiiJoltDynamicActorComponent* GetAttachToActor();
  xiiResult                     DetermineGrabPoint(const xiiComponent* pActor, xiiTransform& out_LocalGrabPoint) const;
  void                          CreateJoint(xiiJoltDynamicActorComponent* pParent, xiiJoltDynamicActorComponent* pChild);
  void                          DetectDistanceViolation(xiiJoltDynamicActorComponent* pGrabbedActor);
  bool                          IsCharacterStandingOnObject(xiiGameObjectHandle hActorToGrab) const;

  void OnMsgReleaseObjectGrab(xiiMsgReleaseObjectGrab& msg); // [ message handler ]

  xiiComponentHandle m_hGrabbedActor;
  float              m_fGrabbedActorGravity = 1.0f;
  float              m_fGrabbedActorMass    = 0.0f;

  xiiTime                m_LastValidTime;
  xiiTransform           m_ChildAnchorLocal;
  xiiComponentHandle     m_hCharacterControllerComponent;
  JPH::SixDOFConstraint* m_pConstraint = nullptr;

private:
  const char* DummyGetter() const { return nullptr; }
};
