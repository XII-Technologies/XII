#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GraphicsCore/AnimationSystem/AnimationPose.h>

using xiiJointOverrideComponentManager = xiiComponentManager<class xiiJointOverrideComponent, xiiBlockStorageType::FreeList>;

/// \brief Overrides the local transform of a bone in a skeletal animation.
///
/// Every time a new animation pose is prepared, this component replaces the transform of the chosen bone
/// to be the same as the local transform of the owner game object.
///
/// That allows you to do a simple kind of forward kinematics. For example it can be used to modify a targeting bone,
/// so that an animated object points into the right direction.
///
/// The global transform of the game object is irrelevant, but the local transform is used to copy over.
class XII_GAMEENGINE_DLL xiiJointOverrideComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJointOverrideComponent, xiiComponent, xiiJointOverrideComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJointOverrideComponent

public:
  xiiJointOverrideComponent();
  ~xiiJointOverrideComponent();

  /// \brief The name of the bone whose transform should be replaced with the transform of this game object.
  void        SetJointName(const char* szName); // [ property ]
  const char* GetJointName() const;             // [ property ]

  /// \brief If true, the position of the bone will be overridden.
  bool m_bOverridePosition = false; // [ property ]

  /// \brief If true, the rotation of the bone will be overridden.
  bool m_bOverrideRotation = true; // [ property ]

  /// \brief If true, the scale of the bone will be overridden.
  bool m_bOverrideScale = false; // [ property ]

protected:
  void OnAnimationPosePreparing(xiiMsgAnimationPosePreparing& msg) const; // [ msg handler ]

  xiiHashedString   m_sJointToOverride;

  mutable xiiUInt16 m_uiJointIndex = xiiInvalidJointIndex;
};
