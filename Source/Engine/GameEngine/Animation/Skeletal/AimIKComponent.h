#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiAimIKComponentManager = xiiComponentManager<class xiiAimIKComponent, xiiBlockStorageType::FreeList>;

struct xiiIkJointEntry
{
  xiiHashedString   m_sJointName;
  float             m_fWeight    = 1.0f;
  mutable xiiUInt16 m_uiJointIdx = 0;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiIkJointEntry);

/// \brief Adds inverse kinematics for a single joint of an animated mesh to point towards a target.
///
/// This can be used to make a creature look at something or to aim at a target.
/// The component has to be attached to a child object of an animated mesh.
/// The animated mesh needs to be driven by another component that generates an animation pose,
/// such as a xiiAnimationControllerComponent or a xiiSimpleAnimationComponent.
/// On those "EnableIK" must be set, then they will forward the pose to all their child objects and give them the
/// opportunity to override the pose using IK.
class XII_GAMEENGINE_DLL xiiAimIKComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiAimIKComponent, xiiComponent, xiiAimIKComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiAimIKComponent

public:
  xiiAimIKComponent();
  ~xiiAimIKComponent();

  void SetPoleVectorReference(const char* szReference); // [ property ]

  xiiGameObjectHandle                m_hPoleVector;                             // [ property ] An optional other object used as the pole vector for the joint to align with.
  xiiEnum<xiiBasisAxis>              m_ForwardVector = xiiBasisAxis::PositiveX; // [ property ] The local forward direction of the joint to orient towards the position of this object.
  xiiEnum<xiiBasisAxis>              m_UpVector      = xiiBasisAxis::PositiveZ; // [ property ] The local up direction of the joint to orient towards the pole vector.
  float                              m_fWeight       = 1.0f;                    // [ property ] Factor between 0 and 1 for how much to apply the IK.
  xiiHybridArray<xiiIkJointEntry, 2> m_Joints;                                  // [ property ] A list of joints to apply the aim IK to. If multiple joints along a chain are used, set a weight of less than 1 for the first joints and a factor of 1 for the last joint, to distribute gradual aiming along the chain.

protected:
  void OnMsgAnimationPoseGeneration(xiiMsgAnimationPoseGeneration& msg) const; // [ msg handler ]

  const char* DummyGetter() const { return nullptr; }
};
