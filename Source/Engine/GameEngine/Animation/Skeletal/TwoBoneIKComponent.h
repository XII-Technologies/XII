#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/ComponentManager.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiTwoBoneIKComponentManager = xiiComponentManager<class xiiTwoBoneIKComponent, xiiBlockStorageType::FreeList>;

/// \brief Adds inverse kinematics for a chain of three joints (two-bones) of an animated mesh to reach a target position.
///
/// This can be used to make a creature grab something or to implement foot IK.
/// The component has to be attached to a child object of an animated mesh.
/// The animated mesh needs to be driven by another component that generates an animation pose,
/// such as a xiiAnimationControllerComponent or a xiiSimpleAnimationComponent.
/// On those "EnableIK" must be set, then they will forward the pose to all their child objects and give them the
/// opportunity to override the pose using IK.
class XII_GAMEENGINE_DLL xiiTwoBoneIKComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiTwoBoneIKComponent, xiiComponent, xiiTwoBoneIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiTwoBoneIKComponent

public:
  xiiTwoBoneIKComponent();
  ~xiiTwoBoneIKComponent();

  void SetPoleVectorReference(const char* szReference); // [ property ]

  xiiGameObjectHandle   m_hPoleVector;    // [ property ] An optional other object used as the pole vector for the middle joint to point towards.
  float                 m_fWeight = 1.0f; // [ property ] Factor between 0 and 1 for how much to apply the IK.
  xiiHashedString       m_sJointStart;    // [ property ] First joint in the chain.
  xiiHashedString       m_sJointMiddle;   // [ property ] Second joint in the chain.
  xiiHashedString       m_sJointEnd;      // [ property ] Third joint in the chain. This one tries to reach the target position.
  xiiEnum<xiiBasisAxis> m_MidAxis;        // [ property ] The axis of the middle joint around which to bend.

  // currently these are not exposed, to reduce the number of parameters to fiddle with
  // float m_fSoften = 1.0f;
  // xiiAngle m_TwistAngle;

protected:
  void OnMsgAnimationPoseGeneration(xiiMsgAnimationPoseGeneration& msg) const; // [ msg handler ]

  mutable xiiUInt16 m_uiJointIdxStart  = 0;
  mutable xiiUInt16 m_uiJointIdxMiddle = 0;
  mutable xiiUInt16 m_uiJointIdxEnd    = 0;

  const char* DummyGetter() const { return nullptr; }
};
